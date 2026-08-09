//
// Created for the web demo; mirrors Game::playAIEpisode one step at a time.
//

#include "../../include/core/HandSession.h"

HandSession::HandSession()
    : player("AI", false), settled(false), finalReward(0.0) {
    player.addCard(deck.dealCard());
    player.addCard(deck.dealCard());
    dealer.addCard(deck.dealCard());
    dealer.addCard(deck.dealCard());
}

State HandSession::state() const {
    // dealerHand[0] is the upcard the agent conditions on, matching
    // Game::getAIState. (Dealer::showHand renders index 0 as the hidden card,
    // but the state encoding has always used it -- kept identical here so the
    // demo and the CLI agree.)
    return rules::computeState(player.getHand(), dealer.getHand()[0]);
}

bool HandSession::playerCanAct() const {
    return !settled && !player.isBusted() && player.getHandValue() < 21;
}

void HandSession::hit() {
    if (settled) {
        return;
    }
    player.addCard(deck.dealCard());

    // Matches playAIEpisode: a bust ends the hand at -1.0 without the dealer
    // ever drawing.
    if (player.isBusted()) {
        settled = true;
        finalReward = -1.0;
    }
}

void HandSession::stand() {
    if (settled) {
        return;
    }
    dealer.playTurn(deck);
    settle();
}

void HandSession::settle() {
    settled = true;
    finalReward = rules::computeReward(player, dealer);
}
