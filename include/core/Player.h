//
// Created by Upi Shanker on 10/26/2025.
//

#ifndef BLACKJACK_AI_PLAYER_H
#define BLACKJACK_AI_PLAYER_H

#include <vector>
#include <string>
#include "Card.h"

class Player {
private:
    std::string name;
    bool isDealer;
protected:
    std::vector<Card> hand;

public:
    Player(const std::string& playerName, bool dealer = false);

    void addCard(const Card& card);
    int getHandValue() const;
    void clearHand();

    bool isBusted() const;
    bool hasBlackjack() const;

    const std::vector<Card>& getHand() const { return hand; }
    std::string getName() const { return name; }
    bool getIsDealer() const { return isDealer; }

    void showHand(bool hideFirstCard = false) const; // For GUI/text display

    virtual ~Player() = default;
};

#endif //BLACKJACK_AI_PLAYER_H
