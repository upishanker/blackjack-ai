//
// Created by Upi Shanker on 10/26/2025.
//

#include "../include/core/Game.h"
#include "../include/ai/QLearningAI.h"
#include <iostream>
#include <limits>

using namespace std;

void displayMenu() {
    cout << "\n=========================\n";
    cout << "   Blackjack with AI\n";
    cout << "=========================\n";
    cout << "1. Play Blackjack (Human)\n";
    cout << "2. Train AI Agent\n";
    cout << "3. Evaluate AI Performance\n";
    cout << "4. Watch AI Play\n";
    cout << "5. Load Q-Table\n";
    cout << "6. Save Q-Table\n";
    cout << "7. View AI Statistics\n";
    cout << "8. Exit\n";
    cout << "=========================\n";
    cout << "Enter choice: ";
}

void playHumanMode() {
    int numPlayers;
    cout << "\nEnter number of players: ";
    cin >> numPlayers;

    if (cin.fail() || numPlayers < 1) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Defaulting to 1 player.\n";
        numPlayers = 1;
    }

    Game game(numPlayers);

    char choice;
    do {
        game.playRound();

        cout << "\nPlay another round? (y/n): ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }

        if (choice == 'y' || choice == 'Y') {
            game.resetGame();
        }
    } while (choice == 'y' || choice == 'Y');
}

void trainAIMode(QLearningAI& ai) {
    int numEpisodes;
    cout << "\nEnter number of training episodes (recommended: 10000-100000): ";
    cin >> numEpisodes;

    if (cin.fail() || numEpisodes < 1) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Defaulting to 10000 episodes.\n";
        numEpisodes = 10000;
    }

    char verbose;
    cout << "Show progress during training? (y/n): ";
    cin >> verbose;

    Game trainingGame(0); // 0 human players for training
    trainingGame.trainAI(ai, numEpisodes, (verbose == 'y' || verbose == 'Y'));

    cout << "\nTraining complete! Don't forget to save the Q-table (option 6).\n";
}

void evaluateAIMode(QLearningAI& ai) {
    int numGames;
    cout << "\nEnter number of evaluation games (recommended: 1000-10000): ";
    cin >> numGames;

    if (cin.fail() || numGames < 1) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Defaulting to 1000 games.\n";
        numGames = 1000;
    }

    Game evalGame(0);
    evalGame.evaluateAI(ai, numGames);
}

void watchAIPlay(QLearningAI& ai) {
    cout << "\n=== Watching AI Play ===\n";
    cout << "The AI will play 10 games using its learned policy.\n\n";

    Game demoGame(0);

    for (int i = 0; i < 10; ++i) {
        cout << "Game " << (i + 1) << ": ";
        double reward = demoGame.playAIEpisode(ai, false);

        if (reward > 0) {
            cout << "AI Wins! (+1)\n";
        } else if (reward < 0) {
            cout << "AI Loses! (-1)\n";
        } else {
            cout << "Push (0)\n";
        }
    }

    cout << "\nDemo complete!\n";
}

void loadQTable(QLearningAI& ai) {
    string filename;
    cout << "\nEnter filename to load (default: data/q_table.csv): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, filename);

    if (filename.empty()) {
        filename = "data/q_table.csv";
    }

    ai.loadQTable(filename);
}

void saveQTable(QLearningAI& ai) {
    string filename;
    cout << "\nEnter filename to save (default: data/q_table.csv): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, filename);

    if (filename.empty()) {
        filename = "data/q_table.csv";
    }

    ai.saveQTable(filename);
}

int main() {
    // Initialize AI agent with default hyperparameters
    QLearningAI ai(0.1, 0.9, 1.0); // alpha=0.1, gamma=0.9, epsilon=1.0

    // Try to load existing Q-table on startup
    cout << "Attempting to load existing Q-table...\n";
    ai.loadQTable("data/q_table.csv");

    int choice;
    bool running = true;

    while (running) {
        displayMenu();
        cin >> choice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1:
                playHumanMode();
                break;

            case 2:
                trainAIMode(ai);
                break;

            case 3:
                evaluateAIMode(ai);
                break;

            case 4:
                watchAIPlay(ai);
                break;

            case 5:
                loadQTable(ai);
                break;

            case 6:
                saveQTable(ai);
                break;

            case 7:
                ai.printStats();
                break;

            case 8:
                cout << "\nSave Q-table before exiting? (y/n): ";
                char save;
                cin >> save;
                if (save == 'y' || save == 'Y') {
                    ai.saveQTable("data/q_table.csv");
                }
                cout << "\nThanks for playing Blackjack with AI!\n";
                running = false;
                break;

            default:
                cout << "Invalid choice. Please select 1-8.\n";
                break;
        }
    }

    return 0;
}