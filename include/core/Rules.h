//
// Shared blackjack rules: state encoding and payout.
//
// These were originally private members of Game (Game::getAIState and
// Game::calculateReward). They live here so that anything driving a hand --
// the CLI's Game, or HandSession behind the web demo -- observes exactly the
// same state representation and the same payouts. Game now delegates to them.
//

#ifndef BLACKJACK_AI_RULES_H
#define BLACKJACK_AI_RULES_H

#include "Card.h"
#include "Player.h"
#include "Dealer.h"
#include "../ai/AITypes.h"
#include <vector>

namespace rules {

// Encode a hand into the agent's state tuple {playerSum, dealerUpcard, usableAce}.
// Aces are demoted 11 -> 1 while the total busts; usableAce means an ace is
// still being counted as 11.
State computeState(const std::vector<Card>& playerHand, const Card& dealerUpcard);

// Terminal payout from the player's perspective:
//   +1.5 blackjack, +1.0 win, 0.0 push, -1.0 loss or bust.
double computeReward(const Player& player, const Dealer& dealer);

// Published basic strategy, restricted to this engine's rules: hit/stand only
// (no double, split or surrender) against a dealer who stands on all 17s.
//
// This is the benchmark the agents are measured against, not something they
// learn from. It is defined here rather than in the browser so the policy grid
// overlay and the baseline simulation read from one source.
Action basicStrategy(const State& state);

} // namespace rules

#endif //BLACKJACK_AI_RULES_H
