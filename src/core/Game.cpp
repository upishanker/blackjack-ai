//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Game.h"
#include <iostream>
#include <limits>

using namespace std;

Game::Game(int numPlayers) : numPlayers(numPlayers) {
    players.reserve(numPlayers);
    for (int i = 0; i < numPlayers; ++i) {
        string name;
        cout << "Enter Player " << (i + 1) << "'s name: ";
        cin >> name;
        players.emplace_back(name, false);
    }
}

void Game::initializeGame() {
    deck = Deck();
    dealer.clearHand();
    for (auto& player : players) {
        player.clearHand();
    }
}

void Game::dealInitialCards() {
    for (int i = 0; i < 2; ++i) {
        for (auto& player : players) {
            player.addCard(deck.dealCard());
        }
        dealer.addCard(deck.dealCard());
    }
}

void Game::playerTurn(Player& player) {
    cout << "\n--- " << player.getName() << "'s Turn ---" << endl;
    while (true) {
        player.showHand();
        cout << "\nCurrent value: " << player.getHandValue() << endl;

        if (player.isBusted()) {
            cout << player.getName() << " busted!\n";
            break;
        }

        char choice;
        cout << "Hit or Stand? (h/s): ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input.\n";
            continue;
        }

        if (choice == 'h' || choice == 'H') {
            player.addCard(deck.dealCard());
        } else if (choice == 's' || choice == 'S') {
            cout << player.getName() << " stands.\n";
            break;
        } else {
            cout << "Invalid choice. Try again.\n";
        }
    }
}

void Game::dealerTurn() {
    cout << "\n--- Dealer's Turn ---" << endl;
    dealer.revealHand();
    dealer.playTurn(deck);
}

void Game::determineWinners() {
    cout << "\n--- Round Results ---\n";
    int dealerValue = dealer.getHandValue();
    bool dealerBusted = dealer.isBusted();

    for (auto& player : players) {
        int playerValue = player.getHandValue();
        cout << player.getName() << ": " << playerValue << " | ";

        if (player.isBusted()) {
            cout << "Busted! Dealer wins.\n";
        } else if (dealerBusted) {
            cout << "Dealer busted! " << player.getName() << " wins!\n";
        } else if (playerValue > dealerValue) {
            cout << "Wins!\n";
        } else if (playerValue < dealerValue) {
            cout << "Loses.\n";
        } else {
            cout << "Push (tie).\n";
        }
    }
}

void Game::resetGame() {
    initializeGame();
}

void Game::displayGameState(bool hideDealerCard) const {
    cout << "\n=== Current Game State ===\n";
    dealer.showHand(hideDealerCard);
    for (const auto& player : players) {
        cout << player.getName() << ": ";
        player.showHand(false);
        cout << " (Total: " << player.getHandValue() << ")\n";
    }
    cout << "==========================\n";
}

void Game::playRound() {
    initializeGame();
    dealInitialCards();
    displayGameState();

    // Player turns
    for (auto& player : players) {
        playerTurn(player);
    }

    // Dealer’s turn
    dealerTurn();

    // Show results
    determineWinners();
}

void Game::playAI() {
    cout << "AI gameplay (Monte Carlo or Q-Learning) not yet implemented.\n";
}
