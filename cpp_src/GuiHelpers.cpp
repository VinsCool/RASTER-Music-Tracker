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

static int lastTick;

BOOL RefreshScreen(int frameskip)
{
	// Bail out of this function if it couldn't be performed
	if (!g_hwnd || !g_viewhwnd || g_closeApplication)
		return 0;

	// Frameskip of 1 or higher
	if (frameskip > 0)
	{
		// Frame was already processed
		if (lastTick == g_timerGlobalCount)
			return 0;

		// Skip frame with modulo
		if ((g_timerGlobalCount % frameskip))
			return 0;

		// Remember the last time a frame was processed
		lastTick = g_timerGlobalCount;
	}

	// Force a screen update if the condition is met for it
	AfxGetApp()->GetMainWnd()->Invalidate();
	SCREENUPDATE;
	UpdateWindow(g_viewhwnd);

	// Screen was refreshed
	return 1;
}

void GetTracklineText(char* dest, int line)
{
	if (line < 0 || line>0xff) { dest[0] = 0; return; }
	if (g_tracklinealtnumbering)
	{
		int a = line / g_trackLinePrimaryHighlight;
		if (a >= 35) a = (a - 35) % 26 + 'a' - '9' + 1;
		int b = line % g_trackLinePrimaryHighlight;
		if (b >= 35) b = (b - 35) % 26 + 'a' - '9' + 1;
		if (a <= 8)
			a = '1' + a;
		else
			a = 'A' - 9 + a;
		if (b <= 8)
			b = '1' + b;
		else
			b = 'A' - 9 + b;
		dest[0] = a;
		dest[1] = b;
		dest[2] = 0;
	}
	else
		sprintf(dest, "%02X", line);
}

int EditText(int vk, int shift, int control, char* txt, int& cur, int max)
{
	//returns 1 if TAB or ENTER was pressed
	max--;
	if (vk == VK_BACKSPACE)
	{
		if (cur > 0)
		{
			cur--;
			for (int j = cur; j <= max - 1; j++) txt[j] = txt[j + 1];
			txt[max] = ' ';
		}
	}
	else if (vk == VK_TAB || vk == VK_ENTER)
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
		if (vk >= 'A' && vk <= 'Z')			{ a = (shift) ? vk : vk + 32; }						//letters - uppercase with SHIFT
		else if (vk >= '0' && vk <= '9')	{ a = (shift) ? *(")!@#$%^&*(" + vk - 48) : vk; }	//numbers - special characters with SHIFT
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

BOOL IsHoveredXY(int x, int y, int xLength, int yLength)
{
	int px = g_mouseLastPointX, py = g_mouseLastPointY;
	int xTo = x + xLength, yTo = y + yLength;

	return (px >= x && px < xTo) && (py >= y && py < yTo);
}

void TextXY(const char* txt, int x, int y, int color)
{
	char charToDraw;
	color = color << 4;
	for (int i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		if (charToDraw == 32) continue;	// Don't draw the space
		g_mem_dc->BitBlt(x, y, 8, 16, g_gfx_dc, (charToDraw & 0x7f) << 3, color, SRCCOPY);
	}
}

void TextXYFull(const char* txt, int& x, int& y)
{
	int color = TEXT_COLOR_WHITE << 4, ori_x = x, ori_y = y;

	for (int i = 0; char charToDraw = (txt[i]); i++)
	{
		switch (charToDraw)
		{
		case '\n': x = ori_x; y += 16; continue;
		case ' ': x += 8; continue;
		case '\x80': color = TEXT_COLOR_WHITE << 4; continue;
		case '\x82': color = TEXT_COLOR_YELLOW << 4; continue;
		case '\x83': color = COLOR_SELECTED_PROVE << 4; continue;
		case '\x85': color = TEXT_COLOR_CYAN << 4; continue;
		case '\x86': color = TEXT_COLOR_RED << 4; continue;
		case '\x89': color = COLOR_SELECTED << 4; continue;
		case '\x8B': color = TEXT_COLOR_GREEN << 4; continue;
		case '\x8C': color = TEXT_COLOR_DARK_GRAY << 4; continue;
		case '\x8D': color = TEXT_COLOR_BLUE << 4; continue;
		}

		g_mem_dc->BitBlt(x, y, 8, 16, g_gfx_dc, (charToDraw & 0x7f) << 3, color, SRCCOPY);
		x += 8;
	}
}

void TextXYSelN(const char* txt, int n, int x, int y, int color)
{
	color <<= 4;

	int col = (g_prove ? COLOR_SELECTED_PROVE : COLOR_SELECTED) << 4;
	int cur = COLOR_HOVERED << 4;

	// The characters 'n' will use the "select" color, everything else will use the 'color' parameter, unless they are hovered by the mouse cursor
	for (int i = 0; char charToDraw = txt[i]; i++, x += 8)
	{
		g_mem_dc->BitBlt(x, y, 8, 16, g_gfx_dc, (charToDraw & 0x7F) << 3, IsHoveredXY(x, y, 8, 16) ? cur : i == n ? col : color, SRCCOPY);
	}
}

// Draw 8x16 chars with given color array per char position
// TODO: make a lookup table for the cursor highlight position
void TextXYCol(const char* txt, int x, int y, int acu, int color)
{
	color <<= 4;

	int num = 0, curnum = 0, curoff = 0;
	int col = (g_prove ? COLOR_SELECTED_PROVE : COLOR_SELECTED) << 4;
	int cur = COLOR_HOVERED << 4;

	// RMTE variables
	if (g_isRMTE)
	{
		switch (acu)
		{
		case 0: acu = 0; num = 3; break;	// Note
		case 1: acu = 4; num = 2; break;	// Instrument
		case 2: acu = 7; num = 2; break;	// Volume
		case 3: acu = 10; num = 3; break;	// Effect(s)
		case 4: acu = 14; num = 3; break;
		case 5: acu = 18; num = 3; break;
		case 6: acu = 22; num = 3; break;
		case 7: acu = 26; num = 3; break;
		}
	}

	// Legacy variables
	else
	{
		switch (acu)
		{
		case 0: acu = 1; num = 3; break;	// Note
		case 1: acu = 5; num = 2; break;	// Instrument
		case 2: acu = 8; num = 2; break;	// Volume
		case 3: acu = 11; num = 3; break;	// Effect(s)
		}
	}

	for (int i = 0; char charToDraw = txt[i]; i++, x += 8)
	{
		if (charToDraw == 32) continue;	// Don't draw the space

		g_mem_dc->BitBlt(x, y, 8, 16, g_gfx_dc, (charToDraw & 0x7F) << 3, IsHoveredXY(x, y, 8, 16) ? cur : i >= acu && i < acu + num ? col : color, SRCCOPY);
	}
}

// Draw 8x16 chars vertically (one below the other)
void TextDownXY(const char* txt, int x, int y, int color)
{
	char charToDraw;
	color = color << 4;	// 16 pixels height
	for (int i = 0; charToDraw = (txt[i]); i++, y += 16)
	{
		g_mem_dc->BitBlt(x, y, 8, 16, g_gfx_dc, (charToDraw & 0x7f) << 3, color, SRCCOPY);
	}
}

void NumberMiniXY(const BYTE num, int x, int y, int color)
{
	color = 112 + (color << 3);
	g_mem_dc->BitBlt(x, y, 8, 8, g_gfx_dc, (num & 0xf0) >> 1, color, SRCCOPY);
	g_mem_dc->BitBlt(x + 8, y, 8, 8, g_gfx_dc, (num & 0x0f) << 3, color, SRCCOPY);
}

void TextMiniXY(const char* txt, int x, int y, int color)
{
	char charToDraw;
	color = 112 + (color << 3);
	for (int i = 0; charToDraw = (txt[i]); i++, x += 8)
	{
		if (charToDraw == 32) continue;
		g_mem_dc->BitBlt(x, y, 8, 8, g_gfx_dc, (charToDraw & 0x7f) << 3, color, SRCCOPY);
	}
}

void IconMiniXY(const int icon, int x, int y)
{
	static int c = 128 - 6;
	if (icon >= 1 && icon <= 4)
	{
		g_mem_dc->BitBlt(x, y, 32, 6, g_gfx_dc, (icon - 1) * 32, c, SRCCOPY);
	}
}