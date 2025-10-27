//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/ai/MonteCarloAI.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unordered_set>

MonteCarloAI::MonteCarloAI(double epsilon, double gamma)
    : epsilon(epsilon), gamma(gamma),
      episodeCount(0), totalReward(0.0),
      rng(std::random_device{}()), dist(0.0, 1.0) {

    currentEpisode.reserve(20); // Pre-allocate for typical episode length
}

void MonteCarloAI::startEpisode() {
    currentEpisode.clear();
}

void MonteCarloAI::recordStep(const State& state, Action action, double reward) {
    Step step;
    step.state = state;
    step.action = action;
    step.reward = reward;
    currentEpisode.push_back(step);
}

void MonteCarloAI::endEpisode(double finalReward) {
    // Add final reward to last step if episode exists
    if (!currentEpisode.empty()) {
        currentEpisode.back().reward = finalReward;
    }

    // Update Q-values using the episode
    updateFromEpisode();

    // Record statistics
    episodeCount++;
    totalReward += finalReward;

    // Clear episode for next run
    currentEpisode.clear();
}

void MonteCarloAI::updateFromEpisode() {
    if (currentEpisode.empty()) {
        return;
    }

    // Track which state-action pairs we've already seen (first-visit MC)
    std::unordered_set<StateAction, StateActionHash> visited;

    // Calculate returns backwards through the episode
    double G = 0.0; // Return (cumulative discounted reward)

    // Iterate backwards through episode
    for (int t = static_cast<int>(currentEpisode.size()) - 1; t >= 0; --t) {
        const Step& step = currentEpisode[t];

        // Update return with discounted reward
        G = step.reward + gamma * G;

        // Create state-action pair
        StateAction sa;
        sa.state = step.state;
        sa.action = step.action;

        // First-visit MC: only update if this is the first occurrence
        if (visited.find(sa) == visited.end()) {
            visited.insert(sa);

            // Update returns sum
            returns[sa] += G;

            // Increment visit count
            visitCounts[sa]++;

            // Update Q-value using incremental mean
            // Q(s,a) ← Q(s,a) + (1/N) * [G - Q(s,a)]
            int N = visitCounts[sa];
            double oldQ = qTable[step.state][static_cast<int>(step.action)];
            double newQ = oldQ + (1.0 / N) * (G - oldQ);

            qTable[step.state][static_cast<int>(step.action)] = newQ;
        }
    }
}

Action MonteCarloAI::chooseAction(const State& state) {
    // ε-greedy policy
    if (dist(rng) < epsilon) {
        // Explore: random action
        return (dist(rng) < 0.5) ? Action::HIT : Action::STAND;
    } else {
        // Exploit: best known action
        return getBestAction(state);
    }
}

Action MonteCarloAI::getBestAction(const State& state) const {
    // If state not in Q-table, use heuristic
    if (qTable.find(state) == qTable.end()) {
        // Basic heuristic: hit if < 17, stand otherwise
        return (state.playerSum < 17) ? Action::HIT : Action::STAND;
    }

    const auto& qValues = qTable.at(state);

    // Return action with highest Q-value
    return (qValues[static_cast<int>(Action::HIT)] > qValues[static_cast<int>(Action::STAND)])
           ? Action::HIT : Action::STAND;
}

void MonteCarloAI::saveQTable(const std::string& filename) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }

    // Write header
    file << "playerSum,dealerUpcard,usableAce,action,qValue,visitCount\n";

    // Write Q-table entries
    for (const auto& [state, qValues] : qTable) {
        for (int a = 0; a < 2; ++a) {
            StateAction sa;
            sa.state = state;
            sa.action = static_cast<Action>(a);

            int visits = 0;
            if (visitCounts.find(sa) != visitCounts.end()) {
                visits = visitCounts.at(sa);
            }

            file << state.playerSum << ","
                 << state.dealerUpcard << ","
                 << state.usableAce << ","
                 << a << ","
                 << std::fixed << std::setprecision(6) << qValues[a] << ","
                 << visits << "\n";
        }
    }

    file.close();
    std::cout << "Monte Carlo Q-table saved to " << filename
              << " (" << qTable.size() << " states)\n";
}

void MonteCarloAI::loadQTable(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Warning: Could not open file " << filename << " for reading.\n";
        std::cerr << "Starting with empty Q-table.\n";
        return;
    }

    qTable.clear();
    returns.clear();
    visitCounts.clear();

    std::string line;
    std::getline(file, line); // Skip header

    int loadedStates = 0;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        State state;
        int action;
        double qValue;
        int visits;

        // Parse CSV: playerSum,dealerUpcard,usableAce,action,qValue,visitCount
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

        std::getline(ss, token, ',');
        visits = std::stoi(token);

        qTable[state][action] = qValue;

        StateAction sa;
        sa.state = state;
        sa.action = static_cast<Action>(action);
        visitCounts[sa] = visits;

        // Reconstruct returns sum from Q-value and visit count
        // Since Q = sum(returns) / N, then sum(returns) = Q * N
        returns[sa] = qValue * visits;

        loadedStates++;
    }

    file.close();
    std::cout << "Monte Carlo Q-table loaded from " << filename
              << " (" << loadedStates << " entries)\n";
}

double MonteCarloAI::getQValue(const State& state, Action action) const {
    if (qTable.find(state) == qTable.end()) {
        return 0.0;
    }
    return qTable.at(state)[static_cast<int>(action)];
}

void MonteCarloAI::setEpsilon(double newEpsilon) {
    epsilon = std::max(0.0, std::min(1.0, newEpsilon));
}

void MonteCarloAI::setGamma(double newGamma) {
    gamma = std::max(0.0, std::min(1.0, newGamma));
}

void MonteCarloAI::decayEpsilon(double decayRate) {
    epsilon = std::max(0.01, epsilon * decayRate); // Don't go below 1% exploration
}

int MonteCarloAI::getVisitCount(const State& state, Action action) const {
    StateAction sa;
    sa.state = state;
    sa.action = action;

    if (visitCounts.find(sa) == visitCounts.end()) {
        return 0;
    }
    return visitCounts.at(sa);
}

void MonteCarloAI::printStats() const {
    std::cout << "\n=== Monte Carlo AI Statistics ===\n";
    std::cout << "Episodes trained: " << episodeCount << "\n";
    std::cout << "Total reward: " << std::fixed << std::setprecision(2) << totalReward << "\n";

    if (episodeCount > 0) {
        std::cout << "Average reward: " << (totalReward / episodeCount) << "\n";
    }

    std::cout << "Current epsilon: " << std::fixed << std::setprecision(4) << epsilon << "\n";
    std::cout << "Q-table size: " << qTable.size() << " states\n";
    std::cout << "Gamma (discount): " << gamma << "\n";

    // Calculate total visits
    int totalVisits = 0;
    for (const auto& [sa, count] : visitCounts) {
        totalVisits += count;
    }
    std::cout << "Total state-action visits: " << totalVisits << "\n";

    if (!visitCounts.empty()) {
        std::cout << "Avg visits per state-action: "
                  << (totalVisits / static_cast<double>(visitCounts.size())) << "\n";
    }

    std::cout << "=================================\n\n";
}