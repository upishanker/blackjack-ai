# 🎰 Blackjack AI:

A comprehensive Blackjack simulation engine featuring two competing reinforcement learning agents: **Q-Learning** and **Monte Carlo**. Watch AI agents learn optimal Blackjack strategy through 100,000+ training episodes.

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

## 🎯 Project Overview

This project implements a fully-functional Blackjack game with two AI agents that learn to play through reinforcement learning:

- **Q-Learning AI**: Uses temporal difference learning for fast convergence
- **Monte Carlo AI**: Uses episodic learning with complete episode returns

Both agents learn over the classic Sutton & Barto state representation —
`(player sum, dealer upcard, usable ace)` — with a two-action space, `HIT` and
`STAND`.

There are two front ends over the same engine: the interactive CLI, and a
**browser demo** that shows the learned policy as a strategy grid, plays hands
one decision at a time with live Q-values, and trains the agent while you watch
the table converge. See [Web Demo](#-web-demo).

## ✨ Features

- 🃏 **Complete Blackjack Engine**: Full game logic with dealer AI following standard casino rules
- 🤖 **Dual AI Implementations**: Q-Learning and Monte Carlo reinforcement learning agents
- 📊 **Training Pipeline**: Configurable training with progress tracking and statistics
- 💾 **Q-Table Persistence**: Save and load trained models via CSV files
- 🎮 **Interactive Menu**: Play as human or watch/train AI agents
- 🌐 **Web Demo**: Policy heat map, step-by-step play, live training curves, agent comparison
- 📈 **Performance Analytics**: Comprehensive evaluation and comparison tools
- ⚙️ **Hyperparameter Tuning**: Adjustable learning rates, discount factors, and exploration rates

## 🏆 Performance Metrics

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

## 🚀 Getting Started

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.20 or higher — *optional*, see the CMake-free path below
- **Emscripten** — only to rebuild the hosted WebAssembly bundle
- **Make**

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

## 🌐 Web Demo

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


## 🎮 Usage Guide

### Main Menu Options

```
1.  Play Blackjack (Human)           - Play against the dealer yourself
2.  Train Q-Learning AI              - Train the Q-Learning agent
3.  Evaluate Q-Learning AI           - Test Q-Learning performance
4.  Watch Q-Learning AI Play         - Observe 10 demo games
5.  Load Q-Table                     - Load pre-trained Q-Learning model
6.  Save Q-Table                     - Save Q-Learning model
7.  View Q-Learning Statistics       - Display training metrics
8.  Train Monte Carlo AI             - Train the Monte Carlo agent
9.  Evaluate Monte Carlo AI          - Test Monte Carlo performance
10. Load Monte Carlo Q-Table         - Load pre-trained Monte Carlo model
11. Save Monte Carlo Q-Table         - Save Monte Carlo model
12. View Monte Carlo Statistics      - Display training metrics
13. Compare Q-Learning vs Monte Carlo - Head-to-head AI comparison
14. Exit                             - Quit the program
```

### Quick Start: Training Your First AI

1. **Launch the program**
```bash
./blackjack_ai
```

2. **Select option 2** (Train Q-Learning AI)

3. **Enter training parameters**
```
Enter number of training episodes: 100000
Show progress during training? (y/n): y
```

4. **Wait for training to complete** (~30 seconds for 100K episodes)

5. **Save the trained model** (option 6)
```
Enter filename to save: data/q_table.csv
```

6. **Evaluate performance** (option 3)
```
Enter number of evaluation games: 10000
```

### Example Training Session

```bash
$ ./blackjack_ai

=========================
   Blackjack with AI
=========================
Enter choice: 2

Enter number of training episodes: 50000
Show progress during training? (y/n): y

=== Training Q-Learning AI ===
Episode 10000/50000 | Wins: 3384 | Losses: 6104 | Avg Reward: -0.250
Episode 20000/50000 | Wins: 7158 | Losses: 11702 | Avg Reward: -0.204
...
Episode 50000/50000 | Wins: 19305 | Losses: 27378 | Avg Reward: -0.138

=== Training Complete ===
Total Wins: 19305 (38.61%)
Total Losses: 27378 (54.76%)
Average Reward: -0.138
```

## ⚙️ Configuration & Tuning

### Modifying Hyperparameters

Edit `src/main.cpp` lines 340-341 (the web server mirrors these constants at the
top of `src/server/main.cpp`):

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

## 📊 Understanding the Output

### Training Statistics

```
Episode 100000/100000 | Wins: 40711 | Losses: 52009 | Pushes: 7280 | Avg Reward: -0.089
```

- **Wins**: Games where AI beat the dealer
- **Losses**: Games where AI lost to dealer
- **Pushes**: Ties (same hand value)
- **Avg Reward**: Mean reward per episode (+1 win, -1 loss, 0 push)

### Evaluation Metrics

```
Win Rate: 43.19%           # Percentage of all games won
Win Rate (vs losses): 47.07%  # Win percentage excluding pushes
Average Reward: -0.0318    # Expected value per game
```

**Interpretation:**
- Win rate **>42%** = Good performance
- Win rate **>45%** = Excellent performance
- Win rate **>48%** = Near-optimal (theoretical limit ~49%)

## 🔬 Advanced Usage

### Comparing AI Performance

Use option 13 to run a comprehensive comparison:

```bash
Enter choice: 13
Enter number of evaluation games: 10000

========================================
           COMPARISON RESULTS
========================================
Metric                  | Q-Learning    | Monte Carlo   | Winner
------------------------|---------------|---------------|--------
Win Rate                |        42.64% |        42.05% | Q-Learn
Loss Rate               |        48.98% |        50.45% | Q-Learn
Avg Reward              |         -0.04 |         -0.06 | Q-Learn

Overall Score: Q-Learning 4 - 0 Monte Carlo
Winner: Q-Learning AI! 🏆
```

### Loading Pre-Trained Models

Pre-trained Q-tables are automatically loaded on startup from:
- `data/q_table.csv` (Q-Learning)
- `data/mc_q_table.csv` (Monte Carlo)

To use custom models, place CSV files in the `data/` directory before running.

