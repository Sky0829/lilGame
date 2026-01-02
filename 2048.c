#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

// 遊戲結構定義
typedef struct {
    int cells[4][4];
    int score;
    int best;
} Game;

// --- 核心邏輯函式 ---

// 初始化隨機數
void init_rng() {
    srand((unsigned int)time(NULL));
}

// 清除畫面（支援 Windows ANSI 顏色）
void clear_screen() {
#ifdef _WIN32
    static bool tried = false;
    static bool vt = false;
    if (!tried) {
        tried = true;
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, mode)) vt = true;
        }
    }
    if (vt) printf("\x1b[2J\x1b[H");
    else system("cls");
#else
    printf("\x1b[2J\x1b[H");
#endif
}

// 取得數字顏色
const char* get_color(int v) {
    switch (v) {
        case 0:    return "\x1b[0m";
        case 2:    return "\x1b[48;5;230m\x1b[30m";
        case 4:    return "\x1b[48;5;229m\x1b[30m";
        case 8:    return "\x1b[48;5;215m\x1b[30m";
        case 16:   return "\x1b[48;5;209m\x1b[30m";
        case 32:   return "\x1b[48;5;203m\x1b[37m";
        case 64:   return "\x1b[48;5;196m\x1b[37m";
        case 128:  return "\x1b[48;5;221m\x1b[30m";
        case 256:  return "\x1b[48;5;220m\x1b[30m";
        case 512:  return "\x1b[48;5;214m\x1b[30m";
        case 1024: return "\x1b[48;5;208m\x1b[30m";
        case 2048: return "\x1b[48;5;202m\x1b[30m";
        default:   return "\x1b[48;5;240m\x1b[37m";
    }
}

// 新增隨機方塊
void add_random_tile(Game *g) {
    int empty[16][2], count = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (g->cells[r][c] == 0) {
                empty[count][0] = r;
                empty[count][1] = c;
                count++;
            }
    if (count > 0) {
        int idx = rand() % count;
        g->cells[empty[idx][0]][empty[idx][1]] = (rand() % 10 == 0) ? 4 : 2;
    }
}

// 重新開始遊戲
void restart_game(Game *g) {
    memset(g->cells, 0, sizeof(g->cells));
    g->score = 0;
    add_random_tile(g);
    add_random_tile(g);
}

// 繪製畫面
void draw(Game *g) {
    clear_screen();
    printf("==== 2048 C Version ====\n");
    printf("Score: %-10d Best: %d\n", g->score, g->best);
    printf("Move: Arrows/WASD | R: Restart | Q: Quit\n\n");
    
    for (int r = 0; r < 4; r++) {
        printf("+------+------+------+------+\n|");
        for (int c = 0; c < 4; c++) {
            int v = g->cells[r][c];
            if (v == 0) {
                printf("%s%6s\x1b[0m|", get_color(v), ".");
            } else {
                printf("%s%6d\x1b[0m|", get_color(v), v);
            }
        }
        printf("\n");
    }
    printf("+------+------+------+------+\n\n");
}

// 合併單列數字
int compress_and_merge(int line[4]) {
    int tmp[4] = {0}, k = 0, gained = 0;
    for (int i = 0; i < 4; i++) if (line[i] != 0) tmp[k++] = line[i];
    for (int i = 0; i < 3; i++) {
        if (tmp[i] != 0 && tmp[i] == tmp[i+1]) {
            tmp[i] *= 2;
            gained += tmp[i];
            tmp[i+1] = 0;
            i++;
        }
    }
    memset(line, 0, sizeof(int) * 4);
    k = 0;
    for (int i = 0; i < 4; i++) if (tmp[i] != 0) line[k++] = tmp[i];
    return gained;
}

// 四個方向的移動
bool move_left(Game *g) {
    bool moved = false;
    for (int r = 0; r < 4; r++) {
        int old[4]; memcpy(old, g->cells[r], 16);
        g->score += compress_and_merge(g->cells[r]);
        if (memcmp(old, g->cells[r], 16) != 0) moved = true;
    }
    return moved;
}

bool move_right(Game *g) {
    bool moved = false;
    for (int r = 0; r < 4; r++) {
        int line[4] = {g->cells[r][3], g->cells[r][2], g->cells[r][1], g->cells[r][0]};
        g->score += compress_and_merge(line);
        if (g->cells[r][0] != line[3] || g->cells[r][1] != line[2]) moved = true;
        for (int i = 0; i < 4; i++) g->cells[r][i] = line[3-i];
    }
    return moved;
}

bool move_up(Game *g) {
    bool moved = false;
    for (int c = 0; c < 4; c++) {
        int line[4] = {g->cells[0][c], g->cells[1][c], g->cells[2][c], g->cells[3][c]};
        g->score += compress_and_merge(line);
        if (g->cells[0][c] != line[0] || g->cells[1][c] != line[1]) moved = true;
        for (int r = 0; r < 4; r++) g->cells[r][c] = line[r];
    }
    return moved;
}

bool move_down(Game *g) {
    bool moved = false;
    for (int c = 0; c < 4; c++) {
        int line[4] = {g->cells[3][c], g->cells[2][c], g->cells[1][c], g->cells[0][c]};
        g->score += compress_and_merge(line);
        if (g->cells[3][c] != line[0] || g->cells[2][c] != line[1]) moved = true;
        for (int r = 0; r < 4; r++) g->cells[3-r][c] = line[r];
    }
    return moved;
}

// 檢查是否還能移動
bool can_move(Game *g) {
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) if (g->cells[r][c] == 0) return true;
    for (int r = 0; r < 4; r++) for (int c = 0; c < 3; c++) if (g->cells[r][c] == g->cells[r][c+1]) return true;
    for (int c = 0; c < 4; c++) for (int r = 0; r < 3; r++) if (g->cells[r][c] == g->cells[r+1][c]) return true;
    return false;
}

// --- 被其他程式呼叫的介面 ---

void start_2048_game() {
    init_rng();
    Game game;
    game.best = 0;
    restart_game(&game);

    while (true) {
        if (game.score > game.best) game.best = game.score;
        draw(&game);
        
        if (!can_move(&game)) {
            printf("Game Over! Press R to restart, Q to quit.\n");
        }

        int ch = _getch();
        if (ch == 'q' || ch == 'Q') break; 
        if (ch == 'r' || ch == 'R') { restart_game(&game); continue; }

        bool moved = false;
        if (ch == 0 || ch == 224) { // 方向鍵
            int arrow = _getch();
            if (arrow == 72)      moved = move_up(&game);
            else if (arrow == 80) moved = move_down(&game);
            else if (arrow == 75) moved = move_left(&game);
            else if (arrow == 77) moved = move_right(&game);
        } else { // WASD
            if (ch == 'w' || ch == 'W')      moved = move_up(&game);
            else if (ch == 's' || ch == 'S') moved = move_down(&game);
            else if (ch == 'a' || ch == 'A') moved = move_left(&game);
            else if (ch == 'd' || ch == 'D') moved = move_right(&game);
        }

        if (moved) {
            add_random_tile(&game);
        }
    }
    printf("\nExiting 2048... Press any key.\n");
    _getch();
}