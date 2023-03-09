#include "stdafx.h"
#include "resource.h"
#include <fstream>

#include "Atari6502.h"
#include "IOHelpers.h"

#include "Instruments.h"

#include "global.h"

#include "GuiHelpers.h"


/// <summary>
/// Draw instrument:
/// 4 general areas:
/// - Name
/// - Parameters
/// - Envelope
/// - Note table
/// 
/// </summary>
/// <param name="instrNr"></param>
void CInstruments::DrawInstrument(int instrNr)
{
	int i;
	char szBuffer[128];

	TInstrument* t = GetInstrument(instrNr);
	if (!t) return;

	// Line 8.5: Instrument XX (size xx bytes)
	sprintf(szBuffer, "INSTRUMENT %02X", instrNr);
	TextXY(szBuffer, INSTRS_X, INSTRS_Y, TEXT_COLOR_WHITE);

	int size = (t->parameters[PAR_ENV_LENGTH] + 1) * 3 + (t->parameters[PAR_TBL_LENGTH] + 1) + 12;
	sprintf(szBuffer, "(SIZE %u BYTES)", size);
	TextMiniXY(szBuffer, INSTRS_X + 14 * 8, INSTRS_Y + 5);

	DrawName(instrNr);

	// Draw some headings
	TextMiniXY("EFFECT", INSTRS_PARAM_X, INSTRS_PARAM_Y + 1 * 16 + 8);
	TextMiniXY("AUDCTL", INSTRS_PARAM_X + 0 * 8, INSTRS_PARAM_Y + 5 * 16 + 8);
	TextMiniXY("ENVELOPE", INSTRS_PARAM_X + 15 * 8, INSTRS_PARAM_Y + 16 + 8);
	TextMiniXY("TABLE", INSTRS_PARAM_X + 15 * 8, INSTRS_PARAM_Y + 8 * 16 + 8);

	// Draw envelope volume markers
	TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_ENV_X + 11 * 8 - 1, INSTRS_ENV_Y + 3 * 16, TEXT_COLOR_GRAY);
	//delimitation of space for Envelope VOLUME
	g_mem_dc->MoveTo(INSTRS_ENV_X + 12 * 8 - 1, INSTRS_ENV_Y + 7 * 16 - 1);
	g_mem_dc->LineTo(INSTRS_ENV_X + 12 * 8 + ENVELOPE_MAX_COLUMNS * 8, INSTRS_ENV_Y + 7 * 16 - 1);

	if (t->activeEditSection == INSTRUMENT_SECTION_ENVELOPE)
	{
		// Only when the cursor is on the envelope editor, draw the x position of the envelop index being edited
		sprintf(szBuffer, "POS %02X", t->editEnvelopeX);
		TextXY(szBuffer, INSTRS_ENV_X + 2 * 8, INSTRS_ENV_Y + 5 * 16, TEXT_COLOR_GRAY);
	}

	// Draw the headers of the envelop table parameters
	// Skip "VOLUME R:", hence start at 1
	for (i = 1; i < ENVROWS; i++)
	{
		TextXY(shenv[i].name, shenv[i].xpos, shenv[i].ypos, TEXT_COLOR_WHITE);
	}

	// For 8 channels draw the "VOLUME R:" header and volume markers
	if (g_tracks4_8 > 4)
	{
		TextXY(shenv[0].name, shenv[0].xpos, shenv[0].ypos, TEXT_COLOR_WHITE); //"VOLUME R:"
		TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_ENV_X + 11 * 8 - 1, INSTRS_ENV_Y - 2 * 16, TEXT_COLOR_GRAY);
		g_mem_dc->MoveTo(INSTRS_ENV_X + 12 * 8 - 1, INSTRS_ENV_Y + 2 * 16 - 1);
		g_mem_dc->LineTo(INSTRS_ENV_X + 12 * 8 + ENVELOPE_MAX_COLUMNS * 8, INSTRS_ENV_Y + 2 * 16 - 1);
	}

	for (i = 0; i < NUMBER_OF_PARAMS; i++) DrawParameter(i, instrNr);

	//TABLE TYPE icon (Notes or Freq)
	i = (t->parameters[PAR_TBL_TYPE] == 0) ? INSTRUMENT_TABLE_OF_NOTES : INSTRUMENT_TABLE_OF_FREQ;
	IconMiniXY(i, shpar[PAR_TBL_TYPE].x + 8 * 8 + 2, shpar[PAR_TBL_TYPE].y + 7);

	//inscription at the bottom of TABLE
	TextMiniXY((i == INSTRUMENT_TABLE_OF_NOTES) ? "TABLE OF NOTES" : "TABLE OF FREQS", INSTRS_TABLE_X, INSTRS_TABLE_Y - 8);

	//TABLE MODE icon (Set or Add)
	i = (t->parameters[PAR_TBL_MODE] == 0) ? INSTRUMENT_TABLE_MODE_SET : INSTRUMENT_TABLE_MODE_ADD;
	IconMiniXY(i, shpar[PAR_TBL_MODE].x + 8 * 8 + 2, shpar[PAR_TBL_MODE].y + 7);

	//ENVELOPE
	int len = t->parameters[PAR_ENV_LENGTH];	//par 5 is the length of the envelope
	for (i = 0; i <= len; i++) DrawEnv(i, instrNr);

	//ENVELOPE LOOP ARROWS
	szBuffer[1] = 0;
	int go = t->parameters[PAR_ENV_GOTO];		//par 14 is the GO loop envelope
	if (go < len)
	{
		szBuffer[0] = '\x07';	//Go from here
		TextXY(szBuffer, INSTRS_ENV_X + 12 * 8 + len * 8, INSTRS_ENV_Y + 7 * 16, TEXT_COLOR_WHITE);
		szBuffer[0] = '\x06';	//Go here

		int lengo = len - go;
		if (lengo > 3) NumberMiniXY(lengo + 1, INSTRS_ENV_X + 11 * 8 + 4 + go * 8 + lengo * 4, INSTRS_ENV_Y + 7 * 16 + 4); //len-go number
	}
	else
		szBuffer[0] = '\x16';	//GO from here to here
	TextXY(szBuffer, INSTRS_ENV_X + 12 * 8 + go * 8, INSTRS_ENV_Y + 7 * 16, TEXT_COLOR_WHITE);
	if (go > 2) NumberMiniXY(go, INSTRS_ENV_X + 11 * 8 + go * 4, INSTRS_ENV_Y + 7 * 16 + 4); //GO number

	//TABLE
	len = t->parameters[PAR_TBL_LENGTH];	//length table
	for (i = 0; i <= len; i++) DrawNoteTableValue(i, instrNr);

	//TABLE LOOP ARROWS
	go = t->parameters[PAR_TBL_GOTO];		//table GO loop
	if (len == 0)
	{
		TextXY("\x18", INSTRS_TABLE_X + 4, INSTRS_TABLE_Y + 8 + 16, TEXT_COLOR_WHITE);
	}
	else
	{
		TextXY("\x19", INSTRS_TABLE_X + go * 8 * 3, INSTRS_TABLE_Y + 8 + 16, TEXT_COLOR_WHITE);
		TextXY("\x1a", INSTRS_TABLE_X + 8 + len * 8 * 3, INSTRS_TABLE_Y + 8 + 16, TEXT_COLOR_WHITE);
	}

	//boundaries of all parts of the instrument
	/*
	CBrush br(RGB(112,112,112));
	g_mem_dc->FrameRect(CRect(INSTRS_X-2,INSTRS_Y-2,INSTRS_X+38*8+4,INSTRS_Y+2*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_PX-2,INSTRS_PY+16-2,INSTRS_PX+29*8,INSTRS_PY+15*16+4),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_EX,INSTRS_EY-2*16-2,INSTRS_EX+48*8,INSTRS_EY+15*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_TX-2,INSTRS_TY-2,INSTRS_TX+54*8+4,INSTRS_TY+2*16+8),&br);
	*/

	if (!g_viewInstrumentEditHelp) return; //does not want help => end
	//want help => continue

//separating line
#define HORIZONTALLINE {	g_mem_dc->MoveTo(INSTRS_HELP_X,INSTRS_HELP_Y-1); g_mem_dc->LineTo(INSTRS_HELP_X+93*8,INSTRS_HELP_Y-1); }

	if (t->activeEditSection == INSTRUMENT_SECTION_NAME)		// is the cursor on the instrument name?
		g_isEditingInstrumentName = 1;

	if (t->activeEditSection == INSTRUMENT_SECTION_ENVELOPE)	// is the cursor on the envelope?
	{
		g_isEditingInstrumentName = 0;
		switch (t->editEnvelopeY)
		{
			case ENV_DISTORTION:
			{
				int d = t->envelope[t->editEnvelopeX][ENV_DISTORTION];
				const char* distor_help[8] = {
					"Distortion 0, white noise. (AUDC $0v, Poly5+17/9)",
					"Distortion 2, square-ish tones. (AUDC $2v, Poly5)",
					"Distortion 4, no note table yet, Pure Table by default. (AUDC $4v, Poly4+5)",
					"16-Bit tones in valid channels, use command 6 to set the Distortion. (Distortion A by default)",
					"Distortion 8, white noise. (AUDC $8v, Poly17/9)",
					"Distortion A, pure tones. Special mode: CH1+CH3 1.79mhz + AUTOFILTER = Sawtooth (AUDC $Av)",
					"Distortion C, buzzy bass tones. (AUDC $Cv, Poly4)",
					"Distortion C, gritty bass tones. (AUDC $Cv, Poly4)" };
				const char* hs = distor_help[(d >> 1) & 0x07];
				TextXY(hs, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;

			case ENV_COMMAND:
			{
				int c = t->envelope[t->editEnvelopeX][ENV_COMMAND];
				const char* comm_help[8] = {
					"Play BASE_NOTE + $XY semitones.",
					"Play frequency $XY.",
					"Play BASE_NOTE + frequency $XY.",
					"Set BASE_NOTE += $XY semitones. Play BASE_NOTE.",
					"Set FSHIFT += frequency $XY. Play BASE_NOTE.",
					"Set portamento speed $X, step $Y. Play BASE_NOTE.",
					"Set FILTER_SHFRQ += $XY. $0Y = BASS16 Distortion. $FF/$01 = Sawtooth inversion (Distortion A).",
					"Set instrument AUDCTL. $FF = VOLUME ONLY mode. $FE/$FD = enable/disable Two-Tone Filter." };
				const char* hs = comm_help[c & 0x07];
				TextXY(hs, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;

			case ENV_X:
			case ENV_Y:
			{
				char i = (t->envelope[t->editEnvelopeX][ENV_X] << 4) | t->envelope[t->editEnvelopeX][ENV_Y];
				sprintf(szBuffer, "XY: $%02X = %i = %+i", (unsigned char)i, (unsigned char)i, i);
				TextXY(szBuffer, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;
		}
	}
	else if (t->activeEditSection == INSTRUMENT_SECTION_PARAMETERS)
	{
		// The cursor is on the main parameters
		g_isEditingInstrumentName = 0;
		switch (t->editParameterNr)
		{
			case PAR_DELAY:
			{
				unsigned char i = (t->parameters[t->editParameterNr]);
				if (i > 0)
					sprintf(szBuffer, "$%02X = %i", i, i);
				else
					sprintf(szBuffer, "$00 = no effects.");
				TextXY(szBuffer, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;

			case PAR_VOL_FADEOUT:
			{
				unsigned char i = (t->parameters[t->editParameterNr]);
				double f;
				if (i == 0) f = 0;
				else
					if (i == 0xff) f = 1;
					else
						f = (double)i / 256 + 0.0005;
				sprintf(szBuffer, "$%02X = -%.3f / vbi", (unsigned char)i, f);
				TextXY(szBuffer, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;
		}
	}
	if (t->activeEditSection == INSTRUMENT_SECTION_NOTETABLE)
	{
		// The cursor is on the table
		g_isEditingInstrumentName = 0;
		char i = (t->noteTable[t->editNoteTableCursorPos]);
		sprintf(szBuffer, "$%02X = %+i", (unsigned char)i, i);
		TextXY(szBuffer, INSTRS_HELP_X, INSTRS_HELP_Y, TEXT_COLOR_GRAY);
		//HORIZONTALLINE;
	}
}

/// <summary>
/// Draw the instrument's name.
/// Show edit state with cursor position.
/// Drawn in line 9
/// </summary>
/// <param name="instrNr"># of the instrument</param>
void CInstruments::DrawName(int instrNr)
{
	char* ptrName = GetName(instrNr);
	int cursorPos = -1;
	int color = TEXT_COLOR_TURQUOISE;

	if (g_activepart == PART_INSTRUMENTS && GetActiveEditSection(instrNr) == INSTRUMENT_SECTION_NAME)  // is an active change of instrument name
	{
		cursorPos = GetNameCursorPosition(instrNr);
		color = g_prove ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
		g_isEditingInstrumentName = 1;
	}

	TextXY("NAME:", INSTRS_X, INSTRS_Y + 16, TEXT_COLOR_WHITE);				// Draw the title
	TextXYSelN(ptrName, cursorPos, INSTRS_X + 6 * 8, INSTRS_Y + 16, color);	// Draw the name and highlight the cursor position
}

/// <summary>
/// Draw an instruments parameter:
/// </summary>
/// <param name="p"></param>
/// <param name="instrNr"></param>
void CInstruments::DrawParameter(int p, int instrNr)
{
	CString s = shpar[p].name;
	int x = shpar[p].x;
	int y = shpar[p].y;
	int showpar = GetParameter(instrNr, p) + shpar[p].displayOffset;
	int color = TEXT_COLOR_WHITE;

	TextXY(s, x, y, color);

	// Offset x to the end of parameter name after it was drawn
	x += 8 * (s.GetLength() + 1);

	// If the cursor is on the main parameters
	if (g_activepart == PART_INSTRUMENTS && GetActiveEditSection(instrNr) == INSTRUMENT_SECTION_PARAMETERS && GetParameterNumber(instrNr) == p)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	// Some parameters are 0..x but 1..x + 1 is displayed
	s.Format(shpar[p].maxParameterValue + shpar[p].displayOffset > 0x0F ? "%02X" : " %01X", showpar);
	TextXYSelN(s, -1, x, y, color);
}

BOOL CInstruments::CursorGoto(int instrNr, CPoint point, int pzone)
{
	if (instrNr < 0 || instrNr >= INSTRSNUM) return 0;
	TInstrument* tt = GetInstrument(instrNr);
	int x, y;

	g_isEditingInstrumentName = 0;	//when it is not edited, it shouldn't allow playing notes

	switch (pzone)
	{
		case 0:
			//envelope large table
			g_activepart = PART_INSTRUMENTS;
			tt->activeEditSection = INSTRUMENT_SECTION_ENVELOPE;	//the envelope is active
			x = point.x / 8;
			if (x >= 0 && x <= tt->parameters[PAR_ENV_LENGTH]) tt->editEnvelopeX = x;
			y = point.y / 16 + 1;
			if (y >= 1 && y < ENVROWS) tt->editEnvelopeY = y;
			return 1;
		case 1:
			//envelope line volume number of the right channel
			g_activepart = PART_INSTRUMENTS;
			tt->activeEditSection = INSTRUMENT_SECTION_ENVELOPE;	//the envelope is active
			x = point.x / 8;
			if (x >= 0 && x <= tt->parameters[PAR_ENV_LENGTH]) tt->editEnvelopeX = x;
			tt->editEnvelopeY = 0;
			return 1;
		case 2:
			//TABLE
			g_activepart = PART_INSTRUMENTS;
			tt->activeEditSection = INSTRUMENT_SECTION_NOTETABLE;	//the table is active
			x = (point.x + 4) / (3 * 8);
			if (x >= 0 && x <= tt->parameters[PAR_TBL_LENGTH]) tt->editNoteTableCursorPos = x;
			return 1;
		case 3:
			//INSTRUMENT NAME
			g_activepart = PART_INSTRUMENTS;
			tt->activeEditSection = INSTRUMENT_SECTION_NAME;	//the name is active 
			g_isEditingInstrumentName = 1;	//instrument name is being edited
			x = point.x / 8 - 6;
			if (x >= 0 && x <= INSTRUMENT_NAME_MAX_LEN) tt->editNameCursorPos = x;
			if (x < 0) tt->editNameCursorPos = 0;
			return 1;
		case 4:
			//INSTRUMENT PARAMETERS
		{
			x = point.x / 8;
			y = point.y / 16;
			if (x > 11 && x < 15) return 0; //middle empty part
			if (y < 0 || y>12) return 0; //just in case
			const int xytopar[2][12] =
			{
				{ PAR_DELAY,PAR_VIBRATO,PAR_FREQ_SHIFT,-1,PAR_AUDCTL_15KHZ,PAR_AUDCTL_HPF_CH2,PAR_AUDCTL_HPF_CH1,PAR_AUDCTL_JOIN_3_4,PAR_AUDCTL_JOIN_1_2,PAR_AUDCTL_179_CH3,PAR_AUDCTL_179_CH1,PAR_AUDCTL_POLY9 },
				{ PAR_ENV_LENGTH,PAR_ENV_GOTO,PAR_VOL_FADEOUT,PAR_VOL_MIN,-1,-1,-1,PAR_TBL_LENGTH,PAR_TBL_GOTO,PAR_TBL_SPEED,PAR_TBL_TYPE,PAR_TBL_MODE}
			};
			int p;
			p = xytopar[(x > 11)][y];
			if (p >= 0 && p < NUMBER_OF_PARAMS)
			{
				tt->editParameterNr = p;
				g_activepart = PART_INSTRUMENTS;
				tt->activeEditSection = INSTRUMENT_SECTION_PARAMETERS;	//parameters are active
				return 1;
			}
		}
		return 0;

		case 5:
			//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
			//left mouse button
			//changes GO and moves LEN if necessary
			x = point.x / 8;
			if (x < 0) x = 0; else if (x >= ENVELOPE_MAX_COLUMNS) x = ENVELOPE_MAX_COLUMNS - 1;
			tt->parameters[PAR_ENV_GOTO] = x;
			if (tt->parameters[PAR_ENV_LENGTH] < x) tt->parameters[PAR_ENV_LENGTH] = x;
		CG_InstrumentParametersChanged:
			//because there has been some change in the instrument parameter => this instrument will stop on all channels
			Atari_InstrumentTurnOff(instrNr);
			CheckInstrumentParameters(instrNr);
			//something changed => Save instrument "to Atari"
			WasModified(instrNr);
			return 1;

		case 6:
			//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
			//right mouse button
			//changes LEN and moves GO if necessary
			x = point.x / 8;
			if (x < 0) x = 0; else if (x >= ENVELOPE_MAX_COLUMNS) x = ENVELOPE_MAX_COLUMNS - 1;
			tt->parameters[PAR_ENV_LENGTH] = x;
			if (tt->parameters[PAR_ENV_GOTO] > x) tt->parameters[PAR_ENV_GOTO] = x;
			goto CG_InstrumentParametersChanged;
		case 7:
			//TABLE SET LEN/GO PARAMETER by MOUSE
			//left mouse button
			//changes GO and moves LEN if necessary
			x = (point.x + 4) / (3 * 8);
			if (x < 0) x = 0; else if (x >= NOTE_TABLE_MAX_LEN) x = NOTE_TABLE_MAX_LEN - 1;
			tt->parameters[PAR_TBL_GOTO] = x;
			if (tt->parameters[PAR_TBL_LENGTH] < x) tt->parameters[PAR_TBL_LENGTH] = x;
			goto CG_InstrumentParametersChanged;
		case 8:
			//TABLE SET LEN/GO PARAMETER by MOUSE
			//right mouse button
			//changes LEN and moves GO if necessary
			x = (point.x + 4) / (3 * 8);
			if (x < 0) x = 0; else if (x >= NOTE_TABLE_MAX_LEN) x = NOTE_TABLE_MAX_LEN - 1;
			tt->parameters[PAR_TBL_LENGTH] = x;
			if (tt->parameters[PAR_TBL_GOTO] > x) tt->parameters[PAR_TBL_GOTO] = x;
			goto CG_InstrumentParametersChanged;
	}
	return 0;
}

/// <summary>
/// Query the bounding rectangle of the requested GUI element
/// </summary>
/// <param name="instrNr">Instrument #</param>
/// <param name="zone">Which GUI zone is queried</param>
/// <param name="rect">Write the rectangle info here</param>
/// <returns></returns>
BOOL CInstruments::GetGUIArea(int instrNr, int zone, CRect& rect)
{
	int len = GetParameter(instrNr, PAR_ENV_LENGTH) + 1;
	int tabl = GetParameter(instrNr, PAR_TBL_LENGTH) + 1;

	switch (zone)
	{
		case INSTR_GUI_ZONE_ENVELOPE_LEFT_ENVELOPE:
			//left channel volume curve (lower)
			rect.SetRect(INSTRS_ENV_X + 12 * 8, INSTRS_ENV_Y + 3 * 16 + 4, INSTRS_ENV_X + 12 * 8 + len * 8, INSTRS_ENV_Y + 3 * 16 + 4 + 4 * 16);
			return 1;

		case INSTR_GUI_ZONE_ENVELOPE_RIGHT_ENVELOPE:
			//right channel volume curve (upper)
			if (g_tracks4_8 <= 4) return 0;
			rect.SetRect(INSTRS_ENV_X + 12 * 8, INSTRS_ENV_Y - 2 * 16 + 4, INSTRS_ENV_X + 12 * 8 + len * 8, INSTRS_ENV_Y - 2 * 16 + 4 + 4 * 16);
			return 1;

		case INSTR_GUI_ZONE_ENVELOPE_PARAM_TABLE:
			//envelope area large table
			rect.SetRect(INSTRS_ENV_X + 12 * 8, INSTRS_ENV_Y + 3 * 16 + 0 + 5 * 16, INSTRS_ENV_X + 12 * 8 + len * 8, INSTRS_ENV_Y + 3 * 16 + 0 + 5 * 16 + 7 * 16);
			return 1;

		case INSTR_GUI_ZONE_ENVELOPE_RIGHT_VOL_NUMS:
			//envelope area of volume numbers for right channel
			if (g_tracks4_8 <= 4) return 0;
			rect.SetRect(INSTRS_ENV_X + 12 * 8, INSTRS_ENV_Y - 2 * 16 + 0 + 4 * 16, INSTRS_ENV_X + 12 * 8 + len * 8, INSTRS_ENV_Y - 2 * 16 + 0 + 4 * 16 + 16);
			return 1;

		case INSTR_GUI_ZONE_NOTE_TABLE:
			//instrument table line
			rect.SetRect(INSTRS_TABLE_X, INSTRS_TABLE_Y + 8, INSTRS_TABLE_X + tabl * 24 - 8, INSTRS_TABLE_Y + 8 + 16);
			return 1;
		
		case INSTR_GUI_ZONE_INSTRUMENT_NAME:
			//instrument name
			rect.SetRect(INSTRS_PARAM_X, INSTRS_PARAM_Y - 16, INSTRS_PARAM_X + 6 * 8 + INSTRUMENT_NAME_MAX_LEN * 8, INSTRS_PARAM_Y + 0);
			return 1;

		case INSTR_GUI_ZONE_PARAMETERS:
			//instrument parameters
			rect.SetRect(INSTRS_PARAM_X, INSTRS_PARAM_Y + 32, INSTRS_PARAM_X + 26 * 8, INSTRS_PARAM_Y + 32 + 12 * 16);
			return 1;

		case INSTR_GUI_ZONE_INSTRUMENT_NUMBER_DLG:
			//instrument number
			rect.SetRect(INSTRS_X, INSTRS_Y, INSTRS_X + 13 * 8, INSTRS_Y + 16);
			return 1;

		case 8:
			//envelope area under the left (lower) volume curve
			rect.SetRect(INSTRS_ENV_X + 12 * 8, INSTRS_ENV_Y + 3 * 16 + 0 + 4 * 16, INSTRS_ENV_X + 12 * 8 + ENVELOPE_MAX_COLUMNS * 8, INSTRS_ENV_Y + 3 * 16 + 0 + 4 * 16 + 16);
			return 1;

		case 9:
			//instrument table + 1 line below parameter table 
			rect.SetRect(INSTRS_TABLE_X, INSTRS_TABLE_Y + 8 + 1 * 16, INSTRS_TABLE_X + NOTE_TABLE_MAX_LEN * 24 - 8, INSTRS_TABLE_Y + 8 + 2 * 16);
			return 1;
	}
	return 0;
}

void CInstruments::DrawEnv(int e, int it)
{
	TInstrument* in = GetInstrument(it);
	int volR = in->envelope[e][ENV_VOLUMER] & 0x0f; // Volume Right
	int volL = in->envelope[e][ENV_VOLUMEL] & 0x0f; // Volume Left/Mono
	int color;
	int x = INSTRS_ENV_X + 12 * 8 + e * 8;
	char s[2], a;
	s[1] = 0;
	int ay = (in->activeEditSection == INSTRUMENT_SECTION_ENVELOPE && in->editEnvelopeX == e) ? in->editEnvelopeY : -1;

	// Volume Only mode uses Command 7 with $XY == $FF
	COLORREF fillColor = (in->envelope[e][ENV_COMMAND] == 0x07 && in->envelope[e][ENV_X] == 0x0f && in->envelope[e][ENV_Y] == 0x0f) ?
		RGB(128, 255, 255) : RGB(255, 255, 255);

	// Volume column
	if (volL) g_mem_dc->FillSolidRect(x, INSTRS_ENV_Y + 3 * 16 + 4 + 4 * (15 - volL), 8, volL * 4, fillColor);

	if (g_tracks4_8 > 4 && volR) g_mem_dc->FillSolidRect(x, INSTRS_ENV_Y - 2 * 16 + 4 + 4 * (15 - volR), 8, volR * 4, fillColor);

	for (int j = 0; j < 8; j++)
	{
		if ((a = shenv[j].ch) != 0)
		{
			if (in->envelope[e][j])
				s[0] = a;
			else
				s[0] = 8;	// Character in the envelope
		}
		else
			s[0] = CharL4(in->envelope[e][j]);

		if (j == ay && g_activepart == PART_INSTRUMENTS)
		{
			color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
		}
		else color = TEXT_COLOR_WHITE;

		if (j == 0)
		{
			if (g_tracks4_8 > 4) TextXY(s, x, INSTRS_ENV_Y + 2 * 16, color);		 // Volume R is out of the box
		}
		else
			TextXY(s, x, INSTRS_ENV_Y + 7 * 16 + j * 16, color);
	}
}

/// <summary>
/// Draw a note table value. Two numbers. Top is the table index, below is the note value
/// Each entry is 24 pixels wide.
/// </summary>
/// <param name="noteIdx">Which table entry</param>
/// <param name="instrNr">Which instrument</param>
void CInstruments::DrawNoteTableValue(int noteIdx, int instrNr)
{
	TInstrument* data = GetInstrument(instrNr);
	char szBuffer[3];

	// Draw the position #
	szBuffer[0] = CharH4(noteIdx);
	szBuffer[1] = CharL4(noteIdx);
	szBuffer[2] = 0;
	TextMiniXY(szBuffer, INSTRS_TABLE_X + noteIdx * 24, INSTRS_TABLE_Y);

	// Note Table parameter
	sprintf(szBuffer, "%02X", data->noteTable[noteIdx]);

	int color = TEXT_COLOR_WHITE;
	if (data->activeEditSection == INSTRUMENT_SECTION_NOTETABLE && data->editNoteTableCursorPos == noteIdx && g_activepart == PART_INSTRUMENTS)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	TextXY(szBuffer, INSTRS_TABLE_X + noteIdx * 24, INSTRS_TABLE_Y + 8, color);
}