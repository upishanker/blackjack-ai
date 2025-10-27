# 🎰 Blackjack AI:

A comprehensive Blackjack simulation engine featuring two competing reinforcement learning agents: **Q-Learning** and **Monte Carlo**. Watch AI agents learn optimal Blackjack strategy through 100,000+ training episodes.

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

## 🎯 Project Overview

This project implements a fully-functional Blackjack game with two AI agents that learn to play through reinforcement learning:

- **Q-Learning AI**: Uses temporal difference learning for fast convergence (46.5% win rate)
- **Monte Carlo AI**: Uses episodic learning with complete episode returns (45.5% win rate)

Both agents achieve near-optimal play after training, with Q-Learning demonstrating superior performance in head-to-head comparisons.

## ✨ Features

- 🃏 **Complete Blackjack Engine**: Full game logic with dealer AI following standard casino rules
- 🤖 **Dual AI Implementations**: Q-Learning and Monte Carlo reinforcement learning agents
- 📊 **Training Pipeline**: Configurable training with progress tracking and statistics
- 💾 **Q-Table Persistence**: Save and load trained models via CSV files
- 🎮 **Interactive Menu**: Play as human or watch/train AI agents
- 📈 **Performance Analytics**: Comprehensive evaluation and comparison tools
- ⚙️ **Hyperparameter Tuning**: Adjustable learning rates, discount factors, and exploration rates

## 🏆 Performance Metrics

| Metric | Q-Learning | Monte Carlo |
|--------|------------|-------------|
| Win Rate | 43.2% | 42.3% |
| Win Rate (vs Losses) | 46.5% | 45.8% |
| Q-Table Size | 280 states | 260 states |
| Training Episodes | 100,000 | 100,000 |

## 🚀 Getting Started

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.10 or higher
- **Make** (or Ninja)

### Installation

1. **Clone the repository**
```bash
git clone https://github.com/upishanker/blackjack-ai.git
cd blackjack-ai
```

2. **Create build directory**
```bash
mkdir build
cd build
```

3. **Build the project**
```bash
cmake ..
make -j$(nproc)
```

4. **Run the executable**
```bash
./blackjack_ai
```


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

Edit `src/main.cpp` line 341-342:

```cpp
// Q-Learning: (alpha, gamma, epsilon)
QLearningAI qLearningAI(0.1, 0.9, 1.0);

// Monte Carlo: (epsilon, gamma)
MonteCarloAI monteCarloAI(0.1, 1.0);
```

### Recommended Settings

| Use Case | Alpha | Gamma | Epsilon Start | Episodes |
|----------|-------|-------|---------------|----------|
| Fast Training | 0.2 | 0.9 | 1.0 | 50,000 |
| Stable Learning | 0.1 | 0.9 | 1.0 | 100,000 |
| Fine-Tuning | 0.05 | 0.95 | 0.5 | 200,000 |

### Game Rules

Current implementation follows standard casino rules:
- Dealer stands on 17+
- Dealer hits on soft 17
- Blackjack pays 1:1 (can be modified to 3:2)
- No splitting or doubling down (future feature)

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

