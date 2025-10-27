//
// Created by Upi Shanker on 10/26/2025.
//

#include "../include/core/Game.h"
#include <iostream>

using namespace std;

int main() {
    cout << "=========================\n";
    cout << "   Blackjack with AI\n";
    cout << "=========================\n";

    int numPlayers;
    cout << "Enter number of players: ";
    cin >> numPlayers;

    // Create and initialize game
    Game game(numPlayers);

    char choice;
    do {
        game.playRound();

        cout << "\nPlay another round? (y/n): ";
        cin >> choice;
        if (choice == 'y' || choice == 'Y') {
            game.resetGame();
        }
    } while (choice == 'y' || choice == 'Y');

    cout << "\nWould you like to watch the AI play? (y/n): ";
    cin >> choice;
    if (choice == 'y' || choice == 'Y') {
        game.playAI();
    }

    cout << "\nThanks for playing Blackjack with AI!\n";
    return 0;
}
