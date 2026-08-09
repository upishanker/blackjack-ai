//
// Bodies moved verbatim from Game::getAIState / Game::calculateReward.
//

#include "../../include/core/Rules.h"

namespace rules {

State computeState(const std::vector<Card>& playerHand, const Card& dealerUpcard) {
    State state;

    state.dealerUpcard = dealerUpcard.getValue();

    // Calculate player hand value with proper ace handling
    int value = 0;
    int aceCount = 0;

    for (const Card& card : playerHand) {
        value += card.getValue();
        if (card.getRank() == Rank::Ace) {
            aceCount++;
        }
    }

    // Adjust for aces (same logic as Player::getHandValue())
    while (value > 21 && aceCount > 0) {
        value -= 10;
        aceCount--;
    }

    state.playerSum = value;

    // Usable ace = we have an ace AND it's currently counted as 11
    state.usableAce = (aceCount > 0 && value <= 21);

    return state;
}

double computeReward(const Player& player, const Dealer& dealer) {
    int playerValue = player.getHandValue();
    int dealerValue = dealer.getHandValue();

    if (player.hasBlackjack() && !dealer.hasBlackjack()) {
        return 1.5;  // Blackjack pays 3:2
    }

    // Player busted
    if (player.isBusted()) {
        return -1.0;
    }

    // Dealer busted, player didn't
    if (dealer.isBusted()) {
        return 1.0;
    }

    // Compare values
    if (playerValue > dealerValue) {
        return 1.0;  // Win
    } else if (playerValue < dealerValue) {
        return -1.0; // Loss
    } else {
        return 0.0;  // Push
    }
}

Action basicStrategy(const State& s) {
    if (s.usableAce) {
        // Soft hands: an ace counted as 11 can absorb one bad draw, so these
        // stay live much longer than the equivalent hard total.
        if (s.playerSum >= 19) return Action::STAND;
        if (s.playerSum == 18) return s.dealerUpcard <= 8 ? Action::STAND : Action::HIT;
        return Action::HIT;
    }
    if (s.playerSum >= 17) return Action::STAND;
    // 13-16 are the "stiff" totals: stand only against a dealer weak enough to
    // bust on their own (2-6), otherwise take the risk.
    if (s.playerSum >= 13) return s.dealerUpcard <= 6 ? Action::STAND : Action::HIT;
    if (s.playerSum == 12) {
        return (s.dealerUpcard >= 4 && s.dealerUpcard <= 6) ? Action::STAND : Action::HIT;
    }
    return Action::HIT;   // 11 or below cannot bust
}

} // namespace rules
