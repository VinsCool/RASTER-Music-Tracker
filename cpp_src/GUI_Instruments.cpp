#include "stdafx.h"
#include "resource.h"
#include <fstream>
using namespace std;

#include "Atari6502.h"
#include "IOHelpers.h"

#include "Instruments.h"

#include "global.h"

#include "GUI_Instruments.h"
#include "GuiHelpers.h"


BOOL CInstruments::DrawInstrument(int it)
{
	int i;
	char s[128];
	int sp = g_scaling_percentage;

	TInstrument& t = m_instr[it];

	sprintf(s, "INSTRUMENT %02X", it);
	TextXY(s, INSTRS_X, INSTRS_Y, TEXT_COLOR_WHITE);
	int size = (t.par[PAR_ENVLEN] + 1) * 3 + (t.par[PAR_TABLEN] + 1) + 12;
	sprintf(s, "(SIZE %u BYTES)", size);
	TextMiniXY(s, INSTRS_X + 14 * 8, INSTRS_Y + 5);
	DrawName(it);

	TextMiniXY("EFFECT", INSTRS_PX, INSTRS_PY + 1 * 16 + 8);
	TextMiniXY("AUDCTL", INSTRS_PX + 0 * 8, INSTRS_PY + 5 * 16 + 8);
	TextMiniXY("ENVELOPE", INSTRS_PX + 15 * 8, INSTRS_PY + 16 + 8);
	TextMiniXY("TABLE", INSTRS_PX + 15 * 8, INSTRS_PY + 8 * 16 + 8);
	//
	TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX + 11 * 8 - 1, INSTRS_EY + 3 * 16, TEXT_COLOR_GRAY);

	if (t.act == 2)	//only when the cursor is on the envelope edit
	{
		sprintf(s, "POS %02X", t.activeenvx);
		TextXY(s, INSTRS_EX + 2 * 8, INSTRS_EY + 5 * 16, TEXT_COLOR_GRAY);
	}
	//
	for (i = 1; i < ENVROWS; i++) //omits "VOLUME R:"
	{
		TextXY(shenv[i].name, shenv[i].xpos, shenv[i].ypos, TEXT_COLOR_WHITE);
	}

	if (g_tracks4_8 > 4)
	{
		TextXY(shenv[0].name, shenv[0].xpos, shenv[0].ypos, TEXT_COLOR_WHITE); //"VOLUME R:"
		TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX + 11 * 8 - 1, INSTRS_EY - 2 * 16, TEXT_COLOR_GRAY);
		g_mem_dc->MoveTo(((INSTRS_EX + 12 * 8 - 1) * sp) / 100, ((INSTRS_EY + 2 * 16 - 1) * sp) / 100);
		g_mem_dc->LineTo(((INSTRS_EX + 12 * 8 + ENVCOLS * 8) * sp) / 100, ((INSTRS_EY + 2 * 16 - 1) * sp) / 100);
	}

	for (i = 0; i < NUMBEROFPARS; i++) DrawPar(i, it);

	//TABLE TYPE icon
	i = (t.par[PAR_TABTYPE] == 0) ? 1 : 2;
	IconMiniXY(i, shpar[PAR_TABTYPE].x + 8 * 8 + 2, shpar[PAR_TABTYPE].y + 7);
	//inscription at the bottom of TABLE
	TextMiniXY((i == 1) ? "TABLE OF NOTES" : "TABLE OF FREQS", INSTRS_TX, INSTRS_TY - 8);

	//TABLE MODE icon
	i = (t.par[PAR_TABMODE] == 0) ? 3 : 4;
	IconMiniXY(i, shpar[PAR_TABMODE].x + 8 * 8 + 2, shpar[PAR_TABMODE].y + 7);

	s[1] = 0;

	//ENVELOPE
	int len = t.par[PAR_ENVLEN];	//par 13 is the length of the envelope
	for (i = 0; i <= len; i++) DrawEnv(i, it);

	//ENVELOPE LOOP ARROWS
	int go = t.par[PAR_ENVGO];		//par 14 is the GO loop envelope
	if (go < len)
	{
		s[0] = '\x07';	//Go from here
		TextXY(s, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY + 7 * 16, TEXT_COLOR_WHITE);
		s[0] = '\x06';	//Go here

		int lengo = len - go;
		if (lengo > 3) NumberMiniXY(lengo + 1, INSTRS_EX + 11 * 8 + 4 + go * 8 + lengo * 4, INSTRS_EY + 7 * 16 + 4); //len-go number
	}
	else
		s[0] = '\x16';	//GO from here to here
	TextXY(s, INSTRS_EX + 12 * 8 + go * 8, INSTRS_EY + 7 * 16, TEXT_COLOR_WHITE);
	if (go > 2) NumberMiniXY(go, INSTRS_EX + 11 * 8 + go * 4, INSTRS_EY + 7 * 16 + 4); //GO number

	//TABLE
	len = t.par[PAR_TABLEN];	//length table
	for (i = 0; i <= len; i++) DrawTab(i, it);

	//TABLE LOOP ARROWS
	go = t.par[PAR_TABGO];		//table GO loop
	if (len == 0)
	{
		TextXY("\x18", INSTRS_TX + 4, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
	}
	else
	{
		TextXY("\x19", INSTRS_TX + go * 8 * 3, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
		TextXY("\x1a", INSTRS_TX + 8 + len * 8 * 3, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
	}

	//delimitation of space for Envelope VOLUME
	g_mem_dc->MoveTo(((INSTRS_EX + 12 * 8 - 1) * sp) / 100, ((INSTRS_EY + 7 * 16 - 1) * sp) / 100);
	g_mem_dc->LineTo(((INSTRS_EX + 12 * 8 + ENVCOLS * 8) * sp) / 100, ((INSTRS_EY + 7 * 16 - 1) * sp) / 100);

	//boundaries of all parts of the instrument
	/*
	CBrush br(RGB(112,112,112));
	g_mem_dc->FrameRect(CRect(INSTRS_X-2,INSTRS_Y-2,INSTRS_X+38*8+4,INSTRS_Y+2*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_PX-2,INSTRS_PY+16-2,INSTRS_PX+29*8,INSTRS_PY+15*16+4),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_EX,INSTRS_EY-2*16-2,INSTRS_EX+48*8,INSTRS_EY+15*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_TX-2,INSTRS_TY-2,INSTRS_TX+54*8+4,INSTRS_TY+2*16+8),&br);
	*/

	if (!g_viewinstractivehelp) return 1; //does not want help => end
	//want help => continue

//separating line
#define HORIZONTALLINE {	g_mem_dc->MoveTo(INSTRS_HX,INSTRS_HY-1); g_mem_dc->LineTo(INSTRS_HX+93*8,INSTRS_HY-1); }

	if (t.act == 0)	//is the cursor on the instrument name?
		is_editing_instr = 1;

	if (t.act == 2)	//is the cursor on the envelope?
	{
		is_editing_instr = 0;
		switch (t.activeenvy)
		{
			case ENV_DISTORTION:
			{
				int d = t.env[t.activeenvx][ENV_DISTORTION];
				static char* distor_help[8] = {
					"Distortion 0, white noise. (AUDC $0v, Poly5+17/9)",
					"Distortion 2, square-ish tones. (AUDC $2v, Poly5)",
					"Distortion 4, no note table yet, Pure Table by default. (AUDC $4v, Poly4+5)",
					"16-Bit tones in valid channels, use command 6 to set the Distortion. (Distortion A by default)",
					"Distortion 8, white noise. (AUDC $8v, Poly17/9)",
					"Distortion A, pure tones. Special mode: CH1+CH3 1.79mhz + AUTOFILTER = Sawtooth (AUDC $Av)",
					"Distortion C, buzzy bass tones. (AUDC $Cv, Poly4)",
					"Distortion C, gritty bass tones. (AUDC $Cv, Poly4)" };
				char* hs = distor_help[(d >> 1) & 0x07];
				TextXY(hs, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;

			case ENV_COMMAND:
			{
				int c = t.env[t.activeenvx][ENV_COMMAND];
				static char* comm_help[8] = {
					"Play BASE_NOTE + $XY semitones.",
					"Play frequency $XY.",
					"Play BASE_NOTE + frequency $XY.",
					"Set BASE_NOTE += $XY semitones. Play BASE_NOTE.",
					"Set FSHIFT += frequency $XY. Play BASE_NOTE.",
					"Set portamento speed $X, step $Y. Play BASE_NOTE.",
					"Set FILTER_SHFRQ += $XY. $0Y = BASS16 Distortion. $FF/$01 = Sawtooth inversion (Distortion A).",
					"Set instrument AUDCTL. $FF = VOLUME ONLY mode. $FE/$FD = enable/disable Two-Tone Filter." };
				char* hs = comm_help[c & 0x07];
				TextXY(hs, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;

			case ENV_X:
			case ENV_Y:
			{
				char i = (t.env[t.activeenvx][ENV_X] << 4) | t.env[t.activeenvx][ENV_Y];
				sprintf(s, "XY: $%02X = %i = %+i", (unsigned char)i, (unsigned char)i, i);
				TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
				//HORIZONTALLINE;
			}
			break;
		}
	}
	else
		if (t.act == 1)	//the cursor is on the main parameters
		{
			is_editing_instr = 0;
			switch (t.activepar)
			{
				case PAR_DELAY:
				{
					unsigned char i = (t.par[t.activepar]);
					if (i > 0)
						sprintf(s, "$%02X = %i", i, i);
					else
						sprintf(s, "$00 = no effects.");
					TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
					//HORIZONTALLINE;
				}
				break;

				case PAR_VSLIDE:
				{
					unsigned char i = (t.par[t.activepar]);
					double f;
					if (i == 0) f = 0;
					else
						if (i == 0xff) f = 1;
						else
							f = (double)i / 256 + 0.0005;
					sprintf(s, "$%02X = -%.3f / vbi", (unsigned char)i, f);
					TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
					//HORIZONTALLINE;
				}
				break;
			}
		}
	if (t.act == 3)	//the cursor is on the table
	{
		is_editing_instr = 0;
		char i = (t.tab[t.activetab]);
		sprintf(s, "$%02X = %+i", (unsigned char)i, i);
		TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
		//HORIZONTALLINE;
	}
	return 1;
}

BOOL CInstruments::DrawName(int it)
{
	char* s = GetName(it);
	int n = -1, color = TEXT_COLOR_WHITE;

	if (g_activepart == PART_INSTRUMENTS && m_instr[it].act == 0)  //is an active change of instrument name
	{
		n = m_instr[it].activenam;
		if (g_prove) color = TEXT_COLOR_BLUE;
		else color = TEXT_COLOR_RED;
		is_editing_instr = 1;
	}
	else color = TEXT_COLOR_LIGHT_GRAY;

	TextXY("NAME:", INSTRS_X, INSTRS_Y + 16, TEXT_COLOR_WHITE);
	TextXYSelN(s, n, INSTRS_X + 6 * 8, INSTRS_Y + 16, color);
	return 1;
}

BOOL CInstruments::DrawPar(int p, int it)
{
	int sp = g_scaling_percentage;
	char s[2];
	s[1] = 0;
	char* txt = shpar[p].name;
	int x = shpar[p].x;
	int y = shpar[p].y;

	char a;
	int color = TEXT_COLOR_WHITE;

	for (int i = 0; a = (txt[i]); i++, x += 8)
	{
		//necessary! this is a custom textxy function for displaying instruments...
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, color, 8, 16, SRCCOPY);
	}

	//if the cursor is on the main parameters
	if (g_activepart == PART_INSTRUMENTS && m_instr[it].act == 1 && m_instr[it].activepar == p)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	x += 8;
	int showpar = m_instr[it].par[p] + shpar[p].pfrom;	//some parameters are 0..x but 1..x + 1 is displayed

	if (shpar[p].pmax + shpar[p].pfrom > 0x0f)
	{
		s[0] = CharH4(showpar);
		TextXY(s, x, y, color);
	}

	x += 8;
	s[0] = CharL4(showpar);
	TextXY(s, x, y, color);

	return 1;
}



BOOL CInstruments::CursorGoto(int instr, CPoint point, int pzone)
{
	if (instr < 0 || instr >= INSTRSNUM) return 0;
	TInstrument& tt = m_instr[instr];
	int x, y;

	is_editing_instr = 0;	//when it is not edited, it shouldn't allow playing notes

	switch (pzone)
	{
		case 0:
			//envelope large table
			g_activepart = PART_INSTRUMENTS;
			tt.act = 2;	//the envelope is active
			x = point.x / 8;
			if (x >= 0 && x <= tt.par[PAR_ENVLEN]) tt.activeenvx = x;
			y = point.y / 16 + 1;
			if (y >= 1 && y < ENVROWS) tt.activeenvy = y;
			return 1;
		case 1:
			//envelope line volume number of the right channel
			g_activepart = PART_INSTRUMENTS;
			tt.act = 2;	//the envelope is active
			x = point.x / 8;
			if (x >= 0 && x <= tt.par[PAR_ENVLEN]) tt.activeenvx = x;
			tt.activeenvy = 0;
			return 1;
		case 2:
			//TABLE
			g_activepart = PART_INSTRUMENTS;
			tt.act = 3;	//the table is active
			x = (point.x + 4) / (3 * 8);
			if (x >= 0 && x <= tt.par[PAR_TABLEN]) tt.activetab = x;
			return 1;
		case 3:
			//INSTRUMENT NAME
			g_activepart = PART_INSTRUMENTS;
			tt.act = 0;	//the name is active 
			is_editing_instr = 1;	//instrument name is being edited
			x = point.x / 8 - 6;
			if (x >= 0 && x <= INSTRNAMEMAXLEN) tt.activenam = x;
			if (x < 0) tt.activenam = 0;
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
				{ PAR_DELAY,PAR_VIBRATO,PAR_FSHIFT,-1,PAR_AUDCTL0,PAR_AUDCTL1,PAR_AUDCTL2,PAR_AUDCTL3,PAR_AUDCTL4,PAR_AUDCTL5,PAR_AUDCTL6,PAR_AUDCTL7 },
				{ PAR_ENVLEN,PAR_ENVGO,PAR_VSLIDE,PAR_VMIN,-1,-1,-1,PAR_TABLEN,PAR_TABGO,PAR_TABSPD,PAR_TABTYPE,PAR_TABMODE}
			};
			int p;
			p = xytopar[(x > 11)][y];
			if (p >= 0 && p < NUMBEROFPARS)
			{
				tt.activepar = p;
				g_activepart = PART_INSTRUMENTS;
				tt.act = 1;	//parameters are active
				return 1;
			}
		}
		return 0;

		case 5:
			//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
			//left mouse button
			//changes GO and moves LEN if necessary
			x = point.x / 8;
			if (x < 0) x = 0; else if (x >= ENVCOLS) x = ENVCOLS - 1;
			tt.par[PAR_ENVGO] = x;
			if (tt.par[PAR_ENVLEN] < x) tt.par[PAR_ENVLEN] = x;
		CG_InstrumentParametersChanged:
			//because there has been some change in the instrument parameter => this instrument will stop on all channels
			Atari_InstrumentTurnOff(instr);
			CheckInstrumentParameters(instr);
			//something changed => Save instrument "to Atari"
			ModificationInstrument(instr);
			return 1;

		case 6:
			//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
			//right mouse button
			//changes LEN and moves GO if necessary
			x = point.x / 8;
			if (x < 0) x = 0; else if (x >= ENVCOLS) x = ENVCOLS - 1;
			tt.par[PAR_ENVLEN] = x;
			if (tt.par[PAR_ENVGO] > x) tt.par[PAR_ENVGO] = x;
			goto CG_InstrumentParametersChanged;
		case 7:
			//TABLE SET LEN/GO PARAMETER by MOUSE
			//left mouse button
			//changes GO and moves LEN if necessary
			x = (point.x + 4) / (3 * 8);
			if (x < 0) x = 0; else if (x >= TABLEN) x = TABLEN - 1;
			tt.par[PAR_TABGO] = x;
			if (tt.par[PAR_TABLEN] < x) tt.par[PAR_TABLEN] = x;
			goto CG_InstrumentParametersChanged;
		case 8:
			//TABLE SET LEN/GO PARAMETER by MOUSE
			//right mouse button
			//changes LEN and moves GO if necessary
			x = (point.x + 4) / (3 * 8);
			if (x < 0) x = 0; else if (x >= TABLEN) x = TABLEN - 1;
			tt.par[PAR_TABLEN] = x;
			if (tt.par[PAR_TABGO] > x) tt.par[PAR_TABGO] = x;
			goto CG_InstrumentParametersChanged;
	}
	return 0;
}

BOOL CInstruments::GetInstrArea(int instr, int zone, CRect& rect)
{
	int len = m_instr[instr].par[PAR_ENVLEN] + 1;
	switch (zone)
	{
		case 0:
			//left channel volume curve (lower)
			rect.SetRect(INSTRS_EX + 12 * 8, INSTRS_EY + 3 * 16 + 4, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY + 3 * 16 + 4 + 4 * 16);
			return 1;
		case 1:
			//right channel volume curve (upper)
			if (g_tracks4_8 <= 4) return 0;
			rect.SetRect(INSTRS_EX + 12 * 8, INSTRS_EY - 2 * 16 + 4, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY - 2 * 16 + 4 + 4 * 16);
			return 1;
		case 2:
			//envelope area large table
			rect.SetRect(INSTRS_EX + 12 * 8, INSTRS_EY + 3 * 16 + 0 + 5 * 16, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY + 3 * 16 + 0 + 5 * 16 + 7 * 16);
			return 1;
		case 3:
			//envelope area of volume numbers for right channel
			if (g_tracks4_8 <= 4) return 0;
			rect.SetRect(INSTRS_EX + 12 * 8, INSTRS_EY - 2 * 16 + 0 + 4 * 16, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY - 2 * 16 + 0 + 4 * 16 + 16);
			return 1;
		case 4:
			//instrument table line
		{
			int tabl = m_instr[instr].par[PAR_TABLEN] + 1;
			rect.SetRect(INSTRS_TX, INSTRS_TY + 8, INSTRS_TX + tabl * 24 - 8, INSTRS_TY + 8 + 16);
			return 1;
		}
		case 5:
			//instrument name
			rect.SetRect(INSTRS_PX, INSTRS_PY - 16, INSTRS_PX + 6 * 8 + INSTRNAMEMAXLEN * 8, INSTRS_PY + 0);
			return 1;
		case 6:
			//instrument parameters
			rect.SetRect(INSTRS_PX, INSTRS_PY + 32, INSTRS_PX + 26 * 8, INSTRS_PY + 32 + 12 * 16);
			return 1;
		case 7:
			//instrument number
			rect.SetRect(INSTRS_X, INSTRS_Y, INSTRS_X + 13 * 8, INSTRS_Y + 16);
			return 1;
		case 8:
			//envelope area under the left (lower) volume curve
			rect.SetRect(INSTRS_EX + 12 * 8, INSTRS_EY + 3 * 16 + 0 + 4 * 16, INSTRS_EX + 12 * 8 + ENVCOLS * 8, INSTRS_EY + 3 * 16 + 0 + 4 * 16 + 16);
			return 1;
		case 9:
			//instrument table + 1 line below parameter table 
			rect.SetRect(INSTRS_TX, INSTRS_TY + 8 + 1 * 16, INSTRS_TX + TABLEN * 24 - 8, INSTRS_TY + 8 + 2 * 16);
			return 1;
	}
	return 0;
}

void CInstruments::DrawEnv(int e, int it)
{
	int sp = g_scaling_percentage;
	TInstrument& in = m_instr[it];
	int volR = in.env[e][ENV_VOLUMER] & 0x0f; //volume right
	int volL = in.env[e][ENV_VOLUMEL] & 0x0f; //volume left/mono
	int color;
	int x = INSTRS_EX + 12 * 8 + e * 8;
	char s[2], a;
	s[1] = 0;
	int ay = (in.act == 2 && in.activeenvx == e) ? in.activeenvy : -1;

	//Volume Only mode uses Command 7 with $XY == $FF
	COLORREF fillColor = (in.env[e][ENV_COMMAND] == 0x07 && in.env[e][ENV_X] == 0x0f && in.env[e][ENV_Y] == 0x0f) ?
		RGB(128, 255, 255) : RGB(255, 255, 255);

	//volume column
	if (volL) g_mem_dc->FillSolidRect((x * sp) / 100, ((INSTRS_EY + 3 * 16 + 4 + 4 * (15 - volL)) * sp) / 100, (8 * sp) / 100, ((volL * 4) * sp) / 100, fillColor);

	if (g_tracks4_8 > 4 && volR) g_mem_dc->FillSolidRect((x * sp) / 100, ((INSTRS_EY - 2 * 16 + 4 + 4 * (15 - volR)) * sp) / 100, (8 * sp) / 100, ((volR * 4) * sp) / 100, fillColor);

	for (int j = 0; j < 8; j++)
	{
		if ((a = shenv[j].ch) != 0)
		{
			if (m_instr[it].env[e][j])
				s[0] = a;
			else
				s[0] = 8;	//character in the envelope
		}
		else
			s[0] = CharL4(m_instr[it].env[e][j]);

		if (j == ay && g_activepart == PART_INSTRUMENTS)
		{
			color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
		}
		else color = TEXT_COLOR_WHITE;

		if (j == 0)
		{
			if (g_tracks4_8 > 4) TextXY(s, x, INSTRS_EY + 2 * 16, color);		 //volume R is out of the box
		}
		else
			TextXY(s, x, INSTRS_EY + 7 * 16 + j * 16, color);
	}
}

BOOL CInstruments::DrawTab(int p, int it)
{
	TInstrument& in = m_instr[it];
	char s[4];

	//small number
	s[0] = CharH4(p);
	s[1] = CharL4(p);
	s[2] = 0;
	TextMiniXY(s, INSTRS_TX + p * 24, INSTRS_TY);

	//table parameter
	sprintf(s, "%02X", in.tab[p]);

	int color = TEXT_COLOR_WHITE;

	if (in.act == 3 && in.activetab == p && g_activepart == PART_INSTRUMENTS)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	TextXY(s, INSTRS_TX + p * 24, INSTRS_TY + 8, color);
	return 1;
}