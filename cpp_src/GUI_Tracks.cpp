#include "stdafx.h"
using namespace std;
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"


//highlight colours on active rows in patterns
//	X	X	:		C	#	1		I	I		v	V		!	S	S	
static char csel0[15] = { 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };	//use for active lines
static char c = COLOR_SELECTED;
static char csel1[15] = { 6,c,c,c,6,6,6,6,6,6,6,6,6,6,6 };
static char csel2[15] = { 6,6,6,6,6,c,c,6,6,6,6,6,6,6,6 };
static char csel3[15] = { 6,6,6,6,6,6,6,6,c,c,6,6,6,6,6 };
static char csel4[15] = { 6,6,6,6,6,6,6,6,6,6,6,c,c,c,6 };
static char* colac[] =
{
	csel1,csel2,csel3,csel4
};

static char cselprove0[15] = { 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13 };	//use for active lines
static char p = COLOR_SELECTED_PROVE;
static char cselprove1[15] = { 13,p,p,p,13,13,13,13,13,13,13,13,13,13,13 };
static char cselprove2[15] = { 13,13,13,13,13,p,p,13,13,13,13,13,13,13,13 };
static char cselprove3[15] = { 13,13,13,13,13,13,13,13,p,p,13,13,13,13,13 };
static char cselprove4[15] = { 13,13,13,13,13,13,13,13,13,13,13,p,p,p,13 };
static char* colacprove[] =
{
	cselprove1,cselprove2,cselprove3,cselprove4
};

BOOL CTracks::DrawTrackHeader(int col, int x, int y, int tr)	// , int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu)	//not needed: line_cnt, aline, cactview, pline, isactive, acu
{
	char t[16];
	//int c, len, last, go;
	//len = last = 0;
	int c, len, go;
	len = 0;
	TTrack* tt = NULL;

	//offset Y to be 1 line above
	y -= 16;

	strcpy(t, "  --  -----  ");
	if (tr >= 0 && tr <= 253)
	{
		tt = &m_track[tr];

		t[2] = CharH4(tr);
		t[3] = CharL4(tr);
		t[4] = ':';

		//len = last = tt->len;	//len a last
		len = tt->len;
		go = tt->go;	//go

		if (IsEmptyTrack(tr))
		{
			strncpy(t + 6, "EMPTY", 5);
		}
		else
		{	//non-empty track
			if (len >= 0)
			{
				t[6] = CharH4(len);
				t[7] = CharL4(len);
			}
			if (go >= 0)
			{
				t[9] = CharH4(go);
				t[10] = CharL4(go);
				//last = m_maxtracklen;
			}
		}
	}

	//and now draw the infos above tracks
	c = (GetChannelOnOff(col)) ? 0 : 1;
	TextXY(t, x + 8, TRACKS_Y + 16, c);
	return 1;
}

BOOL CTracks::DrawTrackLine(int col, int x, int y, int tr, int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu, int oob)
{
	char s[16];
	int line, xline, n, color, len, last, go, endline;

	len = last = endline = 0;
	TTrack* tt = NULL;

	line = line_cnt;

	if (tr >= 0 && tr <= 253)
	{
		tt = &m_track[tr];
		len = last = tt->len;	//len a last
		go = tt->go;	//go
		if (go >= 0) last = m_maxTrackLength;
	}

	//fetch the last line infos early, so it could be drawn immediately after the track line was processed
	if (line + 1 == last && len > 0 && last != m_maxTrackLength)
	{
		endline = line;
	}

	if (line >= last)
	{
		if (line == aline && isactive) color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;	//blue or red
		else if (line == pline) color = TEXT_COLOR_YELLOW;	//yellow
		else color = TEXT_COLOR_GRAY;	//gray
		if (oob) color = TEXT_COLOR_DARK_GRAY;	//darker gray, out of bounds
			TextXY(" \x8\x8\x8 \x8\x8 \x8\x8 \x8\x8\x8", x, y, color);	//empty track line
		return 0;
	}

	strcpy(s, " --- -- -- ---");
	if (tt)
	{
		if (line < len || go < 0) xline = line;
		else xline = ((line - len) % (len - go)) + go;

		if ((n = tt->note[xline]) >= 0)
		{
			//notes
			int octave = (n / g_notesperoctave) + 1 + 0x30;	//due to ASCII characters
			int note = n % g_notesperoctave;
			int index = 0;	//standard notation

			if (g_displayflatnotes) index += 1;
			if (g_usegermannotation) index += 2;
			if (g_notesperoctave != 12) index = 4;	//non-12 scales don't yet have proper display

			s[1] = notesandscales[index][note][0];	// B
			s[2] = notesandscales[index][note][1];	// -
			s[3] = octave;							// 1
		}
		if ((n = tt->instr[xline]) >= 0)
		{
			//instrument
			s[5] = CharH4(n);
			s[6] = CharL4(n);
		}
		if ((n = tt->volume[xline]) >= 0)
		{
			//volume
			s[8] = 'v';
			s[9] = CharL4(n);
		}
		if ((n = tt->speed[xline]) >= 0)
		{
			//speed
			s[11] = 'F';			//Fxx is the Famitracker speed command, because one day, RMT *will* have speed commands support
			s[12] = CharH4(n);
			s[13] = CharL4(n);
		}
	}

	if (line == go)
	{
		if (line == len - 1) s[0] = '\x11';			//left-up-right arrow
		else s[0] = '\x0f';			//up-right arrow
	}
	else if (line == len - 1 && go >= 0) s[0] = '\x10'; //left-up arrow

	//colours
	if (line == aline && isactive) color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;	//blue or red
	else if (line == pline) color = TEXT_COLOR_YELLOW;	//yellow
	else if (line >= len) color = TEXT_COLOR_GRAY;	//gray
	else if ((line % g_trackLinePrimaryHighlight) == 0 || (line % g_trackLineSecondaryHighlight) == 0)
		color = ((line % g_trackLinePrimaryHighlight) == 0) ? TEXT_COLOR_CYAN : TEXT_COLOR_GREEN;	//cyan or green
	else color = TEXT_COLOR_WHITE;	//white
	if (oob) color = TEXT_COLOR_DARK_GRAY;

	if (g_activepart == PART_TRACKS && line < len && (line == aline && isactive) && !oob)
	{
		if (g_prove) TextXYCol(s, x, y, colacprove[acu]);
		else TextXYCol(s, x, y, colac[acu]);
	}
	else TextXY(s, x, y, color);

	if (endline)
	{
		TextXY("\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B", x + 7, y + 13, (oob) ? TEXT_COLOR_DARK_GRAY : TEXT_COLOR_WHITE);
	}

	return 1;
}