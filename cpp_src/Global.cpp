#include "stdafx.h"
#include "Rmt.h"
#include "XPokey.h"
#include "RmtView.h"
#include "Atari6502.h"
#include "global.h"

// Helper defines to make the code a bit more readable




unsigned char g_atarimem[65536];
char g_debugmem[65536];	//debug display of g_atarimem bytes directly, slow and terrible, do not use unless there is a purpose for it 

unsigned char SAPRSTREAM[0xFFFFFF];	//SAP-R Dumper memory, TODO: assign memory dynamically instead, however this doesn't seem to harm anything for now

int SAPRDUMP = 0;		//0, the SAPR dumper is disabled; 1, output is currently recorded to memory; 2, recording is finished and will be written to sap file; 3, flag engage the SAPR dumper
int framecount = 0;		//SAPR dumper frame counter used for calculations and memory allignment with bytes 


BOOL g_closeapplication = 0;			// Set when the application is busy shutting down
CDC* g_mem_dc = NULL;
CDC* g_gfx_dc = NULL;

int g_width = 0;
int g_height = 0;
int g_tracklines = 8;
int g_scaling_percentage = 100;

//best known compromise for both regions, they produce identical tables
double g_basetuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
int g_basenote = 3;	//3 = A-

//ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
double g_UNISON = 1;
double g_MIN_2ND = 1;
double g_MAJ_2ND = 1;
double g_MIN_3RD = 1;
double g_MAJ_3RD = 1;
double g_PERF_4TH = 1;
double g_TRITONE = 1;
double g_PERF_5TH = 1;
double g_MIN_6TH = 1;
double g_MAJ_6TH = 1;
double g_MIN_7TH = 1;
double g_MAJ_7TH = 1;
double g_OCTAVE = 2;

//ratio left
int g_UNISON_L = 1;
int g_MIN_2ND_L = 40;
int g_MAJ_2ND_L = 10;
int g_MIN_3RD_L = 20;
int g_MAJ_3RD_L = 5;
int g_PERF_4TH_L = 4;
int g_TRITONE_L = 60;
int g_PERF_5TH_L = 3;
int g_MIN_6TH_L = 30;
int g_MAJ_6TH_L = 5;
int g_MIN_7TH_L = 30;
int g_MAJ_7TH_L = 15;
int g_OCTAVE_L = 2;

//ratio right
int g_UNISON_R = 1;
int g_MIN_2ND_R = 38;
int g_MAJ_2ND_R = 9;
int g_MIN_3RD_R = 17;
int g_MAJ_3RD_R = 4;
int g_PERF_4TH_R = 3;
int g_TRITONE_R = 43;
int g_PERF_5TH_R = 2;
int g_MIN_6TH_R = 19;
int g_MAJ_6TH_R = 3;
int g_MIN_7TH_R = 17;
int g_MAJ_7TH_R = 8;
int g_OCTAVE_R = 1;

//each preset is assigned to a number. 0 means no Temperament, any value that is not assigned defaults to custom
int g_temperament = 0;




HWND g_hwnd = NULL;
HWND g_viewhwnd = NULL;

HINSTANCE g_c6502_dll = NULL;
BOOL volatile g_is6502 = 0;
CString g_aboutpokey;
CString g_about6502;
CString g_driverversion;

BOOL g_changes = 0;	//have there been any changes in the module?

int g_RmtHasFocus;			// Track if RMT has focus, when it does not have focus and is not in prove mode, the MIDI input will be ignored (to avoid overwriting patterns accidentally)
int g_shiftkey;
int g_controlkey;
int g_altkey;	//unfinished implementation, doesn't work yet for some reason

int g_tracks4_8;
BOOL volatile g_screenupdate = 0;
BOOL volatile g_invalidatebytimer = 0;
int volatile g_screena;
int volatile g_screenwait;
BOOL volatile g_rmtroutine;
BOOL volatile g_timerroutineprocessed;

int volatile g_prove;			// Test notes without editing (0 = off, 1 = mono jam, 2 = stereo jam)
int volatile g_respectvolume;	//does not change the volume if it is already there

WORD g_rmtstripped_adr_module;	//address for export RMT stripped file
BOOL g_rmtstripped_sfx;			//sfx offshoot RMT stripped file
BOOL g_rmtstripped_gvf;			//gvs GlobalVolumeFade for feat
BOOL g_rmtstripped_nos;			//nos NoStartingSongline for feat

CString g_rmtmsxtext;
CString g_expasmlabelprefix;	//label prefix for export ASM simple notation

int last_active_ti;			//if equal to g_active_ti, no screen clear necessary
int last_activepart;		//if equal to g_activepart, no block clear necessary
uint64_t last_ms = 0;
uint64_t last_sec = 0;
int real_fps = 0;
double last_fps = 0;
double avg_fps[120] = { 0 };

int g_activepart;			//0 info, 1 edittracks, 2 editinstruments, 3 song
int g_active_ti;			//1 tracks, 2 instrs

BOOL g_isEditingInstrumentName;		//0 no, 1 instrument name is edited
BOOL is_editing_infos;		//0 no, 1 song name is edited

int g_line_y = 0;			//active line coordinate, used to reference g_cursoractview to the correct position

int g_tracklinehighlight = 8;	//line highlighted every x lines
BOOL g_tracklinealtnumbering = 0; //alternative way of line numbering in tracks
int g_linesafter;			//number of lines to scroll after inserting a note (initializes in CSong :: Clear)
BOOL g_ntsc = 0;				//NTSC (60Hz)
BOOL g_nohwsoundbuffer = 0;	//Don't use hardware soundbuffer
int g_cursoractview = 0;		//default position, line 0

BOOL g_displayflatnotes = 0;	//flats instead of sharps
BOOL g_usegermannotation = 0;	//H notes instead of B

int g_channelon[SONGTRACKS];
int g_rmtinstr[SONGTRACKS];

BOOL g_viewMainToolbar = 1;			// 1 yes, 0 no
BOOL g_viewBlockToolbar = 1;		// 1 yes, 0 no
BOOL g_viewStatusBar = 1;			// 1 yes, 0 no
BOOL g_viewPlayTimeCounter = 1;		// 1 yes, 0 no
BOOL g_viewVolumeAnalyzer = 1;		// 1 yes, 0 no - Show the volume analyser bars
BOOL g_viewPokeyRegisters = 1;		// 1 yes, 0 no
BOOL g_viewInstrumentEditHelp = 1;	// 1 yes, 0 no - View useful info when editing various parts of an instrument
BOOL g_viewDoSmoothScrolling = 1;	// True then the track and song line data is smooth scrolled during playback

long g_playtime = 1;	//1 yes, 0 no

UINT g_mousebutt = 0;			//mouse button
int g_mouselastbutt = 0;
int g_mouse_px = 0;
int g_mouse_py = 0;

CString g_prgpath;					//path to the directory from which the program was started (including a slash at the end)
CString g_lastloadpath_songs;		//the path of the last song loaded
CString g_lastloadpath_instruments; //the path of the last instrument loaded
CString g_lastloadpath_tracks;		//the path of the last track loaded

CString g_path_songs;		//default path for songs
CString g_path_instruments;	//default path for instruments
CString g_path_tracks;		//default path for tracks

int g_keyboard_layout = 0;	//1 yes, 0 no, not be useful anymore... should be deleted
BOOL g_keyboard_swapenter = 0;	//1 yes, 0 no, probably not needed anymore but will be kept for now
BOOL g_keyboard_playautofollow = 1;	//1 yes, 0 no
BOOL g_keyboard_updowncontinue = 1;	//1 yes, 0 no
BOOL g_keyboard_RememberOctavesAndVolumes = 1;	// 1 yes, 0 no, the last used octave and volume are stored in the instrument data
BOOL g_keyboard_escresetatarisound = 1;	//1 yes, 0 no
BOOL g_keyboard_askwhencontrol_s = 1;	//1 yes, 0 no

// ----------------------------------------------------------------------------
// Here are the main global objects that make up 99% of RMT.
//
CSong			g_Song;				// There is one active song
CRmtMidi		g_Midi;				// There is one midi interface
CUndo			g_Undo;				// Undo buffer tracker
CXPokey			g_Pokey;			// The simulated Pokey chip
CInstruments	g_Instruments;
CTracks			g_Tracks;
CTrackClipboard g_TrackClipboard;

/*
void UpdateShiftControlKeys()
{
	g_shiftkey = (GetAsyncKeyState(VK_SHIFT) != NULL);
	g_controlkey = (GetAsyncKeyState(VK_CONTROL) != NULL);
	g_altkey = (GetAsyncKeyState(VK_MENU) != NULL);
}
*/
