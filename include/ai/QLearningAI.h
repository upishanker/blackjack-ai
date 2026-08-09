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

    // Visits per (state, action). Drives the decaying learning rate, and is
    // worth having anyway as a confidence signal for analysis.
    std::unordered_map<State, std::array<int, 2>, StateHash> visitCounts;

    // Exponent on the 1/n learning-rate schedule. Must stay in (0.5, 1] for
    // Robbins-Monro convergence; lower means faster forgetting.
    static constexpr double LEARNING_RATE_EXPONENT = 0.7;

    // Hyperparameters
    double alpha;      // Minimum learning rate. The step size is
                       // max(alpha, 1/visits^0.7), so alpha is the floor that
                       // keeps the agent adaptive rather than a constant rate.
    double gamma;      // Discount factor (1.0 — blackjack is episodic and
                       // undiscounted: a hand's payout is worth the same
                       // whether it took one hit or four)
    double epsilon;    // Exploration rate (start 1.0 → decay to a 0.05 floor)

    // Training stats
    int episodeCount;
    double totalReward;

    // RNG
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;

public:
    QLearningAI(double alpha = 0.01, double gamma = 1.0, double epsilon = 1.0);

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
    int getVisitCount(const State& state, Action action) const;
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