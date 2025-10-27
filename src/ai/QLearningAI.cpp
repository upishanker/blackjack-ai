//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/ai/QLearningAI.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

QLearningAI::QLearningAI(double alpha, double gamma, double epsilon)
    : alpha(alpha), gamma(gamma), epsilon(epsilon),
      episodeCount(0), totalReward(0.0),
      rng(std::random_device{}()), dist(0.0, 1.0) {

    // Initialize Q-table with zeros (optional - unordered_map defaults to 0)
    // We'll use lazy initialization: Q-values default to 0.0 on first access
}

Action QLearningAI::chooseAction(const State& state) {
    // ε-greedy policy
    if (dist(rng) < epsilon) {
        // Explore: random action
        return (dist(rng) < 0.5) ? Action::HIT : Action::STAND;
    } else {
        // Exploit: best known action
        return getBestAction(state);
    }
}

Action QLearningAI::getBestAction(const State& state) const {
    // If state not in Q-table, default to STAND (conservative)
    if (qTable.find(state) == qTable.end()) {
        // Basic heuristic: hit if < 17, stand otherwise
        return (state.playerSum < 17) ? Action::HIT : Action::STAND;
    }

    const auto& qValues = qTable.at(state);

    // Return action with highest Q-value
    return (qValues[static_cast<int>(Action::HIT)] > qValues[static_cast<int>(Action::STAND)])
           ? Action::HIT : Action::STAND;
}

void QLearningAI::updateQValue(const State& state,
                                 Action action,
                                 double reward,
                                 const State& nextState,
                                 bool isTerminal) {

    // Get current Q-value (defaults to 0.0 if not in table)
    double currentQ = qTable[state][static_cast<int>(action)];

    double maxNextQ = 0.0;

    if (!isTerminal) {
        // Find max Q-value for next state
        if (qTable.find(nextState) != qTable.end()) {
            const auto& nextQValues = qTable[nextState];
            maxNextQ = std::max(nextQValues[static_cast<int>(Action::HIT)],
                               nextQValues[static_cast<int>(Action::STAND)]);
        }
    }
    // If terminal, maxNextQ stays 0.0

    // Q-learning update: Q(s,a) ← Q(s,a) + α[r + γ max Q(s',a') - Q(s,a)]
    double newQ = currentQ + alpha * (reward + gamma * maxNextQ - currentQ);

    qTable[state][static_cast<int>(action)] = newQ;
}

void QLearningAI::saveQTable(const std::string& filename) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Write header
    file << "playerSum,dealerUpcard,usableAce,action,qValue\n";

    // Write Q-table entries
    for (const auto& [state, qValues] : qTable) {
        for (int a = 0; a < 2; ++a) {
            file << state.playerSum << ","
                 << state.dealerUpcard << ","
                 << state.usableAce << ","
                 << a << ","
                 << std::fixed << std::setprecision(6) << qValues[a] << "\n";
        }
    }

    file.close();
    std::cout << "Q-table saved to " << filename << " (" << qTable.size() << " states)\n";
}

void QLearningAI::loadQTable(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Warning: Could not open file " << filename << " for reading.\n";
        std::cerr << "Starting with empty Q-table.\n";
        return;
    }

    qTable.clear();

    std::string line;
    std::getline(file, line); // Skip header

    int loadedStates = 0;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        State state;
        int action;
        double qValue;

        // Parse CSV: playerSum,dealerUpcard,usableAce,action,qValue
        std::getline(ss, token, ',');
        state.playerSum = std::stoi(token);

        std::getline(ss, token, ',');
        state.dealerUpcard = std::stoi(token);

        std::getline(ss, token, ',');
        state.usableAce = (std::stoi(token) == 1);

        std::getline(ss, token, ',');
        action = std::stoi(token);

        std::getline(ss, token, ',');
        qValue = std::stod(token);

        qTable[state][action] = qValue;
        loadedStates++;
    }

    file.close();
    std::cout << "Q-table loaded from " << filename << " (" << loadedStates << " entries)\n";
}

double QLearningAI::getQValue(const State& state, Action action) const {
    if (qTable.find(state) == qTable.end()) {
        return 0.0;
    }
    return qTable.at(state)[static_cast<int>(action)];
}

void QLearningAI::setEpsilon(double newEpsilon) {
    epsilon = std::max(0.0, std::min(1.0, newEpsilon)); // Clamp to [0,1]
}

void QLearningAI::setAlpha(double newAlpha) {
    alpha = std::max(0.0, std::min(1.0, newAlpha));
}

void QLearningAI::setGamma(double newGamma) {
    gamma = std::max(0.0, std::min(1.0, newGamma));
}

void QLearningAI::decayEpsilon(double decayRate) {
    epsilon = std::max(0.01, epsilon * decayRate); // Don't go below 5% exploration
}

void QLearningAI::recordEpisode(double reward) {
    episodeCount++;
    totalReward += reward;
}

void QLearningAI::printStats() const {
    std::cout << "\n=== Q-Learning AI Statistics ===\n";
    std::cout << "Episodes trained: " << episodeCount << "\n";
    std::cout << "Total reward: " << std::fixed << std::setprecision(2) << totalReward << "\n";

    if (episodeCount > 0) {
        std::cout << "Average reward: " << (totalReward / episodeCount) << "\n";
    }

    std::cout << "Current epsilon: " << std::fixed << std::setprecision(4) << epsilon << "\n";
    std::cout << "Q-table size: " << qTable.size() << " states\n";
    std::cout << "Alpha (learning rate): " << alpha << "\n";
    std::cout << "Gamma (discount): " << gamma << "\n";
    std::cout << "================================\n\n";
}