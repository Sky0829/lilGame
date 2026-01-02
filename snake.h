// snake.h
#ifndef SNAKE_H
#define SNAKE_H

#include <stdio.h>
#include <windows.h>
#include <conio.h>

// 1. 定義你的結構體
typedef struct {
	int x, y;
} Point;

// 2. 宣告你會用到的功能 (這就是目錄)
void Setup();
void Draw();
void Input();
void Logic();

#endif