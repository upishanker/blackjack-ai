//
// Created by Upi Shanker on 10/26/2025.
//

#include "../include/core/Game.h"
#include "../include/ai/QLearningAI.h"
#include "../include/ai/MonteCarloAI.h"
#include <iostream>
#include <limits>
#include <iomanip>

using namespace std;

void displayMenu() {
    cout << "\n=========================\n";
    cout << "   Blackjack with AI\n";
    cout << "=========================\n";
    cout << "1. Play Blackjack (Human)\n";
    cout << "2. Train Q-Learning AI\n";
    cout << "3. Evaluate Q-Learning AI\n";
    cout << "4. Watch Q-Learning AI Play\n";
    cout << "5. Load Q-Table\n";
    cout << "6. Save Q-Table\n";
    cout << "7. View Q-Learning Statistics\n";
    cout << "8. Train Monte Carlo AI\n";
    cout << "9. Evaluate Monte Carlo AI\n";
    cout << "10. Load Monte Carlo Q-Table\n";
    cout << "11. Save Monte Carlo Q-Table\n";
    cout << "12. View Monte Carlo Statistics\n";
    cout << "13. Compare Q-Learning vs Monte Carlo\n";
    cout << "14. Exit\n";
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
    cout << "\n=== Watching Q-Learning AI Play ===\n";
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

// Monte Carlo AI functions
void trainMonteCarloMode(MonteCarloAI& ai) {
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

    Game trainingGame(0);
    trainingGame.trainMonteCarlo(ai, numEpisodes, (verbose == 'y' || verbose == 'Y'));

    cout << "\nTraining complete! Don't forget to save the Q-table (option 11).\n";
}

void evaluateMonteCarloMode(MonteCarloAI& ai) {
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
    evalGame.evaluateMonteCarlo(ai, numGames);
}

void loadMonteCarloQTable(MonteCarloAI& ai) {
    string filename;
    cout << "\nEnter filename to load (default: data/mc_q_table.csv): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, filename);

    if (filename.empty()) {
        filename = "data/mc_q_table.csv";
    }

    ai.loadQTable(filename);
}

void saveMonteCarloQTable(MonteCarloAI& ai) {
    string filename;
    cout << "\nEnter filename to save (default: data/mc_q_table.csv): ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    getline(cin, filename);

    if (filename.empty()) {
        filename = "data/mc_q_table.csv";
    }

    ai.saveQTable(filename);
}

void compareAIs(QLearningAI& qLearning, MonteCarloAI& monteCarlo) {
    cout << "\n========================================\n";
    cout << "   Q-Learning vs Monte Carlo Comparison\n";
    cout << "========================================\n\n";

    int numGames;
    cout << "Enter number of evaluation games for each AI (recommended: 5000-10000): ";
    cin >> numGames;

    if (cin.fail() || numGames < 1) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Defaulting to 5000 games.\n";
        numGames = 5000;
    }

    Game evalGame(0);

    // Evaluate Q-Learning
    cout << "\n--- Evaluating Q-Learning AI ---\n";
    int qWins = 0, qLosses = 0, qPushes = 0;
    double qTotalReward = 0.0;

    for (int i = 0; i < numGames; ++i) {
        double reward = evalGame.playAIEpisode(qLearning, false);
        qTotalReward += reward;
        if (reward > 0) qWins++;
        else if (reward < 0) qLosses++;
        else qPushes++;
    }

    // Evaluate Monte Carlo
    cout << "\n--- Evaluating Monte Carlo AI ---\n";
    int mcWins = 0, mcLosses = 0, mcPushes = 0;
    double mcTotalReward = 0.0;

    for (int i = 0; i < numGames; ++i) {
        double reward = evalGame.playMonteCarloEpisode(monteCarlo, false);
        mcTotalReward += reward;
        if (reward > 0) mcWins++;
        else if (reward < 0) mcLosses++;
        else mcPushes++;
    }

    // Display comparison
    cout << "\n========================================\n";
    cout << "           COMPARISON RESULTS\n";
    cout << "========================================\n\n";

    cout << fixed << setprecision(2);

    cout << "Metric                  | Q-Learning    | Monte Carlo   | Winner\n";
    cout << "------------------------|---------------|---------------|--------\n";

    // Win Rate
    double qWinRate = 100.0 * qWins / numGames;
    double mcWinRate = 100.0 * mcWins / numGames;
    cout << "Win Rate                | " << setw(12) << qWinRate << "% | "
         << setw(12) << mcWinRate << "% | "
         << (qWinRate > mcWinRate ? "Q-Learn" : (mcWinRate > qWinRate ? "MC" : "Tie")) << "\n";

    // Loss Rate
    double qLossRate = 100.0 * qLosses / numGames;
    double mcLossRate = 100.0 * mcLosses / numGames;
    cout << "Loss Rate               | " << setw(12) << qLossRate << "% | "
         << setw(12) << mcLossRate << "% | "
         << (qLossRate < mcLossRate ? "Q-Learn" : (mcLossRate < qLossRate ? "MC" : "Tie")) << "\n";

    // Push Rate
    double qPushRate = 100.0 * qPushes / numGames;
    double mcPushRate = 100.0 * mcPushes / numGames;
    cout << "Push Rate               | " << setw(12) << qPushRate << "% | "
         << setw(12) << mcPushRate << "% | "
         << "N/A" << "\n";

    // Average Reward
    double qAvgReward = qTotalReward / numGames;
    double mcAvgReward = mcTotalReward / numGames;
    cout << "Avg Reward              | " << setw(13) << qAvgReward << " | "
         << setw(13) << mcAvgReward << " | "
         << (qAvgReward > mcAvgReward ? "Q-Learn" : (mcAvgReward > qAvgReward ? "MC" : "Tie")) << "\n";

    // Win Rate (excluding pushes)
    double qWinRateNoPush = 100.0 * qWins / (qWins + qLosses);
    double mcWinRateNoPush = 100.0 * mcWins / (mcWins + mcLosses);
    cout << "Win Rate (vs losses)    | " << setw(12) << qWinRateNoPush << "% | "
         << setw(12) << mcWinRateNoPush << "% | "
         << (qWinRateNoPush > mcWinRateNoPush ? "Q-Learn" : (mcWinRateNoPush > qWinRateNoPush ? "MC" : "Tie")) << "\n";

    cout << "\n========================================\n\n";

    // Summary
    int qScore = 0, mcScore = 0;
    if (qWinRate > mcWinRate) qScore++;
    else if (mcWinRate > qWinRate) mcScore++;

    if (qLossRate < mcLossRate) qScore++;
    else if (mcLossRate < qLossRate) mcScore++;

    if (qAvgReward > mcAvgReward) qScore++;
    else if (mcAvgReward > qAvgReward) mcScore++;

    if (qWinRateNoPush > mcWinRateNoPush) qScore++;
    else if (mcWinRateNoPush > qWinRateNoPush) mcScore++;

    cout << "Overall Score: Q-Learning " << qScore << " - " << mcScore << " Monte Carlo\n";

    if (qScore > mcScore) {
        cout << "Winner: Q-Learning AI! 🏆\n";
    } else if (mcScore > qScore) {
        cout << "Winner: Monte Carlo AI! 🏆\n";
    } else {
        cout << "Result: Tie! Both AIs perform equally well. 🤝\n";
    }

    cout << "\n";
}

int main() {
    // Initialize AI agents with default hyperparameters
    QLearningAI qLearningAI(0.1, 0.9, 1.0); // alpha=0.1, gamma=0.9, epsilon=1.0
    MonteCarloAI monteCarloAI(0.1, 1.0);     // epsilon=0.1, gamma=1.0

    // Try to load existing Q-tables on startup
    cout << "Attempting to load existing Q-Learning Q-table...\n";
    qLearningAI.loadQTable("data/q_table.csv");

    cout << "Attempting to load existing Monte Carlo Q-table...\n";
    monteCarloAI.loadQTable("data/mc_q_table.csv");

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
                trainAIMode(qLearningAI);
                break;

            case 3:
                evaluateAIMode(qLearningAI);
                break;

            case 4:
                watchAIPlay(qLearningAI);
                break;

            case 5:
                loadQTable(qLearningAI);
                break;

            case 6:
                saveQTable(qLearningAI);
                break;

            case 7:
                qLearningAI.printStats();
                break;

            case 8:
                trainMonteCarloMode(monteCarloAI);
                break;

            case 9:
                evaluateMonteCarloMode(monteCarloAI);
                break;

            case 10:
                loadMonteCarloQTable(monteCarloAI);
                break;

            case 11:
                saveMonteCarloQTable(monteCarloAI);
                break;

            case 12:
                monteCarloAI.printStats();
                break;

            case 13:
                compareAIs(qLearningAI, monteCarloAI);
                break;

            case 14:
                cout << "\nSave Q-Learning Q-table before exiting? (y/n): ";
                char saveQ;
                cin >> saveQ;
                if (saveQ == 'y' || saveQ == 'Y') {
                    qLearningAI.saveQTable("data/q_table.csv");
                }

                cout << "Save Monte Carlo Q-table before exiting? (y/n): ";
                char saveMC;
                cin >> saveMC;
                if (saveMC == 'y' || saveMC == 'Y') {
                    monteCarloAI.saveQTable("data/mc_q_table.csv");
                }

                cout << "\nThanks for playing Blackjack with AI!\n";
                running = false;
                break;

            default:
                cout << "Invalid choice. Please select 1-14.\n";
                break;
        }
    }

    return 0;
}