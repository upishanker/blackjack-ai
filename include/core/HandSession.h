//
// A single blackjack hand you can drive one action at a time.
//
// Game::playAIEpisode runs a whole hand to completion behind a local Player and
// returns only the final reward, which is fine for training but useless for a
// UI that wants to render each decision. HandSession exposes the same hand
// incrementally: deal, inspect, act, inspect again.
//
// It follows exactly the flow in Game::playAIEpisode -- act while not busted
// and below 21, standing hands the turn to the dealer -- and shares the state
// encoding and payout via rules::. It never touches stdin, unlike Game's
// constructor.
//

#ifndef BLACKJACK_AI_HANDSESSION_H
#define BLACKJACK_AI_HANDSESSION_H

#include "Deck.h"
#include "Dealer.h"
#include "Player.h"
#include "Rules.h"
#include "../ai/AITypes.h"
#include <vector>

class HandSession {
private:
    Deck deck;
    Dealer dealer;
    Player player;
    bool settled;
    double finalReward;

    void settle();

public:
    HandSession();

    // The tuple the agent actually sees.
    State state() const;

    // True while the player may still act: not busted and below 21.
    bool playerCanAct() const;

    // Draw one card. Busting settles the hand immediately.
    void hit();

    // End the player's turn; the dealer plays out and the hand settles.
    void stand();

    bool finished() const { return settled; }

    // Terminal payout. Only meaningful once finished().
    double reward() const { return finalReward; }

    const std::vector<Card>& playerHand() const { return player.getHand(); }
    const std::vector<Card>& dealerHand() const { return dealer.getHand(); }

    int playerValue() const { return player.getHandValue(); }
    int dealerValue() const { return dealer.getHandValue(); }
    bool playerBusted() const { return player.isBusted(); }
    bool dealerBusted() const { return dealer.isBusted(); }
    bool playerHasBlackjack() const { return player.hasBlackjack(); }
    bool dealerHasBlackjack() const { return dealer.hasBlackjack(); }
};

#endif //BLACKJACK_AI_HANDSESSION_H
