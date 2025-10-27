//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_AITYPES_H
#define BLACKJACK_AI_AITYPES_H

#include <cstddef>

// Action enum shared by both AI implementations
enum class Action { HIT = 0, STAND = 1 };

// State representation for Blackjack
struct State {
    int playerSum;
    int dealerUpcard;
    bool usableAce;

    bool operator==(const State& other) const {
        return playerSum == other.playerSum &&
               dealerUpcard == other.dealerUpcard &&
               usableAce == other.usableAce;
    }
};

// Hash function for State (for unordered_map)
struct StateHash {
    size_t operator()(const State& s) const {
        return ((s.playerSum - 4) * 20 + (s.dealerUpcard - 2) * 2 + s.usableAce);
    }
};

#endif //BLACKJACK_AI_AITYPES_H