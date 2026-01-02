// game.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//
#include "games.h"
#include "snake.h"

// 全域變數定義
int width = 20, height = 15;
int gameOver, score;
Point head, food;
int tailX[100], tailY[100]; // 儲存蛇身座標 (Array)
int nTail = 0;
enum eDirection { STOP = 0, LEFT, RIGHT, UP, DOWN } dir;

// 1. 初始化遊戲
void Setup() {
    gameOver = 0;      // 1. 重置遊戲狀態
    dir = STOP;        // 2. 讓蛇先停住，等玩家按鍵
    score = 0;         // 3. 分數歸零
    nTail = 0;         // 4. 關鍵：長度一定要設回 0！

    // 5. 設定蛇頭在目前地圖的中心
    head.x = width / 2;
    head.y = height / 2;

    // 6. 重新生成食物
    food.x = rand() % width;
    food.y = rand() % height;

    // 7. (選修) 為了保險，可以把身體陣列也清空
    for (int i = 0; i < 100; i++) {
        tailX[i] = 0;
        tailY[i] = 0;
    }
}

// 2. 繪製畫面 (包含 ANSI 顏色與牆壁)
void Draw() {
    // 1. 嘗試回到左上角
    COORD cursorPosition = { 0, 0 };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);

    // 2. 繪製牆壁與遊戲主體
    for (int i = 0; i < width + 2; i++) printf("#");
    printf("\n");

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (j == 0) printf("#"); 

            if (i == head.y && j == head.x)
                printf("\033[1;32mO\033[0m"); 
            else if (i == food.y && j == food.x)
                printf("\033[1;31m*\033[0m"); 
            else {
                int isTail = 0;
                for (int k = 0; k < nTail; k++) {
                    if (tailX[k] == j && tailY[k] == i) {
                        printf("\033[1;32m0\033[0m"); 
                        isTail = 1;
                        break;
                    }
                }
                if (!isTail) printf(" ");
            }
            if (j == width - 1) printf("#");
        }
        printf("\n");
    }
    for (int i = 0; i < width + 2; i++) printf("#");
    printf("\nScore: %d\n", score);

    // 3. 強制刷新緩衝區 (解決畫面不動的關鍵)
    fflush(stdout); 
}

// 3. 偵測按鍵
void Input() {
    if (_kbhit()) {
        switch (_getch()) {
        case 'a':
            // 如果長度為 0，或者目前不是向右，才准許向左
            if (nTail == 0 || dir != RIGHT) dir = LEFT;
            break;
        case 'd':
            // 如果長度為 0，或者目前不是向左，才准許向右
            if (nTail == 0 || dir != LEFT) dir = RIGHT;
            break;
        case 'w':
            // 如果長度為 0，或者目前不是向下，才准許向上
            if (nTail == 0 || dir != DOWN) dir = UP;
            break;
        case 's':
            // 如果長度為 0，或者目前不是向上，才准許向下
            if (nTail == 0 || dir != UP) dir = DOWN;
            break;
        case 'x':
            gameOver = 1;
            break;
        }
    }
}

// 4. 遊戲邏輯 (移動、碰撞、長大)
void Logic() {
    // 1. 移動蛇身座標
    int prevX = tailX[0], prevY = tailY[0], prev2X, prev2Y;
    tailX[0] = head.x; tailY[0] = head.y;
    for (int i = 1; i < nTail; i++) {
        prev2X = tailX[i]; prev2Y = tailY[i];
        tailX[i] = prevX; tailY[i] = prevY;
        prevX = prev2X; prevY = prev2Y;
    }

    // 2. 移動蛇頭
    switch (dir) {
    case LEFT:  head.x--; break;
    case RIGHT: head.x++; break;
    case UP:    head.y--; break;
    case DOWN:  head.y++; break;
    default: break;
    }

    // 3. 判定：撞牆
    if (head.x >= width || head.x < 0 || head.y >= height || head.y < 0)
        gameOver = 1;

    // 4. 判定：撞到自己 (新增邏輯)
    for (int i = 0; i < nTail; i++) {
        if (tailX[i] == head.x && tailY[i] == head.y) {
            gameOver = 1;
        }
    }

    // 5. 判定：吃到食物
    if (head.x == food.x && head.y == food.y) {
        score += 10;
        food.x = rand() % width;
        food.y = rand() % height;
        nTail++;
    }
}

void play_snake() {
    char choice;
    int diff;

    do {
        system("cls");
        printf("=== 選擇難度 (影響地圖大小) ===\n");
        printf("      1. 簡單 (25x20)\n");
        printf("      2. 中等 (20x15)\n");
        printf("      3. 困難 (15x10)\n");
        printf("請輸入選擇: ");
        
        // 修正 1: 將 scanf_s 改為 scanf
        scanf("%d", &diff);

        if (diff == 1) { width = 25; height = 20; }
        else if (diff == 3) { width = 15; height = 10; }
        else { width = 20; height = 15; }

        system("cls");
        Setup();

        while (!gameOver) {
            Draw();
            Input();
            Logic();
            Sleep(100);
        }

        printf("\nGame Over! 是否重新開始? (y/n): ");
        // 修正 2: 將 scanf_s 改為 scanf
        // 注意 %c 前面加一個空格，用來跳過緩衝區的換行符
        scanf(" %c", &choice);

    } while (choice == 'y' || choice == 'Y');

    system("cls");
    printf("\033[1;33m");
    printf("\n");
    printf("  ____   __     __  ______      ____   __     __  ______ \n");
    printf(" |  _ \\  \\ \\   / / |  ____|    |  _ \\  \\ \\   / / |  ____|\n");
    printf(" | |_) |  \\ \\_/ /  | |__       | |_) |  \\ \\_/ /  | |__   \n");
    printf(" |  _ <    \\   /   |  __|      |  _ <    \\   /   |  __|  \n");
    printf(" | |_) |    | |    | |____     | |_) |    | |    | |____ \n");
    printf(" |____/     |_|    |______|    |____/     |_|    |______|\n");
    printf("\033[0m");

    printf("\n------------------------------------------------------------\n");
    printf("                遊戲結束！感謝你的遊玩。\n");
    printf("------------------------------------------------------------\n");
    printf("\n按任意鍵退出程式...");
    
    // 這裡用 _getch() 或 system("pause") 都可以，但不要 return 0
    _getch(); 
    // 修正 3: 刪除 return 0; 因為函式是 void
}
// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的提示: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案