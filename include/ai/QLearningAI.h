//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_QLEARNINGAI_H
#define BLACKJACK_AI_QLEARNINGAI_H

#include "AITypes.h"
#include <unordered_map>
#include <array>
#include <string>
#include <random>


class QLearningAI {
private:
    // Q-table: State → [Q(s,HIT), Q(s,STAND)]
    std::unordered_map<State, std::array<double, 2>, StateHash> qTable;

    // Hyperparameters
    double alpha;      // Learning rate (0.1)
    double gamma;      // Discount factor (0.9)
    double epsilon;    // Exploration rate (start 1.0 → decay to 0.05)

    // Training stats
    int episodeCount;
    double totalReward;

    // RNG
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

public:
    QLearningAI(double alpha = 0.1, double gamma = 0.9, double epsilon = 1.0);

    // ε-greedy action selection
    Action chooseAction(const State& state);

    // Greedy action (for evaluation/play)
    Action getBestAction(const State& state) const;

    // Q-learning update: Q(s,a) ← Q(s,a) + α[r + γ max Q(s',a') - Q(s,a)]
    void updateQValue(const State& state,
                      Action action,
                      double reward,
                      const State& nextState,
                      bool isTerminal);

    // Persistence
    void saveQTable(const std::string& filename) const;
    void loadQTable(const std::string& filename);

    // Getters/Setters
    double getQValue(const State& state, Action action) const;
    void setEpsilon(double newEpsilon);
    void setAlpha(double newAlpha);
    void setGamma(double newGamma);

    // Training utilities
    void decayEpsilon(double decayRate = 0.9995);  // Gradual exploration decay
    void recordEpisode(double reward);
    void printStats() const;
    int getEpisodeCount() const { return episodeCount; }
};

#endif //BLACKJACK_AI_QLEARNINGAI_H