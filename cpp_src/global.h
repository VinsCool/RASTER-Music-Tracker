//
// global.h
// originally made by Raster, 2002-2009
// experimental changes and additions by VinsCool, 2021-2022
//

#ifndef RMT_GLOBAL_
#define RMT_GLOBAL_

#include <iostream>
#include <fstream>

extern unsigned char g_atarimem[65536];
extern char g_debugmem[65536];	//debug display of g_atarimem bytes directly, slow and terrible, do not use unless there is a purpose for it 

extern unsigned char SAPRSTREAM[0xFFFFFF];	//SAP-R Dumper memory, TODO: assign memory dynamically instead, however this doesn't seem to harm anything for now

extern int SAPRDUMP;		//0, the SAPR dumper is disabled; 1, output is currently recorded to memory; 2, recording is finished and will be written to sap file; 3, flag engage the SAPR dumper
extern int framecount;		//SAPR dumper frame counter used for calculations and memory allignment with bytes 

extern BOOL g_closeapplication;
extern CDC* g_mem_dc;
extern CDC* g_gfx_dc;

extern int g_width;
extern int g_height;
extern int g_tracklines;
extern int g_scaling_percentage;

//best known compromise for both regions, they produce identical tables
extern double g_basetuning;
extern int g_basenote;

//ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
extern double g_UNISON;
extern double g_MIN_2ND;
extern double g_MAJ_2ND;
extern double g_MIN_3RD;
extern double g_MAJ_3RD;
extern double g_PERF_4TH;
extern double g_TRITONE;
extern double g_PERF_5TH;
extern double g_MIN_6TH;
extern double g_MAJ_6TH;
extern double g_MIN_7TH;
extern double g_MAJ_7TH;
extern double g_OCTAVE;

//ratio left
extern int g_UNISON_L;
extern int g_MIN_2ND_L;
extern int g_MAJ_2ND_L;
extern int g_MIN_3RD_L;
extern int g_MAJ_3RD_L;
extern int g_PERF_4TH_L;
extern int g_TRITONE_L;
extern int g_PERF_5TH_L;
extern int g_MIN_6TH_L;
extern int g_MAJ_6TH_L;
extern int g_MIN_7TH_L;
extern int g_MAJ_7TH_L;
extern int g_OCTAVE_L;

//ratio right
extern int g_UNISON_R;
extern int g_MIN_2ND_R;
extern int g_MAJ_2ND_R;
extern int g_MIN_3RD_R;
extern int g_MAJ_3RD_R;
extern int g_PERF_4TH_R;
extern int g_TRITONE_R;
extern int g_PERF_5TH_R;
extern int g_MIN_6TH_R;
extern int g_MAJ_6TH_R;
extern int g_MIN_7TH_R;
extern int g_MAJ_7TH_R;
extern int g_OCTAVE_R;

//each preset is assigned to a number. 0 means no Temperament, any value that is not assigned defaults to custom
extern int g_temperament;

extern HWND g_hwnd;
extern HWND g_viewhwnd;

extern HINSTANCE g_c6502_dll;
extern BOOL volatile g_is6502;
extern CString g_aboutpokey;
extern CString g_about6502;
extern CString g_driverversion;

extern BOOL g_changes;	//have there been any changes in the module?

extern int g_focus;
extern int g_shiftkey;
extern int g_controlkey;
extern int g_altkey;	//unfinished implementation, doesn't work yet for some reason

extern int g_tracks4_8;
extern BOOL volatile g_screenupdate;
extern BOOL volatile g_invalidatebytimer;
extern int volatile g_screena;
extern int volatile g_screenwait;
extern BOOL volatile g_rmtroutine;
extern BOOL volatile g_timerroutineprocessed;

extern int volatile g_prove;			//test notes without editing (0 = off, 1 = mono, 2 = stereo)
extern int volatile g_respectvolume;	//does not change the volume if it is already there

extern WORD g_rmtstripped_adr_module;	//address for export RMT stripped file
extern BOOL g_rmtstripped_sfx;			//sfx offshoot RMT stripped file
extern BOOL g_rmtstripped_gvf;			//gvs GlobalVolumeFade for feat
extern BOOL g_rmtstripped_nos;			//nos NoStartingSongline for feat

extern CString g_rmtmsxtext;
extern CString g_expasmlabelprefix;	//label prefix for export ASM simple notation

extern int g_activepart;			//0 info, 1 edittracks, 2 editinstruments, 3 song
extern int g_active_ti;			//1 tracks, 2 instrs

extern BOOL is_editing_instr;		//0 no, 1 instrument name is edited
extern BOOL is_editing_infos;		//0 no, 1 song name is edited

extern int g_tracklinehighlight;	//line highlighted every x lines
extern BOOL g_tracklinealtnumbering; //alternative way of line numbering in tracks
extern int g_linesafter;			//number of lines to scroll after inserting a note (initializes in CSong :: Clear)
extern BOOL g_ntsc;				//NTSC (60Hz)
extern BOOL g_nohwsoundbuffer;	//Don't use hardware soundbuffer
extern int g_cursoractview;		//default position, line 0

extern BOOL g_displayflatnotes;	//flats instead of sharps
extern BOOL g_usegermannotation;	//H notes instead of B

extern int g_channelon[SONGTRACKS];
extern int g_rmtinstr[SONGTRACKS];

extern BOOL g_viewmaintoolbar;		//1 yes, 0 no
extern BOOL g_viewblocktoolbar;		//1 yes, 0 no
extern BOOL g_viewstatusbar;		//1 yes, 0 no
extern BOOL g_viewplaytimecounter;	//1 yes, 0 no
extern BOOL g_viewanalyzer;			//1 yes, 0 no
extern BOOL g_viewpokeyregs;		//1 yes, 0 no
extern BOOL g_viewinstractivehelp;	//1 yes, 0 no

extern long g_playtime;				//1 yes, 0 no

extern UINT g_mousebutt;			//mouse button

extern CString g_prgpath;					//path to the directory from which the program was started (including a slash at the end)
extern CString g_lastloadpath_songs;		//the path of the last song loaded
extern CString g_lastloadpath_instruments; //the path of the last instrument loaded
extern CString g_lastloadpath_tracks;		//the path of the last track loaded

extern CString g_path_songs;		//default path for songs
extern CString g_path_instruments;	//default path for instruments
extern CString g_path_tracks;		//default path for tracks

extern int g_keyboard_layout;			//1 yes, 0 no, not be useful anymore... should be deleted
extern BOOL g_keyboard_swapenter;		//1 yes, 0 no, probably not needed anymore but will be kept for now
extern BOOL g_keyboard_playautofollow;	//1 yes, 0 no
extern BOOL g_keyboard_updowncontinue;	//1 yes, 0 no
extern BOOL g_keyboard_rememberoctavesandvolumes;	//1 yes, 0 no
extern BOOL g_keyboard_escresetatarisound;	//1 yes, 0 no
extern BOOL g_keyboard_askwhencontrol_s;	//1 yes, 0 no

extern int g_midi_notech[16];			//last recorded notes on each MIDI channel
extern int g_midi_voluch[16];			//note volume on individual MIDI channels
extern int g_midi_instch[16];			//last set instrument numbers on individual MIDI channels

extern BOOL g_midi_tr;
extern int g_midi_volumeoffset;	//starts from volume 1 by default
extern BOOL g_midi_noteoff;		//by default, noteoff is turned off

#endif