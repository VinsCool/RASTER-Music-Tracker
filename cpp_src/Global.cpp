#include "stdafx.h"
#include "Rmt.h"
#include "XPokey.h"
#include "RmtView.h"
#include "Atari6502.h"
#include "PokeyStream.h"
#include "ModuleV2.h"
#include "Tuning.h"
#include "Keyboard.h"
#include "global.h"

//-- Atari 6502 and POKEY Emulation Variables (TODO: Delete and replace with a better method) --//

CString g_aboutpokey;
CString g_about6502;

HINSTANCE g_c6502_dll = NULL;

BOOL volatile g_is6502 = 0;
BOOL volatile g_rmtroutine;

unsigned char g_atarimem[65536];

int g_channelon[SONGTRACKS];	// TODO: Move elsewhere or delete
int g_rmtinstr[SONGTRACKS];		// TODO: Move elsewhere or delete


//-- Global Variables for most Program Parameters --//

BOOL g_closeApplication = 0;			// Set when the application is busy shutting down
BOOL volatile g_screenupdate = 0;

HWND g_hwnd = NULL;
HWND g_viewhwnd = NULL;

CDC* g_mem_dc = NULL;
CDC* g_gfx_dc = NULL;

int g_width = 0;
int g_height = 0;
int g_tracklines = 8;
int g_scaling_percentage = 100;

int g_RmtHasFocus;			// Track if RMT has focus, when it does not have focus and is not in prove mode, the MIDI input will be ignored (to avoid overwriting patterns accidentally)

BOOL g_viewMainToolbar = 1;			// 1 yes, 0 no
BOOL g_viewBlockToolbar = 1;		// 1 yes, 0 no
BOOL g_viewStatusBar = 1;			// 1 yes, 0 no
BOOL g_viewPlayTimeCounter = 1;		// 1 yes, 0 no
BOOL g_viewVolumeAnalyzer = 1;		// 1 yes, 0 no - Show the volume analyser bars
BOOL g_viewPokeyRegisters = 1;		// 1 yes, 0 no
BOOL g_viewInstrumentEditHelp = 1;	// 1 yes, 0 no - View useful info when editing various parts of an instrument
BOOL g_viewDoSmoothScrolling = 1;	// True then the track and song line data is smooth scrolled during playback
BOOL g_viewDebugDisplay = 1;		// Display Debug informations on screen if enabled
BOOL g_displayflatnotes = 0;		// Note Accidentals are displayed Flat (b) instead of Sharp (#)
BOOL g_usegermannotation = 0;		// H- notes instead of B-

BOOL g_nohwsoundbuffer = 0;			// Don't use hardware soundbuffer (not needed?)

BOOL g_ntsc = 0;						// NTSC (60Hz)
BOOL g_tracklinealtnumbering = 0;		// Alternative way of line numbering in tracks

int g_keyboard_layout = 1;						// Keyboard layout is used by RMT. eg: QWERTY, AZERTY, etc
BOOL g_keyboard_swapenter = 0;					// 1 yes, 0 no, probably not needed anymore but will be kept for now
BOOL g_keyboard_playautofollow = 1;				// 1 yes, 0 no
BOOL g_keyboard_updowncontinue = 1;				// 1 yes, 0 no
BOOL g_keyboard_RememberOctavesAndVolumes = 1;	// 1 yes, 0 no, the last used octave and volume are stored in the instrument data
BOOL g_keyboard_escresetatarisound = 1;			// 1 yes, 0 no
BOOL g_keyboard_askwhencontrol_s = 1;			// 1 yes, 0 no

CString g_prgpath;								// Path to the directory from which the program was started (including a slash at the end)

CString g_lastLoadPath_Songs;					// Path of the last song loaded
CString g_lastLoadPath_Instruments;				// Path of the last instrument loaded
CString g_lastLoadPath_Tracks;					// Path of the last track loaded

CString g_defaultSongsPath;						// Default path for songs
CString g_defaultInstrumentsPath;				// Default path for instruments
CString g_defaultTracksPath;					// Default path for tracks

int g_lastImportTypeIndex = -1;

int g_lastKeyPressed = 0;			// For debugging vk input
int g_mousebutt = 0;				// Mouse button
int g_mouseLastPointX = 0;
int g_mouseLastPointY = 0;
int g_mouseLastButton = 0;
int g_mouseLastWheelDelta = 0;


//-- Common Music Tracker Variables --//

BOOL g_changes = 0;				// Have there been any changes in the module?

BOOL g_isEditingInstrumentName;		// 0 no, 1 instrument name is edited (TODO: Delete, this is an old hack that is no longer needed)

int g_activepart;			//0 info, 1 edittracks, 2 editinstruments, 3 song
int g_active_ti;			//1 tracks, 2 instrs

int g_line_y = 0;			// Active line coordinate, used to reference g_cursoractview to the correct position

int g_trackLinePrimaryHighlight = 8;	// Primary line highlighted every x lines
int g_trackLineSecondaryHighlight = 4;	// Secondary line highlighted every x lines

int g_linesafter;						// Number of lines to scroll after inserting a note (initializes in CSong :: Clear)

int g_cursoractview = 0;		//default position, line 0

int g_tracks4_8;

int volatile g_prove;			// Test notes without editing (0 = off, 1 = mono jam, 2 = stereo jam)
int volatile g_respectvolume;	//does not change the volume if it is already there

int g_trackerDriverVersion = TRACKER_DRIVER_PATCH16;

bool g_isRMTE = true;			// Hack!!! Necessary until most legacy code is commented out


//-- Tuning Variables (TODO: Move to Tuning.cpp and Tuning.h) --//

// Best known compromise for both regions, they produce identical tables
double g_baseTuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
int g_baseNote = 3;				// 3 = A-
int g_baseOctave = 4;	// 4 * 12 Semitones added to Base Note
//int g_temperament = 0;			// Each preset is assigned to a number. 0 means no Temperament, any value that is not assigned defaults to custom
//int g_notesperoctave = 12;		// By default there are 12 notes per octave

//-- RMT Export Variables (TODO: Move elsewhere or replace with a better method) --//

WORD g_rmtstripped_adr_module;	//address for export RMT stripped file
BOOL g_rmtstripped_sfx;			//sfx offshoot RMT stripped file
BOOL g_rmtstripped_gvf;			//gvs GlobalVolumeFade for feat
BOOL g_rmtstripped_nos;			//nos NoStartingSongline for feat

CString g_rmtmsxtext;
CString g_PrefixForAllAsmLabels;	//label prefix for export ASM simple notation

CString g_AsmLabelForStartOfSong;	// Label for relocatable ASM for RMTPlayer.asm

BOOL g_AsmWantRelocatableInstruments = 0;
BOOL g_AsmWantRelocatableTracks = 0;
BOOL g_AsmWantRelocatableSongLines = 0;

CString g_AsmInstrumentsLabel;
CString g_AsmTracksLabel;
CString g_AsmSongLinesLabel;

int g_AsmFormat = ASSEMBLER_FORMAT_XASM;


// ----------------------------------------------------------------------------
// Here are the main global objects that make up 99% of RMT.
// FIXME: Find a way to make all of these 'extern' in a clean way, the current setup is a mess!
//
CSong			g_Song;				// There is one active song
CRmtMidi		g_Midi;				// There is one midi interface
CKeyboard		g_Keyboard;			// Computer Keyboard Mapping configuration and Input Handler functions
CUndo			g_Undo;				// Undo buffer tracker, TODO: Replace or Delete
CXPokey			g_Pokey;			// The simulated Pokey chip, TODO: Replace or Delete
CInstruments	g_Instruments;		// Instrument routines, TODO: Replace or Delete
CTracks			g_Tracks;			// Track and Pattern routines, TODO: Replace or Delete
CTrackClipboard g_TrackClipboard;	// Clipboard functions, TODO: Replace or Delete
CTuning			g_Tuning;			// Tuning calculations and POKEY notation tables generation
CPokeyStream	g_PokeyStream;		// POKEY registers state stream buffer functions
CModule			g_Module;			// Extended RMT Module Format variables and functions, used for playback, editing, importing, exporting, etc
