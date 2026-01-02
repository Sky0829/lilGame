#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "games.h"

int main() {
    int choice = 0;
    SetConsoleOutputCP(65001);

    while (1) {
        printf("\n==== Classic Game Collection ====\n");
        printf("1. Tetris\n");
        printf("2. Snake Game\n");
        printf("3. 2048\n");
        printf("0. Exit\n");
        printf("Please enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input! Please enter a number.\n");
            while (getchar() != '\n'); // Clear buffer
            continue;
        }

        if (choice == 0) break;

        switch (choice) {
            case 1:
                play_tetris();
                break;
            case 2:
                play_snake();
                break;
            case 3:
                start_2048_game();
                break;
            default:
                printf("Invalid choice, please try again.\n");
        }
    }

    printf("Program terminated. Thank you!\n");
    return 0;
}