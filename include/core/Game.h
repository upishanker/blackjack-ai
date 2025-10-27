//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_GAME_H
#define BLACKJACK_AI_GAME_H

#include "Deck.h"
#include "Player.h"
#include "Dealer.h"
#include "../ai/QLearningAI.h"
#include <vector>
#include <string>

class Game {
private:
    Deck deck;
    Dealer dealer;
    std::vector<Player> players;
    int numPlayers;

    // Helper: Convert game state to AI state representation
    State getAIState(const Player& player) const;

    // Helper: Calculate reward for AI
    double calculateReward(const Player& player) const;

public:
    explicit Game(int numPlayers = 1);

    void initializeGame();
    void dealInitialCards();
    void playerTurn(Player& player);
    void dealerTurn();
    void determineWinners();
    void resetGame();
    void displayGameState(bool hideDealerCard = true) const;

    // Human gameplay
    void playRound();

    // AI Training
    void trainAI(QLearningAI& ai, int numEpisodes, bool verbose = false);

    // AI vs Dealer (single episode for training)
    double playAIEpisode(QLearningAI& ai, bool training = true);

    // Evaluate AI performance
    void evaluateAI(QLearningAI& ai, int numGames);
};

#endif //BLACKJACK_AI_GAME_H