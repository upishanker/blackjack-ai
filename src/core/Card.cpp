//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Card.h"

Card::Card(Suit s, Rank r) : suit(s), rank(r) {
    switch (r) {
        case Rank::Jack:
        case Rank::Queen:
        case Rank::King:
            value = 10;
            break;
        case Rank::Ace:
            value = 11;
            break;
        default:
            value = static_cast<int>(r);
    }
}

int Card::getValue() const {
    return value;
}

std::string Card::getName() const {
    static const char* rankNames[] = {
        "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten",
        "Jack", "Queen", "King", "Ace"
    };
    static const char* suitNames[] = {"Hearts","Diamonds","Clubs","Spades"};
    return string(rankNames[static_cast<int>(rank) - 2]) + " of " + suitNames[static_cast<int>(suit)];
}