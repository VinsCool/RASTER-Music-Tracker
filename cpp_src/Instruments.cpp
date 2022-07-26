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

const Tshpar shpar[NUMBEROFPARS] =
{
	//TABLE: LEN GO SPD TYPE MODE
	{ 0,INSTRS_PX + 16 * 8,INSTRS_PY + 9 * 16,"LENGTH:", 0x1f, 0x1f, 1, 8, 1,15,15 },
	{ 1,INSTRS_PX + 18 * 8,INSTRS_PY + 10 * 16,  "GOTO:", 0x1f, 0x1f, 0, 0, 2,16,16 },
	{ 2,INSTRS_PX + 17 * 8,INSTRS_PY + 11 * 16, "SPEED:", 0x3f, 0x3f, 1, 1, 3,17,17 },
	{ 3,INSTRS_PX + 18 * 8,INSTRS_PY + 12 * 16,  "TYPE:", 0x01, 0x01, 0, 2, 4,18,18 },
	{ 4,INSTRS_PX + 18 * 8,INSTRS_PY + 13 * 16,  "MODE:", 0x01, 0x01, 0, 3, 5,19,19 },
	//ENVELOPE: LEN GO VSLIDE VMIN
	{ 5,INSTRS_PX + 16 * 8,INSTRS_PY + 2 * 16, "LENGTH:"  ,0x3f, 0x2f, 1, 4, 6, 9, 9 },
	{ 6,INSTRS_PX + 18 * 8,INSTRS_PY + 3 * 16,   "GOTO:"   ,0x3f, 0x2f, 0, 5, 7,10,10 },
	{ 7,INSTRS_PX + 15 * 8,INSTRS_PY + 4 * 16,"FADEOUT:",0xff, 0xff, 0, 6, 8,11,11 },
	{ 8,INSTRS_PX + 15 * 8,INSTRS_PY + 5 * 16,"VOL MIN:"  ,0x0f, 0x0f, 0, 7, 0,11,11 },
	//EFFECT: DELAY VIBRATO FSHIFT
	{ 9,INSTRS_PX + 3 * 8,INSTRS_PY + 2 * 16,     "DELAY:",  0xff,0xff, 0,19,10, 5, 5 },
	{10,INSTRS_PX + 1 * 8,INSTRS_PY + 3 * 16,   "VIBRATO:",0x03,0x03, 0, 9,11, 6, 6 },
	{11,INSTRS_PX + -1 * 8,INSTRS_PY + 4 * 16,"FREQSHIFT:", 0xff,0xff, 0,10,12, 7, 7 },
	//AUDCTL: 00-07
	{12,INSTRS_PX + 3 * 8,INSTRS_PY + 6 * 16,   "15KHZ:",0x01,0x01,0,11,13, 0, 0 },
	{13,INSTRS_PX + 1 * 8,INSTRS_PY + 7 * 16, "HPF 2+4:",0x01,0x01,0,12,14, 0, 0 },
	{14,INSTRS_PX + 1 * 8,INSTRS_PY + 8 * 16, "HPF 1+3:",0x01,0x01,0,13,15, 0, 0 },
	{15,INSTRS_PX + 0 * 8,INSTRS_PY + 9 * 16,"JOIN 3+4:",0x01,0x01,0,14,16, 0, 0 },
	{16,INSTRS_PX + 0 * 8,INSTRS_PY + 10 * 16,"JOIN 1+2:",0x01,0x01,0,15,17, 1, 1 },
	{17,INSTRS_PX + 0 * 8,INSTRS_PY + 11 * 16,"1.79 CH3:",0x01,0x01,0,16,18, 2, 2 },
	{18,INSTRS_PX + 0 * 8,INSTRS_PY + 12 * 16,"1.79 CH1:",0x01,0x01,0,17,19, 3, 3 },
	{19,INSTRS_PX + 3 * 8,INSTRS_PY + 13 * 16,   "POLY9:",0x01,0x01,0,18, 9, 4, 4 }
};


const Tshenv shenv[ENVROWS] =
{
	//ENVELOPE
	{   0,0x0f,1,-1,   "VOLUME R:",INSTRS_EX + 2 * 8,INSTRS_EY + 2 * 16 },	//volume right
	{   0,0x0f,1,-1,   "VOLUME L:",INSTRS_EX + 2 * 8,INSTRS_EY + 8 * 16 },	//volume left
	{   0,0x0e,2,-2, "DISTORTION:",INSTRS_EX + 0 * 8,INSTRS_EY + 9 * 16 },	//distortion 0,2,4,6,...
	{   0,0x07,1,-1,    "COMMAND:",INSTRS_EX + 3 * 8,INSTRS_EY + 10 * 16 },	//command 0-7
	{   0,0x0f,1,-1,         "X/:",INSTRS_EX + 8 * 8,INSTRS_EY + 11 * 16 },	//X
	{   0,0x0f,1,-1,        "Y\\:",INSTRS_EX + 8 * 8,INSTRS_EY + 12 * 16 },	//Y
	{   9,0x01,1,-1, "AUTOFILTER:",INSTRS_EX + 0 * 8,INSTRS_EY + 13 * 16 },	//filter *
	{   9,0x01,1,-1, "PORTAMENTO:",INSTRS_EX + 0 * 8,INSTRS_EY + 14 * 16 }	//portamento *
};


CInstruments::CInstruments()
{
	InitInstruments();
}

BOOL CInstruments::InitInstruments()
{
	for (int i = 0; i < INSTRSNUM; i++)
	{
		ClearInstrument(i);
	}
	return 1;
}

BOOL CInstruments::ClearInstrument(int it)
{
	Atari_InstrumentTurnOff(it); //turns off this instrument on all channels

	int i, j;
	char* s = m_instr[it].name;
	memset(s, ' ', INSTRNAMEMAXLEN);
	sprintf(s, "Instrument %02X", it);
	s[strlen(s)] = ' '; //overrides 0x00 after the end of the text
	s[INSTRNAMEMAXLEN] = 0;

	//m_instr[it].act=0;				//active name
	m_instr[it].act = 2;				//active envelope, so testing instruments wouldn't cause accidental rename
	m_instr[it].activenam = 0;			//0 character name
	m_instr[it].activepar = PAR_ENVLEN;	//default is ENVELOPE LEN
	m_instr[it].activeenvx = 0;
	m_instr[it].activeenvy = 1;			//volume left
	m_instr[it].activetab = 0;			//0 element in the table

	m_instr[it].octave = 0;
	m_instr[it].volume = MAXVOLUME;

	for (i = 0; i < PARCOUNT; i++) m_instr[it].par[i] = 0;
	for (i = 0; i < ENVCOLS; i++)
	{
		for (j = 0; j < ENVROWS; j++) m_instr[it].env[i][j] = 0; //rand()&0x0f;			//0;
	}
	for (i = 0; i < TABLEN; i++) m_instr[it].tab[i] = 0;

	m_iflag[it] = 0;	//init instrument flag

	ModificationInstrument(it);			//apply to Atari Mem

	return 1;
}

/*
void CInstruments::RandomInstrument(int it)
{
	ClearInstrument(it);
	unsigned int rid = (unsigned)time( NULL );
	srand( rid );
	CString s;
	s.Format("Random instrument %u",rid);	//jmeno
	strncpy(m_instr[it].name,(LPCTSTR)s,s.GetLength());
	int len=rand()%ENVCOLS;
	int go=rand()%(len+1);
	int i,j;
	for(i=0; i<=len; i++)
	{
		for(j=0; j<ENVROWS-2; j++)
		{
			int ran = rand() & 0x0f & shenv[j].pand;
			if (j==ENV_COMMAND) ran = 1+ (rand() % 2);	//command 1-2
			m_instr[it].env[i][j] = ran;
		}
	}
	m_instr[it].par[PAR_ENVLEN]=len;
	m_instr[it].par[PAR_ENVGO]=go;

	ModificationInstrument(it);			//promitne do Atari mem
}
*/


void CInstruments::CheckInstrumentParameters(int instr)
{
	TInstrument& ai = m_instr[instr];
	//ENVELOPE len-go loop control
	if (ai.par[PAR_ENVGO] > ai.par[PAR_ENVLEN]) ai.par[PAR_ENVGO] = ai.par[PAR_ENVLEN];
	//TABLE len-go loop control
	if (ai.par[PAR_TABGO] > ai.par[PAR_TABLEN]) ai.par[PAR_TABGO] = ai.par[PAR_TABLEN];
	//check the cursor in the envelope
	if (ai.activeenvx > ai.par[PAR_ENVLEN]) ai.activeenvx = ai.par[PAR_ENVLEN];
	//check the cursor in the table
	if (ai.activetab > ai.par[PAR_TABLEN]) ai.activetab = ai.par[PAR_TABLEN];
	//something changed => Save instrument "to Atari"
}

BOOL CInstruments::RecalculateFlag(int instr)
{
	BYTE flag = 0;
	TInstrument& ti = m_instr[instr];
	int i;
	int envl = ti.par[PAR_ENVLEN];
	//filter?
	for (i = 0; i <= envl; i++)
	{
		if (ti.env[i][ENV_FILTER]) { flag |= IF_FILTER; break; }
	}

	//bass16?
	for (i = 0; i <= envl; i++)
	{
		//the filter takes priority over bass16, ie if the filter is enabled as well as bass16, bass16 does not become active
		if (ti.env[i][ENV_DISTORTION] == 6 && !ti.env[i][ENV_FILTER]) { flag |= IF_BASS16; break; }
	}

	//portamento?
	for (i = 0; i <= envl; i++)
	{
		if (ti.env[i][ENV_PORTAMENTO]) { flag |= IF_PORTAMENTO; break; }
	}
	//audctl?
	for (i = PAR_AUDCTL0; i <= PAR_AUDCTL7; i++)
	{
		if (ti.par[i]) { flag |= IF_AUDCTL; break; }
	}
	//
	m_iflag[instr] = flag;
	return 1;
}

BOOL CInstruments::CalculateNoEmpty(int instr)
{
	TInstrument& it = m_instr[instr];
	int i, j;
	int len = it.par[PAR_ENVLEN];
	for (i = 0; i <= len; i++)
	{
		for (j = 0; j < ENVROWS; j++)
		{
			if (it.env[i][j] != 0) return 1;
		}
	}
	for (i = 0; i < PARCOUNT; i++)
	{
		if (it.par[i] != 0) return 1;
	}
	return 0; //is empty
}

void CInstruments::SetEnvVolume(int instr, BOOL right, int px, int py)
{
	int len = m_instr[instr].par[PAR_ENVLEN] + 1;
	if (px < 0 || px >= len) return;
	if (py < 0 || py>15) return;
	int ep = (right && g_tracks4_8 > 4) ? ENV_VOLUMER : ENV_VOLUMEL;
	m_instr[instr].env[px][ep] = py;
	ModificationInstrument(instr);
}

int CInstruments::GetFrequency(int instr, int note)
{
	if (instr < 0 || instr >= INSTRSNUM || note < 0 || note >= NOTESNUM) return -1;
	TInstrument& tt = m_instr[instr];
	if (tt.par[PAR_TABTYPE] == 0)  //only for TABTYPE NOTES
	{
		int nsh = tt.tab[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note < 0 || note >= NOTESNUM) return -1;
	}
	int frq = -1;
	int dis = tt.env[0][ENV_DISTORTION];
	if (dis == 0x0c) frq = g_atarimem[RMT_FRQTABLES + 64 + note];
	else
		if (dis == 0x0e || dis == 0x06) frq = g_atarimem[RMT_FRQTABLES + 128 + note];
		else
			frq = g_atarimem[RMT_FRQTABLES + 192 + note];
	return frq;
}

int CInstruments::GetNote(int instr, int note)
{
	if (instr < 0 || instr >= INSTRSNUM || note < 0 || note >= NOTESNUM) return -1;
	TInstrument& tt = m_instr[instr];
	if (tt.par[PAR_TABTYPE] == 0)  //only for TABTYPE NOTES
	{
		int nsh = tt.tab[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note < 0 || note >= NOTESNUM) return -1;
	}
	return note;
}

void CInstruments::MemorizeOctaveAndVolume(int instr, int oct, int vol)
{
	if (g_keyboard_rememberoctavesandvolumes)
	{
		if (oct >= 0) m_instr[instr].octave = oct;
		if (vol >= 0) m_instr[instr].volume = vol;
	}
}

void CInstruments::RememberOctaveAndVolume(int instr, int& oct, int& vol)
{
	if (g_keyboard_rememberoctavesandvolumes)
	{
		oct = m_instr[instr].octave;
		vol = m_instr[instr].volume;
	}
}

