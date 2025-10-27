//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Dealer.h"

void Dealer::playTurn(Deck &deck) {
    while (getHandValue() < 17) {
        addCard(deck.dealCard());
    }
}
void Dealer::showHand(bool hideFirstCard) const {
    std::cout << "Dealer's hand: ";
    for (size_t i = 0; i < hand.size(); ++i) {
        if (hideFirstCard && i == 0) {
            std::cout << "[Hidden] ";
        } else {
            std::cout << hand[i].getName() << " ";
        }
    }
    if (hideFirstCard)
        std::cout << "\n(One card hidden)";
    std::cout << std::endl;
}

void Dealer::revealHand() const {
    std::cout << "Dealer reveals hand: ";
    for (const auto& card : hand) {
        std::cout << card.getName() << " ";
    }
    std::cout << "(Total: " << getHandValue() << ")" << std::endl;
}

