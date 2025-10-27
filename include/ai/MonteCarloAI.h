//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_MONTECARLOAI_H
#define BLACKJACK_AI_MONTECARLOAI_H

#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <random>
#include <utility>
#include "AITypes.h"

// State-Action pair for episode tracking
struct StateAction {
    State state;
    Action action;

    bool operator==(const StateAction& other) const {
        return state == other.state && action == other.action;
    }
};

struct StateActionHash {
    size_t operator()(const StateAction& sa) const {
        StateHash sh;
        return sh(sa.state) * 3 + static_cast<int>(sa.action);
    }
};

class MonteCarloAI {
private:
    // Q-values: State → [Q(s,HIT), Q(s,STAND)]
    std::unordered_map<State, std::array<double, 2>, StateHash> qTable;

    // Returns: Sum of returns for each state-action pair
    std::unordered_map<StateAction, double, StateActionHash> returns;

    // Visit counts: Number of times each state-action pair was visited
    std::unordered_map<StateAction, int, StateActionHash> visitCounts;

    // Hyperparameters
    double epsilon;    // Exploration rate
    double gamma;      // Discount factor

    // Training stats
    int episodeCount;
    double totalReward;

    // RNG
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

    // Episode storage
    struct Step {
        State state;
        Action action;
        double reward;
    };
    std::vector<Step> currentEpisode;

public:
    MonteCarloAI(double epsilon = 0.1, double gamma = 1.0);

    // Episode management
    void startEpisode();
    void recordStep(const State& state, Action action, double reward);
    void endEpisode(double finalReward);

    // Action selection
    Action chooseAction(const State& state);
    Action getBestAction(const State& state) const;

    // Monte Carlo update (called at end of episode)
    void updateFromEpisode();

    // Persistence
    void saveQTable(const std::string& filename) const;
    void loadQTable(const std::string& filename);

    // Getters/Setters
    double getQValue(const State& state, Action action) const;
    void setEpsilon(double newEpsilon);
    void setGamma(double newGamma);

    // Training utilities
    void decayEpsilon(double decayRate = 0.9995);
    void printStats() const;
    int getEpisodeCount() const { return episodeCount; }

    // Get visit count for a state-action pair (for analysis)
    int getVisitCount(const State& state, Action action) const;
};

#endif //BLACKJACK_AI_MONTECARLOAI_H