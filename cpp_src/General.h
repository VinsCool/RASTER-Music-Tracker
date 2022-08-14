#pragma once

// ----------------------------------------------------------------------------
// GUI color setup
// The text is defined in IDB_GFX as bitmap font in various colours
// Text color is defined as a vertical offset in the bitmap font file

#define TEXT_COLOR_WHITE 0
#define TEXT_COLOR_GRAY 1
#define TEXT_COLOR_YELLOW 2
#define TEXT_COLOR_INVERSE_BLUE 3
#define TEXT_COLOR_INVERSE_WHITE 4
#define TEXT_COLOR_CYAN 5
#define TEXT_COLOR_RED 6
#define TEXT_COLOR_INVERSE_RED 9
#define TEXT_COLOR_EXTRA 10
#define TEXT_COLOR_PURPLE 11
#define TEXT_COLOR_TURQUOISE 12
#define TEXT_COLOR_BLUE 13
#define TEXT_COLOR_LIGHT_GRAY 14


#define COLOR_SELECTED			TEXT_COLOR_INVERSE_RED	//highlight colour
#define COLOR_SELECTED_PROVE	TEXT_COLOR_INVERSE_BLUE	//highlight colour in PROVE mode

#define TEXT_MINI_COLOR_GRAY 0
#define TEXT_MINI_COLOR_BLUE 1
#define TEXT_MINI_COLOR_WHITE 2
#define TEXT_MINI_COLOR_YELLOW 3



// ---------------------
// Analyser and other RGB colors
#define COL_BLOCK		56
#define RGB_MUTE		RGB(120,160,240)		// Channel is muted
#define RGB_NORMAL		RGB(255,255,255)		// Volume bar in white
#define RGB_VOLUME_ONLY	RGB(128,255,255)		// Turquoise for volume only channel
#define RGB_TWO_TONE	RGB(128,255,0)			// Green for two tone channel
#define RGB_BACKGROUND	RGB(34,50,80)			// Dark blue
#define RGB_LINES		RGB(149,194,240)		// Blue gray
#define RGB_BLACK		RGB(0,0,0)				// Black

// ----------------------------------------------------------------------------
// GUI edit modes
#define PROVE_EDIT_MODE				0		// Hit the Jam mode button to switch between
#define PROVE_JAM_MONO_MODE			1		// the first three modes
#define PROVE_JAM_STEREO_MODE		2		// Can only get here in stereo mode
#define PROVE_EDIT_AND_JAM_MODES	3		// < this is edit and jam
#define PROVE_MIDI_CH15_MODE		3		// Hit RECORD key in Midi channel 15 to cycle to this mode
#define PROVE_POKEY_EXPLORER_MODE	4		// Ctrl + Shift + F5
#define PROVE_MODE_MAX				4		// <-- Adjust as you add/remove modes

// ----------------------------------------------------------------------------
// Keyboard layouts that may be used with RMT for Notes input
#define KEYBOARD_QWERTY	0
#define KEYBOARD_AZERTY	1

// ----------------------------------------------------------------------------
// TODO: add more keys definition to simplify things
#define VK_BACKSPACE	8
#define VK_ENTER		13
#define VK_PAGE_UP		33
#define VK_PAGE_DOWN	34

//TODO: make tuning configuration separate, also add theme configuration in the future
#define CONFIG_FILENAME "rmt.ini"

#define EOL "\x0d\x0a"			//Carriage Return (\r) and Line Feed (\n), for strings used during Exports

#define TRACKS_X 2*8
#define TRACKS_Y 8*16+8
#define	SONG_X	768
#define SONG_Y	16

// Info area
// Shown at top-left
// 6 lines of text
#define INFO_X	2*8
#define INFO_Y	1*16

#define INFO_Y_LINE_1	INFO_Y
#define INFO_Y_LINE_2	INFO_Y+1*16
#define INFO_Y_LINE_3	INFO_Y+2*16
#define INFO_Y_LINE_4	INFO_Y+3*16
#define INFO_Y_LINE_5	INFO_Y+4*16
#define INFO_Y_LINE_6	INFO_Y+5*16

// Which part of the info area is active for editing (drawn in red)
#define INFO_ACTIVE_NAME			0			// song name can be edited
#define INFO_ACTIVE_SPEED			1			// song speed can be changed
#define INFO_ACTIVE_MAIN_SPEED		2			// over all song speed
#define INFO_ACTIVE_INSTR_SPEED		3			// How many times per frame is the instrument code called (1-8)
#define INFO_ACTIVE_1ST_HIGHLIGHT	4			// Primary line highlight can be edited
#define INFO_ACTIVE_2ND_HIGHLIGHT	5			// Secondary line highlight can be edited

// Which part of the data is currently active/visible/primary
#define PART_INFO			0
#define PART_TRACKS			1
#define PART_INSTRUMENTS	2
#define PART_SONG			3

// Which section of an instrument's data is currently being editied (is active)
#define INSTRUMENT_SECTION_NAME			0
#define INSTRUMENT_SECTION_PARAMETERS	1
#define INSTRUMENT_SECTION_ENVELOPE		2
#define INSTRUMENT_SECTION_NOTETABLE	3


// ----------------------------------------------------------------------------
// RMT file format
//
#define RMTFORMATVERSION	2	//the version number that is saved into modules, highest means more recent
#define TRACKLEN	256			//drive 128
#define TRACKSNUM	254			//0-253
#define SONGLEN		256
#define SONGTRACKS	8
#define INSTRSNUM	64
#define NOTESNUM	61			//notes 0-60 inclusive
#define MAXVOLUME	15			//maximum volume
#define PARCOUNT	24			//24 instrument parameters
#define ENVELOPE_MAX_COLUMNS	48			// 48 columns in envelope (drive 32) (48 from version 1.25)
#define ENVROWS		8			//8 line (parameter) in the envelope
#define NOTE_TABLE_MAX_LEN		32		// maximum 32 steps in the note table

#define INSTRUMENT_NAME_MAX_LEN	32		// maximum length of instrument name
#define SONG_NAME_MAX_LEN		64		// maximum length of song name
#define TRACKMAXSPEED	256		//maximum speed values, highest the slowest

#define MAXATAINSTRLEN	256		//16+(ENVCOLS*3)	//atari instrument has a maximum of 16 parameters + 32 * 3 bytes envelope
#define MAXATATRACKLEN	256		//atari track has maximum 256 bytes (track index is 0-255)
#define MAXATASONGLEN	SONGTRACKS*SONGLEN	//maximum data size atari song part



#define MPLAY_STOP	0
#define MPLAY_SONG	1
#define MPLAY_FROM	2
#define MPLAY_TRACK	3
#define MPLAY_BLOCK	4
#define MPLAY_BOOKMARK 5
#define MPLAY_SEEK_NEXT	6	//added for Media keys
#define MPLAY_SEEK_PREV	7	//added for Media keys

#define MPLAY_SAPR_SONG		255	//SAPR dump from song start
#define MPLAY_SAPR_FROM		254	//SAPR dump from song cursor position
#define MPLAY_SAPR_TRACK	253	//SAPR dump from track (loop optional)
#define MPLAY_SAPR_BLOCK	252	//SAPR dump from selection block (loop optional)
#define MPLAY_SAPR_BOOKMARK	251	//SAPR dump from bookmak position

#define IOTYPE_RMT			1
#define IOTYPE_RMW			2
#define IOTYPE_RMTSTRIPPED	3
#define IOTYPE_SAP			4
#define IOTYPE_XEX			5
#define IOTYPE_TXT			6
#define IOTYPE_ASM			7
#define IOTYPE_RMF			8

#define IOTYPE_SAPR			9
#define IOTYPE_LZSS			10
#define IOTYPE_LZSS_SAP		11
#define IOTYPE_LZSS_XEX		12

#define IOTYPE_TMC			101		//import TMC

#define IOINSTR_RTI			1		//corresponding IOTYPE_RMT
#define IOINSTR_RMW			2		//corresponding IOTYPE_RMW
#define IOINSTR_TXT			6		//corresponding IOTYPE_TXT

#define MAXSUBSONGS			128		//in exported SAP maximum number of subsongs

//bits in TRACKFLAG
#define TF_NOEMPTY		1
#define TF_USED			2

//bits in INSTRUMENTFLAG
#define IF_NOEMPTY		1
#define IF_USED			2
#define IF_FILTER		4
#define IF_BASS16		8
#define IF_PORTAMENTO	16
#define IF_AUDCTL		32

// Instument definitions
#define PAR_TBL_LENGTH		0
#define PAR_TBL_GOTO		1
#define PAR_TBL_SPEED		2
#define PAR_TBL_TYPE		3
#define PAR_TBL_MODE		4

#define PAR_ENV_LENGTH		5
#define PAR_ENV_GOTO		6
#define PAR_VOL_FADEOUT		7
#define PAR_VOL_MIN			8
#define PAR_DELAY			9
#define PAR_VIBRATO			10
#define PAR_FREQ_SHIFT		11

#define PAR_AUDCTL_15KHZ		12
#define PAR_AUDCTL_HPF_CH2		13
#define PAR_AUDCTL_HPF_CH1		14
#define PAR_AUDCTL_JOIN_3_4		15
#define PAR_AUDCTL_JOIN_1_2		16
#define PAR_AUDCTL_179_CH3		17
#define PAR_AUDCTL_179_CH1		18
#define PAR_AUDCTL_POLY9		19

#define INSTRUMENT_TABLE_OF_NOTES	1
#define INSTRUMENT_TABLE_OF_FREQ	2
#define INSTRUMENT_TABLE_MODE_SET	3
#define INSTRUMENT_TABLE_MODE_ADD	4

#define INSTR_GUI_ZONE_ENVELOPE_LEFT_ENVELOPE	0		// 368,220	8x64 -> 384x64
#define INSTR_GUI_ZONE_ENVELOPE_RIGHT_ENVELOPE	1		// 368,140	8x64 -> 384x64
#define INSTR_GUI_ZONE_ENVELOPE_PARAM_TABLE		2		// 368,296	8x64 -> 384x64
#define INSTR_GUI_ZONE_ENVELOPE_RIGHT_VOL_NUMS	3		// 368,200	8x64 -> 384x64
#define INSTR_GUI_ZONE_NOTE_TABLE				4		// 16,424	16x16 -> 760x16
#define INSTR_GUI_ZONE_INSTRUMENT_NAME			5		// 16,152	304x16
#define INSTR_GUI_ZONE_PARAMETERS				6		// 16,200	208x192
#define INSTR_GUI_ZONE_INSTRUMENT_NUMBER_DLG	7		// 16,136	104x16
#define INSTR_GUI_ZONE_LEN_AND_GOTO_ARROWS		8		// 368,280	384x16
#define INSTR_GUI_ZONE_NOTE_TBL_LEN_AND_GOTO	9		// 16,440	760x16


// GUI instrument definitions
#define INSTRS_X			2*8
#define INSTRS_Y			8*16+8
#define INSTRS_PARAM_X		INSTRS_X			// parameter X
#define INSTRS_PARAM_Y		INSTRS_Y+2*16		// parametry Y
#define INSTRS_ENV_X		INSTRS_X+32*8		// envelope X  (29)
#define INSTRS_ENV_Y		INSTRS_Y+2*16		// envelope Y
#define INSTRS_TABLE_X		INSTRS_X+0*8		// table X	(16)(37)
#define INSTRS_TABLE_Y		INSTRS_Y+18*16-8	// table Y
#define INSTRS_HELP_X		INSTRS_X			// active help X
#define INSTRS_HELP_Y		INSTRS_Y+21*16		// active help Y
#define NUMBER_OF_PARAMS	20


#define	ENV_VOLUMER		0
#define	ENV_VOLUMEL		1
#define	ENV_DISTORTION	2
#define ENV_COMMAND		3
#define	ENV_X			4
#define	ENV_Y			5
#define	ENV_FILTER		6
#define	ENV_PORTAMENTO	7