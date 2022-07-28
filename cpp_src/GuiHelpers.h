#pragma once

#include "General.h"

// Helper defines to make the code a bit more readabl
#define SCALE(x) ((x) * g_scaling_percentage) / 100

#define SCREENUPDATE	g_screenupdate=1


#define COL_BLOCK		56
#define RGBMUTE			RGB(120,160,240)
#define RGBNORMAL		RGB(255,255,255)
#define RGBVOLUMEONLY	RGB(128,255,255)
#define RGBTWOTONE		RGB(128,255,0) 
#define RGBBACKGROUND	RGB(34,50,80)	//dark blue
#define RGBLINES		RGB(149,194,240)	//blue gray
#define RGBBLACK		RGB(0,0,0)	//black

extern void SetStatusBarText(const char* text);
extern int EditText(int vk, int shift, int control, char* txt, int& cur, int max);

extern void TextXY(char* txt, int x, int y, int color = TEXT_COLOR_WHITE);
extern void TextXYSelN(char* txt, int n, int x, int y, int color = TEXT_COLOR_WHITE);
extern void TextXYCol(char* txt, int x, int y, char* col);
extern void TextDownXY(char* txt, int x, int y, int color = TEXT_COLOR_WHITE);
extern void NumberMiniXY(BYTE num, int x, int y, int color = TEXT_MINI_COLOR_GRAY);
extern void TextMiniXY(char* txt, int x, int y, int color = TEXT_MINI_COLOR_GRAY);
extern void IconMiniXY(int icon, int x, int y);
