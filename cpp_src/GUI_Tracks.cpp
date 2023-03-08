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

/*
#define ROW_ON	'\xFF'
#define ROW_OFF	'\x00'

const char row0[15] = { ROW_OFF, ROW_ON, ROW_ON, ROW_ON, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF };
const char row1[15] = { ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_ON, ROW_ON, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF };
const char row2[15] = { ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_ON, ROW_ON, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF };
const char row3[15] = { ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_ON, ROW_ON, ROW_ON, ROW_OFF };
const char row4[15] = { ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF, ROW_OFF };
const char* colac[] = { row0, row1, row2, row3, row4 };
*/

void CTracks::DrawTrackHeader(int x, int y, int tr, int col)
{
	TTrack* tt = GetTrack(tr);
	CString s = "--  -----";

	if (tt)
	{
		s.Format(IsValidTrack(tr) ? "%02X: " : "--  ", tr);
		if (IsEmptyTrack(tr)) s.AppendFormat("EMPTY");
		else
		{
			s.AppendFormat(IsValidLength(tt->len) ? "%02X-" : "---", tt->len);
			s.AppendFormat(IsValidGo(tt->go) ? "%02X" : "--", tt->go);
		}
	}

	TextXY(s, x, y, col);
	TextXYSelN("<>", -1, x + 8 * 11, y, col);
	TextMiniXY("FX1", x + 8 * 10, y - 8);
}

void CTracks::DrawTrackLine(int col, int x, int y, int tr, int line, int aline, int cactview, int pline, BOOL isactive, int acu, int oob)
{
	TTrack* tt;
	char s[16] = " \x8\x8\x8 \x8\x8 \x8\x8 \x8\x8\x8";
	int len = -1, last = -1, go = -1, color = TEXT_COLOR_WHITE;
	int n, xline;
	
	if (tt = GetTrack(tr))
	{
		strcpy(s, " --- -- -- ---");

		len = tt->len;
		go = tt->go; 
		last = go >= 0 ? m_maxTrackLength : len;
		xline = line < len || go < 0 ? line : ((line - len) % (len - go)) + go;

		if (go >= 0) s[0] = line == len - 1 ? '\x10' : ' '; // Left-up arrow or nothing
		if (line == go) s[0] = line == len - 1 ? '\x11' : '\x0F';	// Left-up-right or up-right arrow

		// Note -- FIXME: set the Notation elsewhere instead of calculating it every time
		if ((n = tt->note[xline]) >= 0)
		{
			int octave = (n / g_notesperoctave) + 1 + 0x30;	// Due to ASCII characters
			int note = n % g_notesperoctave;
			int index = 0;	// Standard notation

			if (g_displayflatnotes) index += 1;
			if (g_usegermannotation) index += 2;
			if (g_notesperoctave != 12) index = 4;	// Non-12 scales don't yet have proper display

			s[1] = notesandscales[index][note][0];	// B
			s[2] = notesandscales[index][note][1];	// -
			s[3] = octave;							// 1
		}

		// Instrument
		if ((n = tt->instr[xline]) >= 0)
		{
			s[5] = CharH4(n);
			s[6] = CharL4(n);
		}

		// Volume
		if ((n = tt->volume[xline]) >= 0)
		{
			s[8] = 'v';
			s[9] = CharL4(n);
		}

		// Speed
		if ((n = tt->speed[xline]) >= 0)
		{
			s[11] = 'F';		// Fxx is for speed commands, but eventually, more commands could be used...
			s[12] = CharH4(n);
			s[13] = CharL4(n);
		}

		// Display the line highlight colours only in valid patterns
		if (line % g_trackLineSecondaryHighlight == 0)  color = TEXT_COLOR_GREEN;
		if (line % g_trackLinePrimaryHighlight == 0) color = TEXT_COLOR_CYAN;
	}

	// The displayed colours are set from lowest to highest priority, depending on the matching conditions
	if (line >= len) color = TEXT_COLOR_GRAY;
	if (line == pline) color = TEXT_COLOR_YELLOW;
	if (line == aline) color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
	if (oob) color = TEXT_COLOR_DARK_GRAY;

	// Output the constructed row once it's ready, using the cursor position for highlighted column 
	//TextXYCol(s, x, y, colac[g_activepart == PART_TRACKS && (isactive && line == aline && !oob) ? acu : 4], color);
	TextXYCol(s, x, y, g_activepart == PART_TRACKS && (isactive && line == aline && !oob) ? acu : -1, color);

	// Mark the end of a pattern here, if it ends on the next line
	if (line + 1 == last && len > 0 && last != m_maxTrackLength)
	{
		TextXY("\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B\x0B", x + 7, y + 13, (oob) ? TEXT_COLOR_DARK_GRAY : TEXT_COLOR_WHITE);
	}

}