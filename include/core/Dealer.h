//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_DEALER_H
#define BLACKJACK_AI_DEALER_H

#include "Player.h"
#include "Deck.h"
#include <iostream>

class Dealer: public Player {
public:
    Dealer() : Player("Dealer", true) {};

    void playTurn(Deck& deck);
    void showHand(bool hideFirstCard = true) const;
    void revealHand() const;

};


#endif //BLACKJACK_AI_DEALER_H