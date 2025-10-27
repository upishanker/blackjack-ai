//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_DECK_H
#define BLACKJACK_AI_DECK_H
#include "Card.h"
#include <vector>
#include <algorithm>
#include <random>

using namespace std;


class Deck {
private:
    std::vector<Card> cards;
    int currentCardIndex;
public:
    Deck();

    void generateDeck();
    void shuffleDeck();
    Card dealCard();

    bool isEmpty() const;


    const vector<Card>& getCards() const { return cards; }


};


#endif //BLACKJACK_AI_DECK_H