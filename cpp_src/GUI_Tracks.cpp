#include "stdafx.h"
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"


const char* notesandscales[5][40] =
{
	// Standard Western Notation, Sharp (#) accidentals 
	{ "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" },

	// Standard Western Notation, Flat (b) accidentals 
	{ "C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "Bb", "B-" },

	// German Notation, Sharp (#) accidentals 
	{ "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-" },

	// German Notation, Flat (b) accidentals 
	{ "C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "B-", "H-" },

	// Test Notation
	{ "1-", "2-", "3-", "4-", "5-", "6-", "7-", "8-", "9-", "A-", "B-", "C-",
	"D-", "E-", "F-", "G-", "H-", "I-", "J-", "K-", "L-", "M-", "N-", "O-",
	"P-", "Q-", "R-", "S-", "T-", "U-", "V-", "W-", "X-", "Y-", "Z-" }
};

//TODO: Optimise the notes arrays to be more compact, there is a lot of duplicates
const char* notes[] =
{ "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","B-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","B-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","B-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","B-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","B-5",
  "C-6","???","???","???"
};

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

BOOL CTracks::DrawTrackHeader(int col, int x, int y, int tr)
{
	TTrack* tt;
	char t[16] = "  --  -----  ";
	
	if (tt = GetTrack(tr))
	{
		t[2] = CharH4(tr);
		t[3] = CharL4(tr);
		t[4] = ':';
		if (tt->len >= 0)
		{
			t[6] = CharH4(tt->len);
			t[7] = CharL4(tt->len);
		}
		if (tt->go >= 0)
		{
			t[9] = CharH4(tt->go);
			t[10] = CharL4(tt->go);
		}
	}

	if (IsEmptyTrack(tr)) strncpy(t + 6, "EMPTY", 5);
	TextXY(t, x + 8, TRACKS_Y + 16, GetChannelOnOff(col) ? 0 : 1);
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