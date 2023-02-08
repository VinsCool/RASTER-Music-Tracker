#include "stdafx.h"
#include "resource.h"
#include <fstream>

#include "Atari6502.h"
#include "IOHelpers.h"

#include "Instruments.h"
#include "global.h"

#include "GuiHelpers.h"

/// <summary>
/// Define information about each instrument parameter (not envelope table)
/// </summary>
const Tshpar shpar[NUMBER_OF_PARAMS] =
{
	//Nr, Draw Position															NAME			AND		MAX			Offset	"next param on cursor movement", Fieldname in txt file
	//TABLE: LEN GO SPD TYPE MODE
	{ PAR_TBL_LENGTH,		INSTRS_PARAM_X + 16 * 8,INSTRS_PARAM_Y + 9 * 16,	"LENGTH:",		0x1f,	0x1f,		1,		 8,  1, 15, 15,		"LENGTH:"},
	{ PAR_TBL_GOTO,			INSTRS_PARAM_X + 18 * 8,INSTRS_PARAM_Y + 10 * 16,	  "GOTO:",		0x1f,	0x1f,		0,		 0,  2, 16, 16,		"GOTO:"},
	{ PAR_TBL_SPEED,		INSTRS_PARAM_X + 17 * 8,INSTRS_PARAM_Y + 11 * 16,	 "SPEED:",		0x3f,	0x3f,		1,		 1,  3, 17, 17,		"SPEED:"},
	{ PAR_TBL_TYPE,			INSTRS_PARAM_X + 18 * 8,INSTRS_PARAM_Y + 12 * 16,	  "TYPE:",		0x01,	0x01,		0,		 2,  4, 18, 18,		"TYPE:"},
	{ PAR_TBL_MODE,			INSTRS_PARAM_X + 18 * 8,INSTRS_PARAM_Y + 13 * 16,	  "MODE:",		0x01,	0x01,		0,		 3,  5, 19, 19,		"MODE:"},
	//ENVELOPE: LEN GO VSLIDE VMIN																	     
	{ PAR_ENV_LENGTH,		INSTRS_PARAM_X + 16 * 8,INSTRS_PARAM_Y + 2 * 16,	"LENGTH:",		0x3f,	0x2f,		1,		 4,  6,  9,  9,		"ENV_LENGTH:"},
	{ PAR_ENV_GOTO,			INSTRS_PARAM_X + 18 * 8,INSTRS_PARAM_Y + 3 * 16,	  "GOTO:",		0x3f,	0x2f,		0,		 5,  7, 10, 10,		"ENV_GOTO:"},
	{ PAR_VOL_FADEOUT,		INSTRS_PARAM_X + 15 * 8,INSTRS_PARAM_Y + 4 * 16,   "FADEOUT:",		0xff,	0xff,		0,		 6,  8, 11, 11,		"FADEOUT:"},
	{ PAR_VOL_MIN,			INSTRS_PARAM_X + 15 * 8,INSTRS_PARAM_Y + 5 * 16,   "VOL MIN:",		0x0f,	0x0f,		0,		 7,  0, 11, 11,		"VOL_MIN:"},
	//EFFECT: DELAY VIBRATO FSHIFT																	    
	{ PAR_DELAY,			INSTRS_PARAM_X + 3 * 8,INSTRS_PARAM_Y + 2 * 16,      "DELAY:",		0xff,	0xff,		0,		19, 10,  5,  5,		"EFF_DELAY:"},
	{ PAR_VIBRATO,			INSTRS_PARAM_X + 1 * 8,INSTRS_PARAM_Y + 3 * 16,    "VIBRATO:",		0x03,	0x03,		0,		 9, 11,  6,  6,		"EFF_VIBRATO:"},
	{ PAR_FREQ_SHIFT,		INSTRS_PARAM_X + -1 * 8,INSTRS_PARAM_Y + 4 * 16, "FREQSHIFT:",		0xff,	0xff,		0,		10, 12,  7,  7,		"EFF_FREQSHIFT:"},
	//AUDCTL: 00-07
	{ PAR_AUDCTL_15KHZ,		INSTRS_PARAM_X + 3 * 8,INSTRS_PARAM_Y + 6 * 16,      "15KHZ:",		0x01,	0x01,		0,		11, 13,  0,  0,		"AUD_15KHZ:"},
	{ PAR_AUDCTL_HPF_CH2,	INSTRS_PARAM_X + 1 * 8,INSTRS_PARAM_Y + 7 * 16,    "HPF 2+4:",		0x01,	0x01,		0,		12, 14,  0,  0,		"AUD_HPF_CH2:"},
	{ PAR_AUDCTL_HPF_CH1,	INSTRS_PARAM_X + 1 * 8,INSTRS_PARAM_Y + 8 * 16,    "HPF 1+3:",		0x01,	0x01,		0,		13, 15,  0,  0,		"AUD_HPF_CH1:"},
	{ PAR_AUDCTL_JOIN_3_4,	INSTRS_PARAM_X + 0 * 8,INSTRS_PARAM_Y + 9 * 16,   "JOIN 3+4:",		0x01,	0x01,		0,		14, 16,  0,  0,		"AUD_JOIN34:"},
	{ PAR_AUDCTL_JOIN_1_2,	INSTRS_PARAM_X + 0 * 8,INSTRS_PARAM_Y + 10 * 16,  "JOIN 1+2:",		0x01,	0x01,		0,		15, 17,  1,  1,		"AUD_JOIN12:"},
	{ PAR_AUDCTL_179_CH3,	INSTRS_PARAM_X + 0 * 8,INSTRS_PARAM_Y + 11 * 16,  "1.79 CH3:",		0x01,	0x01,		0,		16, 18,  2,  2,		"AUD_179_CH3:"},
	{ PAR_AUDCTL_179_CH1,	INSTRS_PARAM_X + 0 * 8,INSTRS_PARAM_Y + 12 * 16,  "1.79 CH1:",		0x01,	0x01,		0,		17, 19,  3,  3,		"AUD_179_CH1:"},
	{ PAR_AUDCTL_POLY9,		INSTRS_PARAM_X + 3 * 8,INSTRS_PARAM_Y + 13 * 16,     "POLY9:",		0x01,	0x01,		0,		18,  9,  4,  4,		"AUD_POLY9:"}
};

/// <summary>
/// Define information about envelope table
/// </summary>
const Tshenv shenv[ENVROWS] =
{
	//ENVELOPE
	{   0,0x0f,1,-1,   "VOLUME R:",INSTRS_ENV_X + 2 * 8,INSTRS_ENV_Y + 2 * 16,		"ENV_VOLUME_R:"},	//volume right
	{   0,0x0f,1,-1,   "VOLUME L:",INSTRS_ENV_X + 2 * 8,INSTRS_ENV_Y + 8 * 16,		"ENV_VOLUME_L:"},	//volume left
	{   0,0x0e,2,-2, "DISTORTION:",INSTRS_ENV_X + 0 * 8,INSTRS_ENV_Y + 9 * 16,		"ENV_DISTORTION:"},	//distortion 0,2,4,6,...
	{   0,0x07,1,-1,    "COMMAND:",INSTRS_ENV_X + 3 * 8,INSTRS_ENV_Y + 10 * 16,		"ENV_COMMNAND:"},	//command 0-7
	{   0,0x0f,1,-1,         "X/:",INSTRS_ENV_X + 8 * 8,INSTRS_ENV_Y + 11 * 16,		"ENV_X:"},			//X
	{   0,0x0f,1,-1,        "Y\\:",INSTRS_ENV_X + 8 * 8,INSTRS_ENV_Y + 12 * 16,		"ENV_Y:"},			//Y
	{   9,0x01,1,-1, "AUTOFILTER:",INSTRS_ENV_X + 0 * 8,INSTRS_ENV_Y + 13 * 16,		"ENV_AUTOFILTER:"},		//filter *
	{   9,0x01,1,-1, "PORTAMENTO:",INSTRS_ENV_X + 0 * 8,INSTRS_ENV_Y + 14 * 16,		"ENV_PORTAMENTO:"}	//portamento *
};


CInstruments::CInstruments()
{
	if (m_instr) delete m_instr;
	m_instr = new TInstrument[INSTRSNUM];
	//InitInstruments();
}

CInstruments::~CInstruments()
{
	if (m_instr) delete m_instr;
	m_instr = NULL;
}

/// <summary>
/// Reset all 64 instruments to startup defaults
/// </summary>
void CInstruments::InitInstruments()
{
	for (int i = 0; i < INSTRSNUM; i++)
	{
		ClearInstrument(i);
	}
}

/// <summary>
/// Reset an instrument to startup defaults
/// </summary>
/// <param name="instrumentNr">Index of the instrument 0-63</param>
void CInstruments::ClearInstrument(int instrNr)
{
	// Turn off this instrument on all channels
	Atari_InstrumentTurnOff(instrNr);

	// Clear everything/All zero
	TInstrument* instrument = GetInstrument(instrNr);
	memset(instrument, 0, sizeof(TInstrument));

	// Init the name "Instrument XX"
	int len = sprintf(instrument->name, "Instrument %02X", instrNr);

	// Replace all the remaining characters with spaces
	memset(instrument->name + len, ' ', INSTRUMENT_NAME_MAX_LEN - len);

	// Set some initial values
	instrument->activeEditSection = INSTRUMENT_SECTION_ENVELOPE;	// Activate on the Envelope, so testing instruments wouldn't cause accidental rename
	instrument->editNameCursorPos = 0;								// 0 character name
	instrument->editParameterNr = PAR_ENV_LENGTH;					// Envelope length is the default parameter to edit
	instrument->editEnvelopeX = 0;
	instrument->editEnvelopeY = 1;									// Volume left
	instrument->editNoteTableCursorPos = 0;							// 0 element in the table
	instrument->octave = 0;
	instrument->volume = MAXVOLUME;

	// Apply to Atari Mem
	WasModified(instrNr);
}

void CInstruments::CheckInstrumentParameters(int instr)
{
	TInstrument* ai = GetInstrument(instr);

	//ENVELOPE len-go loop control
	if (ai->parameters[PAR_ENV_GOTO] > ai->parameters[PAR_ENV_LENGTH]) ai->parameters[PAR_ENV_GOTO] = ai->parameters[PAR_ENV_LENGTH];
	
	//TABLE len-go loop control
	if (ai->parameters[PAR_TBL_GOTO] > ai->parameters[PAR_TBL_LENGTH]) ai->parameters[PAR_TBL_GOTO] = ai->parameters[PAR_TBL_LENGTH];
	
	//check the cursor in the envelope
	if (ai->editEnvelopeX > ai->parameters[PAR_ENV_LENGTH]) ai->editEnvelopeX = ai->parameters[PAR_ENV_LENGTH];
	
	//check the cursor in the table
	if (ai->editNoteTableCursorPos > ai->parameters[PAR_TBL_LENGTH]) ai->editNoteTableCursorPos = ai->parameters[PAR_TBL_LENGTH];
	
	//something changed => Save instrument "to Atari"
	// NOTE: Done from the outside
}

/// <summary>
/// Calculate some text hints for this instrument.
/// When the instrument name is rendered there will be some hints below it
/// </summary>
/// <param name="instr"></param>
void CInstruments::RecalculateFlag(int instr)
{
	BYTE flag = 0;
	TInstrument* ti = GetInstrument(instr);
	int i;
	int envl = ti->parameters[PAR_ENV_LENGTH];

	//filter?
	for (i = 0; i <= envl; i++)
	{
		if (ti->envelope[i][ENV_FILTER]) { flag |= IF_FILTER; break; }
	}

	//bass16?
	for (i = 0; i <= envl; i++)
	{
		//the filter takes priority over bass16, ie if the filter is enabled as well as bass16, bass16 does not become active
		if (ti->envelope[i][ENV_DISTORTION] == 6 && !ti->envelope[i][ENV_FILTER]) { flag |= IF_BASS16; break; }
	}

	//portamento?
	for (i = 0; i <= envl; i++)
	{
		if (ti->envelope[i][ENV_PORTAMENTO]) { flag |= IF_PORTAMENTO; break; }
	}

	//audctl?
	for (i = PAR_AUDCTL_15KHZ; i <= PAR_AUDCTL_POLY9; i++)
	{
		if (ti->parameters[i]) { flag |= IF_AUDCTL; break; }
	}
	//
	//m_iflag[instr] = flag;

	ti->displayHintFlag = flag;
}

/// <summary>
/// Check if an instrument is empty.
/// Empty is defined as NO volume and all parameters are 0
/// </summary>
/// <param name="instr">Instrument #</param>
/// <returns>true if the instrument has values, False if it is in default state</returns>
BOOL CInstruments::CalculateNotEmpty(int instr)
{
	TInstrument* ti = GetInstrument(instr);
	int i, j;
	int len = ti->parameters[PAR_ENV_LENGTH];
	for (i = 0; i <= len; i++)
	{
		for (j = 0; j < ENVROWS; j++)
		{
			if (ti->envelope[i][j] != 0) return 1;
		}
	}
	for (i = 0; i < PARCOUNT; i++)
	{
		if (ti->parameters[i] != 0) return 1;
	}
	return 0; //is empty
}

/// <summary>
/// Set the volume level for a channel
/// </summary>
/// <param name="instr">Instrument #</param>
/// <param name="right">True - then use the stereo/right channels</param>
/// <param name="px">X position in the envelope</param>
/// <param name="newVolume">volume level to set</param>
void CInstruments::SetEnvelopeVolume(int instr, BOOL right, int px, int newVolume)
{
	TInstrument* ti = GetInstrument(instr);

	// Validate
	int len = ti->parameters[PAR_ENV_LENGTH] + 1;
	if (px < 0 || px >= len) return;
	if (newVolume < 0 || newVolume > 15) return;

	int ep = (right && g_tracks4_8 > 4) ? ENV_VOLUMER : ENV_VOLUMEL;
	ti->envelope[px][ep] = newVolume;

	// Recalc some info about the updated instrument
	WasModified(instr);
}

/// <summary>
/// Convert the note to a frequency according to distortion in first
/// envelope column or first entry in the note table.
/// </summary>
/// <param name="instr">Instrument #</param>
/// <param name="note">which note</param>
/// <returns>frequency</returns>
int CInstruments::GetFrequency(int instr, int note)
{
	if (instr < 0 || instr >= INSTRSNUM || note < 0 || note >= NOTESNUM) return -1;

	TInstrument* tt = GetInstrument(instr);
	if (tt->parameters[PAR_TBL_TYPE] == 0)  //only for NOTES table
	{
		int nsh = tt->noteTable[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note < 0 || note >= NOTESNUM) return -1;
	}
	int frq = -1;
	int dis = tt->envelope[0][ENV_DISTORTION];
	if (dis == 0x0c) frq = g_atarimem[RMT_FRQTABLES + 64 + note];
	else if (dis == 0x0e || dis == 0x06) frq = g_atarimem[RMT_FRQTABLES + 128 + note];
	else
		frq = g_atarimem[RMT_FRQTABLES + 192 + note];
	return frq;
}

/// <summary>
/// Calculate the note according to distortion in the  first entry in the note table.
/// </summary>
/// <param name="instr">Instrument #</param>
/// <param name="note">which note</param>
/// <returns>note</returns>
int CInstruments::GetNote(int instr, int note)
{
	if (instr < 0 || instr >= INSTRSNUM || note < 0 || note >= NOTESNUM) return -1;

	TInstrument* tt = GetInstrument(instr);
	if (tt->parameters[PAR_TBL_TYPE] == 0)  //only for NOTES table
	{
		int nsh = tt->noteTable[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note < 0 || note >= NOTESNUM) return -1;
	}
	return note;
}

/// <summary>
/// Save octave and volume info in the instrument.
/// </summary>
/// <param name="instr">Instrument #</param>
/// <param name="oct">Last used octave</param>
/// <param name="vol">Last used volume</param>
void CInstruments::MemorizeOctaveAndVolume(int instr, int oct, int vol)
{
	if (g_keyboard_RememberOctavesAndVolumes)
	{
		TInstrument* ti = GetInstrument(instr);
		if (oct >= 0) ti->octave = oct;
		if (vol >= 0) ti->volume = vol;
	}
}

/// <summary>
/// Load octave and volume info from the instrument
/// </summary>
/// <param name="instr">Instrument #</param>
/// <param name="oct">Last used octave</param>
/// <param name="vol">Last used volume</param>
void CInstruments::RememberOctaveAndVolume(int instr, int& oct, int& vol)
{
	if (g_keyboard_RememberOctavesAndVolumes)
	{
		TInstrument* ti = GetInstrument(instr);
		oct = ti->octave;
		vol = ti->volume;
	}
}

