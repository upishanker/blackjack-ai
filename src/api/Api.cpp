//
// All demo endpoints, shared by the httplib server and the WebAssembly build.
//
// Moved here wholesale from src/server/main.cpp so the hosted static build and
// the local server cannot drift apart. The only transport-specific detail left
// is threading: see advanceTraining.
//

#include "../../include/api/Api.h"
#include "Json.h"

#include "../../include/core/Game.h"
#include "../../include/core/HandSession.h"
#include "../../include/core/Rules.h"
#include "../../include/ai/QLearningAI.h"
#include "../../include/ai/MonteCarloAI.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

namespace api {
namespace {

// Hyperparameters mirror src/main.cpp so the demo and the CLI train the same
// agent. Keep these in sync if the CLI's defaults change.
constexpr double Q_ALPHA          = 0.01;      // learning-rate floor, not a constant rate
constexpr double Q_GAMMA          = 1.0;       // episodic, undiscounted
constexpr double Q_EPSILON_START  = 1.0;
constexpr double Q_EPSILON_DECAY  = 0.99995;   // Game::trainAI
constexpr double Q_EPSILON_FLOOR  = 0.05;      // QLearningAI::decayEpsilon
constexpr double MC_EPSILON_FLOOR = 0.01;      // MonteCarloAI::decayEpsilon
constexpr double MC_EPSILON_START = 0.1;
constexpr double MC_GAMMA         = 1.0;
constexpr double MC_EPSILON_DECAY = 0.9995;    // MonteCarloAI::decayEpsilon default

// A table loaded from disk represents an already-trained agent, so continuing
// to train it should not restart exploration from scratch.
constexpr double EPSILON_AFTER_LOAD = 0.05;

constexpr int MIN_PLAYER_SUM = 4;
constexpr int MAX_PLAYER_SUM = 21;
constexpr int MIN_UPCARD     = 2;
constexpr int MAX_UPCARD     = 11;

constexpr int MAX_SIMULATE_GAMES = 500000;
constexpr int MAX_TRAIN_EPISODES = 5000000;
constexpr int MAX_OPEN_HANDS     = 200;
constexpr int PROGRESS_POINTS    = 200;

// ---------------------------------------------------------------------------
// Agent handles
//
// QLearningAI and MonteCarloAI share an identical const inference surface
// (getBestAction / getQValue) but have no common base class, so these thin
// adapters unify them for the API. Neither AI class is modified.
// ---------------------------------------------------------------------------

struct Agent {
    // Reads (policy, simulate, save) take a shared lock; training and
    // reset/load take it exclusively. In the single-threaded WebAssembly build
    // these are uncontended no-ops.
    mutable std::shared_mutex mu;
    std::string id;
    std::string label;
    std::string tablePath;
    double epsilon;
    bool tableLoaded = false;

    Agent(std::string id_, std::string label_, std::string path_, double eps)
        : id(std::move(id_)), label(std::move(label_)),
          tablePath(std::move(path_)), epsilon(eps) {}
    virtual ~Agent() = default;

    virtual Action best(const State&) const = 0;
    virtual double qValue(const State&, Action) const = 0;
    virtual int visitCount(const State&, Action) const = 0;
    virtual bool hasLearned(const State&) const = 0;
    virtual int episodes() const = 0;

    virtual void save() const = 0;
    virtual void load() = 0;
    virtual void reset() = 0;
    virtual void setEpsilon(double e) = 0;

    // One episode against a freshly dealt hand, plus whatever bookkeeping the
    // algorithm needs afterwards. Mirrors Game::trainAI / Game::trainMonteCarlo.
    virtual double runEpisode(Game& game, bool training) = 0;
    virtual void afterTrainingEpisode(double reward) = 0;

    int learnedStateCount() const {
        int count = 0;
        for (int sum = MIN_PLAYER_SUM; sum <= MAX_PLAYER_SUM; ++sum) {
            for (int up = MIN_UPCARD; up <= MAX_UPCARD; ++up) {
                for (int ace = 0; ace <= 1; ++ace) {
                    if (hasLearned(State{sum, up, ace != 0})) ++count;
                }
            }
        }
        return count;
    }
};

struct QAgent : Agent {
    QLearningAI ai{Q_ALPHA, Q_GAMMA, Q_EPSILON_START};

    QAgent() : Agent("q", "Q-Learning", "data/q_table.csv", Q_EPSILON_START) {}

    Action best(const State& s) const override { return ai.getBestAction(s); }
    double qValue(const State& s, Action a) const override { return ai.getQValue(s, a); }
    int visitCount(const State& s, Action a) const override { return ai.getVisitCount(s, a); }

    bool hasLearned(const State& s) const override {
        return ai.getVisitCount(s, Action::HIT) > 0 || ai.getVisitCount(s, Action::STAND) > 0;
    }

    int episodes() const override { return ai.getEpisodeCount(); }

    void save() const override { ai.saveQTable(tablePath); }
    void load() override {
        ai.loadQTable(tablePath);
        if (learnedStateCount() > 0) {
            tableLoaded = true;
            epsilon = EPSILON_AFTER_LOAD;
            ai.setEpsilon(EPSILON_AFTER_LOAD);
        }
    }
    void reset() override {
        ai = QLearningAI(Q_ALPHA, Q_GAMMA, Q_EPSILON_START);
        epsilon = Q_EPSILON_START;
        tableLoaded = false;
    }
    void setEpsilon(double e) override { epsilon = e; ai.setEpsilon(e); }

    double runEpisode(Game& game, bool training) override {
        return game.playAIEpisode(ai, training);
    }
    void afterTrainingEpisode(double reward) override {
        ai.recordEpisode(reward);
        ai.decayEpsilon(Q_EPSILON_DECAY);
        // Mirror the floor applied inside decayEpsilon, or the reported value
        // drifts to zero while the agent is still exploring at 5%.
        epsilon = std::max(Q_EPSILON_FLOOR, epsilon * Q_EPSILON_DECAY);
    }
};

struct MCAgent : Agent {
    MonteCarloAI ai{MC_EPSILON_START, MC_GAMMA};

    MCAgent() : Agent("mc", "Monte Carlo", "data/mc_q_table.csv", MC_EPSILON_START) {}

    Action best(const State& s) const override { return ai.getBestAction(s); }
    double qValue(const State& s, Action a) const override { return ai.getQValue(s, a); }
    int visitCount(const State& s, Action a) const override { return ai.getVisitCount(s, a); }

    bool hasLearned(const State& s) const override {
        return ai.getVisitCount(s, Action::HIT) > 0 || ai.getVisitCount(s, Action::STAND) > 0;
    }

    int episodes() const override { return ai.getEpisodeCount(); }

    void save() const override { ai.saveQTable(tablePath); }
    void load() override {
        ai.loadQTable(tablePath);
        if (learnedStateCount() > 0) {
            tableLoaded = true;
            epsilon = EPSILON_AFTER_LOAD;
            ai.setEpsilon(EPSILON_AFTER_LOAD);
        }
    }
    void reset() override {
        ai = MonteCarloAI(MC_EPSILON_START, MC_GAMMA);
        epsilon = MC_EPSILON_START;
        tableLoaded = false;
    }
    void setEpsilon(double e) override { epsilon = e; ai.setEpsilon(e); }

    double runEpisode(Game& game, bool training) override {
        return game.playMonteCarloEpisode(ai, training);
    }
    void afterTrainingEpisode(double) override {
        ai.decayEpsilon(MC_EPSILON_DECAY);
        epsilon = std::max(MC_EPSILON_FLOOR, epsilon * MC_EPSILON_DECAY);
    }
};

QAgent  gQ;
MCAgent gMC;

Agent* agentFor(const std::string& id) {
    if (id == "mc") return &gMC;
    if (id == "q" || id.empty()) return &gQ;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Training job
// ---------------------------------------------------------------------------

struct ProgressPoint {
    long long episode;
    double winRate;      // wins / (wins + losses), over the rolling window
    double avgReward;    // over the rolling window
    double epsilon;
    int statesLearned;
};

struct TrainingJob {
    std::mutex mu;
    bool running = false;
    bool cancel = false;
    std::string agentId;
    Agent* agent = nullptr;
    long long done = 0;
    long long total = 0;
    long long chunk = 1;
    std::vector<ProgressPoint> points;
    std::unique_ptr<Game> game;

    // Rolling window since the last progress point.
    long long wins = 0, losses = 0, pushes = 0;
    double reward = 0.0;

    void resetWindow() { wins = losses = pushes = 0; reward = 0.0; }
};

TrainingJob gJob;

void emitPoint() {
    long long n = gJob.wins + gJob.losses + gJob.pushes;
    int learned;
    {
        std::shared_lock<std::shared_mutex> lk(gJob.agent->mu);
        learned = gJob.agent->learnedStateCount();
    }
    ProgressPoint p;
    p.episode       = gJob.done;
    p.winRate       = (gJob.wins + gJob.losses) > 0
                          ? static_cast<double>(gJob.wins) / (gJob.wins + gJob.losses)
                          : 0.0;
    p.avgReward     = n > 0 ? gJob.reward / static_cast<double>(n) : 0.0;
    p.epsilon       = gJob.agent->epsilon;
    p.statesLearned = learned;
    gJob.points.push_back(p);
    gJob.resetWindow();
}

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

const char* rankName(Rank r) {
    switch (r) {
        case Rank::Two:   return "2";
        case Rank::Three: return "3";
        case Rank::Four:  return "4";
        case Rank::Five:  return "5";
        case Rank::Six:   return "6";
        case Rank::Seven: return "7";
        case Rank::Eight: return "8";
        case Rank::Nine:  return "9";
        case Rank::Ten:   return "10";
        case Rank::Jack:  return "J";
        case Rank::Queen: return "Q";
        case Rank::King:  return "K";
        case Rank::Ace:   return "A";
    }
    return "?";
}

const char* suitName(Suit s) {
    switch (s) {
        case Suit::Hearts:   return "hearts";
        case Suit::Diamonds: return "diamonds";
        case Suit::Clubs:    return "clubs";
        case Suit::Spades:   return "spades";
    }
    return "?";
}

std::string cardJson(const Card& c) {
    return json::Writer()
        .kv("rank", rankName(c.getRank()))
        .kv("suit", suitName(c.getSuit()))
        .kv("value", c.getValue())
        .done();
}

std::string handJson(const std::vector<Card>& hand) {
    json::Writer w(true);
    for (const Card& c : hand) w.raw(cardJson(c));
    return w.done();
}

std::string stateJson(const State& s) {
    return json::Writer()
        .kv("playerSum", s.playerSum)
        .kv("dealerUpcard", s.dealerUpcard)
        .kv("usableAce", s.usableAce)
        .done();
}

const char* outcomeFor(const HandSession& h) {
    if (!h.finished()) return "in_progress";
    if (h.playerBusted()) return "bust";
    double r = h.reward();
    if (r > 1.0)  return "blackjack";
    if (r > 0.0)  return "win";
    if (r < 0.0)  return "loss";
    return "push";
}

// Full snapshot of a hand. The dealer's *second* card is withheld until the
// hand settles: the agent's state conditions on dealerHand[0], so that is the
// card shown face up.
std::string handJsonFull(int id, const HandSession& h, const Agent& agent) {
    json::Writer w;
    w.kv("handId", id);
    w.kv("agent", agent.id);

    std::vector<Card> dealerVisible;
    const auto& dh = h.dealerHand();
    if (h.finished()) {
        dealerVisible = dh;
    } else if (!dh.empty()) {
        dealerVisible.push_back(dh[0]);
    }
    w.kraw("playerHand", handJson(h.playerHand()));
    w.kraw("dealerHand", handJson(dealerVisible));
    w.kv("dealerHiddenCards", static_cast<int>(dh.size() - dealerVisible.size()));
    w.kv("playerValue", h.playerValue());
    w.kv("dealerValue", h.finished() ? h.dealerValue() : (dh.empty() ? 0 : dh[0].getValue()));
    w.kv("playerBusted", h.playerBusted());
    w.kv("dealerBusted", h.finished() && h.dealerBusted());
    w.kv("playerBlackjack", h.playerHasBlackjack());
    w.kv("canAct", h.playerCanAct());
    w.kv("finished", h.finished());
    w.kv("outcome", outcomeFor(h));
    if (h.finished()) {
        w.kv("reward", h.reward());
    }

    State s = h.state();
    w.kraw("state", stateJson(s));

    double qHit, qStand;
    Action rec;
    bool learned;
    {
        std::shared_lock<std::shared_mutex> lk(agent.mu);
        qHit    = agent.qValue(s, Action::HIT);
        qStand  = agent.qValue(s, Action::STAND);
        rec     = agent.best(s);
        learned = agent.hasLearned(s);
    }
    w.kraw("policy", json::Writer()
        .kv("action", rec == Action::HIT ? "hit" : "stand")
        .kv("qHit", qHit)
        .kv("qStand", qStand)
        .kv("margin", qHit - qStand)
        // Unlearned states fall back to a fixed "hit below 17" heuristic
        // inside both agents -- worth surfacing rather than hiding.
        .kv("learned", learned)
        .done());

    return w.done();
}

std::string agentSummaryJson(const Agent& a) {
    std::shared_lock<std::shared_mutex> lk(a.mu);
    return json::Writer()
        .kv("id", a.id)
        .kv("label", a.label)
        .kv("episodes", a.episodes())
        .kv("statesLearned", a.learnedStateCount())
        .kv("statesPossible", (MAX_PLAYER_SUM - MIN_PLAYER_SUM + 1) * (MAX_UPCARD - MIN_UPCARD + 1) * 2)
        .kv("epsilon", a.epsilon)
        .kv("tableLoaded", a.tableLoaded)
        .kv("tablePath", a.tablePath)
        .done();
}

// ---------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------

struct Tally {
    int wins = 0, losses = 0, pushes = 0, blackjacks = 0;
    double total = 0.0, totalSq = 0.0;

    void add(double r) {
        total += r;
        totalSq += r * r;
        if (r > 1.0) ++blackjacks;
        if (r > 0)      ++wins;
        else if (r < 0) ++losses;
        else            ++pushes;
    }

    std::string json(const std::string& id, const std::string& label, int games) const {
        int decided = wins + losses;
        double ev = games > 0 ? total / games : 0.0;
        // Standard error on the mean reward: the demo compares policies whose
        // EV differs by ~0.01, so the interval matters as much as the estimate.
        double var = games > 0 ? (totalSq / games - ev * ev) : 0.0;
        double se = games > 0 ? std::sqrt(std::max(0.0, var) / games) : 0.0;

        return json::Writer()
            .kv("agent", id)
            .kv("label", label)
            .kv("games", games)
            .kv("wins", wins)
            .kv("losses", losses)
            .kv("pushes", pushes)
            .kv("blackjacks", blackjacks)
            .kv("avgReward", ev)
            .kv("avgRewardCI95", 1.96 * se)
            // Win rate excludes pushes, matching Game::evaluateAI's reporting.
            .kv("winRate", decided > 0 ? static_cast<double>(wins) / decided : 0.0)
            .kv("winRateAllHands", games > 0 ? static_cast<double>(wins) / games : 0.0)
            .done();
    }
};

std::string simulate(Agent& agent, int games) {
    Game game(0);   // 0 players: Game's constructor reads names from stdin otherwise
    Tally t;
    {
        std::shared_lock<std::shared_mutex> lk(agent.mu);
        for (int i = 0; i < games; ++i) {
            t.add(agent.runEpisode(game, false));
        }
    }
    return t.json(agent.id, agent.label, games);
}

// The fixed benchmark: no Q-table, no learning, just published basic strategy
// played through the same HandSession the demo deals from.
std::string simulateBasic(int games) {
    Tally t;
    for (int i = 0; i < games; ++i) {
        HandSession h;
        while (h.playerCanAct()) {
            if (rules::basicStrategy(h.state()) == Action::STAND) break;
            h.hit();
        }
        if (!h.finished()) h.stand();
        t.add(h.reward());
    }
    return t.json("basic", "Basic strategy", games);
}

// ---------------------------------------------------------------------------
// Hand sessions
// ---------------------------------------------------------------------------

std::mutex gHandsMu;
std::map<int, std::unique_ptr<HandSession>> gHands;
int gNextHandId = 1;

// ---------------------------------------------------------------------------
// Param helpers
// ---------------------------------------------------------------------------

std::string param(const Params& p, const std::string& key) {
    auto it = p.find(key);
    return it == p.end() ? std::string() : it->second;
}

long long paramInt(const Params& p, const std::string& key, long long fallback) {
    auto it = p.find(key);
    if (it == p.end() || it->second.empty()) return fallback;
    try {
        return std::stoll(it->second);
    } catch (...) {
        return fallback;
    }
}

Response json_(const std::string& body, int status = 200) {
    Response r;
    r.status = status;
    r.body = body;
    return r;
}

Response error_(const std::string& message, int status) {
    return json_(json::error(message), status);
}

} // namespace

// ---------------------------------------------------------------------------
// Training advance (shared by both front ends)
// ---------------------------------------------------------------------------

long long trainingChunkSize() {
    std::lock_guard<std::mutex> lk(gJob.mu);
    return gJob.running ? gJob.chunk : 0;
}

void advanceTraining(long long budget) {
    std::lock_guard<std::mutex> lk(gJob.mu);
    if (!gJob.running || gJob.agent == nullptr) return;

    long long ran = 0;
    while (gJob.running && ran < budget && gJob.done < gJob.total) {
        if (gJob.cancel) break;

        double r;
        {
            // Per-episode locking (rather than per-chunk) so that policy and
            // simulate requests stay responsive while training runs.
            std::unique_lock<std::shared_mutex> alk(gJob.agent->mu);
            r = gJob.agent->runEpisode(*gJob.game, true);
            gJob.agent->afterTrainingEpisode(r);
        }

        gJob.reward += r;
        if (r > 0)      ++gJob.wins;
        else if (r < 0) ++gJob.losses;
        else            ++gJob.pushes;

        ++gJob.done;
        ++ran;

        if (gJob.done % gJob.chunk == 0 || gJob.done == gJob.total) {
            emitPoint();
        }
    }

    if (gJob.cancel || gJob.done >= gJob.total) {
        if (gJob.wins + gJob.losses + gJob.pushes > 0) emitPoint();
        gJob.running = false;
        gJob.game.reset();
    }
}

void init() {
    gQ.load();
    gMC.load();
}

// ---------------------------------------------------------------------------
// Routing
// ---------------------------------------------------------------------------

Response handle(const std::string& path, const Params& params) {
    // --- status ----------------------------------------------------------
    if (path == "/api/status") {
        std::string training;
        {
            std::lock_guard<std::mutex> lk(gJob.mu);
            training = json::Writer()
                .kv("running", gJob.running)
                .kv("agent", gJob.agentId)
                .kv("done", gJob.done)
                .kv("total", gJob.total)
                .kv("chunk", gJob.chunk)
                .done();
        }
        return json_(json::Writer()
            .kraw("q", agentSummaryJson(gQ))
            .kraw("mc", agentSummaryJson(gMC))
            .kraw("training", training)
            .done());
    }

    // --- hand play -------------------------------------------------------
    if (path == "/api/hand/new") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);

        std::lock_guard<std::mutex> lk(gHandsMu);
        // Bound memory: this is a demo, old hands are not worth keeping.
        while (gHands.size() >= MAX_OPEN_HANDS) gHands.erase(gHands.begin());

        int id = gNextHandId++;
        auto hand = std::make_unique<HandSession>();
        std::string body = handJsonFull(id, *hand, *agent);
        gHands.emplace(id, std::move(hand));
        return json_(body);
    }

    if (path == "/api/hand/step") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);

        int id = static_cast<int>(paramInt(params, "id", 0));
        std::string action = param(params, "action");

        std::lock_guard<std::mutex> lk(gHandsMu);
        auto it = gHands.find(id);
        if (it == gHands.end()) return error_("unknown or expired hand", 404);

        HandSession& hand = *it->second;
        if (hand.finished()) return json_(handJsonFull(id, hand, *agent));

        if (action == "auto") {
            State s = hand.state();
            Action chosen;
            {
                std::shared_lock<std::shared_mutex> alk(agent->mu);
                chosen = agent->best(s);
            }
            action = (chosen == Action::HIT) ? "hit" : "stand";
        }

        if (action == "hit") {
            if (!hand.playerCanAct()) return error_("player cannot hit", 409);
            hand.hit();
        } else if (action == "stand") {
            hand.stand();
        } else {
            return error_("action must be hit, stand or auto", 400);
        }

        std::string body = handJsonFull(id, hand, *agent);
        if (hand.finished()) gHands.erase(it);
        return json_(body);
    }

    // --- policy grid -----------------------------------------------------
    if (path == "/api/policy") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);

        json::Writer cells(true);
        {
            std::shared_lock<std::shared_mutex> lk(agent->mu);
            for (int sum = MIN_PLAYER_SUM; sum <= MAX_PLAYER_SUM; ++sum) {
                for (int up = MIN_UPCARD; up <= MAX_UPCARD; ++up) {
                    for (int ace = 0; ace <= 1; ++ace) {
                        State s{sum, up, ace != 0};
                        double qh = agent->qValue(s, Action::HIT);
                        double qs = agent->qValue(s, Action::STAND);
                        int visits = agent->visitCount(s, Action::HIT)
                                   + agent->visitCount(s, Action::STAND);
                        cells.raw(json::Writer()
                            .kv("playerSum", sum)
                            .kv("dealerUpcard", up)
                            .kv("usableAce", ace != 0)
                            .kv("action", agent->best(s) == Action::HIT ? "hit" : "stand")
                            .kv("basic", rules::basicStrategy(s) == Action::HIT ? "hit" : "stand")
                            .kv("qHit", qh)
                            .kv("qStand", qs)
                            .kv("margin", qh - qs)
                            .kv("learned", agent->hasLearned(s))
                            .kv("visits", visits)
                            .done());
                    }
                }
            }
        }
        return json_(json::Writer()
            .kv("agent", agent->id)
            .kv("label", agent->label)
            .kraw("cells", cells.done())
            .done());
    }

    // --- evaluation ------------------------------------------------------
    if (path == "/api/simulate") {
        int games = static_cast<int>(std::max<long long>(1,
            std::min<long long>(MAX_SIMULATE_GAMES, paramInt(params, "games", 10000))));
        if (param(params, "agent") == "basic") return json_(simulateBasic(games));

        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);
        return json_(simulate(*agent, games));
    }

    if (path == "/api/compare") {
        int games = static_cast<int>(std::max<long long>(1,
            std::min<long long>(MAX_SIMULATE_GAMES, paramInt(params, "games", 20000))));

        std::string qRes    = simulate(gQ, games);
        std::string mcRes   = simulate(gMC, games);
        std::string baseRes = simulateBasic(games);

        // Where do the two learned policies actually disagree?
        json::Writer disagreements(true);
        int disagree = 0, comparable = 0;
        {
            std::shared_lock<std::shared_mutex> lq(gQ.mu);
            std::shared_lock<std::shared_mutex> lm(gMC.mu);
            for (int sum = MIN_PLAYER_SUM; sum <= MAX_PLAYER_SUM; ++sum) {
                for (int up = MIN_UPCARD; up <= MAX_UPCARD; ++up) {
                    for (int ace = 0; ace <= 1; ++ace) {
                        State s{sum, up, ace != 0};
                        // Only states both agents actually learned are a fair
                        // comparison; otherwise we compare the shared fallback.
                        if (!gQ.hasLearned(s) || !gMC.hasLearned(s)) continue;
                        ++comparable;
                        Action aq = gQ.best(s), am = gMC.best(s);
                        if (aq == am) continue;
                        ++disagree;
                        disagreements.raw(json::Writer()
                            .kv("playerSum", sum)
                            .kv("dealerUpcard", up)
                            .kv("usableAce", ace != 0)
                            .kv("q", aq == Action::HIT ? "hit" : "stand")
                            .kv("mc", am == Action::HIT ? "hit" : "stand")
                            .done());
                    }
                }
            }
        }

        return json_(json::Writer()
            .kv("games", games)
            .kraw("q", qRes)
            .kraw("mc", mcRes)
            .kraw("basic", baseRes)
            .kv("comparableStates", comparable)
            .kv("disagreements", disagree)
            .kraw("disagreementCells", disagreements.done())
            .done());
    }

    // --- training --------------------------------------------------------
    if (path == "/api/train") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);
        long long episodes = std::max<long long>(1,
            std::min<long long>(MAX_TRAIN_EPISODES, paramInt(params, "episodes", 100000)));

        std::lock_guard<std::mutex> lk(gJob.mu);
        if (gJob.running) return error_("training already in progress", 409);

        if (param(params, "reset") == "true") {
            std::unique_lock<std::shared_mutex> alk(agent->mu);
            agent->reset();
        }
        if (params.count("epsilon")) {
            try {
                double e = std::stod(param(params, "epsilon"));
                std::unique_lock<std::shared_mutex> alk(agent->mu);
                agent->setEpsilon(e);
            } catch (...) { /* keep current epsilon */ }
        }

        gJob.agentId = agent->id;
        gJob.agent   = agent;
        gJob.done    = 0;
        gJob.total   = episodes;
        gJob.chunk   = std::max<long long>(1, episodes / PROGRESS_POINTS);
        gJob.cancel  = false;
        gJob.points.clear();
        gJob.resetWindow();
        gJob.game    = std::make_unique<Game>(0);
        gJob.running = true;

        return json_(json::Writer()
            .kv("started", true)
            .kv("agent", agent->id)
            .kv("episodes", episodes)
            .kv("chunk", gJob.chunk)
            .done());
    }

    // Runs a slice of training and returns immediately. The browser drives this
    // itself; the native server's worker thread uses advanceTraining directly.
    if (path == "/api/train/step") {
        long long budget = std::max<long long>(1, paramInt(params, "episodes", 2000));
        advanceTraining(budget);
        std::lock_guard<std::mutex> lk(gJob.mu);
        return json_(json::Writer()
            .kv("running", gJob.running)
            .kv("done", gJob.done)
            .kv("total", gJob.total)
            .done());
    }

    if (path == "/api/train/progress") {
        long long since = std::max<long long>(0, paramInt(params, "since", 0));

        std::lock_guard<std::mutex> lk(gJob.mu);
        json::Writer pts(true);
        for (size_t i = static_cast<size_t>(since); i < gJob.points.size(); ++i) {
            const ProgressPoint& p = gJob.points[i];
            pts.raw(json::Writer()
                .kv("episode", p.episode)
                .kv("winRate", p.winRate)
                .kv("avgReward", p.avgReward)
                .kv("epsilon", p.epsilon)
                .kv("statesLearned", p.statesLearned)
                .done());
        }
        return json_(json::Writer()
            .kv("running", gJob.running)
            .kv("agent", gJob.agentId)
            .kv("done", gJob.done)
            .kv("total", gJob.total)
            .kv("cursor", static_cast<long long>(gJob.points.size()))
            .kraw("points", pts.done())
            .done());
    }

    if (path == "/api/train/stop") {
        std::lock_guard<std::mutex> lk(gJob.mu);
        bool was = gJob.running;
        gJob.cancel = true;
        return json_(json::Writer().kv("stopping", was).done());
    }

    // --- persistence -----------------------------------------------------
    if (path == "/api/save") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);
        {
            std::shared_lock<std::shared_mutex> lk(agent->mu);
            agent->save();
        }
        return json_(json::Writer().kv("saved", agent->tablePath).done());
    }

    if (path == "/api/reset") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);
        {
            std::lock_guard<std::mutex> lk(gJob.mu);
            if (gJob.running) return error_("cannot reset while training", 409);
        }
        {
            std::unique_lock<std::shared_mutex> lk(agent->mu);
            agent->reset();
        }
        return json_(agentSummaryJson(*agent));
    }

    if (path == "/api/qtable.csv") {
        Agent* agent = agentFor(param(params, "agent"));
        if (!agent) return error_("unknown agent", 400);
        {
            std::shared_lock<std::shared_mutex> lk(agent->mu);
            agent->save();
        }
        std::ifstream in(agent->tablePath, std::ios::binary);
        if (!in) return error_("no table on disk", 404);

        Response r;
        r.body.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        r.contentType = "text/csv";
        r.downloadName = agent->id + "_q_table.csv";
        return r;
    }

    return error_("unknown endpoint", 404);
}

} // namespace api
