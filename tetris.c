#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <string.h>
#include "games.h"

// --- 定義常數與按鍵 ---
#define ROW 29      // 遊戲畫面行數(高度)
#define COL 20      // 遊戲畫面列數(寬度)

#define UP 72       // 方向鍵上
#define DOWN 80     // 方向鍵下
#define LEFT 75     // 方向鍵左
#define RIGHT 77    // 方向鍵右

#define SPACE 32    // 空白鍵
#define ESC 27      // ESC鍵

#define KEY_ENTER  13   // Enter鍵
#define KEY_EXT1   0    // 擴充鍵值1 (方向鍵前導碼)
#define KEY_EXT2   224  // 擴充鍵值2 (方向鍵前導碼)

// --- Boss 模式版面參數 ---
#define GAP 2       // 區塊間距
#define L_OX 0      // 左側版面起始X
#define P_W  10     // 中間資訊欄寬度
#define P_OX (L_OX + COL + GAP) // 中間資訊欄起始X
#define R_OX (P_OX + P_W + GAP) // 右側版面起始X
#define OY   0      // 起始Y座標

// --- 資料結構定義 ---
struct FaceN {
    int data[ROW][COL + 10];   // 存放一般模式版面資料 (1有方塊, 0無)
    int color[ROW][COL + 10];  // 存放對應方塊的顏色
} faceN;

typedef struct {
    int data[ROW][COL];        // Boss模式版面資料
    int color[ROW][COL];       // Boss模式顏色
} Face;

Face faceL, faceR; // 定義左右兩個遊戲面 (Boss模式用)

struct Block {
    int space[4][4]; // 定義方塊形狀矩陣 (4x4)
} block[12][4];      // 12種形狀，每種4個旋轉狀態

typedef enum {
    MODE_NORMAL = 0,    // 一般模式
    MODE_SPEEDED_UP,    // 加速模式
    MODE_STRANGE,       // 特殊方塊模式
    MODE_BOSS,          // Boss雙手模式
    MODE_COUNT          // 模式總數
} GameMode;

typedef struct {
    int shape, form;       // 目前形狀, 目前旋轉狀態
    int x, y;              // 目前座標
    int nextShape, nextForm; // 下一個形狀與狀態
    int t;                 // 下落計時器
    int alive;             // 方塊是否還存活(未固定)
} Falling;

Falling fallL, fallR; // 左右落下的方塊 (Boss模式用)

typedef enum {
    BEND_NONE = 0,
    BEND_GAMEOVER, // 遊戲結束
    BEND_RESTART,  // 按 R 重來
    BEND_EXIT      // 按 ESC 離開
} BossEnd;

BossEnd bossEndReason = BEND_NONE; // 紀錄Boss模式結束原因

// --- 函數宣告原型 ---
int GetShapeCount(GameMode mode);
int GetDropDelay(GameMode mode);
int GameNormal(int mode,  int dropDelay);
int GameBoss(int shapeCount,  int dropDelay);
void HideCursor();
void CursorJump(int x, int y);
void SetAttr(WORD attr);
void DrawMenu(int selected);
int HandleMainMenu(void);
void InitInterface(void);
void InitBossInterface(void);
void InitBossFace(Face* f);
void DrawBossBoard(int ox);
void InitBlockInfo(int shapeCount);
void color(int num);
void DrawBlock(int shape, int form, int x, int y);
void DrawSpace(int shape, int form, int x, int y);
int IsLegal(int shape, int form, int x, int y);
int JudeFunc();
void StartGame(int shapeCount,  int dropDelay);
void StartBossGame(int shapeCount, int dropDelay);
void SaveRecordIfNeeded();
void LoadHighScore();
void SaveHighScore();
void PrintButton(int x, int y, const char* text, int selected, int bg);
int IsLegalPF(Face* f, int shape, int form, int x, int y);
void MergePF(Face* f, int shape, int form, int x, int y);
int ClearLinesPF(Face* f, int ox);
void DrawBlockAt(int ox, int shape, int form, int x, int y);
void EraseBlockAt(int ox, int shape, int form, int x, int y);
void EraseBlockAtPF(Face* f, int ox, int shape, int form, int x, int y);
int BossStepOne(Face* f, int ox, Falling* p, int shapeCount, int dropDelay);
void BossHandleInput(Falling* L, Falling* R);
int BossGameOverUI(void);
void DrawNextInPanel(int x, int y, int shape, int form);
int BossSpawn(Face* f, Falling* p, int shapeCount);

// --- 全域變數 ---
int max, grade; // 最高分, 目前分數
int gBossShapeCount = 7;
int gBossDropDelay  = 300;

void play_tetris(void)
{
    #pragma warning (disable:4996) // 忽略scanf等安全警告
    system("title TETRIS"); // 設定視窗標題
    HideCursor(); // 隱藏游標

    while (1) // 遊戲主迴圈
    {
        int mode = HandleMainMenu(); // 選擇模式
        if (mode == -1) ; // 選擇離開

        // 根據模式設定視窗大小
        if (mode == MODE_BOSS)
            system("mode con lines=29 cols=120"); // Boss模式較寬
        else
            system("mode con lines=29 cols=60");  // 一般模式較窄

        system("cls"); // 清除螢幕

        int shapeCount = GetShapeCount((GameMode)mode); // 取得該模式方塊種類數
        int dropDelay = GetDropDelay((GameMode)mode);   // 取得該模式下落延遲(速度)

        if (mode == MODE_NORMAL || mode == MODE_STRANGE || mode == MODE_SPEEDED_UP ) {
            GameNormal(shapeCount, dropDelay); // 執行一般類遊戲
        } else if(mode == MODE_BOSS) {
            GameBoss(shapeCount, dropDelay);   // 執行Boss模式
        }
    }
}

int GetShapeCount(GameMode mode) // 模式方塊數
{
    switch (mode) {
        case MODE_NORMAL: return 7;
        case MODE_SPEEDED_UP: return 7;
        case MODE_STRANGE: return 11; // 怪異模式有更多方塊
        case MODE_BOSS: return 7;
        default: return 7;
    }
}

int GetDropDelay(GameMode mode) // 模式下落速度 (數字越小越快)
{
    switch (mode) {
        case MODE_SPEEDED_UP: return 7;   // 極快
        case MODE_NORMAL: return 30;      // 正常
        case MODE_STRANGE: return 30;
        case MODE_BOSS: return 300;       // Boss模式因機制不同，數值邏輯不同
        default: return 30;
    }
}

int GameNormal(int shapeCount, int dropDelay) // 一般模式遊戲執行
{
    LoadHighScore(); // 載入最高分
    grade = 0;       // 分數歸零
    InitInterface(); // 繪製介面
    InitBlockInfo(shapeCount); // 初始化方塊資料
    srand((unsigned int)time(NULL)); // 設定隨機數種子
    StartGame(shapeCount, dropDelay); // 開始遊戲邏輯
    return 0;
}

int GameBoss(int shapeCount, int dropDelay) // Boss模式遊戲執行
{
    int action;
    while (1) {
        LoadHighScore();
        grade = 0;
        InitBossInterface();
        InitBlockInfo(shapeCount);
        srand((unsigned int)time(NULL));

        StartBossGame(shapeCount, dropDelay);

        action = BossGameOverUI(); // 顯示結束畫面並詢問
        if (action == 0) return 0; // 回到選單或重來邏輯
        if (action == -1) return 0; // 結束程式
    }
}

void HideCursor() // 隱藏游標
{
    CONSOLE_CURSOR_INFO curInfo;
    curInfo.dwSize = 1;
    curInfo.bVisible = FALSE;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorInfo(handle, &curInfo);
}

void CursorJump(int x, int y) // 游標跳轉到指定座標 (X, Y)
{
    COORD pos;
    pos.X = x;
    pos.Y = y;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(handle, pos);
}

void SetAttr(WORD attr) // 設定文字顏色屬性
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

void DrawMenu(int selected) // 繪製Menu
{
    CursorJump(0, 0);
    SetAttr(7);
    CursorJump(28, 3);
    printf("TETRIS");
    CursorJump(13, 5);
    printf("=====================================");
    
    int x = 22;
    int y0 = 8;
    int gap = 3;

    // 繪製各個模式按鈕
    PrintButton(x, y0 + gap * 0, "Normal", selected == 0, 11);
    PrintButton(x, y0 + gap * 1, "Speeded-up", selected == 1, 13);
    PrintButton(x, y0 + gap * 2, "Strange", selected == 2, 10);
    PrintButton(x, y0 + gap * 3, "Boss", selected == 3, 14);

    CursorJump(10, y0 + gap * 4 + 2);
    printf("Up/Down: Choose  Enter: Confirm   Esc: Exit");
}

int HandleMainMenu(void) // Menu控制
{
    int selected = 0;
    int lastSelected = -1;
    system("cls");
    DrawMenu(selected);

    while (1) {
        if (!kbhit()) continue; // 若無按鍵則繼續等待
        int ch = getch();
        if (ch == KEY_EXT1 || ch == KEY_EXT2) { // 處理方向鍵
            int k = getch();
            if (k == UP)
                selected = (selected + MODE_COUNT - 1) % MODE_COUNT; // 往上選
            else if (k == DOWN)
                selected = (selected + 1) % MODE_COUNT; // 往下選
        } else if (ch == KEY_ENTER) {
            return selected; // 確認選擇
        } else if (ch == ESC) {
            return -1; // 離開
        }
        if (selected != lastSelected) { // 只在選擇改變時重繪，減少閃爍
            DrawMenu(selected);
            lastSelected = selected;
        }
    }
}

void InitInterface(void) // 初始版面設定 
{
    int i, j;
    SetAttr(7);// 設定邊框顏色
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL + 10; j++) {
            if (j == 0 || j == COL - 1 || j == COL + 9) { // 繪製直線 
                faceN.data[i][j] = 1;
                CursorJump(2 * j, i);
                printf("■");
            } else if (i == ROW - 1) { // 繪製底部 
                faceN.data[i][j] = 1;
                printf("■ ");
            } else {
                faceN.data[i][j] = 0; // 其餘地方資料歸零(沒有方塊) 
            }
        }
    }
    for (i = COL; i < COL + 10; i++) { // 繪製分區線 
        faceN.data[8][i] = 1;
        CursorJump(2 * i, 8);
        printf("■");
    }

    // 繪製右側資訊欄文字
    CursorJump(2 * COL, 1);
    printf("NEXT：");
    CursorJump(2 * COL + 3, ROW - 19);
    printf("LEFT：←");
    CursorJump(2 * COL + 3, ROW - 17);
    printf("RIGHT：→");
    CursorJump(2 * COL + 3, ROW - 15);
    printf("SPEEDED：↓");
    CursorJump(2 * COL + 3, ROW - 13);
    printf("HARD DROP：↑");
    CursorJump(2 * COL + 3, ROW - 11);
    printf("SPIN：space");
    CursorJump(2 * COL + 3, ROW - 9);
    printf("PAUSE: S");
    CursorJump(2 * COL + 3, ROW - 7);
    printf("EXIT: Esc");
    CursorJump(2 * COL + 3, ROW - 5);
    printf("RESTART:R");
    CursorJump(2 * COL + 3, ROW - 3);
    printf("RECORD:%d", max);
    CursorJump(2, 1);
    printf("POINT：%d", grade);
}

void InitBossInterface(void) // Boss模式初始版面設定
{
    system("cls");
    int i, j;
    SetAttr(7);
    InitBossFace(&faceL); // 初始化左邊界
    InitBossFace(&faceR); // 初始化右邊界

    DrawBossBoard(L_OX); // 繪製左框
    DrawBossBoard(R_OX); // 繪製右框
   
    for (j = 0; j < ROW; j++) { // 繪製中間格
        CursorJump(2 * (P_OX + 0), j);        printf("■");
        CursorJump(2 * (P_OX + P_W - 1), j);  printf("■");
    }
    for (j = 0; j < P_W; j++) {
        CursorJump(2 * (P_OX + j), 8);    printf("■");
        CursorJump(2 * (P_OX + j), 19);   printf("■");
        CursorJump(2 * (P_OX + j), ROW - 1);    printf("■");
    }

    // Boss模式中間文字
    CursorJump(2 * (P_OX + 2), 1);   printf("NEXT(L):");
    CursorJump(2 * (P_OX + 2), 21);  printf("NEXT(R):");
    CursorJump(2 * (P_OX + 2) + 1, ROW - 17); printf("RECORD:%d", max);
    CursorJump(2 * (P_OX + 2), ROW - 15); printf("YOUR POINT:");
    CursorJump(2 * (P_OX + 2) + 5, ROW - 13); printf("%d", grade);
    CursorJump(2 * (P_OX + 2) + 1, 7); printf("L:ASD+E");
    CursorJump(2 * (P_OX + 2), ROW - 2); printf("R:Arrows+Sp");
}

void InitBossFace(Face* f) // Boss模式邊界設定
{
    int i, j;
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL; j++) {
            if (j == 0 || j == COL - 1 || i == ROW - 1)
                f->data[i][j] = 1;   // 邊界牆
            else
                f->data[i][j] = 0;   // 空白
            f->color[i][j] = 7;
        }
    }
}

void DrawBossBoard(int ox) // Boss模式邊界繪製
{
    int i, j;
    // 左右邊框
    for (i = 0; i < ROW; i++) {
        CursorJump(2 * (ox + 0), i);        printf("■");
        CursorJump(2 * (ox + COL - 1), i);  printf("■");
    }
    // 底線
    for (j = 0; j < COL; j++) {
        CursorJump(2 * (ox + j), ROW - 1);
        printf("■");
    }
}

void InitBlockInfo(int shapeCount) // 初始化方塊形狀資料
{
    int i, j;
    int shape, form;
              
    // 先清空所有方塊資料
    for (shape = 0; shape < shapeCount; shape++)
        for (form = 0; form < 4; form++)
            for (i = 0; i < 4; i++)
                for (j = 0; j < 4; j++)
                   block[shape][form].space[i][j] = 0;
                   
    // "T"形
    for (i = 0; i <= 2; i++) block[0][0].space[1][i] = 1;
    block[0][0].space[2][1] = 1;

    // "L"形
    for (i = 1; i <= 3; i++) block[1][0].space[i][1] = 1;
    block[1][0].space[3][2] = 1;
              
    // "J"形
    for (i = 1; i <= 3; i++) block[2][0].space[i][2] = 1;
    block[2][0].space[3][1] = 1;
              
    for (i = 0; i <= 1; i++) {
        // "Z"形
        block[3][0].space[1][i] = 1;
        block[3][0].space[2][i + 1] = 1;
        // "S"形
        block[4][0].space[1][i + 1] = 1;
        block[4][0].space[2][i] = 1;
        // "O"形
        block[5][0].space[1][i + 1] = 1;
        block[5][0].space[2][i + 1] = 1;
    }
              
    // "I"形
    for (i = 0; i <= 3; i++) block[6][0].space[i][1] = 1;
              
    // "斜I" 型
    for (i = 0; i <= 3; i++) block[7][0].space[i][i] = 1; 
              
    // "大L" 型
    for (i = 1; i <= 3; i++) block[8][0].space[i][1] = 1;
    for (i = 2; i <= 3; i++) block[8][0].space[3][i] = 1;
              
    // "U"形
    for (i = 1; i <= 2; i++) {
        block[9][0].space[1][i] = 1;
        block[9][0].space[3][i] = 1;
    }
    block[9][0].space[2][1] = 1;
              
    // "歪w"形
    for (i = 0; i <= 1; i++) {
        block[10][0].space[2][i + 1] = 1;
        block[10][0].space[3][i] = 1;
    }
    block[10][0].space[1][2] = 1;
              
    // 自動生成其他3種旋轉狀態
    int temp[4][4];
    for (shape = 0; shape < shapeCount; shape++) { // 針對每種形狀
        for (form = 0; form < 3; form++) { // 產生下一個旋轉型態
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    temp[i][j] = block[shape][form].space[i][j];
                }
            }
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    // 旋轉算法
                    block[shape][form + 1].space[i][j] = temp[3 - j][i];
                }
            }
        }
    }
}

void color(int c) // 自定義顏色編號轉換為Windows API顏色
{
    switch (c) {
        case 0: c = 13; break;
        case 1:
        case 2: c = 12; break;
        case 3:
        case 4: c = 10; break;
        case 5: c = 14; break;
        case 6: c = 11; break;
        case 7: c = 9;  break;
        case 8: c = 4;  break;
        case 9: c = 2;  break;
        case 10: c = 5; break;
        case 11: c = 3; break;
        default: c = 7; break;
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void DrawBlock(int shape, int form, int x, int y) // 方塊繪製
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (block[shape][form].space[i][j] == 1) {
                CursorJump(2 * (x + j), y + i);
                printf("■");
            }
        }
    }
}

void DrawSpace(int shape, int form, int x, int y) // 空格繪製 (清除方塊軌跡用)
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (block[shape][form].space[i][j] == 1) {
                CursorJump(2 * (x + j), y + i);
                printf("  ");
            }
        }
    }
}

int IsLegal(int shape, int form, int x, int y) // 碰撞判斷 
{
    int i, j;
    int nx, ny;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (block[shape][form].space[i][j] == 0)
                continue;

            nx = x + j;
            ny = y + i;
           
            // 檢查是否超出邊界
            if (nx < 0 || nx >= COL + 10 || ny < 0 || ny >= ROW)
                return 0;

            // 檢查是否撞到既有方塊
            if (faceN.data[ny][nx] == 1)
                return 0;
        }
    }
    return 1;
}

int JudeFunc() // 檢查滿行並消行、更新分數與畫面，判斷遊戲結束
{
    int i, j;
    int m, n;
    // 從下往上掃描每一行
    for (i = ROW - 2; i > 4; i--) {
        int sum = 0;
        for (j = 1; j < COL - 1; j++) {
            sum += faceN.data[i][j];
        }
        if (sum == 0) break; // 空行則提早結束
        if (sum == COL - 2) { // 滿行
            grade += 10;
            SetAttr(7);
            CursorJump(2 * COL + 3, ROW - 3);
            printf("YOUR POINT：%d", grade);
            
            // 清除該行視覺
            for (j = 1; j < COL - 1; j++) {
                faceN.data[i][j] = 0;
                CursorJump(2 * j, i); 
                printf("  ");
            }
            
            // 上方方塊下移
            for (m = i; m > 1; m--) {
                sum = 0;
                for (n = 1; n < COL - 1; n++) {
                    sum += faceN.data[m - 1][n];
                    faceN.data[m][n] = faceN.data[m - 1][n];
                    faceN.color[m][n] = faceN.color[m - 1][n];
                    if (faceN.data[m][n] == 1) {
                        CursorJump(2 * n, m);
                        color(faceN.color[m][n]);
                        printf("■");
                    } else {
                        CursorJump(2 * n, m);
                        printf("  ");
                    }
                }
                if (sum == 0) return 1; // 上方已無方塊
            }
        }
    }
    // 檢查頂部是否有方塊 (GameOver判斷)
    for (j = 1; j < COL - 1; j++) {
        if (faceN.data[1][j] == 1) {
            Sleep(1000);
            system("cls");
            SetAttr(7);
            CursorJump(2 * (COL / 3), ROW / 2 - 6);
            if (grade > max) {
                printf("Congratulations on breaking the record!\n");
                printf("The highest record has been updated to %d", grade);
                SaveRecordIfNeeded();
            } else if (grade == max) {
	printf("Your points is %d", grade);
	CursorJump(2 * (COL / 3), ROW / 2 - 3);
                printf("You've already matched the best record! Keep it up!", grade);
            } else {
	printf("Your points is %d", grade);
	CursorJump(2 * (COL / 3), ROW / 2 - 3);
                printf("Keep it up! You're only %d points away from the record!", max - grade);
            }
            CursorJump(2 * (COL / 3), ROW / 2);
            printf("GAME OVER");
            while (1) {
                char ch;
                CursorJump(2 * (COL / 3), ROW / 2 + 3);
                printf("DO you want to try again?(y/n):");
                scanf(" %c", &ch);
                if (ch == 'y' || ch == 'Y') {
                    system("cls");
                    return -1; // 重來信號
                } else if (ch == 'n' || ch == 'N') {
                    CursorJump(2 * (COL / 3), ROW / 2 + 5);
                    return -1;
                } else {
                    CursorJump(2 * (COL / 3), ROW / 2 + 4);
                    printf("Error, please enter again");
                }
            }
        }
    }
    return 0;
}

void StartGame(int shapeCount, int dropDelay) // 遊戲主迴圈
{
    int i, j;
    int shape = rand() % shapeCount, form = rand() % 4; // 隨機初始方塊
    while (1) {
        int t = 0;
        int nextShape = rand() % shapeCount, nextForm = rand() % 4;
        int x = COL / 2 - 2, y = 0; // 起始位置
        color(nextShape);
        DrawBlock(nextShape, nextForm, COL + 3, 3); // 顯示下一個方塊

        while (1) { // 單一方塊下落迴圈
            if (t == 0) { // 下落計時重置 
                t = dropDelay;
            }
            Sleep(1);
            t--;

            if (t <= 0) { // 時間到，自動下落一格
                if (IsLegal(shape, form, x, y + 1) == 0) { // 若碰到東西
                    // 固定方塊到版面
                    for (i = 0; i < 4; i++) {
                        for (j = 0; j < 4; j++) {
                            if (block[shape][form].space[i][j] == 1) {
                                faceN.data[y + i][x + j] = 1;
                                faceN.color[y + i][x + j] = shape;
                            }
                        }
                    }
                    while (1){ // 檢查消行
                        int r = JudeFunc();
                        if (r == 1) continue;
                        if (r == 0) break;
                        if (r == -1) return;
                    }
                    break; // 跳出，換下一塊
                } else { // 若下方是空的
                    DrawSpace(shape, form, x, y); // 清除舊位置
                    y++;
                    color(shape);
                    DrawBlock(shape, form, x, y); // 繪製新位置
                }
            } else { // 處理按鍵輸入
                if (kbhit() == 0) continue;
                
                int ch = getch();
                if (ch == KEY_EXT1 || ch == KEY_EXT2) { // 方向鍵
                    int k = getch();
                    if (k == UP) { // 硬降 (Hard Drop)
                        DrawSpace(shape, form, x, y);
                        while (IsLegal(shape, form, x, y + 1) == 1) y++;
                        color(shape);
                        DrawBlock(shape, form, x, y);
                        
                        // 固定
                        for (i = 0; i < 4; i++)
                            for (j = 0; j < 4; j++)
                                if (block[shape][form].space[i][j] == 1) {
                                    faceN.data[y + i][x + j] = 1;
                                    faceN.color[y + i][x + j] = shape;
                                }
                        
                        while (1) {
                            int r = JudeFunc();
                            if (r == 1) continue;
                            if (r == 0) break;
                            if (r == -1) return;
                        }
                        break;
                    } else if (k == DOWN) { // 加速下落
                        if (IsLegal(shape, form, x, y + 1) == 1) {
                            DrawSpace(shape, form, x, y);
                            y++;
                            color(shape);
                            DrawBlock(shape, form, x, y);
                        }
                    } else if (k == LEFT) { // 左移
                        if (IsLegal(shape, form, x - 1, y) == 1) {
                            DrawSpace(shape, form, x, y);
                            x--;
                            color(shape);
                            DrawBlock(shape, form, x, y);
                        }
                    } else if (k == RIGHT) { // 右移
                        if (IsLegal(shape, form, x + 1, y) == 1) {
                            DrawSpace(shape, form, x, y);
                            x++;
                            color(shape);
                            DrawBlock(shape, form, x, y);
                        }
                    }
                } else { // 其他按鍵
                    switch (ch) {
                    case SPACE: // 旋轉
                        if (IsLegal(shape, (form + 1) % 4, x, y) == 1) {
                            DrawSpace(shape, form, x, y);
                            form = (form + 1) % 4;
                            color(shape);
                            DrawBlock(shape, form, x, y);
                        }
                        break;
                    case ESC: // 離開
                        SaveRecordIfNeeded();
                        system("cls");
                        SetAttr(7);
                        CursorJump(COL, ROW / 2);
                        printf("  END GAME ");
                        CursorJump(COL, ROW / 2 + 2);
                        return;
                    case 's':
                    case 'S': // 暫停
                        system("pause>nul");
                        break;
                    case 'r':
                    case 'R': // 重來
                        SaveRecordIfNeeded();
                        system("cls");
                        return;
                    }
                }
            }
        }
        shape = nextShape, form = nextForm;
        DrawSpace(nextShape, nextForm, COL + 3, 3);
    }
}

void StartBossGame(int shapeCount, int dropDelay) // Boss模式主迴圈
{
    gBossShapeCount = shapeCount;
    gBossDropDelay  = dropDelay;

    InitBossFace(&faceL);
    InitBossFace(&faceR);

    // 初始化左右方塊
    fallL.nextShape = rand() % shapeCount; fallL.nextForm = rand() % 4; fallL.alive = 1;
    fallR.nextShape = rand() % shapeCount; fallR.nextForm = rand() % 4; fallR.alive = 1;

    // 生成第一顆
    if (!BossSpawn(&faceL, &fallL, shapeCount)) { fallL.alive = 0; bossEndReason = BEND_GAMEOVER; }
    if (!BossSpawn(&faceR, &fallR, shapeCount)) { fallR.alive = 0; bossEndReason = BEND_GAMEOVER; }
              
    color(fallL.shape);
    DrawBlockAt(L_OX, fallL.shape, fallL.form, fallL.x, fallL.y);
              
    color(fallR.shape);
    DrawBlockAt(R_OX, fallR.shape, fallR.form, fallR.x, fallR.y);

    while (fallL.alive && fallR.alive) { // 左右存活迴圈
        BossHandleInput(&fallL, &fallR); // 處理輸入
        if (!fallL.alive || !fallR.alive) break; // 若有一邊死則跳出

        if (BossStepOne(&faceL, L_OX, &fallL, shapeCount, dropDelay)) {
            bossEndReason = BEND_GAMEOVER;
            break;
        }
        if (BossStepOne(&faceR, R_OX, &fallR, shapeCount, dropDelay)) {
            bossEndReason = BEND_GAMEOVER;
            break;
        }         
        Sleep(1);
    }
}

void SaveRecordIfNeeded() // 判斷是否要記錄
{
    if (grade > max) {
        max = grade;
        SaveHighScore();
    }
}

void LoadHighScore() // 紀錄最高分
{
    FILE* pf = fopen("tetris_highscore.dat", "rb");
    if (pf == NULL) {
        max = 0;
        pf = fopen("tetris_highscore.dat", "wb");
        if (pf != NULL) {
            fwrite(&max, sizeof(int), 1, pf);
            fclose(pf);
        }
        return;
    }
    fread(&max, sizeof(int), 1, pf);
    fclose(pf);
}

void SaveHighScore() // 儲存最高分
{
    FILE* pf = fopen("tetris_highscore.dat", "wb");
    if (pf == NULL) {
        printf("Failed to save high score!\n");
        return;
    }
    fwrite(&max, sizeof(int), 1, pf);
    fclose(pf);
}

void PrintButton(int x, int y, const char* text, int selected, int bg) // 繪製Menu按鈕
{
    const int width = 18;
    int i;
    int len = (int)strlen(text);
    WORD attr = selected ? (WORD)((bg << 4) | 0) : (WORD)((0 << 4) | 15);
    SetAttr(attr);
    CursorJump(x, y);
    for (i = 0; i < width; i++) putchar(' ');
    CursorJump(x + (width - len) / 2, y);
    printf("%s", text);
    SetAttr(7);
}

int IsLegalPF(Face* f, int shape, int form, int x, int y) // Boss模式碰撞判斷
{
    int i, j, nx, ny;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            if (!block[shape][form].space[i][j]) continue;
            nx = x + j; ny = y + i;
            if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) return 0;
            if (f->data[ny][nx]) return 0;
        }
    return 1;
}

void MergePF(Face* f, int shape, int form, int x, int y) // 將落下方塊變為固定方塊
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (block[shape][form].space[i][j]) {
                f->data[y + i][x + j] = 1;
                f->color[y + i][x + j] = shape;
            }
        }
    }
}

int ClearLinesPF(Face* f, int ox) // 判斷是否滿行並執行
{
    int i, j, m, n;
    int cleared = 0;

    for (i = ROW - 2; i > 0; i--) {
        int sum = 0;
        for (j = 1; j < COL - 1; j++) sum += f->data[i][j];
        if (sum == 0) break;

        if (sum == COL - 2) {
            cleared++;
            for (j = 1; j < COL - 1; j++) {
                f->data[i][j] = 0;
                CursorJump(2*(ox + j), i);
                printf("  ");
            }
            // 方塊下移
            for (m = i; m > 1; m--) {
                for (n = 1; n < COL - 1; n++) {
                    f->data[m][n] = f->data[m-1][n];
                    f->color[m][n] = f->color[m-1][n];
                    CursorJump(2*(ox + n), m);
                    if (f->data[m][n]) { color(f->color[m][n]); printf("■"); }
                    else { printf("  "); }
                }
            }
            i++;
        }
    }
    return cleared;
}

void DrawBlockAt(int ox, int shape, int form, int x, int y) // 繪製下落中的方塊
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            if (block[shape][form].space[i][j]) {
                CursorJump(2*(ox + x + j), y + i);
                printf("■");
            }
}

void EraseBlockAtPF(Face* f, int ox, int shape, int form, int x, int y) // 擦掉下落中的方塊
{
    int i, j, bx, by;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            if (!block[shape][form].space[i][j]) continue;
            bx = x + j;
            by = y + i;
            if (bx < 0 || bx >= COL || by < 0 || by >= ROW) continue;
            CursorJump(2 * (ox + bx), by);
            if (f->data[by][bx]) {
                color(f->color[by][bx]);
                printf("■");
            } else {
                printf("  ");
            }
        }
}

void DrawNextInPanel(int x, int y, int shape, int form) // Boss模式Next方塊繪製
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            CursorJump(2*(x + j), y + i);
            printf("  ");
        }
    color(shape);
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            if (block[shape][form].space[i][j]) {
                CursorJump(2*(x + j), y + i);
                printf("■");
            }
        }
    SetAttr(7);
}

int BossSpawn(Face* f, Falling* p, int shapeCount) // 產生一顆新的下落方塊
{
    p->shape = p->nextShape;
    p->form  = p->nextForm;
    p->x = COL/2 - 2;
    p->y = 0;
    p->t = 0;

    p->nextShape = rand() % shapeCount;
    p->nextForm  = rand() % 4;

    if (!IsLegalPF(f, p->shape, p->form, p->x, p->y)) // 生成即碰撞則結束
        return 0;
        
    if (f == &faceL)
        DrawNextInPanel(P_OX + 3, 3,  p->nextShape, p->nextForm);
    else
        DrawNextInPanel(P_OX + 3, 22, p->nextShape, p->nextForm);

    return 1;
}

int BossStepOne(Face* f, int ox, Falling* p, int shapeCount, int dropDelayMs) // 單一盤面執行遊戲 (時間控制)
{
    DWORD now;
    int cleared;

    if (!p->alive) return 1;

    now = GetTickCount(); // 取得系統時間
              
    if (p->t <= 0) p->t = (int)now;
    if ((DWORD)now - (DWORD)p->t < (DWORD)dropDelayMs) // 檢查時間間隔
        return 0;
    p->t = (int)now;

    if (IsLegalPF(f, p->shape, p->form, p->x, p->y + 1)) { // 若下方可移動
        EraseBlockAtPF(f, ox, p->shape, p->form, p->x, p->y);
        p->y++;
        color(p->shape);
        DrawBlockAt(ox, p->shape, p->form, p->x, p->y);
        return 0;
    }

    MergePF(f, p->shape, p->form, p->x, p->y); // 固定

    cleared = ClearLinesPF(f, ox); // 檢查消行
    if (cleared > 0) {
        grade += 10 * cleared;
        color(12);
        CursorJump(2 * (P_OX + 2) + 5, ROW - 13);
        printf("%d   ", grade);
        SaveRecordIfNeeded();
    }

    if (!BossSpawn(f, p, shapeCount)) { // 生成失敗則結束
        p->alive = 0;
        return 1;
    }

    color(p->shape);
    DrawBlockAt(ox, p->shape, p->form, p->x, p->y);
    return 0;
}

void BossHandleInput(Falling* L, Falling* R) // 處理玩家鍵盤輸入 (左右控制)
{
    int ch, k, nf;
    if (!kbhit()) return;
    ch = getch();

    // --- 右盤控制 (方向鍵) ---
    if (ch == KEY_EXT1 || ch == KEY_EXT2) {
        k = getch();
        if (k == UP) { // 右盤硬降
             EraseBlockAtPF(&faceR, R_OX, R->shape, R->form, R->x, R->y);
             while (IsLegalPF(&faceR, R->shape, R->form, R->x, R->y + 1))
                 R->y++;
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
             MergePF(&faceR, R->shape, R->form, R->x, R->y);
             {
                 int cleared = ClearLinesPF(&faceR, R_OX);
                 if (cleared > 0) {
                     grade += 10 * cleared;
                     color(12);
                     CursorJump(2 * (P_OX + 2) + 5, ROW - 13);
                     printf("%d   ", grade);
                     SaveRecordIfNeeded();
                 }
             }
             if (!BossSpawn(&faceR, R, gBossShapeCount)) {
                 R->alive = 0;
                 bossEndReason = BEND_GAMEOVER;
                 return;
             }
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
             R->t = (int)GetTickCount();
             return;
        }
        if (k == LEFT && IsLegalPF(&faceR, R->shape, R->form, R->x - 1, R->y)){
             EraseBlockAtPF(&faceR, R_OX, R->shape, R->form, R->x, R->y);
             R->x--;
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
        } else if (k == RIGHT && IsLegalPF(&faceR, R->shape, R->form, R->x + 1, R->y)){
             EraseBlockAtPF(&faceR, R_OX, R->shape, R->form, R->x, R->y);
             R->x++;
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
        } else if (k == DOWN && IsLegalPF(&faceR, R->shape, R->form, R->x, R->y + 1)){
             EraseBlockAtPF(&faceR, R_OX, R->shape, R->form, R->x, R->y);
             R->y++;
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
        }
        return;
    }

    // --- 右盤控制 (旋轉) ---
    if (ch == SPACE) {
        nf = (R->form + 1) % 4;
        if (IsLegalPF(&faceR, R->shape, nf, R->x, R->y)){
             EraseBlockAtPF(&faceR, R_OX, R->shape, R->form, R->x, R->y);
             R->form = nf;
             color(R->shape);
             DrawBlockAt(R_OX, R->shape, R->form, R->x, R->y);
        }
        return;
    }

    // --- 左盤控制 (WASD) ---
    if (ch == 'a' || ch == 'A') { // 左移
        if (IsLegalPF(&faceL, L->shape, L->form, L->x - 1, L->y)){
             EraseBlockAtPF(&faceL, L_OX, L->shape, L->form, L->x, L->y);
             L->x--;
             color(L->shape);
             DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
        }
    } else if (ch == 'd' || ch == 'D') { // 右移
        if (IsLegalPF(&faceL, L->shape, L->form, L->x + 1, L->y)){
             EraseBlockAtPF(&faceL, L_OX, L->shape, L->form, L->x, L->y);
             L->x++;
             color(L->shape);
             DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
        }
    } else if (ch == 's' || ch == 'S') { // 加速下落
        if (IsLegalPF(&faceL, L->shape, L->form, L->x, L->y + 1)){
             EraseBlockAtPF(&faceL, L_OX, L->shape, L->form, L->x, L->y);
             L->y++;
             color(L->shape);
             DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
        }
    } else if (ch == 'e' || ch == 'E') { // 旋轉
        nf = (L->form + 1) % 4;
        if (IsLegalPF(&faceL, L->shape, nf, L->x, L->y)){
             EraseBlockAtPF(&faceL, L_OX, L->shape, L->form, L->x, L->y);
             L->form = nf;
             color(L->shape);
             DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
        }
    } else if (ch == 'w' || ch == 'W') { // 左盤硬降
         EraseBlockAtPF(&faceL, L_OX, L->shape, L->form, L->x, L->y);
         while (IsLegalPF(&faceL, L->shape, L->form, L->x, L->y + 1))
             L->y++;
         color(L->shape);
         DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
         MergePF(&faceL, L->shape, L->form, L->x, L->y);
         {
             int cleared = ClearLinesPF(&faceL, L_OX);
             if (cleared > 0) {
                 grade += 10 * cleared;
                 color(12);
                 CursorJump(2 * (P_OX + 2) + 5, ROW - 13);
                 printf("%d   ", grade);
                 SaveRecordIfNeeded();
             }
         }
         if (!BossSpawn(&faceL, L, gBossShapeCount)) {
             L->alive = 0;
             bossEndReason = BEND_GAMEOVER;
             return;
         }
         color(L->shape);
         DrawBlockAt(L_OX, L->shape, L->form, L->x, L->y);
         L->t = (int)GetTickCount();
    }

    // --- 共同控制 ---
    if (ch == ESC) {
        SaveRecordIfNeeded();
        bossEndReason = BEND_EXIT;
        L->alive = 0;
        R->alive = 0;
        return;
    } else if (ch == 'r' || ch == 'R') {
        SaveRecordIfNeeded();
        bossEndReason = BEND_RESTART;
        L->alive = 0;
        R->alive = 0;
        return;
    }
}

int BossGameOverUI(void) // BOSS 模式結束畫面
{
    char ch;
    system("cls");
    SetAttr(7);
    CursorJump(2 * (COL / 3), ROW / 2 - 3);

    if (bossEndReason == BEND_EXIT || bossEndReason == BEND_RESTART)
        printf("END GAME");
    else
        printf("GAME OVER");

    CursorJump(2 * (COL / 3), ROW / 2 - 1);
    if (grade > max) {
        printf("New record! %d", grade);
        SaveRecordIfNeeded();
    } else {
        printf("Record: %d  Your: %d", max, grade);
    }

    while (1) {
        CursorJump(2 * (COL / 3), ROW / 2 + 2);
        printf("Do you want to try again?(y / n)");
        scanf(" %c", &ch);
        if (ch == 'y' || ch == 'Y') return 0;
        if (ch == 'n' || ch == 'N') return -1;
        CursorJump(2 * (COL / 3), ROW / 2 + 3);
        printf("Invalid input, enter y/n/q.");
    }
}


