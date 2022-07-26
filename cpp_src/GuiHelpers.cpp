#include "stdafx.h"
#include "General.h"
#include "resource.h"
#include "MainFrm.h"

#include "GuiHelpers.h"

#include "global.h"

void SetStatusBarText(const char* text)
{
	CStatusBar& sb = ((CMainFrame*)AfxGetApp()->GetMainWnd())->m_wndStatusBar;
	sb.SetWindowText(text);
}

int EditText(int vk, int shift, int control, char* txt, int& cur, int max)
{
	//returns 1 if TAB or ENTER was pressed
	max--;
	if (vk == 8) //VK_BACKSPACE
	{
		if (cur > 0)
		{
			cur--;
			for (int j = cur; j <= max - 1; j++) txt[j] = txt[j + 1];
			txt[max] = ' ';
		}
	}
	else if (vk == VK_TAB || vk == 13)		//VK_ENTER
	{
		return 1;
	}
	else if (vk == VK_INSERT)
	{
		for (int j = max - 1; j >= cur; j--) txt[j + 1] = txt[j];
		txt[cur] = ' ';
	}
	else if (vk == VK_DELETE)
	{
		for (int j = cur; j <= max - 1; j++) txt[j] = txt[j + 1];
		txt[max] = ' ';
	}
	else
	{
		if (control) return 0;
		char a = 0;
		if (vk >= 65 && vk <= 90) { a = (shift) ? vk : vk + 32; }		//letters - uppercase with SHIFT
		else if (vk >= 48 && vk <= 57) { a = (shift) ? *(")!@#$%^&*(" + vk - 48) : vk; }			//numbers - special characters with SHIFT
		else if (vk == ' ')			a = ' ';	//space
		else if (vk == 189)	a = (shift) ? '_' : '-';
		else if (vk == 187)	a = (shift) ? '+' : '=';
		else if (vk == 219)	a = (shift) ? '{' : '[';
		else if (vk == 221)	a = (shift) ? '}' : ']';
		else if (vk == 186)	a = (shift) ? ':' : ';';
		else if (vk == 222)	a = (shift) ? '"' : '\'';
		else if (vk == 188)	a = (shift) ? '<' : ',';
		else if (vk == 190)	a = (shift) ? '>' : '.';
		else if (vk == 191)	a = (shift) ? '?' : '/';
		else if (vk == 220)	a = (shift) ? '|' : '\\';
		else if (vk == VK_RIGHT)
		{
			if (cur < max) cur++;
		}
		else if (vk == VK_LEFT)
		{
			if (cur > 0) cur--;
		}
		else if (vk == VK_HOME) cur = 0;
		else if (vk == VK_END)
		{
			int j;
			for (j = max; j >= 0 && (txt[j] == ' '); j--);
			cur = (j < max) ? j + 1 : max;
		}

		if (a > 0)
		{
			for (int j = max - 1; j >= cur; j--) txt[j + 1] = txt[j];
			txt[cur] = a;
			if (cur < max) cur++;
		}
	}
	return 0;
}


void TextXY(char* txt, int x, int y, int color)
{
	int sp = g_scaling_percentage;

	char charToDraw;
	color = color << 4;
	for (int i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		if (charToDraw == 32) continue;	// Don't draw the space
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(16), g_gfx_dc, (charToDraw & 0x7f) << 3, color, 8, 16, SRCCOPY);
	}
}

void TextXYSelN(char* txt, int n, int x, int y, int color)
{
	//the index character n will have a "select" color, the rest c
	int sp = g_scaling_percentage;
	char charToDraw;
	color = color << 4;

	int col = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;

	int i;
	for (i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(16), g_gfx_dc, (charToDraw & 0x7f) << 3, (i == n) ? col * 16 : color, 8, 16, SRCCOPY);
	}
	if (i == n) //the first character after the end of the string
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(16), g_gfx_dc, 32 << 3, col * 16, 8, 16, SRCCOPY);
}

// Draw 8x16 chars with given color array per char position
void TextXYCol(char* txt, int x, int y, char* col)
{
	int sp = g_scaling_percentage;
	char charToDraw;
	for (int i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(16), g_gfx_dc, (charToDraw & 0x7f) << 3, col[i] << 4, 8, 16, SRCCOPY);
	}
}

// Draw 8x16 chars vertically (one below the other)
void TextDownXY(char* txt, int x, int y, int color)
{
	int sp = g_scaling_percentage;
	char charToDraw;
	color = color << 4;	// 16 pixels height
	for (int i = 0; charToDraw = (txt[i]); i++, y += 16)
	{
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(16), g_gfx_dc, (charToDraw & 0x7f) << 3, color, 8, 16, SRCCOPY);
	}
}

void NumberMiniXY(BYTE num, int x, int y, int color)
{
	int sp = g_scaling_percentage;
	color = 112 + (color << 3);
	g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(8), g_gfx_dc, (num & 0xf0) >> 1, color, 8, 8, SRCCOPY);
	g_mem_dc->StretchBlt(SCALE(x + 8), SCALE(y), SCALE(8), SCALE(8), g_gfx_dc, (num & 0x0f) << 3, color, 8, 8, SRCCOPY);
}

void TextMiniXY(char* txt, int x, int y, int color)
{
	int sp = g_scaling_percentage;
	char charToDraw;
	color = 112 + (color << 3);
	for (int i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		if (charToDraw == 32) continue;
		g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(8), SCALE(8), g_gfx_dc, (charToDraw & 0x7f) << 3, color, 8, 8, SRCCOPY);
	}
}

void IconMiniXY(int icon, int x, int y)
{
	int sp = g_scaling_percentage;
	static int c = 128 - 6;
	if (icon >= 1 && icon <= 4) g_mem_dc->StretchBlt(SCALE(x), SCALE(y), SCALE(32), SCALE(6), g_gfx_dc, (icon - 1) * 32, c, 32, 6, SRCCOPY);
}