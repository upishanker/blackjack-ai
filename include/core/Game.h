//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_GAME_H
#define BLACKJACK_AI_GAME_H

#include "Deck.h"
#include "Player.h"
#include "Dealer.h"
#include <vector>
#include <string>

class Game {
private:
    Deck deck;
    Dealer dealer;
    std::vector<Player> players;
    int numPlayers;

public:
    explicit Game(int numPlayers = 1);

    void initializeGame();

    void dealInitialCards();

    void playerTurn(Player& player);

    void dealerTurn();

    void determineWinners();

    void resetGame();

    void playRound();

    void playAI();

    void displayGameState(bool hideDealerCard = true) const;
};

#endif //BLACKJACK_AI_GAME_H
