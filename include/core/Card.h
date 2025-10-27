//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_CARD_H
#define BLACKJACK_AI_CARD_H

#include <string>
using namespace std;

enum class Suit { Hearts, Diamonds, Clubs, Spades };
enum class Rank { Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace };

class Card {
private:
    Suit suit;
    Rank rank;
    int value;

public:
    Card(Suit s, Rank r);

    int getValue() const;
    string getName() const;
    Rank getRank() const { return rank; }
    Suit getSuit() const { return suit; }

};


#endif //BLACKJACK_AI_CARD_H