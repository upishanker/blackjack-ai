//
// Created by Upi Shanker on 10/26/2025.
//

#include "../../include/core/Game.h"
#include <iostream>
#include <limits>
#include <iomanip>

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
    deck.generateDeck();
    deck.shuffleDeck();
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
            cout << "Invalid input. Try again.\n";
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

    // Dealer's turn
    dealerTurn();

    // Show results
    determineWinners();
}

// ============================================================================
// AI TRAINING METHODS
// ============================================================================
State Game::getAIState(const Player& player) const {
    State state;

    // Get dealer's upcard
    const auto& dealerHand = dealer.getHand();
    if (dealerHand.empty()) {
        throw std::runtime_error("Dealer has no cards!");
    }
    state.dealerUpcard = dealerHand[0].getValue();

    // Calculate player hand value with proper ace handling
    int value = 0;
    int aceCount = 0;

    for (const Card& card : player.getHand()) {
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
    // This means: (1) we have an ace, (2) value <= 21, (3) we haven't converted all aces to 1
    state.usableAce = (aceCount > 0 && value <= 21);

    return state;
}

double Game::calculateReward(const Player& player) const {
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

double Game::playAIEpisode(QLearningAI& ai, bool training) {
    // Initialize fresh game
    initializeGame();

    // Create AI player
    Player aiPlayer("AI", false);
    aiPlayer.addCard(deck.dealCard());
    aiPlayer.addCard(deck.dealCard());

    dealer.addCard(deck.dealCard());
    dealer.addCard(deck.dealCard());

    // AI's turn
    while (!aiPlayer.isBusted() && aiPlayer.getHandValue() < 21) {
        State currentState = getAIState(aiPlayer);

        // Choose action
        Action action = training ? ai.chooseAction(currentState) : ai.getBestAction(currentState);

        if (action == Action::STAND) {
            break;
        }

        // HIT
        aiPlayer.addCard(deck.dealCard());
        State nextState = getAIState(aiPlayer);

        if (training) {
            double stepReward = aiPlayer.isBusted() ? -1.0 : -0.01;
            ai.updateQValue(currentState, action, stepReward, nextState, aiPlayer.isBusted());
        }

        // If busted, immediate penalty
        if (aiPlayer.isBusted()) {
            if (training) {
                ai.updateQValue(currentState, action, -1.0, nextState, true);
            }
            return -1.0;
        }

        // Non-terminal step (reward = 0 for intermediate steps)
        if (training) {
            ai.updateQValue(currentState, action, 0.0, nextState, false);
        }
    }

    // Dealer's turn
    dealer.playTurn(deck);

    // Calculate final reward
    double reward = calculateReward(aiPlayer);

    // Final update for STAND action
    if (training && !aiPlayer.isBusted()) {
        State finalState = getAIState(aiPlayer);
        ai.updateQValue(finalState, Action::STAND, reward, finalState, true);
    }

    return reward;
}

void Game::trainAI(QLearningAI& ai, int numEpisodes, bool verbose) {
    cout << "\n=== Training Q-Learning AI ===\n";
    cout << "Episodes: " << numEpisodes << "\n";
    cout << "Initial epsilon: " << ai.getEpisodeCount() << "\n\n";

    int wins = 0, losses = 0, pushes = 0;
    double totalReward = 0.0;

    for (int episode = 0; episode < numEpisodes; ++episode) {
        double reward = playAIEpisode(ai, true);

        ai.recordEpisode(reward);
        totalReward += reward;

        if (reward > 0) wins++;
        else if (reward < 0) losses++;
        else pushes++;

        // Decay exploration
        ai.decayEpsilon(0.99995);

        // Progress report every 10% of episodes
        if (verbose && (episode + 1) % (numEpisodes / 10) == 0) {
            cout << "Episode " << (episode + 1) << "/" << numEpisodes
                 << " | Wins: " << wins << " | Losses: " << losses
                 << " | Pushes: " << pushes
                 << " | Avg Reward: " << fixed << setprecision(3)
                 << (totalReward / (episode + 1)) << "\n";
        }
    }

    cout << "\n=== Training Complete ===\n";
    cout << "Total Wins: " << wins << " (" << (100.0 * wins / numEpisodes) << "%)\n";
    cout << "Total Losses: " << losses << " (" << (100.0 * losses / numEpisodes) << "%)\n";
    cout << "Total Pushes: " << pushes << " (" << (100.0 * pushes / numEpisodes) << "%)\n";
    cout << "Average Reward: " << (totalReward / numEpisodes) << "\n\n";

    ai.printStats();
}

void Game::evaluateAI(QLearningAI& ai, int numGames) {
    cout << "\n=== Evaluating AI (Greedy Policy) ===\n";

    int wins = 0, losses = 0, pushes = 0;
    double totalReward = 0.0;


    for (int game = 0; game < numGames; ++game) {
        double reward = playAIEpisode(ai, false); // training=false uses greedy policy

        totalReward += reward;

        if (reward > 0) wins++;
        else if (reward < 0) losses++;
        else pushes++;
    }

    cout << "\nResults over " << numGames << " games:\n";
    cout << "Wins: " << wins << " (" << (100.0 * wins / numGames) << "%)\n";
    cout << "Losses: " << losses << " (" << (100.0 * losses / numGames) << "%)\n";
    cout << "Pushes: " << pushes << " (" << (100.0 * pushes / numGames) << "%)\n";
    cout << "Average Reward: " << (totalReward / numGames) << "\n";
    cout << "Win Rate: " << fixed << setprecision(2)
         << (100.0 * wins / (wins + losses)) << "%\n\n";
}

double Game::playMonteCarloEpisode(MonteCarloAI& ai, bool training) {
    initializeGame();

    Player aiPlayer("AI", false);
    aiPlayer.addCard(deck.dealCard());
    aiPlayer.addCard(deck.dealCard());

    dealer.addCard(deck.dealCard());
    dealer.addCard(deck.dealCard());

    if (training) {
        ai.startEpisode();
    }

    // AI's turn
    while (!aiPlayer.isBusted() && aiPlayer.getHandValue() < 21) {
        State currentState = getAIState(aiPlayer);

        Action action = training ? ai.chooseAction(currentState) : ai.getBestAction(currentState);

        if (action == Action::STAND) {
            if (training) {
                ai.recordStep(currentState, action, 0.0);
            }
            break;
        }

        // HIT
        if (training) {
            ai.recordStep(currentState, action, -0.01); // Small step penalty
        }

        aiPlayer.addCard(deck.dealCard());

        if (aiPlayer.isBusted()) {
            if (training) {
                ai.endEpisode(-1.0);
            }
            return -1.0;
        }
    }

    // Dealer's turn
    dealer.playTurn(deck);

    // Calculate final reward
    double reward = calculateReward(aiPlayer);

    if (training) {
        ai.endEpisode(reward);
    }

    return reward;
}

void Game::trainMonteCarlo(MonteCarloAI& ai, int numEpisodes, bool verbose) {
    cout << "\n=== Training Monte Carlo AI ===\n";
    cout << "Episodes: " << numEpisodes << "\n\n";

    int wins = 0, losses = 0, pushes = 0;
    double totalReward = 0.0;

    for (int episode = 0; episode < numEpisodes; ++episode) {
        double reward = playMonteCarloEpisode(ai, true);

        totalReward += reward;

        if (reward > 0) wins++;
        else if (reward < 0) losses++;
        else pushes++;

        ai.decayEpsilon();

        if (verbose && (episode + 1) % (numEpisodes / 10) == 0) {
            cout << "Episode " << (episode + 1) << "/" << numEpisodes
                 << " | Wins: " << wins << " | Losses: " << losses
                 << " | Pushes: " << pushes
                 << " | Avg Reward: " << fixed << setprecision(3)
                 << (totalReward / (episode + 1)) << "\n";
        }
    }

    cout << "\n=== Training Complete ===\n";
    cout << "Total Wins: " << wins << " (" << (100.0 * wins / numEpisodes) << "%)\n";
    cout << "Total Losses: " << losses << " (" << (100.0 * losses / numEpisodes) << "%)\n";
    cout << "Total Pushes: " << pushes << " (" << (100.0 * pushes / numEpisodes) << "%)\n";
    cout << "Average Reward: " << (totalReward / numEpisodes) << "\n\n";

    ai.printStats();
}

void Game::evaluateMonteCarlo(MonteCarloAI& ai, int numGames) {
    cout << "\n=== Evaluating Monte Carlo AI (Greedy Policy) ===\n";

    int wins = 0, losses = 0, pushes = 0;
    double totalReward = 0.0;

    for (int game = 0; game < numGames; ++game) {
        double reward = playMonteCarloEpisode(ai, false);

        totalReward += reward;

        if (reward > 0) wins++;
        else if (reward < 0) losses++;
        else pushes++;
    }

    cout << "\nResults over " << numGames << " games:\n";
    cout << "Wins: " << wins << " (" << (100.0 * wins / numGames) << "%)\n";
    cout << "Losses: " << losses << " (" << (100.0 * losses / numGames) << "%)\n";
    cout << "Pushes: " << pushes << " (" << (100.0 * pushes / numGames) << "%)\n";
    cout << "Average Reward: " << (totalReward / numGames) << "\n";
    cout << "Win Rate: " << fixed << setprecision(2)
         << (100.0 * wins / (wins + losses)) << "%\n\n";
}