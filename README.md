# Blackjack AI

Two reinforcement learning agents, **Q-learning** and **Monte Carlo control**,
learn Blackjack strategy in a C++17 engine and are measured against published
basic strategy.

**Live demo:** https://upishanker.github.io/blackjack-ai/ (the engine compiled
to WebAssembly, running in your browser)

Both agents learn over the classic Sutton & Barto state representation —
`(player sum, dealer upcard, usable ace)` — with a two-action space, `HIT` and
`STAND`. The committed Q-tables in `data/` were each trained for 2,000,000
episodes.

There are two front ends over the same engine: an interactive CLI, and a
**browser demo** that shows the learned policy as a strategy grid, plays hands
one decision at a time with live Q-values, and trains the agent while you watch
the table converge. See [Web Demo](#web-demo).

## Performance Metrics

**Average reward is the metric, not win rate.** Win rate is capped near 44% by
the rules — you lose immediately when you bust, no matter what the dealer then
does, and that asymmetry is the house edge. Every policy below, from optimal to
terrible, sits within three points of the same win rate, while their actual
profitability differs by a factor of five.

Measured over 500,000 greedy-policy hands through this engine, against the
tables committed in `data/` (2,000,000 training episodes each):

| Policy | Avg reward / hand | Win rate | Excl. pushes |
|---|---|---|---|
| Basic strategy *(benchmark)* | **−0.014** ±0.003 | 43.7% | 47.9% |
| Q-Learning | **−0.022** ±0.003 | 43.4% | 47.5% |
| Monte Carlo | **−0.021** ±0.003 | 43.4% | 47.6% |
| "Hit below 17" heuristic | −0.053 ±0.003 | 41.3% | 45.8% |
| Never bust (stand on 12+) | −0.075 ±0.003 | 42.0% | 44.8% |

| | Q-Learning | Monte Carlo |
|---|---|---|
| Q-Table Size | 280 states | 260 states |
| States differing from basic strategy | 9 | 18 |

Basic strategy is a fixed reference policy (`rules::basicStrategy`), not
something the agents learn from — it is the yardstick. Both agents land within
about 0.008 units per hand of it; the residual gap is concentrated in rarely
visited states (soft hands get ~3,000 visits where hard 16 gets ~80,000).

A negative average reward is expected. Without doubling or splitting there is no
positive-EV strategy here; the agents learn to lose slowly, not to win.

## Getting Started

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.20+ *or* **Make** — either one builds it
- **Emscripten** — only to rebuild the hosted WebAssembly bundle

No third-party libraries are required. The web server uses
[cpp-httplib](https://github.com/yhirose/cpp-httplib), a single MIT-licensed
header vendored at `third_party/httplib.h`.

### Installation

1. **Clone the repository**
```bash
git clone https://github.com/upishanker/blackjack-ai.git
cd blackjack-ai
```

2. **Build** — either with CMake:
```bash
cmake -B build
cmake --build build -j$(nproc)
```

   or without it, straight from the repo root:
```bash
make -f build.mk          # produces bin/blackjack_ai and bin/blackjack_server
```

3. **Run**
```bash
./bin/blackjack_ai        # interactive CLI
./bin/blackjack_server    # web demo on http://localhost:8080
```

> Both binaries resolve `data/` and `web/` relative to the working directory,
> so launch them from the repository root.

## Web Demo

```bash
make -f build.mk run-server     # then open http://localhost:8080
```

The browser is a view onto the C++ process — no game rules and no learning
logic are reimplemented in JavaScript. Every card, action and Q-value in the
page came out of the same `Game`, `QLearningAI` and `MonteCarloAI` the CLI uses.

**Play** — deal a hand and step through it one decision at a time. The panel
shows the state tuple the agent actually sees, the Q-value of each action, and
which one it picks. Hands where the agent has never been trained are flagged:
both agents silently fall back to a fixed "hit below 17" heuristic on unvisited
states, and the demo says so rather than hiding it. Switch to *You* to make the
calls yourself and see how often you agree with the policy.

**Policy** — the whole learned table as a heat map: player sum against dealer
upcard, hard and soft totals, coloured by the chosen action and saturated by
`Q(hit) − Q(stand)`. Hover any cell for raw Q-values and visit counts. Toggle
the overlay to outline every state where the agent diverges from published
basic strategy.

**Training** — run episodes and watch the average-reward, win-rate and
exploration curves fill in while the live policy grid resolves out of noise. The
reward chart draws basic strategy as a target line, so the curve is read against
what is actually achievable rather than against zero. Locally this runs on a
server worker thread; in the hosted build the browser drives it in slices.

**Compare** — both agents against the basic-strategy benchmark over N hands,
reported as average reward with a 95% interval so you can tell a real difference
from noise, plus the exact set of states where the two agents disagree.

### Hosting it

The demo ships in two forms, built from the same C++ sources:

| | Local | Hosted |
|---|---|---|
| Build | `make -f build.mk` | `./build-wasm.sh` |
| Runs | `bin/blackjack_server` | WebAssembly, in the visitor's browser |
| Serves | `web/` | `docs/` |
| Backend | httplib on :8080 | none |

To publish on GitHub Pages:

```bash
source /path/to/emsdk/emsdk_env.sh
./build-wasm.sh                       # -> docs/
python3 -m http.server -d docs 8000   # check it locally first
git add docs && git commit -m "Build web demo" && git push
```

Then **Settings → Pages → Source: main, folder `/docs`**.

There is no server to pay for, nothing to keep awake, and no cold start. Each
visitor gets their own private agent, so one person training for five million
episodes cannot affect anyone else's page — which also means none of the
endpoints need rate limiting or an auth story.

The whole page is about 460 KB: a 325 KB `.wasm`, 71 KB of Emscripten glue, and
the same HTML/CSS/JS the local server serves. The trained Q-tables are embedded
into the module, so `loadQTable("data/q_table.csv")` finds them in Emscripten's
in-memory filesystem exactly as it does on disk.

Two consequences of running in a tab worth knowing:

- **Training is chunked, not threaded.** Emscripten's pthreads need
  `SharedArrayBuffer`, which needs COOP/COEP response headers that GitHub Pages
  cannot send. So the browser calls `/api/train/step` in slices of a few
  thousand episodes and yields between them. It is not slow — 200,000 episodes
  take about 2.5 seconds.
- **"Save q-table" becomes "Download q-table."** There is no disk to write to,
  so the trained table comes back as a CSV you can drop into `data/`.

### API

Both front ends route through the same `api::handle` in `src/api/Api.cpp`, so
there is no second implementation to drift. The frontend is plain HTML/CSS/JS
with no build step and no CDN, talking to:

| Endpoint | Purpose |
|---|---|
| `GET /api/status` | states learned, episode count, epsilon, per agent |
| `POST /api/hand/new?agent=` | deal a hand |
| `POST /api/hand/step?id=&action=hit\|stand\|auto` | apply one action; `auto` uses the policy |
| `GET /api/policy?agent=` | the full Q-table as a grid |
| `GET /api/simulate?agent=&games=` | greedy-policy results over N hands; `agent=basic` runs the benchmark |
| `GET /api/compare?games=` | both agents against basic strategy, plus policy disagreements |
| `POST /api/train?agent=&episodes=&reset=` | start training |
| `POST /api/train/step?episodes=` | run a slice of episodes (drives the WASM build) |
| `GET /api/train/progress?since=` | learning-curve points |
| `POST /api/save?agent=` · `GET /api/qtable.csv?agent=` | persist / download |

Q-table reads and writes are guarded by a shared mutex, so the policy grid and
simulations stay responsive while the server's training thread runs. In the
single-threaded WebAssembly build those locks are uncontended no-ops.


## Usage Guide

### CLI

`./bin/blackjack_ai` opens a numbered menu: play a hand yourself, and for each
agent train, evaluate, watch it play, view its statistics, or load and save its
Q-table. The last options compare the two agents head to head.

The committed Q-tables load automatically on startup, so evaluation works
without training first. To train from scratch, choose *Train Q-Learning AI*,
enter an episode count (the committed tables used 2,000,000), then save to
`data/q_table.csv`.

## Configuration & Tuning

### Modifying Hyperparameters

Hyperparameters are the constructor arguments at the top of `main()` in
`src/main.cpp` (the web server mirrors these constants at the top of
`src/server/main.cpp`):

```cpp
// Q-Learning: (alpha floor, gamma, epsilon start)
QLearningAI qLearningAI(0.01, 1.0, 1.0);

// Monte Carlo: (epsilon, gamma)
MonteCarloAI monteCarloAI(0.1, 1.0);
```

### Why these values

Three of these are not free parameters — they follow from the problem:

- **`gamma = 1.0`.** Blackjack is episodic and undiscounted: a hand's payout is
  worth the same whether it took one hit or four. Discounting systematically
  undervalues outcomes reached after several draws, which biases the agent
  toward standing early.

- **`alpha` is a floor, not a constant rate.** The step size is
  `max(alpha, 1/visits^0.7)`, decayed per state-action pair. A constant rate
  makes `Q(s,a)` an exponential moving average over roughly the last `1/alpha`
  hands — at `alpha = 0.1`, about ten. With ±1 rewards those estimates never
  tighten below a spread of several tenths, which is wider than the gap between
  the two actions in the states that matter, so the greedy policy ends up
  picking by noise. The exponent 0.7 satisfies Robbins-Monro while still
  tracking the moving bootstrap target. Monte Carlo's exact `1/N` sample mean is
  the same idea.

- **No reward shaping.** The only reward is the terminal payout. An earlier
  version applied `-0.01` per hit; that is not potential-based, so it changes
  the optimal policy rather than merely speeding up learning — a direct tax on
  hitting, in a game where correct play often requires it.

Epsilon floors differ by algorithm on purpose: Q-learning holds at 0.05 since it
is off-policy and extra exploration only buys state coverage, while Monte Carlo
holds at 0.01 because epsilon-greedy MC control is on-policy and converges to
the best epsilon-*soft* policy — more exploration there means a worse greedy
policy to extract.

### Recommended Settings

| Use Case | Alpha floor | Gamma | Epsilon Start | Episodes |
|----------|-------------|-------|---------------|----------|
| Quick look | 0.01 | 1.0 | 1.0 | 200,000 |
| Standard | 0.01 | 1.0 | 1.0 | 2,000,000 |
| Rare-state coverage | 0.005 | 1.0 | 1.0 | 5,000,000 |

Evaluate with at least 500,000 hands. The 95% interval on average reward is
roughly ±0.003 at that size, and the differences worth detecting are ~0.01.

### Game Rules

Current implementation:
- Dealer stands on all 17s, **including soft 17** (`Dealer::playTurn` hits while
  the total is under 17, and does not distinguish soft totals)
- Blackjack pays **3:2** — a natural returns a reward of `+1.5`
  (`Game::calculateReward`, which delegates to `rules::computeReward`)
- Single 52-card deck, reshuffled every hand
- No splitting or doubling down (future feature), so the action space is just
  `HIT` / `STAND`
- No reward shaping: an intermediate hit carries a reward of `0.0`, and the only
  reward is the terminal payout. An earlier version applied a `-0.01` per-hit
  penalty; it was removed because it biases the policy towards standing (see
  [Why these values](#why-these-values))

## Reading the Results

Evaluation reports win rate, win rate excluding pushes, and average reward per
hand (+1 win, −1 loss, 0 push, +1.5 for a natural). Judge a policy by average
reward, not win rate — see [Performance Metrics](#performance-metrics) for why
every reasonable policy lands near the same win rate.

At 10,000 hands the 95% interval on average reward is about ±0.02, wider than
the gap between the two agents, so a CLI comparison at that size mostly
measures noise. The web demo's *Compare* tab reports the interval alongside
each result.

### Pre-Trained Models

Pre-trained Q-tables are automatically loaded on startup from:
- `data/q_table.csv` (Q-Learning)
- `data/mc_q_table.csv` (Monte Carlo)

To use custom models, place CSV files in the `data/` directory before running.

