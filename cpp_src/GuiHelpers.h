#pragma once

#include "General.h"

// Helper defines to make the code a bit more readabl
#define SCALE(x) ((x) * g_scaling_percentage) / 100
#define INVERSE_SCALE(x) ((x) * 100) / g_scaling_percentage

#define SCREENUPDATE	g_screenupdate = 1


extern void SetStatusBarText(const char* text);
extern int EditText(int vk, int shift, int control, char* txt, int& cur, int max);

extern void TextXY(char* txt, int x, int y, int color = TEXT_COLOR_WHITE);
extern void TextXYSelN(char* txt, int n, int x, int y, int color = TEXT_COLOR_WHITE);
extern void TextXYCol(char* txt, int x, int y, char* col);
extern void TextDownXY(char* txt, int x, int y, int color = TEXT_COLOR_WHITE);
extern void NumberMiniXY(BYTE num, int x, int y, int color = TEXT_MINI_COLOR_GRAY);
extern void TextMiniXY(char* txt, int x, int y, int color = TEXT_MINI_COLOR_GRAY);
extern void IconMiniXY(int icon, int x, int y);
