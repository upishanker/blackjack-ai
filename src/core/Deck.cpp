//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Deck.h"
#include <random>
#include <algorithm>
#include <stdexcept>

Deck::Deck() {
    generateDeck();
    shuffleDeck();
    currentCardIndex = 0;
}

void Deck::generateDeck() {
    cards.clear();
    cards.reserve(52);

    for (int s = 0; s < 4; s++) {
        for (int r = static_cast<int>(Rank::Two); r <= static_cast<int>(Rank::Ace); r++) {
            cards.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }

    currentCardIndex = 0;
}

void Deck::shuffleDeck() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cards.begin(), cards.end(), gen);
    currentCardIndex = 0;
}
Card Deck::dealCard() {
    if (isEmpty()) {
        throw std::runtime_error("No more cards in the deck");
    }
    return cards[currentCardIndex++];
}

bool Deck::isEmpty() const {
    return currentCardIndex >= static_cast<int>(cards.size());
}