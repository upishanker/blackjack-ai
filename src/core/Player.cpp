//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Player.h"
#include <iostream>

Player::Player(const std::string& playerName, bool dealer)
    : name(playerName), isDealer(dealer) {}

void Player::addCard(const Card& card) {
    hand.push_back(card);
}

int Player::getHandValue() const {
    int value = 0;
    int aceCount = 0;

    for (const Card& card : hand) {
        value += card.getValue();
        if (card.getRank() == Rank::Ace) {
            aceCount++;
        }
    }

    // Adjust Ace values (11 → 1) if needed
    while (value > 21 && aceCount > 0) {
        value -= 10;
        aceCount--;
    }

    return value;
}

void Player::clearHand() {
    hand.clear();
}

bool Player::isBusted() const {
    return getHandValue() > 21;
}

bool Player::hasBlackjack() const {
    return getHandValue() == 21 && hand.size() == 2;
}

void Player::showHand(bool hideFirstCard) const {
    for (size_t i = 0; i < hand.size(); ++i) {
        if (hideFirstCard && i == 0) {
            std::cout << "[Hidden Card]" << std::endl;
        } else {
            std::cout << hand[i].getName() << std::endl;
        }
    }
}
