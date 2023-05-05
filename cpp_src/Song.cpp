#include "StdAfx.h"
#include <fstream>
#include <chrono>

#include "GuiHelpers.h"
#include "Song.h"

// MFC interface code
#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"
#include "EffectsDlg.h"
#include "MainFrm.h"

#include "Atari6502.h"
#include "XPokey.h"
#include "IOHelpers.h"
#include "Instruments.h"
#include "Clipboard.h"
#include "global.h"
#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"
#include "PokeyStream.h"
#include "ModuleV2.h"
#include "Tuning.h"

extern CSong g_Song;
extern CInstruments g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CXPokey g_Pokey;
extern CPokeyStream g_PokeyStream;
extern CModule g_Module;
extern CTuning g_Tuning;

// ----------------------------------------------------------------------------

std::chrono::steady_clock::time_point m_deltaTimerRoutine;
static bool volatile m_timerRoutineSwitch;

void CALLBACK G_TimerRoutine(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
	std::this_thread::sleep_until(m_deltaTimerRoutine);
	m_deltaTimerRoutine = std::chrono::steady_clock::now() + std::chrono::milliseconds(g_ntsc ? 16 + (m_timerRoutineSwitch ^= 1) : 20);
	g_Song.TimerRoutine();
}

// ----------------------------------------------------------------------------

CSong::CSong()
{
	m_timerRoutine = NULL;
	m_songVariables = NULL;
	//m_quantization_note = m_quantization_instr = m_quantization_vol = -1;
}

CSong::~CSong()
{
	KillTimer();
	ClearSongVariables();
}

/// <summary>
/// Change the timing of how often the CSong::TimerRoutine is being called.
/// Depends on PAL or NTSC timing.
/// </summary>
/// <param name="ms">ms between calls (17=NTSC, 20=PAL)</param>
void CSong::ChangeTimer(int ms)
{
	KillTimer();
	m_timerRoutine = timeSetEvent(ms, 0, G_TimerRoutine, (ULONG)(NULL), TIME_PERIODIC);
}

/// <summary>
/// Immediately kill the timer event
/// </summary>
void CSong::KillTimer()
{
	if (m_timerRoutine) 
		timeKillEvent(m_timerRoutine);

	m_timerRoutine = NULL;
}

/// <summary>
/// Reset the song data to empty and return RMT into a default state
/// </summary>
/// <param name="numOfTracks">How many tracks are supported 4 or 8</param>
void CSong::ClearSong(int numOfTracks)
{
	Stop();

	g_tracks4_8 = numOfTracks;			// Track for 4/8 channels
	g_rmtroutine = 1;					// RMT routine execution enabled
	g_prove = 0;
	g_respectvolume = 0;
	g_rmtstripped_adr_module = 0x4000;	// Default standard address for stripped RMT modules
	g_rmtstripped_sfx = 0;				// Is not a standard sfx variety stripped RMT
	g_rmtstripped_gvf = 0;				// Default does not use Feat GlobalVolumeFade
	g_rmtmsxtext = "";					// Clear the text for MSX export
	g_PrefixForAllAsmLabels = "MUSIC";	// Default label prefix for exporting simple ASM notation

	//PlayPressedTonesInit();

	m_isFollowPlay = 1;
	m_mainSpeed = m_playSpeed = m_speedTimer = 16;
	m_instrumentSpeed = 1;

	g_activepart = g_active_ti = PART_TRACKS;

	m_playSongline = m_activeSongline = 0;
	m_activeRow = m_playRow = 0;
	m_activeChannel = m_activeCursor = 0;
	m_activeinstr = 0;
	m_octave = 0;
	m_volume = MAXVOLUME;

	//ClearBookmark();

	m_infoact = INFO_ACTIVE_NAME;

	memset(m_songname, ' ', SONG_NAME_MAX_LEN);
	strncpy(m_songname, "Noname song", 11);
	m_songname[SONG_NAME_MAX_LEN] = 0;

	m_songnamecur = 0;

	m_filename = "";
	m_filetype = IOTYPE_NONE;
	m_lastExportType = IOTYPE_NONE;

	m_TracksOrderChange_songlinefrom = 0x00;
	m_TracksOrderChange_songlineto = SONGLEN - 1;

	// Number of lines after inserting a note/space
	g_linesafter = 1; // Initial value
	CMainFrame* mf = ((CMainFrame*)AfxGetMainWnd());
	if (mf) mf->m_comboSkipLinesAfterNoteInsert.SetCurSel(g_linesafter);

	for (int i = 0; i < SONGLEN; i++)
	{
		for (int j = 0; j < SONGTRACKS; j++)
		{
			m_song[i][j] = -1;	// TRACK --
		}
		m_songgo[i] = -1;		// Is not GO
	}

	// Empty clipboards
	g_TrackClipboard.Empty();
	m_instrclipboard.activeEditSection = -1;	// According to -1 it knows that it is empty
	m_songgoclipboard = -2;						// According to -2 it knows that it is empty

	// Delete all tracks and instruments
	g_Tracks.InitTracks();
	g_Instruments.InitInstruments();

	// Undo initialization
	g_Undo.Init();

	// Changes in the module
	g_changes = 0;

	// Initialise RMT routine, to clear anything leftover in Atari memory
	Atari_InitRMTRoutine();

	// Initialise the RMTE Module as well, since it will progressively replace the Legacy format, and will use most of the same functions
	//g_Module.InitialiseModule();

	// Initialise Song variables
	InitialiseSongVariables();
}

void CSong::ClearSongVariables()
{
	if (m_songVariables)
		delete m_songVariables;

	m_songVariables = NULL;
}

void CSong::InitialiseSongVariables()
{
	ClearSongVariables();
	m_songVariables = new TSongVariables[SONGTRACKS];

	for (int i = 0; i < SONGTRACKS; i++)
		ResetChannelVariables(&m_songVariables[i]);
}

void CSong::ResetChannelVariables(TSongVariables* p)
{
	if (!p)
		return;

	p->isDelayEnabled = false;
	p->isNoteActive = false;
	p->isNoteRelease = false;
	p->isNoteSustain = false;
	p->isNoteTrigger = false;
	p->isPortamentoEnabled = false;
	p->isTremoloEnabled = false;
	p->isVibratoEnabled = false;
	p->isVolumeOnlyEnabled = false;
	p->arpeggioScheme = 0x00;
	p->channelAUDCTL = 0x00;
	p->channelDistortion = TIMBRE_PURE;
	p->channelFreq = 0x0000;
	p->channelInstrument = INVALID;
	p->channelNote = INVALID;
	p->channelVolume = 0x00;
	p->delayedRow = NULL;
	p->delayOffset = 0x00;
	p->finetuneOffset = EFFECT_PARAMETER_DEFAULT;
	p->frameCount = 0x00;
	p->instrumentVolume = 0x00;
	p->portamentoSpeed = 0x00;
	p->portamentoTarget = 0x0000;
	p->tremoloDepth = 0x00;
	p->tremoloSpeed = 0x00;
	p->vibratoDepth = 0x00;
	p->vibratoSpeed = 0x00;
	p->volumeSlide = 0x00;
}

//---

/*
int CSong::GetSubsongParts(CString& resultstr)
{
	CString s;
	int songp[SONGLEN];
	int i, j, n, lastgo, apos, asub;
	BOOL ok;
	lastgo = -1;
	for (i = 0; i < SONGLEN; i++)
	{
		songp[i] = -1;
		if (m_songgo[i] >= 0) lastgo = i;
	}

	resultstr = "";
	apos = 0;
	asub = 0;
	ok = 0;	//if it found any non-zero tracks in the given subsong (different from --)

	for (i = 0; i <= lastgo; i++)
	{
		if (songp[i] < 0)
		{
			apos = i;
			while (songp[apos] < 0)
			{
				n = m_songgo[apos];
				songp[apos] = asub;
				if (n >= 0) //jump to another line
					apos = n;
				else
				{
					if (!ok)
					{	//has not found any tracks in this subsong yet
						for (j = 0; j < g_tracks4_8; j++)
						{
							if (m_song[apos][j] >= 0)
							{	//if then found, this will be the beginning of the subsong
								s.Format("%02X ", apos);
								resultstr += s;
								ok = 1;		//the beginning of this subsong is already written
								break;
							}
						}
					}
					apos++;
					if (apos >= SONGLEN) break;
				}
			}
			if (ok) asub++;	//will move to the next if the subsong contains anything at all
			ok = 0; //initialization for further search
		}
	}
	return asub;
}
*/

/// <summary>
/// Mark all tracks that are referenced in the song as USED
/// </summary>
/// <param name="arrayTRACKSNUM">Array where each used track if marked off</param>
void CSong::MarkTF_USED(BYTE* arrayTRACKSNUM)
{
	//all tracks used in the song
	for (int i = 0; i < SONGLEN; i++)
	{
		if (m_songgo[i] < 0)
		{
			for (int channelNr = 0; channelNr < g_tracks4_8; channelNr++)
			{
				int tr = m_song[i][channelNr];
				if (tr >= 0 && tr < TRACKSNUM) arrayTRACKSNUM[tr] = TF_USED;
			}
		}
	}
}

void CSong::MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM)
{
	for (int i = 0; i < TRACKSNUM; i++)
	{
		if (g_Tracks.CalculateNotEmpty(i)) arrayTRACKSNUM[i] |= TF_NOEMPTY;
	}
}

/*
int CSong::MakeTuningBlock(unsigned char* mem, int addr)
{
	int len = 80;				// 80 bytes of general data
	for (int i = 0; i < len; ++i) mem[addr + i] = 0;
	
	// Block indicator: 0xF3
	mem[addr] = 0xF3;			// Tuning block indicator

	// First 16 bytes
	mem[addr + 0x01] = g_ntsc;						//RMT module region, 0 -> PAL, 1 -> NTSC
	mem[addr + 0x02] = g_basenote;					//base note used in tuning calculations, eg A-4
	mem[addr + 0x03] = g_temperament;				//tuning temperament, 0 -> no temperament, any number above preset number is custom (saving ratios not yet implemented)
	mem[addr + 0x04] = g_trackLinePrimaryHighlight;	//track primary line highlight
	mem[addr + 0x05] = g_trackLineSecondaryHighlight;//track secondary line highlight
	// 6 - 0xf is unused

	// 64 bytes
	memcpy((mem + addr + 0x10), &g_basetuning, 8);	//base tuning frequency, double type uses 8 bytes in memory
	memcpy((mem + addr + 0x18), &g_UNISON_L, 2);		//tuning ratio variables, each values are truncated to use 2 bytes (16-bit precision) 
	memcpy((mem + addr + 0x1A), &g_UNISON_R, 2);
	memcpy((mem + addr + 0x1C), &g_MIN_2ND_L, 2);
	memcpy((mem + addr + 0x1E), &g_MIN_2ND_R, 2);
	memcpy((mem + addr + 0x20), &g_MAJ_2ND_L, 2);
	memcpy((mem + addr + 0x22), &g_MAJ_2ND_R, 2);
	memcpy((mem + addr + 0x24), &g_MIN_3RD_L, 2);
	memcpy((mem + addr + 0x26), &g_MIN_3RD_R, 2);
	memcpy((mem + addr + 0x28), &g_MAJ_3RD_L, 2);
	memcpy((mem + addr + 0x2A), &g_MAJ_3RD_R, 2);
	memcpy((mem + addr + 0x2C), &g_PERF_4TH_L, 2);
	memcpy((mem + addr + 0x2E), &g_PERF_4TH_R, 2);
	memcpy((mem + addr + 0x30), &g_TRITONE_L, 2);
	memcpy((mem + addr + 0x32), &g_TRITONE_R, 2);
	memcpy((mem + addr + 0x34), &g_PERF_5TH_L, 2);
	memcpy((mem + addr + 0x36), &g_PERF_5TH_R, 2);
	memcpy((mem + addr + 0x38), &g_MIN_6TH_L, 2);
	memcpy((mem + addr + 0x3A), &g_MIN_6TH_R, 2);
	memcpy((mem + addr + 0x3C), &g_MAJ_6TH_L, 2);
	memcpy((mem + addr + 0x3E), &g_MAJ_6TH_R, 2);
	memcpy((mem + addr + 0x40), &g_MIN_7TH_L, 2);
	memcpy((mem + addr + 0x42), &g_MIN_7TH_R, 2);
	memcpy((mem + addr + 0x44), &g_MAJ_7TH_L, 2);
	memcpy((mem + addr + 0x46), &g_MAJ_7TH_R, 2);
	memcpy((mem + addr + 0x48), &g_OCTAVE_L, 2);
	memcpy((mem + addr + 0x4A), &g_OCTAVE_R, 2);
	// 4 unused bytes at the end

	return len;
}
*/

// Reset all tuning variables
void CSong::ResetTuningVariables()
{
	g_baseTuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
	g_baseNote = 3;	// 3 = A-
	g_baseOctave = 4;
	//g_temperament = 0;	//no temperament
}

/*
int CSong::DecodeTuningBlock(unsigned char* mem, int addr, int endAddr)
{
	// Check the block header
	if (mem[addr] != 0xF3)
	{
		ResetTuningVariables();
		return 0;
	}
	// Get the basics
	g_ntsc							= mem[addr + 0x01];
	g_basenote						= mem[addr + 0x02];
	g_temperament					= mem[addr + 0x03];
	g_trackLinePrimaryHighlight		= mem[addr + 0x04];
	if (!g_trackLinePrimaryHighlight) g_trackLinePrimaryHighlight = 8;	//default
	g_trackLineSecondaryHighlight	= mem[addr + 0x05];
	if (!g_trackLineSecondaryHighlight) g_trackLineSecondaryHighlight = 4;	//default

	memcpy(&g_basetuning, (mem + addr + 0x10), 8);
	memcpy(&g_UNISON_L, (mem + addr + 0x18), 2);
	memcpy(&g_UNISON_R, (mem + addr + 0x1A), 2);
	memcpy(&g_MIN_2ND_L, (mem + addr + 0x1C), 2);
	memcpy(&g_MIN_2ND_R, (mem + addr + 0x1E), 2);
	memcpy(&g_MAJ_2ND_L, (mem + addr + 0x20), 2);
	memcpy(&g_MAJ_2ND_R, (mem + addr + 0x22), 2);
	memcpy(&g_MIN_3RD_L, (mem + addr + 0x24), 2);
	memcpy(&g_MIN_3RD_R, (mem + addr + 0x26), 2);
	memcpy(&g_MAJ_3RD_L, (mem + addr + 0x28), 2);
	memcpy(&g_MAJ_3RD_R, (mem + addr + 0x2A), 2);
	memcpy(&g_PERF_4TH_L, (mem + addr + 0x2C), 2);
	memcpy(&g_PERF_4TH_R, (mem + addr + 0x2E), 2);
	memcpy(&g_TRITONE_L, (mem + addr + 0x30), 2);
	memcpy(&g_TRITONE_R, (mem + addr + 0x32), 2);
	memcpy(&g_PERF_5TH_L, (mem + addr + 0x34), 2);
	memcpy(&g_PERF_5TH_R, (mem + addr + 0x36), 2);
	memcpy(&g_MIN_6TH_L, (mem + addr + 0x38), 2);
	memcpy(&g_MIN_6TH_R, (mem + addr + 0x3A), 2);
	memcpy(&g_MAJ_6TH_L, (mem + addr + 0x3C), 2);
	memcpy(&g_MAJ_6TH_R, (mem + addr + 0x3E), 2);
	memcpy(&g_MIN_7TH_L, (mem + addr + 0x40), 2);
	memcpy(&g_MIN_7TH_R, (mem + addr + 0x42), 2);
	memcpy(&g_MAJ_7TH_L, (mem + addr + 0x44), 2);
	memcpy(&g_MAJ_7TH_R, (mem + addr + 0x46), 2);
	memcpy(&g_OCTAVE_L, (mem + addr + 0x48), 2);
	memcpy(&g_OCTAVE_R, (mem + addr + 0x4A), 2);

	return endAddr - addr;
}
*/

/// <summary>
/// Create the RMT data in memory.
/// Sets the 'instrumentSavedFlags' and 'trackSavedFlags' if a specific instrument or track is used.
/// </summary>
/// <param name="mem">Atari 64K of memory</param>
/// <param name="addr">Where in memory the start of the module will be</param>
/// <param name="iotype"></param>
/// <param name="instrumentSavedFlags"></param>
/// <param name="trackSavedFlags"></param>
/// <returns></returns>
int CSong::MakeModule(unsigned char* mem, int addr, int iotype, BYTE* instrumentSavedFlags, BYTE* trackSavedFlags)
{
	int i, j;
	TTrack* tr;

	// Returns maxadr (points to the first free address after the module) and sets the instrsaved and tracksaved fields
	if (iotype == IOTYPE_RMF) return MakeRMFModule(mem, addr, instrumentSavedFlags, trackSavedFlags);

	// Clear the instrument and tracks used flags
	memset(instrumentSavedFlags, 0, INSTRSNUM);
	memset(trackSavedFlags, 0, TRACKSNUM);

	// Write out the RMT header (part 1)
	// 0: RMT4 or RMT8
	// 4: Track length
	// 5: Song speed
	// 6: Instrument speed
	// 7: RMT version (1 for now)
	strncpy((char*)(mem + addr), "RMT", 3);	
	mem[addr + 3] = g_tracks4_8 + '0';			// 4 or 8
	mem[addr + 4] = g_Tracks.GetMaxTrackLength() & 0xff;
	mem[addr + 5] = m_mainSpeed & 0xff;
	mem[addr + 6] = m_instrumentSpeed;			// 1-4 player calls per frame
	mem[addr + 7] = RMTFORMATVERSION;			// RMT format version number

	// Note:
	// When saving in RMT format ALL non-empty tracks and non-empty instruments will be stored
	// In other formats only the USED tracks and USED instruments will be stored

	MarkTF_USED(trackSavedFlags);			// Mark all tracks as used
	if (iotype == IOTYPE_RMT)
	{
		MarkTF_NOEMPTY(trackSavedFlags);	// In addition to the used ones, all non-empty tracks are added to the RMT, all non-empty tracks
	}

	// Mark all used instruments in the tracks that will be saved
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (trackSavedFlags[i] > 0)
		{
			tr = g_Tracks.GetTrack(i);
			for (j = 0; j < tr->len; j++)
			{
				if (g_Tracks.IsValidInstrument(tr->instr[j])) instrumentSavedFlags[tr->instr[j]] = IF_USED;
			}
		}
	}

	if (iotype == IOTYPE_RMT)
	{
		// In addition to the instruments used in the tracks that are in the song, all non-empty instruments are stored in the RMT
		for (i = 0; i < INSTRSNUM; i++)
		{
			if (g_Instruments.CalculateNotEmpty(i)) instrumentSavedFlags[i] |= IF_NOEMPTY;
		}
	}

	//---
	// Find how many tracks and instruments to save
	int numTracks = 0;
	for (i = TRACKSNUM - 1; i >= 0; i--)
	{
		if (trackSavedFlags[i] > 0) { numTracks = i + 1; break; }
	}

	int numInstruments = 0;
	for (i = INSTRSNUM - 1; i >= 0; i--)
	{
		if (instrumentSavedFlags[i] > 0) { numInstruments = i + 1; break; }
	}

	// Calculate the offsets for instruments, tracks (lo & hi) and song lines
	// RMT header is 16 bytes, so instrument ptrs start there
	// Each instrument ptr is 2 bytes, and the track pointers are 2 bytes
	// but split into low and high storage areas
	int ptrInstruments		= addr + 16;
	int ptrTracksLoBytes	= ptrInstruments + numInstruments * 2;
	int ptrTracksHiBytes	= ptrTracksLoBytes + numTracks;
	int ptrInstrumentData	= ptrTracksHiBytes + numTracks; // behind the track byte table

	// Saves instrument data and writes their beginnings to the table
	for (i = 0; i < numInstruments; i++)
	{
		if (instrumentSavedFlags[i])
		{
			// Create instrument data
			int thisInstrumentLength = g_Instruments.InstrToAta(i, mem + ptrInstrumentData, MAXATAINSTRLEN);

			// Save where the instrument data is to be found
			mem[ptrInstruments + i * 2]		= ptrInstrumentData & 0xff;	// lo byte
			mem[ptrInstruments + i * 2 + 1] = ptrInstrumentData >> 8;	// hi byte

			// Move where the next instruments data is to be saved
			ptrInstrumentData += thisInstrumentLength;
		}
		else
		{
			// Oi, nothing to save here, just emit 0
			// This happens if there are unused instruments between the used ones.
			// Best to rearrange the instruments to be one continous block
			mem[ptrInstruments + i * 2] = mem[ptrInstruments + i * 2 + 1] = 0;
		}
	}

	// Just after of the instrument data we start with the track data
	int ptrTrackData = ptrInstrumentData;

	// Saves track data and write their beginnings to the table
	for (i = 0; i < numTracks; i++)
	{
		if (trackSavedFlags[i])
		{
			// Create the track data
			int thisTrackLength = g_Tracks.TrackToAta(i, mem + ptrTrackData, MAXATATRACKLEN);

			// Check that the track data is valid
			if (thisTrackLength < 1)
			{	
				// Track cannot be saved to RMT
				CString msg;
				msg.Format("Fatal error in track %02X.\n\nThis track contains too many events (notes and speed commands),\nthat's why it can't be coded to RMT internal code format.", i);
				MessageBox(g_hwnd, msg, "Internal format problem.", MB_ICONERROR);
				return -1;
			}

			// Save where the track data is to be found
			mem[ptrTracksLoBytes + i] = ptrTrackData & 0xff;	// lo byte
			mem[ptrTracksHiBytes + i] = ptrTrackData >> 8;		// hi byte

			// Move where the next track's data is to be saved
			ptrTrackData += thisTrackLength;
		}
		else
		{
			// Oi, nothing to save here, just emit 0
			mem[ptrTracksLoBytes + i] = mem[ptrTracksHiBytes + i] = 0;
		}
	}

	// Just after the track data we store the song lines
	int ptrSongData = ptrTrackData;

	int thisSongLength = SongToAta(mem + ptrSongData, 0x10000 - ptrSongData, ptrSongData);

	int endOfModule = ptrSongData + thisSongLength;

	// Writes computed pointers to the header
	mem[addr + 8] = ptrInstruments & 0xff;		// lo byte pointer to instrument table
	mem[addr + 9] = ptrInstruments >> 8;		// hi byte
	//
	mem[addr + 10] = ptrTracksLoBytes & 0xff;	// lo byte pointer to low bytes of track data table
	mem[addr + 11] = ptrTracksLoBytes >> 8;		// hi byte
	mem[addr + 12] = ptrTracksHiBytes & 0xff;	// lo byte pointer to high bytes of track data table
	mem[addr + 13] = ptrTracksHiBytes >> 8;		// hi byte
	//
	mem[addr + 14] = ptrSongData & 0xff;		// lo byte pointer to song data (arrangements of tracks)
	mem[addr + 15] = ptrSongData >> 8;			// hi byte

	// Return the address of the first byte past the last one used
	return endOfModule;
}

int CSong::MakeRMFModule(unsigned char* mem, int adr, BYTE* instrsaved, BYTE* tracksaved)
{
	//returns maxadr (points to the first available address behind the module) and sets the instrsaved and tracksaved fields
	
	TTrack* tr;
	
	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;

	memset(instrsave, 0, INSTRSNUM);	//init
	memset(tracksave, 0, TRACKSNUM); //init

	mem[adr + 0] = m_instrumentSpeed;		//instr speed 1-4
	mem[adr + 1] = m_mainSpeed & 0xff;

	//all non-empty tracks and non-empty instruments will be stored in the RMT, in others only non-empty tracks and instruments used in them will be stored
	int i, j;

	//all tracks used in the song
	MarkTF_USED(tracksave);

	//all instruments in the tracks that will be saved
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (tracksave[i] > 0)
		{
			tr = g_Tracks.GetTrack(i);
			for (j = 0; j < tr->len; j++)
			{
				int ins = tr->instr[j];
				if (ins >= 0 && ins < INSTRSNUM) instrsave[ins] = IF_USED;
			}
		}
	}

	//---

	int numtracks = 0;
	for (i = TRACKSNUM - 1; i >= 0; i--)
	{
		if (tracksave[i] > 0) { numtracks = i + 1; break; }
	}

	int numinstrs = 0;
	for (i = INSTRSNUM - 1; i >= 0; i--)
	{
		if (instrsave[i] > 0) { numinstrs = i + 1; break; }
	}

	//and now save:
	//songlines
	//instruments
	//tracks 

	//Find out the length of a song and the shortest lengths of individual tracks in songlines
	int songlines = 0, a;
	int emptytrackusedinline = -1;
	BOOL emptytrackused = 0;
	int songline_trackslen[SONGLEN];
	for (i = 0; i < SONGLEN; i++)
	{
		int minlen = g_Tracks.GetMaxTrackLength();	//init
		if (m_songgo[i] >= 0)  //Go to line 
		{
			songlines = i + 1;	//temporary end
			if (emptytrackusedinline >= 0) emptytrackused = 1;
		}
		else
		{
			for (j = 0; j < g_tracks4_8; j++)
			{
				a = m_song[i][j];
				if (a >= 0 && a < TRACKSNUM)
				{
					songlines = i + 1;	//temporary end
					int tl = g_Tracks.GetLength(a);
					if (tl < minlen) minlen = tl; //is less than the shortest
				}
				else
					emptytrackusedinline = i;
			}
		}
		songline_trackslen[i] = minlen;
	}
	int lensong = (songlines - 1) * (g_tracks4_8 * 2 + 3);	//songlines-1, because the last GOTO does not have to be put there

	//stores instrument data and writes their beginnings to the table in the temporary memory meminstruments with respect to the beginning 0
	unsigned char meminstruments[65536];
	memset(meminstruments, 0, 65536);
	int adrinstrdata = numinstrs * 2;
	for (i = 0; i < numinstrs; i++)
	{
		if (instrsave[i])
		{
			int leninstr = g_Instruments.InstrToAtaRMF(i, meminstruments + adrinstrdata, MAXATAINSTRLEN);
			meminstruments[i * 2] = adrinstrdata & 0xff;	//dbyte
			meminstruments[i * 2 + 1] = adrinstrdata >> 8;	//hbyte
			adrinstrdata += leninstr;
		}
	}
	int leninstruments = adrinstrdata;

	//saves track data and writes their beginnings to a table in the memtracks temporary memory due to the beginning 0
	unsigned char memtracks[65536];
	WORD trackpointers[TRACKSNUM];
	memset(memtracks, 0, 65536);
	memset(trackpointers, 0, TRACKSNUM * 2);
	int adrtrackdata = 0;	//from the beginning
	//empty track
	int adremptytrack = 0;
	if (emptytrackused)
	{
		//memtracks[0]=62;	//pause
		//memtracks[1]=0;		//infinite
		memtracks[0] = 255;	//infinite pause (FASTER modification)
		adrtrackdata += 1; //for empty track
	}
	for (i = 0; i < numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = g_Tracks.TrackToAtaRMF(i, memtracks + adrtrackdata, MAXATATRACKLEN);
			if (lentrack < 1)
			{	//cannot be saved to RMT
				CString msg;
				msg.Format("Fatal error in track %02X.\n\nThis track contains too many events (notes and speed commands),\nthat's why it can't be coded to RMT internal code format.", i);
				MessageBox(g_hwnd, msg, "Internal format problem.", MB_ICONERROR);
				return -1;
			}
			trackpointers[i] = adrtrackdata;
			adrtrackdata += lentrack;
		}
	}
	int lentracks = adrtrackdata;

	int adrsong = adr + 6;
	int adrinstruments = adrsong + lensong;
	int adrtracks = adrinstruments + leninstruments;
	int endofmodule = adrtracks + lentracks;

	//Construct the Song now
	int apos, go;
	for (int sline = 0; sline < songlines; sline++)
	{
		apos = adrsong + sline * (g_tracks4_8 * 2 + 3);
		if ((go = m_songgo[sline]) >= 0)
		{
			//thee is a goto line
			//=> to songline by 1 above, this changes GO nextline to GO somewhere else
			//
			if (apos >= 0) //GOTO songline 0
			{
				WORD goadr = adrsong + (go * (g_tracks4_8 * 2 + 3));
				mem[apos - 2] = goadr & 0xff;		//low byte
				mem[apos - 1] = (goadr >> 8);		//high byte
			}
			for (int j = 0; j < g_tracks4_8 * 2 + 3; j++) mem[apos + j] = 255; //just to make sure
			mem[apos + g_tracks4_8 * 2 + 2] = go;
		}
		else
		{
			//there are track numbers
			for (int i = 0; i < g_tracks4_8; i++)
			{
				j = m_song[sline][i];
				WORD at = 0;
				if (j >= 0 && j < TRACKSNUM)
					at = trackpointers[j] + adrtracks;
				else
					at = adremptytrack + adrtracks;	//empty track --
				mem[apos + i] = at & 0xff;					//db
				mem[apos + i + g_tracks4_8] = (at >> 8);	//hb

				mem[apos + g_tracks4_8 * 2] = songline_trackslen[sline]; //maxtracklen for this songline
				WORD nextsongline = apos + g_tracks4_8 * 2 + 3;
				mem[apos + g_tracks4_8 * 2 + 1] = nextsongline & 0xff;	//db
				mem[apos + g_tracks4_8 * 2 + 2] = (nextsongline >> 8);	//hb
			}
		}
	}

	//INSTRUMENTS cast (instrument pointers table and instruments data) add an offset in the pointer table
	for (i = 0; i < numinstrs; i++)
	{
		WORD ai = meminstruments[i * 2] + (meminstruments[i * 2 + 1] << 8) + adrinstruments;
		meminstruments[i * 2] = ai & 0xff;
		meminstruments[i * 2 + 1] = (ai >> 8);
	}
	//write the instrument table pointers and instrument data
	memcpy(mem + adrinstruments, meminstruments, leninstruments);

	//TRACKS cast
	memcpy(mem + adrtracks, memtracks, lentracks);

	//writes the calculated pointers to the header
	mem[adr + 2] = adrsong & 0xff;	//dbyte
	mem[adr + 3] = (adrsong >> 8);		//hbyte
	//
	mem[adr + 4] = adrinstruments & 0xff;		//dbyte
	mem[adr + 5] = (adrinstruments >> 8);		//hbyte

	return endofmodule;
}

/// <summary>
/// Decode the RMT header
/// </summary>
/// <param name="mem">Atari 64K memory</param>
/// <param name="fromAddr">Address where the header is loaded</param>
/// <param name="endAddr">Address of the first byte past the header</param>
/// <param name="instrumentLoadedFlags">64 byte memory buffer to indicate if a specific instrument was loaded</param>
/// <param name="trackLoadedFlags"></param>
/// <returns>0-If the module could not be loaded, version nr otherwise</returns>
int CSong::DecodeModule(unsigned char* mem, int fromAddr, int endAddr, BYTE* instrumentLoadedFlags, BYTE* trackLoadedFlags)
{
	int addr = fromAddr;

	memset(instrumentLoadedFlags, 0, INSTRSNUM);
	memset(trackLoadedFlags, 0, TRACKSNUM);

	unsigned char data;
	int i, j;

	BOOL loadState;

	// Check that the header starts with "RMT"
	if (strncmp((char*)(mem + addr), "RMT", 3) != 0) return 0; //there is no RMT

	// 4th byte: # of channels (4 or 8)
	data = mem[addr + 3];
	if (data != '4' && data != '8') return 0;	//it is not RMT4 or RMT8
	g_tracks4_8 = data & 0x0f;					// Store how many channels this module uses

	// 5th byte: track length
	data = mem[addr + 4];
	g_Tracks.SetMaxTrackLength((data > 0) ? data : 256);	//0 => 256

	// 6th byte: song speed
	data = mem[addr + 5];
	m_mainSpeed = data;
	if (data < 1) return 0;						// there can be no zero speed

	// 7th byte: Instrument speed
	data = mem[addr + 6];
	if (data < 1 || data > 8) return 0;			// Instrument speed is less than 1 or greater than 8 (note: should be max 4, but allows up to 8 and will only display a warning)
	m_instrumentSpeed = data;

	// 8th byte: RMT format version nr.
	int version = mem[addr + 7];
	if (version > RMTFORMATVERSION)	return 0;	//the byte version is above the current one

	// Now g_Tracks.m_maxTrackLength is set to the value in the RMT header, 
	// so re-initialize the tracks to set all tracks to this new length
	g_Tracks.InitTracks();

	// Get various pointers
	int ptrInstruments = mem[addr + 8] + (mem[addr + 9] << 8);			// Get ptr to the instuments
	int ptrTracksLow = mem[addr + 10] + (mem[addr + 11] << 8);			// Get ptr to tracks table low
	int ptrTracksHigh = mem[addr + 12] + (mem[addr + 13] << 8);			// Get ptr to tracks table high
	int ptrSong = mem[addr + 14] + (mem[addr + 15] << 8);				// Get ptr to track list (the song)

	// Calculate how long each of the sections are
	int numInstruments = (ptrTracksLow - ptrInstruments) / 2;
	int numTracks = (ptrTracksHigh - ptrTracksLow);
	int lengthSong = endAddr - ptrSong;

	// Decoding of individual instruments
	for (int instrumentNr = 0; instrumentNr < numInstruments; instrumentNr++)
	{
		// Get the ptr to an instruments configuration data
		int ptrOneInstrument = mem[ptrInstruments + instrumentNr * 2] + (mem[ptrInstruments + instrumentNr * 2 + 1] << 8);

		// Skip over empty instruments
		if (ptrOneInstrument == 0) continue; // Empty instruments have a NULL ptr

		// Depending on the file version load the instrument data into g_Instruments
		if (version == 0)
			loadState = g_Instruments.AtaV0ToInstr(mem + ptrOneInstrument, instrumentNr);
		else
			loadState = g_Instruments.AtaToInstr(mem + ptrOneInstrument, instrumentNr);

		g_Instruments.WasModified(instrumentNr);	//writes to Atari ram

		if (!loadState) return 0; // some problem with the instrument => END

		// Mark the instrument as loaded
		instrumentLoadedFlags[instrumentNr] = 1;
	}

	// Track data ptrs are split over two tables.  Low and high bytes, each indexed by the track number
	// Decoding individual tracks
	for (i = 0; i < numTracks; i++)
	{
		int trackNr = i;
		int ptrTrack = mem[ptrTracksLow + i] + (mem[ptrTracksHigh + i] << 8);
		if (ptrTrack == 0) continue; // Omitted tracks have pointer of 0

		// Identify the end of the track by the starting address of the next track,
		// and at the end by the starting address of the song data that follows the data of the last track
		int ptrTrackEnd = 0;
		for (j = i; j < numTracks; j++)
		{
			ptrTrackEnd = (j + 1 == numTracks) ? ptrSong : mem[ptrTracksLow + j + 1] + (mem[ptrTracksHigh + j + 1] << 8);
			if (ptrTrackEnd != 0) break;
			i++;	//continue from the next and skip the omitted one
		}

		int trackLength = ptrTrackEnd - ptrTrack;
		if (!g_Tracks.AtaToTrack(mem + ptrTrack, trackLength, trackNr)) return 0; //some problem with the track => END
		
		// Mark the track as loaded
		trackLoadedFlags[trackNr] = 1;
	}

	// Decoded song
	if (!AtaToSong(mem + ptrSong, lengthSong, ptrSong)) return 0; //some problem with the song => END

	//return version;	// BUG: RMT Module Version 0 is incorrectly returning error while it is valid!
	return 1;
}

//---

/*
BOOL CSong::PlayPressedTonesInit()
{
	for (int t = 0; t < SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t, -1, -1, -1);
	return 1;
}
*/

/*
BOOL CSong::SetPlayPressedTonesSilence()
{
	for (int t = 0; t < SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t, -1, -1, 0);
	return 1;
}
*/

/*
BOOL CSong::PlayPressedTones()
{
	int t, n, i, v;
	for (t = 0; t < SONGTRACKS; t++)
	{
		if ((v = m_playptvolume[t]) >= 0) //volume is set last
		{
			n = m_playptnote[t];
			i = m_playptinstr[t];
			if (n >= 0 && i >= 0)
				Atari_SetTrack_NoteInstrVolume(t, n, i, v);
			else
				Atari_SetTrack_Volume(t, v);
			SetPlayPressedTonesTNIV(t, -1, -1, -1);
		}
	}
	return 1;
}
*/

void CSong::ActiveInstrSet(int instr)
{
	g_Instruments.MemorizeOctaveAndVolume(m_activeinstr, m_octave, m_volume);
	m_activeinstr = instr;
	g_Instruments.RememberOctaveAndVolume(m_activeinstr, m_octave, m_volume);
}

/*
// Legacy Function
BOOL CSong::TrackUp(int lines)
{
	// Prevent movements during playback if followplay is enabled
	if (m_playMode && m_isFollowPlay)
		return 0;

	g_Undo.Separator();
	m_activeRow -= lines;	//subtract the number of lines from active track line 

	//GetSmallestMaxtracklen() seems to do a really good job for the navigation within the "compact" tracks display so far
	int trlen = GetSmallestMaxtracklen(m_activeSongline);

	if (m_activeRow < 0)	//track line is below 0
	{
		if (ISBLOCKSELECTED)	//a selection block is currently in use
		{
			m_activeRow = 0;	//prevent moving anywhere else
			return 1;
		}
		if (g_keyboard_updowncontinue)	//navigation between tracks is enabled
		{
			BLOCKDESELECT;
			SongUp();	//go to the next songline with current trackline position
			trlen = GetSmallestMaxtracklen(m_activeSongline);	//fetch the new pattern length as well
		}
		m_activeRow = m_activeRow + trlen;	//active line should appear at the bottom line, from the previous pattern movement 
		if (m_activeRow < 0)	//active line is still below 0? assume max track length to be the correct position, so the next movement up will rectify itself
			m_activeRow = trlen - lines;
	}
	if (m_activeRow > trlen)
		m_activeRow = trlen - lines;	//above max track length, snap back in-bounds, and take the number of used for movements as well 
	return 1;
}

// Legacy Function
BOOL CSong::TrackDown(int lines, BOOL stoponlastline)
{
	// Prevent movements during playback if followplay is enabled
	if (m_playMode && m_isFollowPlay)
		return 0;

	// An invalid combination should be ignored
	if (!g_keyboard_updowncontinue && stoponlastline && m_activeRow + lines > TrackGetLastLine())
		return 0;

	g_Undo.Separator();
	m_activeRow += lines;	//add the number of lines to move down to the current active trackline

	//GetSmallestMaxtracklen() seems to do a really good job for the navigation within the "compact" tracks display so far
	int trlen = GetSmallestMaxtracklen(m_activeSongline);	//identify the true track length in song line 
	if (!trlen) trlen = g_Tracks.GetMaxTrackLength();	//in case the smallest max track length returned zero (eg from a goto line)

	if (m_activeRow >= trlen)	//active line is equal or above max track length
	{
		if (ISBLOCKSELECTED)
		{
			//m_activeRow = g_Tracks.m_maxtracklen - 1;	//prevent moving anywhere else
			m_activeRow = trlen - 1;	//prevent moving anywhere else
			return 1;
		}
		//m_activeRow = m_activeRow % g_Tracks.m_maxtracklen;
		m_activeRow = m_activeRow % trlen;	//active line is modulo of track length, it will roll over 
		if (g_keyboard_updowncontinue)	//navigation between tracks is enabled
		{
			BLOCKDESELECT;
			SongDown();	//go to the next songline with current trackline position
			trlen = GetSmallestMaxtracklen(m_activeSongline);	//fetch the new pattern length as well
		}
		if (m_activeRow < 0)	//active line is still below 0? assume max track length to be the correct position, so the next movement up will rectify itself
			m_activeRow = 0 + lines;
	}
	if (m_activeRow > trlen)
		m_activeRow = 0 + lines;	//above max track length, snap back in-bounds, and take the number of used for movements as well 
	return 1;
}

// Legacy Function
BOOL CSong::TrackLeft(BOOL column)
{
	g_Undo.Separator();
	if (column) goto track_leftcolumn;
	m_trackactivecur--;
	if (m_trackactivecur < 0)
	{
		m_trackactivecur = 3;	//previous speed column
	track_leftcolumn:
		m_trackactivecol--;
		if (m_trackactivecol < 0) m_trackactivecol = g_tracks4_8 - 1;
	}
	return 1;
}

// Legacy Function
BOOL CSong::TrackRight(BOOL column)
{
	g_Undo.Separator();
	if (column) goto track_rightcolumn;
	m_trackactivecur++;
	if (m_trackactivecur > 3)	//speed column
	{
		m_trackactivecur = 0;
	track_rightcolumn:
		m_trackactivecol++;
		if (m_trackactivecol >= g_tracks4_8) m_trackactivecol = 0;
	}
	return 1;
}
*/

void CSong::PatternLeft()
{
	if (--m_activeCursor < 0)
	{
		ChannelLeft();
		m_activeCursor = g_Module.GetEffectCommandCount(m_activeChannel) + 2;
	}
}

void CSong::PatternRight()
{
	if (++m_activeCursor > g_Module.GetEffectCommandCount(m_activeChannel) + 2)
	{
		ChannelRight();
		m_activeCursor = 0;
	}
}

void CSong::PatternUp(int rows)
{
	// Prevent movements during playback if followplay is enabled
	if (m_playMode != MPLAY_STOP && m_isFollowPlay)
		return;

	if ((m_activeRow -= rows) < 0)
	{
		// Moving between Songlines from Pattern boundaries is enabled
		if (g_keyboard_updowncontinue)
			SonglineUp();
		
		// Adjustment for the Row Index between Songlines, done after updating the Songline
		m_activeRow += g_Module.GetShortestPatternLength(m_activeSongline);
	}
}

void CSong::PatternDown(int rows)
{
	// Prevent movements during playback if followplay is enabled
	if (m_playMode != MPLAY_STOP && m_isFollowPlay)
		return;

	if ((m_activeRow += rows) >= g_Module.GetShortestPatternLength(m_activeSongline))
	{
		// Adjustment for the Row Index between Songlines, done before updating the Songline
		m_activeRow %= g_Module.GetShortestPatternLength(m_activeSongline);

		// Moving between Songlines from Pattern boundaries is enabled
		if (g_keyboard_updowncontinue)
			SonglineDown();
	}
}

void CSong::ChannelLeft()
{
	if (--m_activeChannel < 0)
		m_activeChannel += g_Module.GetChannelCount();

	if (m_activeCursor < 0)
		m_activeCursor = 0;

	if (m_activeCursor >= g_Module.GetEffectCommandCount(m_activeChannel) + 2)
		m_activeCursor = g_Module.GetEffectCommandCount(m_activeChannel) + 2;
}

void CSong::ChannelRight()
{
	++m_activeChannel %= g_Module.GetChannelCount();

	if (m_activeCursor < 0)
		m_activeCursor = 0;

	if (m_activeCursor >= g_Module.GetEffectCommandCount(m_activeChannel) + 2)
		m_activeCursor = g_Module.GetEffectCommandCount(m_activeChannel) + 2;
}

/*
// FIXME: Not actually needed, but required for working around issues with the Legacy functions...
void CSong::RespectBoundaries()
{
	int songline = SongGetActiveLine();

	if (songline > SONGLEN) songline = SONGLEN - 1;
	if (songline < 0) songline = 0;

	int length = GetSmallestMaxtracklen(songline);
	int line = GetActiveLine();

	if (line > length) line = length - 1;
	if (line < 0) line = 0;

	SetActiveLine(line);
	SongSetActiveLine(songline);
}
*/

/*
void CSong::TrackGetLoopingNoteInstrVol(int track, int& note, int& instr, int& vol)
{
	// Set the current visible note to a possible goto loop
	int line, len, go;
	len = g_Tracks.GetLastLine(track) + 1;
	go = g_Tracks.GetGoLine(track);
	if (m_activeRow < len)
		line = m_activeRow;
	else
	{
		int loop = (go - len) + go;
		if (go >= 0 && loop)
		{
			line = (m_activeRow - len) % loop;
		}
		else
		{
			note = instr = vol = -1;
			return;
		}
	}
	note = g_Tracks.GetNote(track, line);
	instr = g_Tracks.GetInstr(track, line);
	vol = g_Tracks.GetVol(track, line);
}
*/

int* CSong::GetUECursor(int part)
{
	int* cursor;
	switch (part)
	{
		case PART_TRACKS:
			cursor = new int[4];
			cursor[0] = m_activeSongline;
			cursor[1] = m_activeRow;
			cursor[2] = m_activeChannel;
			cursor[3] = m_activeCursor;
			break;

		case PART_SONG:
			cursor = new int[2];
			cursor[0] = m_activeSongline;
			cursor[1] = m_activeChannel;
			break;

		case PART_INSTRUMENTS:
		{
			cursor = new int[6];
			cursor[0] = m_activeinstr;
			TInstrument* in = g_Instruments.GetInstrument(m_activeinstr);
			cursor[1] = in->activeEditSection;
			cursor[2] = in->editEnvelopeX;
			cursor[3] = in->editEnvelopeY;
			cursor[4] = in->editParameterNr;
			cursor[5] = in->editNoteTableCursorPos;
			//=in->activenam; It omits that any change in the cursor position in the name is not a reason for undo separation
		}
		break;

		case PART_INFO:
		{
			cursor = new int[1];
			cursor[0] = m_infoact;
		}
		break;

		default:
			cursor = NULL;
	}
	return cursor;
}

void CSong::SetUECursor(int part, int* cursor)
{
	switch (part)
	{
		case PART_TRACKS:
			m_activeSongline = cursor[0];
			m_activeRow = cursor[1];
			m_activeChannel = cursor[2];
			m_activeCursor = cursor[3];
			g_activepart = g_active_ti = PART_TRACKS;
			break;

		case PART_SONG:
			m_activeSongline = cursor[0];
			m_activeChannel = cursor[1];
			g_activepart = PART_SONG;
			break;

		case PART_INSTRUMENTS:
			m_activeinstr = cursor[0];
			//the other parameters 1-5 are within the instrument (TInstrument structure), so it is not necessary to set
			g_activepart = g_active_ti = PART_INSTRUMENTS;
			break;

		case PART_INFO:
			m_infoact = cursor[0];
			g_activepart = PART_INFO;
			break;

		default:
			return;	//don't change g_activepart !!!

	}
}

BOOL CSong::UECursorIsEqual(int* cursor1, int* cursor2, int part)
{
	int len;
	switch (part)
	{
		case PART_TRACKS:	len = 4;
			break;
		case PART_SONG:		len = 2;
			break;
		case PART_INSTRUMENTS:	len = 6;
			break;
		case PART_INFO:		len = 1;
			break;
		default:
			return 0;
	}
	for (int i = 0; i < len; i++) if (cursor1[i] != cursor2[i]) return 0;
	return 1;
}


//----------

/*
// Legacy Function
BOOL CSong::SongUp()
{
	BLOCKDESELECT;
	g_Undo.Separator();

	m_activeSongline--;

	if (!IsValidSongline(m_activeSongline))
		m_activeSongline = SONGLEN - 1;

	if (m_playMode && m_isFollowPlay)
	{
		// Play track in loop, else, play from cursor position
		int mode = (m_playMode == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;
		Stop();

		// This is a Gotoline, skip another line above it
		if (IsSongGo(m_activeSongline))
			m_activeSongline--;

		// If the line is no longer valid, force it to the last line instead
		if (!IsValidSongline(m_activeSongline))
			m_activeSongline = SONGLEN - 1;

		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;

		// Continue playing using the correct parameters
		Play(mode, m_isFollowPlay);
	}
	return 1;
}

// Legacy Function
BOOL CSong::SongDown()
{
	BLOCKDESELECT;
	g_Undo.Separator();

	m_activeSongline++;

	if (!IsValidSongline(m_activeSongline))
		m_activeSongline = 0;

	if (m_playMode && m_isFollowPlay)
	{
		// Play track in loop, else, play from cursor position
		int mode = (m_playMode == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;
		Stop();

		// This is a Gotoline, skip another line below it
		if (IsSongGo(m_activeSongline))
			m_activeSongline++;

		// If the line is no longer valid, force it to the first line instead
		if (!IsValidSongline(m_activeSongline))
			m_activeSongline = 0;

		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;

		// Continue playing using the correct parameters
		Play(mode, m_isFollowPlay);
	}
	return 1;
}

// Legacy Function
BOOL CSong::SongSubsongPrev()
{
	g_Undo.Separator();
	int i = m_activeSongline - 1;

	//only few lines in track have been played, or active line is 0, search for 1 subsong earlier to avoid being sent back to the same line each time 
	if ((m_playMode && m_isFollowPlay && m_playRow < 16) || m_activeRow == 0)
		i--;
	for (; i >= 0; i--)
	{
		if (m_songgo[i] >= 0)
		{
			m_activeSongline = i + 1;
			break;
		}
	}
	if (i < 0) m_activeSongline = 0;
	m_activeRow = 0;
	if (m_playMode && m_isFollowPlay)
	{
		int mode = (m_playMode == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;
		Play(mode, m_isFollowPlay); // continue playing using the correct parameters
	}
	return 1;
}

// Legacy Function
BOOL CSong::SongSubsongNext()
{
	g_Undo.Separator();
	int i;
	for (i = m_activeSongline; i < SONGLEN; i++)
	{
		if (m_songgo[i] >= 0)
		{
			if (i < (SONGLEN - 1))
				m_activeSongline = i + 1;
			else
				m_activeSongline = SONGLEN - 1; //Goto on the last songline (=> it is not possible to set a line below it!)
			m_activeRow = 0;
			break;
		}
	}
	if (m_playMode && m_isFollowPlay)
	{
		int mode = (m_playMode == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;
		Play(mode, m_isFollowPlay); // continue playing using the correct parameters
	}
	return 1;
}
*/

void CSong::SonglineUp()
{
	if (--m_activeSongline < 0)
		m_activeSongline += g_Module.GetSongLength();

	if (m_playMode != MPLAY_STOP && m_isFollowPlay)
	{
		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;
	}

	// Prevent the Active Pattern Row to go out of bounds
	m_activeRow %= g_Module.GetShortestPatternLength(m_activeSongline);
}

void CSong::SonglineDown()
{
	++m_activeSongline %= g_Module.GetSongLength();

	if (m_playMode != MPLAY_STOP && m_isFollowPlay)
	{
		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow = 0;
	}

	// Prevent the Active Pattern Row to go out of bounds
	m_activeRow %= g_Module.GetShortestPatternLength(m_activeSongline);
}

void CSong::SeekNextSubtune()
{
	int activeSubtune = g_Module.GetActiveSubtune();

	++activeSubtune %= g_Module.GetSubtuneCount();

	g_Module.SetActiveSubtune(activeSubtune);

	m_playSongline = m_activeSongline = 0;
	m_activeRow = m_playRow = 0;
	m_activeChannel = m_activeCursor = 0;

	if (m_playMode != MPLAY_STOP)
	{
		Stop();
		Play(MPLAY_START, m_isFollowPlay);
	}
}

void CSong::SeekPreviousSubtune()
{
	int activeSubtune = g_Module.GetActiveSubtune();

	if (--activeSubtune < 0)
		activeSubtune += g_Module.GetSubtuneCount();

	g_Module.SetActiveSubtune(activeSubtune);

	m_playSongline = m_activeSongline = 0;
	m_activeRow = m_playRow = 0;
	m_activeChannel = m_activeCursor = 0;

	if (m_playMode != MPLAY_STOP)
	{
		Stop();
		Play(MPLAY_START, m_isFollowPlay);
	}
}


BOOL CSong::SongTrackSet(int t)
{
	if (t >= -1 && t < TRACKSNUM)
	{
		g_Undo.ChangeSong(m_activeSongline, m_activeChannel, UETYPE_SONGTRACK);
		m_song[m_activeSongline][m_activeChannel] = t;
	}
	return 1;
}

/*
BOOL CSong::SongTrackSetByNum(int num)
{
	int i;
	if (m_songgo[m_activeSongline] < 0) // GO ?
	{	//changes track
		i = SongGetActiveTrack();
		if (i < 0) i = 0;
		i &= 0x0f;	//just the lower digit
		i = (i << 4) | num;
		if (i >= TRACKSNUM) i &= 0x0f;
		return SongTrackSet(i);
	}
	else
	{	//changes GO parameter
		i = m_songgo[m_activeSongline];
		if (i < 0) i = 0;
		i &= 0x0f;	//just the lower digit
		i = (i << 4) | num;
		if (i >= SONGLEN) i &= 0x0f;
		g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_activeSongline] = i;
		return 1;
	}
}
*/

/*
BOOL CSong::SongTrackDec()
{
	if (m_songgo[m_activeSongline] < 0)
	{
		int t = m_song[m_activeSongline][m_trackactivecol] - 1;
		if (t < -1) t = TRACKSNUM - 1;
		g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGTRACK);
		m_song[m_activeSongline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_activeSongline] - 1;
		if (g < 0) g = SONGLEN - 1;
		g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_activeSongline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackInc()
{
	if (m_songgo[m_activeSongline] < 0)
	{
		int t = m_song[m_activeSongline][m_trackactivecol] + 1;
		if (t >= TRACKSNUM) t = -1;
		g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGTRACK);
		m_song[m_activeSongline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_activeSongline] + 1;
		if (g >= SONGLEN) g = 0;
		g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_activeSongline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackEmpty()
{
	g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGTRACK);
	m_song[m_activeSongline][m_trackactivecol] = -1;
	return 1;
}

BOOL CSong::SongTrackGoOnOff()
{
	//GO on/off
	g_Undo.ChangeSong(m_activeSongline, m_trackactivecol, UETYPE_SONGGO);
	m_songgo[m_activeSongline] = (m_songgo[m_activeSongline] < 0) ? 0 : -1;
	return 1;
}
*/

BOOL CSong::SongInsertLine(int line)
{
	g_Undo.ChangeSong(line, m_activeChannel, UETYPE_SONGDATA, 0);
	int j, go;
	for (int i = SONGLEN - 2; i >= line; i--)
	{
		for (j = 0; j < g_tracks4_8; j++) m_song[i + 1][j] = m_song[i][j];
		go = m_songgo[i];
		if (go > 0 && go >= line) go++;
		m_songgo[i + 1] = go;
	}
	for (j = 0; j < g_tracks4_8; j++) m_song[line][j] = -1;
	m_songgo[line] = -1;
	for (int i = 0; i < line; i++)
	{
		if (m_songgo[i] >= line) m_songgo[i]++;
	}
	//if (IsBookmark() && m_bookmark.songline >= line)
	//{
	//	m_bookmark.songline++;
	//	if (m_bookmark.songline >= SONGLEN) ClearBookmark(); //just pushed the bookmark out of the song => cancel the bookmark
	//}
	return 1;
}

BOOL CSong::SongDeleteLine(int line)
{
	g_Undo.ChangeSong(line, m_activeChannel, UETYPE_SONGDATA, 0);
	int j, go;
	for (int i = line; i < SONGLEN - 1; i++)
	{
		for (j = 0; j < g_tracks4_8; j++) m_song[i][j] = m_song[i + 1][j];
		go = m_songgo[i + 1];
		if (go > 0 && go > line) go--;
		m_songgo[i] = go;
	}
	for (int i = 0; i < line; i++)
	{
		if (m_songgo[i] > line) m_songgo[i]--;
	}
	for (j = 0; j < g_tracks4_8; j++) m_song[SONGLEN - 1][j] = -1;
	m_songgo[SONGLEN - 1] = -1;
	//if (IsBookmark() && m_bookmark.songline >= line)
	//{
	//	m_bookmark.songline--;
	//	if (m_bookmark.songline < line) ClearBookmark(); //just deleted the songline with the bookmark
	//}
	return 1;
}

BOOL CSong::SongInsertCopyOrCloneOfSongLines(int& line)
{
	int i, j, k, d, n, sou, des;
	n = (line > 0) ? line - 1 : 0;
	CInsertCopyOrCloneOfSongLinesDlg dlg;

	dlg.m_linefrom = n;
	dlg.m_lineto = n;
	dlg.m_lineinto = line;
	dlg.m_clone = 0;
	dlg.m_tuning = 0;
	dlg.m_volumep = 100;	//100%

	if (dlg.DoModal() != IDOK) return 1;
	BLOCKDESELECT;					//the block is deselected only if it is OK

	BYTE tracks[TRACKSNUM];
	memset(tracks, 0, TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);
	int clonedto[TRACKSNUM];
	for (i = 0; i < TRACKSNUM; i++) clonedto[i] = -1; //init

	for (i = dlg.m_linefrom; i <= dlg.m_lineto; i++)
	{
		n = i - dlg.m_linefrom;
		sou = i;
		des = line + n;
		BOOL diss = (des <= sou);
		if (diss) sou += n;
		BOOL sngo = 0;
		if (sou < SONGLEN) sngo = (m_songgo[sou] >= 0);

		if (diss) sou++;
		if (sou < 0 || sou >= SONGLEN || des < 0 || des >= SONGLEN)
		{
			CString s;
			s.Format("Copy/clone operation had to be aborted\nbecause overrun of song range occured.\nThere was %i song lines inserted only.", n);
			MessageBox(g_hwnd, s, "Warning", MB_ICONSTOP);
			return 0;
		}

		SongInsertLine(des);	//inserted blank line

		if (dlg.m_clone && !sngo)
		{
			//clones
			//SongPrepareNewLine(des,sou,0);	//omits empty columns

			g_Undo.Separator(-1); //associates the previous insert lines to the next change
			g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, 1); //with separator

			for (j = 0; j < g_tracks4_8; j++)
			{
				k = m_song[sou][j]; //original track
				d = -1;				//resulting track (initial initialization)
				if (k < 0) continue;  //is there --
				if (clonedto[k] >= 0)
				{
					d = clonedto[k];	//this one has already been cloned, so it will also use it
				}
				else
				{
					d = FindNearTrackBySongLineAndColumn(sou, j, tracks);
					if (d >= 0)
					{
						tracks[d] = TF_USED;
						clonedto[k] = d;
						TrackCopyFromTo(k, d);
						//edit cloned track according to dlg.m_tuning and dlg.m_volumep
						g_Tracks.ModifyTrack(g_Tracks.GetTrack(d), 0, TRACKLEN - 1, -1, dlg.m_tuning, 0, dlg.m_volumep);
					}
					else
					{
						CString s;
						s.Format("Clone operation had to be aborted\nbecause out of unused empty tracks.\nThere was %i song line(s) inserted only.", n + 1);
						MessageBox(g_hwnd, s, "Warning", MB_ICONSTOP);
						return 0;
					}
				}
				m_song[des][j] = d;
			}
		}
		else
		{
			//copies
			m_songgo[des] = m_songgo[sou];
			for (j = 0; j < g_tracks4_8; j++) m_song[des][j] = m_song[sou][j];
		}
	}

	return 1;
}

BOOL CSong::SongPrepareNewLine(int& line, int sourceline, BOOL alsoemptycolumns) //Inserts a songline with unused empty tracks
{
	int i, k;

	if (sourceline < 0) sourceline = line + sourceline; //for -1 it is line-1

	SongInsertLine(line);	//inserts a blank line

	//prepares an online set of unused empty tracks

	BYTE tracks[TRACKSNUM];
	memset(tracks, 0, TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	int count = 0;
	for (i = 0; i < g_tracks4_8; i++)
	{
		if (!alsoemptycolumns && sourceline >= 0 && m_song[sourceline][i] < 0) continue;

		k = FindNearTrackBySongLineAndColumn(sourceline, i, tracks);
		if (k >= 0)
		{
			m_song[line][i] = k;
			tracks[k] = TF_USED;
			count++;
		}
	}

	if (count < g_tracks4_8)
	{
		if (count == 0)
			MessageBox(g_hwnd, "There isn't any empty unused track in song.", "Error", MB_ICONERROR);
		else
			MessageBox(g_hwnd, "Not enough empty unused tracks in song.", "Error", MB_ICONERROR);
		return 0;
	}

	return 1;
}

int CSong::FindNearTrackBySongLineAndColumn(int songline, int column, BYTE* arrayTRACKSNUM)
{
	int j, k, t;
	for (j = songline; j >= 0; j--)
	{
		if (m_songgo[j] >= 0) continue;
		if ((t = m_song[j][column]) >= 0)
		{
			//found the default track t
			for (k = t + 1; k < TRACKSNUM; k++)
			{
				if (arrayTRACKSNUM[k] == 0) return k;
			}
			//because it did not find any behind it, it will try to look in front of it instead
			for (k = t - 1; k >= 0; k--)
			{
				if (arrayTRACKSNUM[k] == 0) return k;
			}
		}
	}
	//will search for the first one usable from the beginning
	for (k = 0; k < TRACKSNUM; k++)
	{
		if (arrayTRACKSNUM[k] == 0) return k;
	}
	return -1;
}

BOOL CSong::SongPutnewemptyunusedtrack()
{
	int line = SongGetActiveLine();
	if (m_songgo[line] >= 0) return 0;		//it can't be done on the "GO TO LINE" line

	g_Undo.ChangeSong(line, m_activeChannel, UETYPE_SONGTRACK, 0);

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	int k = -1;
	m_song[line][cl] = -1;	//at current position in song --

	BYTE tracks[TRACKSNUM];
	memset(tracks, 0, TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	if (act >= 0 && !tracks[act])
		k = act;
	else
		k = FindNearTrackBySongLineAndColumn(line, cl, tracks);

	if (k < 0)
	{
		m_song[line][cl] = act;
		MessageBox(g_hwnd, "There isn't any empty unused track in song.", "Error", MB_ICONERROR);
		//UpdateShiftControlKeys();
		return 0;
	}

	m_song[line][cl] = k;
	return 1;
}

BOOL CSong::SongMaketracksduplicate()
{
	int line = SongGetActiveLine();
	if (m_songgo[line] >= 0) return 0;		//it can't be done on the "GO TO LINE" line

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	if (act < 0) return 0;			//cannot be duplicated, no track selected

	g_Undo.ChangeSong(line, cl, UETYPE_SONGTRACK, -1); //just cast

	int k = -1;
	m_song[line][cl] = -1;	//at current position in song --

	BYTE tracks[TRACKSNUM];
	memset(tracks, 0, TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	if (!(tracks[act] & TF_USED))
	{
		//not used anywhere else
		m_song[line][cl] = act;
		int r = MessageBox(g_hwnd, "This track is used only once in song.\nAre you sure to make duplicate?", "Make track's duplicate...", MB_OKCANCEL | MB_ICONQUESTION);
		if (r == IDOK)
			k = FindNearTrackBySongLineAndColumn(line, cl, tracks);
		else
		{
			g_Undo.DropLast();
			return 0;
		}
	}
	else
		k = FindNearTrackBySongLineAndColumn(line, cl, tracks);

	if (k < 0)
	{
		m_song[line][cl] = act;
		MessageBox(g_hwnd, "There isn't any empty unused track in song.", "Error", MB_ICONERROR);
		//UpdateShiftControlKeys();
		g_Undo.DropLast();
		return 0;
	}

	g_Undo.ChangeTrack(k, m_activeRow, UETYPE_TRACKDATA, 1);

	//copies source track act to k
	TrackCopyFromTo(act, k);

	m_song[line][cl] = k;

	return 1;
}


//--clipboard functions

void CSong::TrackCopy()
{
	TTrack* at = g_Tracks.GetTrack(SongGetActiveTrack()), * tot = &g_TrackClipboard.m_trackcopy;

	if (at && tot)
	{
		*tot = *at;
	}
}

void CSong::TrackPaste()
{
	TTrack* at = g_Tracks.GetTrack(SongGetActiveTrack()), * fro = &g_TrackClipboard.m_trackcopy;

	if (at && fro)
	{
		if (g_Tracks.IsValidLength(fro->len)) *at = *fro;
	}
}

void CSong::TrackDelete()
{
	g_Tracks.ClearTrack(SongGetActiveTrack());
}

void CSong::TrackCut()
{
	TrackCopy();
	TrackDelete();
}

void CSong::TrackCopyFromTo(int fromtrack, int totrack)
{
	TTrack* at = g_Tracks.GetTrack(fromtrack), * tot = g_Tracks.GetTrack(totrack);

	if (at && tot)
	{
		*tot = *at;
	}
}

void CSong::TrackSwapFromTo(int fromtrack, int totrack)
{
	TTrack buf;
	TTrack* at = g_Tracks.GetTrack(fromtrack), * tot = g_Tracks.GetTrack(totrack);

	if (at && tot)
	{
		buf = *tot;
		*tot = *at;
		*at = buf;
	}
}

void CSong::BlockPaste(int special)
{
	g_Undo.ChangeTrack(SongGetActiveTrack(), m_activeRow, UETYPE_TRACKDATA, 1);
	int lines = g_TrackClipboard.BlockPasteToTrack(SongGetActiveTrack(), m_activeRow, special);
	if (lines > 0)
	{
		int lastl = m_activeRow + lines - 1;
		//resets the beginning of the block to this location
		g_TrackClipboard.BlockDeselect();
		g_TrackClipboard.BlockSetBegin(m_activeChannel, SongGetActiveTrack(), m_activeRow);
		g_TrackClipboard.BlockSetEnd(lastl);
		//moves the current line to the last bottom row of the pasted block
		m_activeRow = lastl;
	}
}

void CSong::InstrCopy()
{
	int i = GetActiveInstr();
	memcpy(&m_instrclipboard, g_Instruments.GetInstrument(i), sizeof(TInstrument));
}

void CSong::InstrPaste(int special)
{
	if (m_instrclipboard.activeEditSection < 0) return;	//he has never been filled with anything

	int i = GetActiveInstr();

	g_Undo.ChangeInstrument(i, 0, UETYPE_INSTRDATA, 1);

	TInstrument* ai = g_Instruments.GetInstrument(i);

	//Atari_InstrumentTurnOff(i); //turns off this instrument on all channels

	int x, y;
	BOOL bl = 0, br = 0, ep = 0;
	BOOL bltor = 0, brtol = 0;

	switch (special)
	{
		case 0: //normal paste
			memcpy(ai, &m_instrclipboard, sizeof(TInstrument));
			ai->activeEditSection = ai->editNameCursorPos = 0; //so that the cursor is at the beginning of the instrument name
			break;

		case 1: //volume L/R
			bl = br = 1;
			goto InstrPaste_Envelopes;
		case 2: //volume R
			br = 1;
			goto InstrPaste_Envelopes;
		case 3: //volume L
			bl = 1;
			goto InstrPaste_Envelopes;
		case 4: //envelope parameters
			ep = 1;
		InstrPaste_Envelopes:
			for (x = 0; x <= m_instrclipboard.parameters[PAR_ENV_LENGTH]; x++)
			{
				if (br) ai->envelope[x][ENV_VOLUMER] = m_instrclipboard.envelope[x][ENV_VOLUMER];
				if (bl) ai->envelope[x][ENV_VOLUMEL] = m_instrclipboard.envelope[x][ENV_VOLUMEL];
				if (bltor) ai->envelope[x][ENV_VOLUMER] = m_instrclipboard.envelope[x][ENV_VOLUMEL];
				if (brtol) ai->envelope[x][ENV_VOLUMEL] = m_instrclipboard.envelope[x][ENV_VOLUMER];
				if (ep)
				{
					for (y = ENV_DISTORTION; y < ENVROWS; y++) ai->envelope[x][y] = m_instrclipboard.envelope[x][y];
				}
			}
			ai->parameters[PAR_ENV_LENGTH] = m_instrclipboard.parameters[PAR_ENV_LENGTH];
			ai->parameters[PAR_ENV_GOTO] = m_instrclipboard.parameters[PAR_ENV_GOTO];
			ai->editEnvelopeX = 0;
			break;

		case 5: //TABLE
			for (x = 0; x <= m_instrclipboard.parameters[PAR_TBL_LENGTH]; x++) ai->noteTable[x] = m_instrclipboard.noteTable[x];
			ai->parameters[PAR_TBL_LENGTH] = m_instrclipboard.parameters[PAR_TBL_LENGTH];
			ai->parameters[PAR_TBL_GOTO] = m_instrclipboard.parameters[PAR_TBL_GOTO];
			ai->editNoteTableCursorPos = 0;
			break;

		case 6: //vol+env
			br = bl = ep = 1;
			goto InstrPaste_Envelopes;
		case 8: //volume L to R
			bltor = 1;
			goto InstrPaste_Envelopes;
		case 9: //volume R to L
			brtol = 1;
			goto InstrPaste_Envelopes;

		case 7: //vol+env insert to cursor
			int sx = m_instrclipboard.parameters[PAR_ENV_LENGTH] + 1;
			if (ai->editEnvelopeX + sx > ENVELOPE_MAX_COLUMNS) sx = ENVELOPE_MAX_COLUMNS - ai->editEnvelopeX;
			for (x = ENVELOPE_MAX_COLUMNS - 2; x >= ai->editEnvelopeX; x--) //offset
			{
				int i = x + sx;
				if (i >= ENVELOPE_MAX_COLUMNS) continue;
				for (y = 0; y < ENVROWS; y++) ai->envelope[i][y] = ai->envelope[x][y];
			}
			for (x = 0; x < sx; x++) //insertion
			{
				int i = ai->editEnvelopeX + x;
				for (y = 0; y < ENVROWS; y++) ai->envelope[i][y] = m_instrclipboard.envelope[x][y];
			}
			int i = ai->parameters[PAR_ENV_LENGTH] + sx;
			if (i >= ENVELOPE_MAX_COLUMNS) i = ENVELOPE_MAX_COLUMNS - 1;
			ai->parameters[PAR_ENV_LENGTH] = i;
			if (ai->parameters[PAR_ENV_GOTO] > ai->editEnvelopeX)
			{
				i = ai->parameters[PAR_ENV_GOTO] + sx;
				if (i >= ENVELOPE_MAX_COLUMNS) i = ENVELOPE_MAX_COLUMNS - 1;
				ai->parameters[PAR_ENV_GOTO] = i;
			}
			i = ai->editEnvelopeX + sx;
			if (i >= ENVELOPE_MAX_COLUMNS) i = ENVELOPE_MAX_COLUMNS - 1;
			ai->editEnvelopeX = i;
			break;

	}
	g_Instruments.WasModified(i); //write to Atari RAM
}

void CSong::InstrCut()
{
	InstrCopy();
	InstrDelete();
}

void CSong::InstrDelete()
{
	g_Instruments.ClearInstrument(GetActiveInstr());
}

void CSong::InstrInfo(int instr, TInstrInfo* iinfo, int instrto)
{
	if (!g_Instruments.IsValidInstrument(instr)) return;

	TTrack* at;
	int i, j, ain;
	int inttrack;
	int intrack[TRACKSNUM];
	int noftrack = 0;
	int globallytimes = 0;
	int withnote[NOTESNUM];
	for (i = 0; i < NOTESNUM; i++) withnote[i] = 0;
	int minnote = NOTESNUM, maxnote = -1;
	int minvol = 16, maxvol = -1;
	int infrom = INSTRSNUM, into = -1;

	if (instrto < instr) instrto = instr;

	for (i = 0; i < TRACKSNUM; i++)
	{
		inttrack = 0;
		at = g_Tracks.GetTrack(i);
		ain = -1;
		for (j = 0; j < at->len; j++)
		{
			if (at->instr[j] >= 0) ain = at->instr[j];
			if (ain >= instr && ain <= instrto)
			{
				inttrack = 1;
				if (ain > into) into = ain;
				if (ain < infrom) infrom = ain;
				int note = at->note[j];
				if (note >= 0 && note < NOTESNUM)
				{
					globallytimes++; //some note with this instrument => started
					withnote[note]++;
					if (note > maxnote) maxnote = note;
					if (note < minnote) minnote = note;
				}
				int vol = at->volume[j];
				if (vol >= 0 && vol <= 15)
				{
					if (vol > maxvol) maxvol = vol;
					if (vol < minvol) minvol = vol;
				}
			}
		}
		intrack[i] = inttrack;
		if (inttrack) noftrack++;
	}

	if (iinfo)
	{	//iinfo != NULL => set values
		iinfo->count = globallytimes;
		iinfo->usedintracks = noftrack;
		iinfo->instrfrom = infrom;
		iinfo->instrto = into;
		iinfo->minnote = minnote;
		iinfo->maxnote = maxnote;
		iinfo->minvol = minvol;
		iinfo->maxvol = maxvol;
	}
	else
	{	//iinfo == NULL => shows dialog
		CString s, s2;
		s.Format("Instrument: %02X\nName: %s\nUsed in %i tracks, globally %i times.\nFrom note: %s\nTo note: %s\nMin volume: %X\nMax volume: %X",
			instr, g_Instruments.GetName(instr), noftrack, globallytimes,
			minnote < NOTESNUM ? notes[minnote] : "-",
			maxnote >= 0 ? notes[maxnote] : "-",
			minvol <= 15 ? minvol : 0,
			maxvol >= 0 ? maxvol : 0);

		if (globallytimes > 0)
		{
			s += "\n\nNote listing:\n";
			int lc = 0;
			for (i = 0; i < NOTESNUM; i++)
			{
				if (withnote[i])
				{
					s += notes[i];
					lc++;
					if (lc < 12)
						s += " ";
					else
					{
						s += "\n"; lc = 0;
					}
				}
			}
			s += "\n\nTrack listing:\n";
			lc = 0;
			for (i = 0; i < TRACKSNUM; i++)
			{
				if (intrack[i])
				{
					s2.Format("%02X", i);
					s += s2;
					lc++;
					if (lc < 16)
						s += " ";
					else
					{
						s += "\n"; lc = 0;
					}
				}
			}
		}
		MessageBox(g_hwnd, (LPCTSTR)s, "Instrument info", MB_ICONINFORMATION);
	}
}

void CSong::InstrChange(int instr)
{
	if (!g_Instruments.IsValidInstrument(instr)) return;

	CInstrumentChangeDlg dlg;

	CString s = "";

	TTrack* st;						// Pointer to original track
	TTrack* nt;						// Pointer to new track
	TTrack at;						// Temporary track

	BYTE tracks[TRACKSNUM];			// New tracks with changes 
	BYTE track_yn[TRACKSNUM];		// Tracks to apply changes

	int track_column[TRACKSNUM];	// The first occurrence in the selected area of the song
	int track_line[TRACKSNUM];		// The first occurrence in the selected area of the song
	int track_changeto[TRACKSNUM];	// Changed tracks to replace in song

	int onlysomething = 0;			// Only apply changes to specific things
	int trackcreated = 0;			// Number of newly created songs
	int songchanges = 0;			// Number of changes in the song

	int i, j, k, t, r, lasti, lastn, changes, note, ins, vol;

	bool error = 0;

	dlg.m_combo9 = dlg.m_combo11 = instr;
	dlg.m_combo10 = dlg.m_combo12 = instr;
	dlg.m_onlytrack = SongGetActiveTrack();
	dlg.m_onlysonglinefrom = dlg.m_onlysonglineto = SongGetActiveLine();

	// Change all the instrument occurences
	if (dlg.DoModal() == IDOK)
	{
		Stop();	// Stop playing before processing further

		// Hide all tracks and the whole song
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, -1);
		g_Undo.ChangeSong(0, 0, UETYPE_SONGDATA, 1);

		// Get the parameters from the dialog box
		int snotefrom = dlg.m_combo1;
		int snoteto = dlg.m_combo2;
		int svolmin = dlg.m_combo3;
		int svolmax = dlg.m_combo4;
		int sinstrfrom = dlg.m_combo11;
		int sinstrto = dlg.m_combo12;
		int dnotefrom = dlg.m_combo5;
		int dnoteto = dlg.m_combo6;
		int dvolmin = dlg.m_combo7;
		int dvolmax = dlg.m_combo8;
		int dinstrfrom = dlg.m_combo9;
		int dinstrto = dlg.m_combo10;
		int onlytrack = dlg.m_onlytrack;
		int onlychannels = dlg.m_onlychannels;
		int onlysonglinefrom = dlg.m_onlysonglinefrom;
		int onlysonglineto = dlg.m_onlysonglineto;

		// Initialise memory
		memset(track_yn, 0, TRACKSNUM);
		for (i = 0; i < TRACKSNUM; i++) track_column[i] = track_line[i] = -1;

		if (onlychannels >= 0 || (onlysonglinefrom >= 0 && onlysonglineto >= 0))
		{
			if (onlychannels <= 0) onlychannels = 0xff;				// All channels
			if (onlysonglinefrom < 0) onlysonglinefrom = 0;			// From the beginning
			if (onlysonglineto < 0) onlysonglineto = SONGLEN - 1;	// To the end
			onlysomething = 1;										// Something specific to change

			for (j = 0; j < SONGLEN; j++)
			{
				if (IsSongGo(j)) continue;

				for (i = 0; i < g_tracks4_8; i++)
				{
					t = m_song[j][i];

					if (!g_Tracks.IsValidTrack(t)) continue;

					r = (onlychannels & (1 << i)) && j >= onlysonglinefrom && j <= onlysonglineto;
					track_yn[t] |= (r) ? 1 : 2;	// 1 = yes, 2 = no, 3 = yesno (copy)

					// The first occurrence in the selected area of the song
					if (r && track_column[t] < 0)
					{
						track_column[t] = i;
						track_line[t] = j;
					}
				}
			}
		}

		else if (onlytrack >= 0)
		{
			track_yn[onlytrack] = 1;	// 1 = yes
			onlysomething = 1;
		}

		if (!g_Tracks.IsValidNote(dnoteto)) dnoteto = dnotefrom + (snoteto - snotefrom);
		if (!g_Tracks.IsValidVolume(dvolmax)) dvolmax = dvolmin + (svolmax - svolmin);
		if (!g_Tracks.IsValidInstrument(dinstrto)) dinstrto = dinstrfrom + (sinstrto - sinstrfrom);

		double notecoef = (snoteto - snotefrom > 0) ? (double)(dnoteto - dnotefrom) / (snoteto - snotefrom) : 0;
		double volcoef = (svolmax - svolmin > 0) ? (double)(dvolmax - dvolmin) / (svolmax - svolmin) : 0;
		double instrcoef = (sinstrto - sinstrfrom > 0) ? (double)(dinstrto - dinstrfrom) / (sinstrto - sinstrfrom) : 0;

		for (i = 0; i < TRACKSNUM; i++)
		{
			track_changeto[i] = -1; // initialise

			// It wants to change only some and this one is not
			if (onlysomething && ((track_yn[i] & 1) != 1)) continue;

			// Copy the original track to temporary track
			st = g_Tracks.GetTrack(i);
			at = *st;

			changes = 0;
			lasti = lastn = -1;

			for (j = 0; j < at.len; j++)
			{
				if (g_Tracks.IsValidInstrument(at.instr[j])) lasti = at.instr[j];
				if (g_Tracks.IsValidNote(at.note[j])) lastn = at.note[j];

				if (lasti >= sinstrfrom && lasti <= sinstrto && lastn >= snotefrom && lastn <= snoteto && at.volume[j] >= svolmin && at.volume[j] <= svolmax)
				{
					if (g_Tracks.IsValidNote(at.note[j]))
					{
						note = dnotefrom + (int)((double)(at.note[j] - snotefrom) * notecoef + 0.5);
						while (!g_Tracks.IsValidNote(note)) note -= 12;
						if (note != at.note[j])
						{
							at.note[j] = note;
							changes = 1;
						}
					}

					if (g_Tracks.IsValidInstrument(at.instr[j]))
					{
						ins = dinstrfrom + (int)((double)(at.instr[j] - sinstrfrom) * instrcoef + 0.5);
						if (!g_Tracks.IsValidInstrument(ins)) ins = INSTRSNUM - 1;
						if (ins != at.instr[j])
						{
							at.instr[j] = ins;
							changes = 1;
						}
					}

					if (g_Tracks.IsValidVolume(at.volume[j]))
					{
						vol = dvolmin + (int)((double)(at.volume[j] - svolmin) * volcoef + 0.5);
						if (!g_Tracks.IsValidVolume(vol)) vol = MAXVOLUME;
						if (vol != at.volume[j])
						{
							at.volume[j] = vol;
							changes = 1;
						}
					}
				}
			}

			// There was something changed
			if (changes)
			{
				// Create a new track if the track occurs both inside and outside the area
				if (track_yn[i] & 2)
				{
					memset(tracks, 0, TRACKSNUM);	// Initialise memory
					MarkTF_USED(tracks);
					MarkTF_NOEMPTY(tracks);
					k = FindNearTrackBySongLineAndColumn(track_line[i], track_column[i], tracks);

					// The process is aborted if there is no unused track available
					if (k < 0)
					{
						error = 1;
						s.AppendFormat("There aren't any more empty unused tracks in song, further changes could not be applied!\n\n");
						s.AppendFormat("Process halted in Track %02X, in Channel %u\n\n", track_line[i], track_column[i]);
						goto abortchanges;
					}

					// Copy the changed track (at) to the new track (nt)
					nt = g_Tracks.GetTrack(k);
					*nt = at;

					trackcreated++;

					// Put it in the song at least once (due to the search in the song used tracks)
					m_song[track_line[i]][track_column[i]] = k;
					songchanges++;

					// Will change all occurrences
					track_changeto[i] = k;
				}

				// Copy the changed track (at) back to the original track (st)
				else
				{
					*st = at;
				}
			}

		}

		// Subsequent changes in the song
		if (onlysomething)
		{
			for (j = 0; j < SONGLEN; j++)
			{
				if (IsSongGo(j)) continue;

				for (i = 0; i < g_tracks4_8; i++)
				{
					t = m_song[j][i];

					if (!g_Tracks.IsValidTrack(t)) continue;

					r = (onlychannels & (1 << i)) && j >= onlysonglinefrom && j <= onlysonglineto;

					if (r && track_changeto[t] >= 0)
					{
						m_song[j][i] = track_changeto[t];
						songchanges++;
					}
				}
			}
		}

	abortchanges:
		s.AppendFormat("Instrument changes were applied ");
		s.AppendFormat(error ? "with errors, beware of data loss!\n\n" : "successfully!\n\n");

		if (trackcreated || songchanges)
		{
			s.AppendFormat("Additional actions were also performed to accommodate the chosen parameters:\n\n");
			s.AppendFormat("New tracks created: %u\n", trackcreated);
			s.AppendFormat("Total changes in song: %u\n", songchanges);
		}

		MessageBox(g_hwnd, s, "Instrument changes", MB_ICONINFORMATION);
	}
}

void CSong::TrackInfo(int track)
{
	if (track < 0 || track >= TRACKSNUM) return;

	const char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };

	int i, ch;
	int trackusedincolumn[SONGTRACKS];
	int lines = 0, total = 0;

	for (ch = 0; ch < SONGTRACKS; ch++) trackusedincolumn[ch] = 0;

	for (int sline = 0; sline < SONGLEN; sline++)
	{
		if (m_songgo[sline] >= 0) continue;	//goto line is ignored

		BOOL thisline = 0;
		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];
			if (n == track) { trackusedincolumn[ch]++; total++; thisline = 1; }
		}

		if (thisline) lines++;
	}

	CString s, s2;
	s.Format("Track: %02X\nUsing in song:\n", track);
	for (ch = 0; ch < g_tracks4_8; ch++)
	{
		i = trackusedincolumn[ch];
		s2.Format("%s: %i   ", cnames[ch], i);
		s += s2;
	}

	s2.Format("\nUsed in %i songlines, globally %i times.", lines, total);
	s += s2;

	MessageBox(g_hwnd, (LPCTSTR)s, "Track Info", MB_ICONINFORMATION);
}

void CSong::SongCopyLine()
{
	for (int i = 0; i < g_tracks4_8; i++) m_songlineclipboard[i] = m_song[m_activeSongline][i];
	m_songgoclipboard = m_songgo[m_activeSongline];
}

void CSong::SongPasteLine()
{
	if (m_songgoclipboard < -1) return;
	g_Undo.ChangeSong(m_activeSongline, m_activeChannel, UETYPE_SONGDATA);
	for (int i = 0; i < g_tracks4_8; i++) m_song[m_activeSongline][i] = m_songlineclipboard[i];
	m_songgo[m_activeSongline] = m_songgoclipboard;
}

void CSong::SongClearLine()
{
	g_Undo.ChangeSong(m_activeSongline, m_activeChannel, UETYPE_SONGDATA);
	for (int i = 0; i < g_tracks4_8; i++) m_song[m_activeSongline][i] = -1;
	m_songgo[m_activeSongline] = -1;
}

void CSong::TracksOrderChange()
{
	Stop();	//stop the sound first
	CSongTracksOrderDlg dlg;
	dlg.m_songlinefrom.Format("%02X", m_TracksOrderChange_songlinefrom);
	dlg.m_songlineto.Format("%02X", m_TracksOrderChange_songlineto);
	if (dlg.DoModal() == IDOK)
	{
		g_Undo.ChangeSong(m_activeSongline, m_activeChannel, UETYPE_SONGDATA, 1);

		int m_buff[8];
		int i, j;

		int f = Hexstr((char*)(LPCTSTR)dlg.m_songlinefrom, 2);
		int t = Hexstr((char*)(LPCTSTR)dlg.m_songlineto, 2);

		if (f < 0 || f >= SONGLEN || t < 0 || t >= SONGLEN || t < f)
		{
			MessageBox(g_hwnd, "Bad songline (from-to) range.", "Error", MB_ICONERROR);
			return;
		}

		m_TracksOrderChange_songlinefrom = f;
		m_TracksOrderChange_songlineto = t;

		int c = 0;
		for (i = 0; i < g_tracks4_8; i++) if (dlg.m_tracksorder[i] < 0) c++;
		if (c > 0)
		{
			CString s;
			s.Format("Warning: %u song column(s) will be cleared completely.\nAre you sure to do it?", c);
			if (MessageBox(g_hwnd, s, "Warning", MB_YESNOCANCEL | MB_ICONWARNING) != IDYES) return;
		}

		for (i = m_TracksOrderChange_songlinefrom; i <= m_TracksOrderChange_songlineto; i++)
		{
			for (j = 0; j < g_tracks4_8; j++)
			{
				m_buff[j] = m_song[i][j];
				m_song[i][j] = -1;
			}
			for (j = 0; j < g_tracks4_8; j++)
			{
				int z = dlg.m_tracksorder[j];
				if (z >= 0)
					m_song[i][j] = m_buff[z];
				else
					m_song[i][j] = -1;
			}
		}
	}
}

void CSong::Songswitch4_8(int tracks4_8)
{
	Stop();	//stop the sound first

	CString wrn = "Warning: Undo operation won't be possible!!!\n";
	int i, j;
	if (tracks4_8 == 4)
	{
		int p = 0;
		for (i = 0; i < SONGLEN; i++)
		{
			for (j = 4; j < 8; j++) if (m_song[i][j] >= 0) p++;
		}

		if (p > 0) wrn += "\nWarning: Song switch to mono 4 tracks will erase all the R1,R2,R3,R4 entries in song list.\n";
	}

	wrn += "\nAre you sure to do it?";
	int res = MessageBox(g_hwnd, wrn, "Song switch mono/stereo", MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (res != IDYES) return;

	g_Undo.Clear();

	if (tracks4_8 == 4)
	{
		if (m_activeChannel >= 4) { m_activeChannel = 3; m_activeCursor = 0; }
		g_tracks4_8 = 4;
		for (i = 0; i < SONGLEN; i++)
		{
			for (j = 4; j < 8; j++) m_song[i][j] = -1;
		}
	}
	else
		if (tracks4_8 == 8) g_tracks4_8 = 8;

	Atari_InitRMTRoutine();
}

int CSong::GetEffectiveMaxtracklen()
{
	//calculate the largest track length used
	int so, i, max = 1;
	for (so = 0; so < SONGLEN; so++)
	{
		if (m_songgo[so] >= 0) continue; //go to line is ignored
		int min = g_Tracks.GetMaxTrackLength();
		int p = 0;
		for (i = 0; i < g_tracks4_8; i++)
		{
			int t = m_song[so][i];
			int m = g_Tracks.GetLength(t);
			if (m < 0) continue;
			p++;
			if (m < min) min = m;
		}
		//min = the shortest track length on this songline
		if (p > 0 && min > max) max = min;
	}
	return max;
}

int CSong::GetSmallestMaxtracklen(int songline)
{
	//calculate the smallest track length used in this songline
	int so = songline;
	int max = 256;
	int min = g_Tracks.GetMaxTrackLength();
	int p = 0;

	if (m_songgo[so] >= 0)	return 0; //go to line is ignored

	for (int i = 0; i < g_tracks4_8; i++)
	{
		int t = m_song[so][i];
		int m = g_Tracks.GetLength(t);
		if (m < 0) continue;
		if (m < max) max = m;
		p++;
	}
	if (!p) return min;	//return 0;	//cannot be from empty tracks

	//min = the shortest track length on this songline
	if (p > 0 && min < max) max = min;

	return max;
}


void CSong::ChangeMaxtracklen(int maxtracklen)
{
	if (!g_Tracks.IsValidLength(maxtracklen)) return;

	int i, j;
	TTrack* tt;

	for (i = 0; i < TRACKSNUM; i++)
	{
		tt = g_Tracks.GetTrack(i);
		//clear
		for (j = tt->len; j < TRACKLEN; j++)
		{
			tt->note[j] = tt->instr[j] = tt->volume[j] = tt->speed[j] = -1;
		}
		//
		if (tt->len >= maxtracklen)
		{
			tt->go = -1; //cancel GO
			tt->len = maxtracklen; //adjust length
		}
	}

	g_Tracks.SetMaxTrackLength(maxtracklen);
}

void CSong::TracksAllBuildLoops(int& tracksmodified, int& beatsreduced)
{
	Stop(); //stop the sound first

	int i;
	int p = 0, u = 0;
	for (i = 0; i < TRACKSNUM; i++)
	{
		int r = g_Tracks.TrackBuildLoop(i);
		if (r > 0) { p++; u += r; }
	}
	tracksmodified = p;
	beatsreduced = u;
}

void CSong::TracksAllExpandLoops(int& tracksmodified, int& loopsexpanded)
{
	Stop(); //stop the sound first

	int i;
	int p = 0, u = 0;
	for (i = 0; i < TRACKSNUM; i++)
	{
		int r = g_Tracks.TrackExpandLoop(i);
		if (r > 0) { p++; u += r; }
	}
	tracksmodified = p;
	loopsexpanded = u;
}

void CSong::SongClearUnusedTracksAndParts(int& clearedtracks, int& truncatedtracks, int& truncatedbeats)
{
	int i, j, ch;
	int ttracks = 0, tbeats = 0, ctracks = 0;
	int tracklen[TRACKSNUM];
	BOOL trackused[TRACKSNUM];
	TTrack* tr;

	// Initialise
	for (i = 0; i < TRACKSNUM; i++)
	{
		tracklen[i] = -1;
		trackused[i] = 0;
	}

	for (int sline = 0; sline < SONGLEN; sline++)
	{
		if (IsSongGo(sline)) continue;	// Goto line is ignored

		int nejkratsi = g_Tracks.GetMaxTrackLength();

		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];

			if (!g_Tracks.IsValidTrack(n)) continue;	// Invalid track is ignored

			trackused[n] = 1;
			tr = g_Tracks.GetTrack(n);

			if (g_Tracks.IsValidGo(tr->go)) continue;	// There is a loop => it has a maximum length

			if (tr->len < nejkratsi) nejkratsi = tr->len;
		}

		// "nejkratsi" is the shortest track in this song line
		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];

			if (!g_Tracks.IsValidTrack(n)) continue;	// Invalid track is ignored

			if (tracklen[n] < nejkratsi) tracklen[n] = nejkratsi; // If it needs a longer size, it will expand to the length it needs
		}
	}

	// And now it cuts those tracks
	for (i = 0; i < TRACKSNUM; i++)
	{
		int nlen = tracklen[i];

		if (nlen < 1) continue;	// If they don't have the length of at least 1 they are skipped

		tr = g_Tracks.GetTrack(i);

		// There is no loop
		if (!g_Tracks.IsValidGo(tr->go))
		{
			if (nlen < tr->len)
			{
				// For what must cut, is there anything at all?
				for (j = nlen; j < tr->len; j++)
				{
					if (g_Tracks.IsValidNote(tr->note[j]) || g_Tracks.IsValidInstrument(tr->instr[j]) || g_Tracks.IsValidVolume(tr->volume[j]) || g_Tracks.IsValidSpeed(tr->speed[j]))
					{
						// Yeah, there's something, so cut it
						ttracks++;
						tbeats += tr->len - nlen;
						tr->len = nlen; // Cut what is not needed
						break;
					}
				}
				// There is no break;
			}
		}
		// There is a loop
		else
		{
			// The beginning of the loop is further than the required track length
			if (tr->len >= nlen)
			{
				ttracks++;
				tbeats += tr->len - nlen;
				tr->len = nlen;	// Cut the track
				tr->go = -1;	// Disable loop
			}
		}
	}

	// Delete empty tracks not used in the song
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (!trackused[i] && !g_Tracks.IsEmptyTrack(i))
		{
			g_Tracks.ClearTrack(i);
			ctracks++;
		}
	}

	clearedtracks = ctracks;
	truncatedtracks = ttracks;
	truncatedbeats = tbeats;
}

int CSong::SongClearDuplicatedTracks()
{
	int i, j, ch;
	int trackto[TRACKSNUM];

	for (i = 0; i < TRACKSNUM; i++) trackto[i] = -1;

	int clearedtracks = 0;
	for (i = 0; i < TRACKSNUM - 1; i++)
	{
		if (g_Tracks.IsEmptyTrack(i)) continue;	//does not compare empty
		for (j = i + 1; j < TRACKSNUM; j++)
		{
			if (g_Tracks.IsEmptyTrack(j)) continue;
			if (g_Tracks.CompareTracks(i, j))
			{
				g_Tracks.ClearTrack(j);	//j is the same as i, so j is deleted.
				trackto[j] = i;			//these tracks have to be replaced by tracks i
				clearedtracks++;
			}
		}
	}

	//analyse the song and make changes to the deleted tracks
	for (int sline = 0; sline < SONGLEN; sline++)
	{
		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];
			if (n < 0 || n >= TRACKSNUM) continue;	//--
			if (trackto[n] >= 0) m_song[sline][ch] = trackto[n];
		}
	}

	return clearedtracks;
}

int CSong::SongClearUnusedTracks()
{
	int i, ch;
	BOOL trackused[TRACKSNUM];

	for (i = 0; i < TRACKSNUM; i++) trackused[i] = 0;

	for (int sline = 0; sline < SONGLEN; sline++)
	{
		if (m_songgo[sline] >= 0) continue;	//goto line is ignored

		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];
			if (n < 0 || n >= TRACKSNUM) continue;	//--
			trackused[n] = 1;
		}
	}

	//delete all tracks unused in the song
	int clearedtracks = 0;
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (!trackused[i])
		{
			if (!g_Tracks.IsEmptyTrack(i)) clearedtracks++;
			g_Tracks.ClearTrack(i);
		}
	}

	return clearedtracks;
}

void CSong::RenumberAllTracks(int type) //1..after columns, 2..after lines
{
	int i, j, sline;
	int movetrackfrom[TRACKSNUM], movetrackto[TRACKSNUM];

	for (i = 0; i < TRACKSNUM; i++) movetrackfrom[i] = movetrackto[i] = -1;

	int order = 0;

	//test the song
	if (type == 2)
	{
		//horizontally along the lines
		for (sline = 0; sline < SONGLEN; sline++)
		{
			if (m_songgo[sline] >= 0) continue;	//goto line is ignored
			for (i = 0; i < g_tracks4_8; i++)
			{
				int n = m_song[sline][i];
				if (n < 0 || n >= TRACKSNUM) continue;	//--
				if (movetrackfrom[n] < 0)
				{
					movetrackfrom[n] = order;
					movetrackto[order] = n;
					order++;
				}
			}
		}
	}
	else
		if (type == 1)
		{
			//vertically in columns
			for (i = 0; i < g_tracks4_8; i++)
			{
				for (sline = 0; sline < SONGLEN; sline++)
				{
					if (m_songgo[sline] >= 0) continue;	//goto line is ignored
					int n = m_song[sline][i];
					if (n < 0 || n >= TRACKSNUM) continue;	//--
					if (movetrackfrom[n] < 0)
					{
						movetrackfrom[n] = order;
						movetrackto[order] = n;
						order++;
					}
				}
			}
		}
		else
			return;	//unknown type

		//then add empty tracks not used in the song
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (movetrackfrom[i] < 0 && !g_Tracks.IsEmptyTrack(i))
		{
			movetrackfrom[i] = order;
			movetrackto[order] = i;
			order++;
		}
	}

	//precisely numbered in the song
	for (sline = 0; sline < SONGLEN; sline++)
	{
		//if (m_songgo[sline]>=0) continue;	//goto line is not omitted here (the numbers mentioned below it will also change)
		for (i = 0; i < g_tracks4_8; i++)
		{
			int n = m_song[sline][i];
			if (n < 0 || n >= TRACKSNUM) continue;	//--
			m_song[sline][i] = movetrackfrom[n];
		}
	}

	// physical data transfer in tracks
	for (i = 0; i < order; i++)
	{
		int n = movetrackto[i];	// swap i <--> n
		if (n == i) continue;	// they are the same, so they don't have to shuffle anything

		TrackSwapFromTo(i, n);

		for (j = i; j < order; j++)
		{
			if (movetrackto[j] == i) movetrackto[j] = n;
		}
	}
}

int CSong::ClearAllInstrumentsUnusedInAnyTrack()
{
	//go through all existing tracks and find unused instruments

	int i, j, t;
	BOOL instrused[INSTRSNUM];
	TTrack* tr;

	for (i = 0; i < INSTRSNUM; i++) instrused[i] = 0;
	for (i = 0; i < TRACKSNUM; i++)
	{
		tr = g_Tracks.GetTrack(i);
		int nlen = tr->len;
		for (j = 0; j < nlen; j++)
		{
			t = tr->instr[j];
			if (t >= 0 && t < INSTRSNUM) instrused[t] = 1;	//instrument "t" is used
		}
	}

	//delete unused instruments here
	int clearedinstruments = 0;
	for (i = 0; i < INSTRSNUM; i++)
	{
		if (!instrused[i])
		{
			//unused
			if (g_Instruments.CalculateNotEmpty(i)) clearedinstruments++;	//is it empty? yes => it will be deleted
			g_Instruments.ClearInstrument(i);
		}
	}

	return clearedinstruments;
}

void CSong::RenumberAllInstruments(int type)
{
	//type=1...remove gaps, 2=order by using in tracks, type=3...order by instrument names

	int i, j, k, ins;
	int moveinstrfrom[INSTRSNUM], moveinstrto[INSTRSNUM];
	TTrack* tr;

	for (i = 0; i < INSTRSNUM; i++) moveinstrfrom[i] = moveinstrto[i] = -1;

	int order = 0;

	//analyse all tracks
	for (i = 0; i < TRACKSNUM; i++)
	{
		tr = g_Tracks.GetTrack(i);
		int tlen = tr->len;
		for (j = 0; j < tlen; j++)
		{
			ins = tr->instr[j];
			if (ins < 0 || ins >= INSTRSNUM) continue;
			if (moveinstrfrom[ins] < 0)
			{
				moveinstrfrom[ins] = order;
				moveinstrto[order] = ins;
				order++;
			}
		}
	}

	//and now it adds even those that are not used in any track
	for (i = 0; i < INSTRSNUM; i++)
	{
		if (moveinstrfrom[i] < 0 && g_Instruments.CalculateNotEmpty(i))
		{
			moveinstrfrom[i] = order;
			moveinstrto[order] = i;
			order++;
		}
	}

	TInstrument bufi;
	if (type == 1)
	{
		//remove gaps
		int di = 0;
		for (i = 0; i < INSTRSNUM; i++)
		{
			if (moveinstrfrom[i] >= 0) //this instrument is used somewhere or is empty
			{
				//move to
				if (i != di)
				{
					memcpy(g_Instruments.GetInstrument(di), g_Instruments.GetInstrument(i), sizeof(TInstrument));
					//and delete instrument i
					g_Instruments.ClearInstrument(i);
				}
				//change table accordingly
				moveinstrfrom[i] = di;
				moveinstrto[di] = i;
				di++;
			}
		}
	}
	else
		if (type == 2)
		{
			//order by using in tracks
			//moveinstrfrom [instr] and moveinstrto [order] have it ready, so it can physically switch straight away
			for (i = 0; i < order; i++)
			{
				int n = moveinstrto[i];	//swap i <--> n
				if (n == i) continue;
				memcpy(&bufi, g_Instruments.GetInstrument(i), sizeof(TInstrument)); // i -> buffer
				memcpy(g_Instruments.GetInstrument(i), g_Instruments.GetInstrument(n), sizeof(TInstrument)); // n -> i
				memcpy(g_Instruments.GetInstrument(n), &bufi, sizeof(TInstrument)); // buffer -> n
				//
				for (j = i; j < order; j++)
				{
					if (moveinstrto[j] == i) moveinstrto[j] = n;
				}
			}
			//and now delete the others (due to the corresponding names of unused empty instruments)
			for (i = order; i < INSTRSNUM; i++) g_Instruments.ClearInstrument(i);
		}
		else
			if (type == 3)
			{
				//order by instrument name
				BOOL iused[INSTRSNUM];
				for (i = 0; i < INSTRSNUM; i++)
				{
					iused[i] = (moveinstrfrom[i] >= 0);
					moveinstrfrom[i] = i;	//the default is to keep the same order
				}
				//and now bubblesort arrange those that are iused [i]
				for (i = INSTRSNUM - 1; i > 0; i--)
				{
					for (j = 0; j < i; j++)
					{
						k = j + 1;
						//compare instrument j and k and either let or swap
						BOOL swap = 0;

						if (iused[j] != iused[k])
						{
							//one is used and one is unused
							if (iused[k]) swap = 1; //the second is used (=> the first is the one used), so swap
						}
						else
						{
							//both are used or both are not used
							char* name1 = g_Instruments.GetName(j);
							char* name2 = g_Instruments.GetName(k);
							if (_strcmpi(name1, name2) > 0) swap = 1; //they are the other way around, so they are swapped
						}

						if (swap)
						{
							//swap j and k
							memcpy(&bufi, g_Instruments.GetInstrument(j), sizeof(TInstrument)); // j -> buffer
							memcpy(g_Instruments.GetInstrument(j), g_Instruments.GetInstrument(k), sizeof(TInstrument)); // k -> j
							memcpy(g_Instruments.GetInstrument(k), &bufi, sizeof(TInstrument)); // buffer -> k
							//adjust table accordingly
							int p;
							for (p = 0; p < INSTRSNUM; p++)
							{
								if (moveinstrfrom[p] == k) moveinstrfrom[p] = j;
								else
									if (moveinstrfrom[p] == j) moveinstrfrom[p] = k;
							}

							BOOL b = iused[j];
							iused[j] = iused[k];
							iused[k] = b;
						}
					}
				}
				//still used unused empty instruments (due to their shift, so the name of their number 20: Instrument 21 did not match)
				for (i = 0; i < INSTRSNUM; i++)
				{
					if (!iused[i]) g_Instruments.ClearInstrument(i);
				}
			}
			else
				return;


	//and now it has to be renumbered in all tracks according to the moveinstrfrom [instr] table
	for (i = 0; i < TRACKSNUM; i++)
	{
		tr = g_Tracks.GetTrack(i);
		int tlen = tr->len;
		for (j = 0; j < tlen; j++)
		{
			ins = tr->instr[j];
			if (ins < 0 || ins >= INSTRSNUM) continue;
			tr->instr[j] = moveinstrfrom[ins];
		}
	}

	//and finally write all the instruments in Atari memory
	for (i = 0; i < INSTRSNUM; i++) g_Instruments.WasModified(i); //writes to Atari

	//Hooray, done
}



//
//--------------------------------------------------------------------------------------
//


/*
// Legacy Function
BOOL CSong::SetBookmark()
{
	if (m_activeSongline >= 0 && m_activeSongline < SONGLEN
		&& m_activeRow >= 0 && m_activeRow < g_Tracks.GetMaxTrackLength()
		&& m_speed >= 0)
	{
		m_bookmark.songline = m_activeSongline;
		m_bookmark.trackline = m_activeRow;
		m_bookmark.speed = m_speed;
		return 1;
	}
	return 0;
}

// Legacy Function
BOOL CSong::Play(int mode, BOOL follow, int special)
{
	g_Undo.Separator();

	if (mode == MPLAY_BOOKMARK && !IsBookmark()) return 0; //if there is no bookmark, then nothing.

	if (m_playMode)
	{
		if (mode != MPLAY_FROM) Stop(); //already playing and wants something other than play from edited pos.
		else if (!m_isFollowPlay) Stop(); //is playing and wants to play from edited pos. but not followplay
	}

	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;

	switch (mode)
	{
		case MPLAY_SONG: //whole song from the beginning including initialization (due to portamentum etc.)
			Atari_InitRMTRoutine();
			m_playSongline = 0;
			m_playRow = 0;
			m_speed = m_mainSpeed;
			break;
		case MPLAY_FROM: //song from the current position
			if (m_playMode && m_isFollowPlay) //is playing with follow play
			{
				m_playMode = MPLAY_FROM;
				m_isFollowPlay = follow;
				return 1;
			}
			m_playSongline = m_activeSongline;
			m_playRow = m_activeRow;
			break;
		case MPLAY_TRACK: //just the current tracks around
		Play3:
			m_playSongline = m_activeSongline;
			m_playRow = (special == 0) ? 0 : m_activeRow;
			break;
		case MPLAY_BLOCK: //only in the block
			if (!g_TrackClipboard.IsBlockSelected())
			{	//no block is selected, so the track plays
				mode = MPLAY_TRACK;
				goto Play3;
			}
			else
			{
				int bfro, bto;
				g_TrackClipboard.GetFromTo(bfro, bto);
				m_playSongline = g_TrackClipboard.m_selsongline;
				m_playRow = m_trackplayblockstart = bfro;
				m_trackplayblockend = bto;
			}
			break;
		case MPLAY_BOOKMARK: //from the bookmark
			m_playSongline = m_bookmark.songline;
			m_playRow = m_bookmark.trackline;
			//m_speed = m_bookmark.speed; //comment out so bookmark keep the same speed in memory, won't force it to reset it each time
			break;

		case MPLAY_SEEK_NEXT: //from seeking next
			m_activeSongline++;
			if (m_activeSongline > 255) m_activeSongline = 255;
			m_playSongline = m_activeSongline;
			m_playRow = m_activeRow = 0;
			if (mode == MPLAY_SEEK_NEXT) mode = MPLAY_FROM;
			break;

		case MPLAY_SEEK_PREV: //from seeking prev
			m_activeSongline--;
			if (m_activeSongline < 0) m_activeSongline = 0;
			m_playSongline = m_activeSongline;
			m_playRow = m_activeRow = 0;
			if (mode == MPLAY_SEEK_PREV) mode = MPLAY_FROM;
			break;

	}

	WaitForTimerRoutineProcessed();

	m_isFollowPlay = follow;
	m_playMode = mode;
	m_speeda = 0;	// Set for first Row to begin immediately

	// Legacy RMT procedure, which is required for the original format
	// TODO: Not do a workaround like this, for obvious reasons
	if (!g_isRMTE)
	{
		if (m_songgo[m_playSongline] >= 0)	//there is a goto
		{
			m_playSongline = m_songgo[m_playSongline];	//goto where
			m_playRow = 0;							//from the beginning of that track
			if (m_songgo[m_playSongline] >= 0)
			{
				//goto into another goto
				MessageBox(g_hwnd, "There is recursive \"Go to line\" to other \"Go to line\" in song.", "Recursive \"Go to line\"...", MB_ICONSTOP);
				return 0;
			}
		}

		PlayBeat();	//sets m_speeda
		m_speeda++;	//(Original comment by Raster, April 27, 2003) adds 1 to m_speed, for what the real thing will take place in Init
	}

	// Cursor following the player
	if (m_isFollowPlay)
	{
		m_activeRow = m_playRow;
		m_activeSongline = m_playSongline;
	}

	g_PokeyStream.CallFromPlay(m_playMode, m_playRow, m_playSongline);
	return 1;
}

// Legacy Function
BOOL CSong::SongPlayNextLine()
{
	m_playRow = 0;	//first track pattern line 

	// Normal play, play from current position, or play from bookmark => shift to the next line  
	if (m_playMode == MPLAY_SONG || m_playMode == MPLAY_FROM || m_playMode == MPLAY_BOOKMARK)
	{
		m_playSongline++;		// Increment the song line by 1
		if (m_playSongline > 255)
			m_playSongline = 0;	// Above 255, roll over to 0
	}

	// When a goto line is encountered, the player will jump right to the defined line and continue playback from that position
	if (m_songgo[m_playSongline] >= 0)				// If a goto line is set here...
		m_playSongline = m_songgo[m_playSongline];	// goto line xy

	if (g_PokeyStream.TrackSongLine(m_playSongline) == true)
	{
		// Song is done, so stop the play back
		m_playMode = MPLAY_STOP;					// Stop the player
	}
	return 1;
}

// Legacy Function
BOOL CSong::PlayBeat()
{
	int t, tt, xline, len, go, speed;
	int note[SONGTRACKS], instr[SONGTRACKS], vol[SONGTRACKS];
	TTrack* tr;

	for (t = 0; t < g_tracks4_8; t++)		//done here to make it behave the same as in the routine
	{
		note[t] = -1;
		instr[t] = -1;
		vol[t] = -1;
	}

TrackLine:
	speed = m_speed;

	for (t = 0; t < g_tracks4_8; t++)
	{
		tt = SongGetTrack(m_playSongline, t);
		tr = g_Tracks.GetTrack(tt);
		if (!tr) continue;	// Invalid track pointer
		len = tr->len;
		go = tr->go;
		if (m_playRow >= len)
		{
			if (go >= 0)
				xline = ((m_playRow - len) % (len - go)) + go;
			else
			{
				//if it is the end of the track, but it is a block play or the first PlayBeat call (when m_playMode = 0)
				if (m_playMode == MPLAY_BLOCK || m_playMode == MPLAY_STOP) { note[t] = -1; instr[t] = -1; vol[t] = -1; continue; }
				//otherwise a normal predecision to the next line in the song
				SongPlayNextLine();
				goto TrackLine;
			}
		}
		else
			xline = m_playRow;

		if (tr->note[xline] >= 0)	note[t] = tr->note[xline];
		instr[t] = tr->instr[xline];	//due to the same behavior as in the routine
		if (tr->volume[xline] >= 0) vol[t] = tr->volume[xline];
		if (tr->speed[xline] > 0) speed = tr->speed[xline];
	}

	//only now is the changed speed set
	m_speeda = m_speed = speed;

	//active note, instrument and volume settings
	for (t = 0; t < g_tracks4_8; t++)
	{
		int n = note[t];
		int i = instr[t];
		int v = vol[t];
		if (v >= 0 && v < 16)
		{
			if (n >= 0 && n < NOTESNUM) //&& i>=0 && i<INSTRSNUM//)		//adjustment for routine compatibility
			{
				if (i < 0 || i >= INSTRSNUM) i = 255;						//adjustment for routine compatibility
				Atari_SetTrack_NoteInstrVolume(t, n, i, v);
			}
			else
			{
				Atari_SetTrack_Volume(t, v);
			}
		}
	}

	if (m_playMode == MPLAY_BLOCK)
	{	
		if (g_PokeyStream.CallFromPlayBeat(m_playRow) == true)
		{
			// Song is done, so stop the play back
			m_playMode = MPLAY_STOP;					// Stop the player
		}
	}

	return 1;
}

// Legacy Function
BOOL CSong::PlayVBI()
{
	if (!m_playMode) return 0;	//not playing

	m_speeda--;
	if (m_speeda > 0) return 0;	//too soon to update

	m_playRow++;

	//m_playMode mode 4 => only plays range in block
	if (m_playMode == MPLAY_BLOCK && m_playRow > m_trackplayblockend) m_playRow = m_trackplayblockstart;

	// If none of the tracks end with "end", then it will end when reaching m_maxtracklen
	if (m_playRow >= g_Tracks.GetMaxTrackLength())
		SongPlayNextLine();

	PlayBeat();	//1 pattern track line play

	if (m_speeda == m_speed && m_isFollowPlay)	//playing and following the player
	{
		m_activeRow = m_playRow;
		m_activeSongline = m_playSongline;

		//Quantization
		if (m_quantization_note >= 0 && m_quantization_note < NOTESNUM
			&& m_quantization_instr >= 0 && m_quantization_instr < INSTRSNUM
			)
		{
			int vol = m_quantization_vol;
			if (g_respectvolume)
			{
				int v = TrackGetVol();
				if (v >= 0 && v <= MAXVOLUME) vol = v;
			}

			if (TrackSetNoteInstrVol(m_quantization_note, m_quantization_instr, vol))
			{
				SetPlayPressedTonesTNIV(m_trackactivecol, m_quantization_note, m_quantization_instr, vol);
			}
		}
		else
			if (m_quantization_note == -2) //Special case (midi NoteOFF)
			{
				TrackSetNoteActualInstrVol(-1);
				TrackSetVol(0);
			}
		m_quantization_note = -1; //cancel the quantized note
		//end of Q
	}

	return 1;
}
*/

/// <summary>
/// Call this X times per second to handle the playing of the song
/// </summary>
void CSong::TimerRoutine()
{
	// If the POKEY Stream is being recorded, the Timer Routine is bypassed entirely to run as fast as possible
	if (g_PokeyStream.IsRecording())
		return;

	// Things that are solved 1x for vbi
	PlayPattern(g_Module.GetSubtuneIndex());

	PlayContinue(g_Module.GetSubtuneIndex());

	g_Pokey.RenderSound_No6502(m_instrumentSpeed);

	// If the Song is currently playing, increment the timer
	UpdatePlayTime();
}

/// <summary>
/// Calculate the Time displayed on screen during playback
/// </summary>
void CSong::CalculatePlayTime()
{
	m_playTimeSecondCount = m_playTimeFrameCount / FRAMERATE % 60;
	m_playTimeMinuteCount = m_playTimeFrameCount / FRAMERATE / 60;
	m_playTimeMillisecondCount = m_playTimeFrameCount % FRAMERATE * 100 / FRAMERATE;
}

/// <summary>
/// Calculate the BPM displayed on screen during playback
/// </summary>
void CSong::CalculatePlayBPM()
{
	// If nothing is playing, set the BPM to 0 and bail out, the previously calculated values will be preserved
	if (m_playMode == MPLAY_STOP)
	{
		m_averageBPM = 0.0;
		return;
	}

	// Reset the current Average Speed
	m_averageSpeed = 0.0;

	// Speed values are refreshed every 8 Rows
	m_rowSpeed[m_playRow % 8] = m_playSpeed;

	// Add all Speed values together
	for (int i = 0; i < 8; i++)
		m_averageSpeed += m_rowSpeed[i];

	// Divide by 8 to get the updated Average Speed
	m_averageSpeed /= 8.0;

	// Calculate the Average BPM using the Average Speed and the Primary Highlight values
	m_averageBPM = 60.0 * FRAMERATE / g_trackLinePrimaryHighlight / m_averageSpeed;
}

/// <summary>
/// (Debug function)
/// Calculate the number of Screen Updates per second, displayed on screen as a FPS counter
/// </summary>
void CSong::CalculateDisplayFPS()
{
	uint64_t millisecond = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	uint64_t second = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	if (m_lastSecondCount != second)
	{
		m_lastFrameCount = m_lastDeltaCount = 0;
		m_lastSecondCount = second;
	}

	m_lastFrameCount++;
	m_lastDeltaCount += millisecond - m_lastMillisecondCount;
	m_lastMillisecondCount = millisecond;
	m_averageFrameCount = 1000.0 / m_lastDeltaCount * m_lastFrameCount;
}

// TODO: Move all these functions to GUI_Song.cpp later

void CSong::DrawSonglines(TSubtune* p)
{
	if (!p)
		return;

	CString s;
	RECT songblock{};
	const int linescount = 9, linesoffset = -4;

	// A Songline Index that is out of bounds will be displayed with a grayed out colour
	bool isOutOfBounds;

	// Caching global variables is necessary in order to display Patterns without random "jumps" around during playback 
	// This is caused by the screen update timing, and this is the main reason why these bugs seem to happen randomly
	BYTE activeSongline = m_activeSongline;
	BYTE playSongline = m_playSongline;
	BYTE activeChannel = m_activeChannel;

	// All Channels used in the Subtune will be displayed within the Songline Index
	for (int i = 0; i < p->channelCount; i++)
	{
		// Offset for the X axis, each character use a 8x16 Bitmap tile
		int x = (3 * 8) + (i * (3 * 8));

		// Default Colour for all Channels
		int colour = GetChannelOnOff(i) ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY;

		// If the Channel is both Active and Enabled, use the Highlight Colour instead
		if (GetChannelOnOff(i) && activeChannel == i)
			colour = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;

		// Each POKEY chips may use up to 4 Channels, numbered between 1 to 4 inclusive
		s.Format("%i", (i % 4) + 1);
		TextXY(s, SONGBLOCK_X + x, SONGBLOCK_Y + 16, colour);

		// For each POKEY chip, display the chip number above the Channel Index
		if (i % 4 == 0)
		{
			bool enabled = false;
			bool active = false;

			// Check if at least one of the associated Channels is either Enabled or Active
			for (int j = 0; j < 4; j++)
			{
				if (GetChannelOnOff((i + j)))
					enabled = true;

				if (activeChannel == i + j)
					active = true;
			}

			// The same Colour condition is used here, this time for the whole POKEY chip
			if (active && enabled)
				colour = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
			else
				colour = enabled ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY;

			// The POKEY chip number is derived from i, divided by 4, plus 1, due to being a Zero Based Index
			s.Format("POKEY %i", (i / 4) + 1);
			TextXY(s, SONGBLOCK_X + x, SONGBLOCK_Y, colour);
		}
	}

	// Draw all the Indexed Songlines that could be displayed at once
	for (int i = 0; i < linescount; i++)
	{
		// Offset for the Y axis, each character use a 8x16 Bitmap tile
		int y = (2 * 16) + (i * 16);

		// Fetch the actual Songline number relative to the Index Offset
		BYTE songline = activeSongline + i + linesoffset;

		// If the Songline Index is out of bounds, wrap around relative to the Song Length itself
		if (isOutOfBounds = songline >= p->songLength)
			songline += p->songLength;

		songline %= p->songLength;

		// Default Colour for all Songlines
		int colour = songline == playSongline ? TEXT_COLOR_YELLOW : TEXT_COLOR_WHITE;

		if (songline == activeSongline)
			colour = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;

		if (isOutOfBounds)
			colour = TEXT_COLOR_DARK_GRAY;

		s.Format("%02X", songline);
		TextXY(s, SONGBLOCK_X, SONGBLOCK_Y + y, colour);

		// Draw all Songlines used in every Channels
		for (int j = 0; j < p->channelCount; j++)
		{
			// Offset for the X axis, each character use a 8x16 Bitmap tile
			int x = (3 * 8) + (j * (3 * 8));

			// Default Colour for all Channels
			colour = GetChannelOnOff(j) ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY;

			if (GetChannelOnOff(j) && songline == playSongline)
				colour = TEXT_COLOR_YELLOW;

			if (GetChannelOnOff(j) && activeSongline == songline)
				colour = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;

			// If the Channel is both Active and Enabled, use the Highlight Colour instead
			if (GetChannelOnOff(j) && activeChannel == j && activeSongline == songline && g_activepart == PART_SONG)
				colour = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;

			if (isOutOfBounds)
				colour = TEXT_COLOR_DARK_GRAY;

			// Fetch the Pattern number used in the Songline's Channel, and draw it in the Songline Block
			s.Format("%02X", p->channel[j].songline[songline]);
			TextXY(s, SONGBLOCK_X + x, SONGBLOCK_Y + y, colour);
		}

	}

	// Draw the lines delimiting boundaries between each elements in the Songline block
	songblock.left = SONGBLOCK_X - 4;
	songblock.top = SONGBLOCK_Y;
	songblock.right = SONGBLOCK_X + (3 * 8) + (p->channelCount * (3 * 8)) - 4;
	songblock.bottom = SONGBLOCK_Y + (2 * 16) + (linescount * 16);

	// Songline Index and Songline Data:
	g_mem_dc->MoveTo(songblock.left + (3 * 8) - 1, songblock.top);
	g_mem_dc->LineTo(songblock.left + (3 * 8) - 1, songblock.bottom);
	
	// Left and Right POKEY:
	g_mem_dc->MoveTo(songblock.left + (5 * (3 * 8)) - 1, songblock.top);
	g_mem_dc->LineTo(songblock.left + (5 * (3 * 8)) - 1, songblock.bottom);

	// Channel Index and Songline Data:
	g_mem_dc->MoveTo(songblock.left, songblock.top + (2 * 16));
	g_mem_dc->LineTo(songblock.right, songblock.top + (2 * 16));

	// Active Songline Highlight:
	g_mem_dc->MoveTo(songblock.left, songblock.top + (6 * 16));
	g_mem_dc->LineTo(songblock.right, songblock.top + (6 * 16));
	g_mem_dc->MoveTo(songblock.left, songblock.top + (7 * 16));
	g_mem_dc->LineTo(songblock.right, songblock.top + (7 * 16));

	// The Songline Block itself:
	g_mem_dc->DrawEdge(&songblock, EDGE_BUMP, BF_RECT);
}

void CSong::DrawSubtuneInfos(TSubtune* p)
{
	if (!p)
		return;

	CString s;
	RECT infoblock{};
	int colour, x, y;

	//-- General Infos --//

	// Set the coordinates to the correct position on screen
	x = INFOBLOCK_X;
	y = INFOBLOCK_Y;

	// A nicer colour for everything that can be edited
	colour = TEXT_COLOR_TURQUOISE;

	// Time
	TextXY("TIME:", x, y);
	CalculatePlayTime();
	s.Format(m_playTimeSecondCount & 1 ? "%02d %02d.%02d" : "%02d:%02d.%02d", m_playTimeMinuteCount, m_playTimeSecondCount, m_playTimeMillisecondCount);
	TextXY(s, x += 6 * 8, y, m_playMode != MPLAY_STOP ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY);

	// BPM
	TextXY("BPM:", x += 10 * 8, y);
	CalculatePlayBPM();
	s.Format("%1.2F", m_averageBPM);
	TextXY(s, x += 5 * 8, y, m_playMode != MPLAY_STOP ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY);

	// Highlights
	TextXY("HIGHLIGHT:", x += 9 * 8, y);
	s.Format("%02X/%02X", g_trackLinePrimaryHighlight, g_trackLineSecondaryHighlight);
	TextXY(s, x += 12 * 8, y, colour);

	// Region
	TextXY(g_ntsc ? "NTSC" : "PAL", x += 7 * 8, y, colour);

	//-- Subtune Parameters --//

	// Set the coordinates to the correct position on screen
	x = INFOBLOCK_X;
	y = INFOBLOCK_Y;

	// Song Length
	TextMiniXY("SONG LENGTH", x, y += 1 * 16 + 8);
	s.Format("%02X", p->songLength);
	TextXY(s, x, y += 8, colour);
	TextXYSelN("<>", -1, x + 3 * 8, y);

	// Pattern Length
	TextMiniXY("PATTERN LENGTH", x, y += 1 * 16 + 8);
	s.Format("%02X", p->patternLength);
	TextXY(s, x, y += 8, colour);
	TextXYSelN("<>", -1, x + 3 * 8, y);

	// Song Speed
	TextMiniXY("SONG SPEED", x, y += 1 * 16 + 8);
	s.Format("%02X", p->songSpeed);
	TextXY(s, x, y += 8, colour);
	TextXYSelN("<>", -1, x + 3 * 8, y);

	// Instrument Speed
	TextMiniXY("INSTRUMENT SPEED", x, y += 1 * 16 + 8);
	s.Format("%02X", p->instrumentSpeed);
	TextXY(s, x, y += 8, colour);
	TextXYSelN("<>", -1, x + 3 * 8, y);

/*
	// Channel Count
	TextMiniXY("CHANNEL COUNT", x, y += 1 * 16 + 8);
	s.Format("%02X", p->channelCount);
	TextXY(s, x, y += 8, colour);
	TextXYSelN("<>", -1, x + 3 * 8, y);
*/

	//-- Subtune Metadata --//

	// Set the coordinates to the correct position on screen
	x = INFOBLOCK_X + 20 * 8;
	y = INFOBLOCK_Y;

	// Module Name
	TextMiniXY("MODULE NAME", x, y += 1 * 16 + 8);
	TextXY(g_Module.GetSongName(), x, y += 8, colour);

	// Author
	TextMiniXY("AUTHOR", x, y += 1 * 16 + 8);
	TextXY(g_Module.GetSongAuthor(), x, y += 8, colour);

	// Copyright
	TextMiniXY("COPYRIGHT", x, y += 1 * 16 + 8);
	TextXY(g_Module.GetSongCopyright(), x, y += 8, colour);

	// Subtune
	s.Format("SUBTUNE %02X/%02X", g_Module.GetActiveSubtune() + 1, g_Module.GetSubtuneCount());
	TextMiniXY(s, x, y += 1 * 16 + 8);
	TextXY(p->name, x, y += 8, colour);

	//-- Line Boundaries

	// Actual dimensions used by the Info block
	infoblock.left = INFOBLOCK_X - 4;
	infoblock.top = INFOBLOCK_Y;
	infoblock.right = INFOBLOCK_X + x + (MODULE_TITLE_NAME_MAX + 1) * 8 - 4;
	infoblock.bottom = INFOBLOCK_Y + 11 * 16;

	// Set the coordinates to the correct position on screen to draw lines around certain parts
	x -= 4;
	y = 3 * 16;

	// Draw lines around textboxes that could be edited
	for (int i = 0; i < 4; i++, y += 2 * 16)
	{
		g_mem_dc->MoveTo(x, y);
		g_mem_dc->LineTo(infoblock.right - 2 * 8 - 1, y);
		g_mem_dc->LineTo(infoblock.right - 2 * 8 - 1, y + 1 * 16);
		g_mem_dc->LineTo(x, y + 1 * 16);
		g_mem_dc->LineTo(x, y);
	}

	// The Info Block itself:
	g_mem_dc->DrawEdge(&infoblock, EDGE_BUMP, BF_RECT);
}

void CSong::DrawRegistersState(TSubtune* p)
{
	if (!p)
		return;
}

void CSong::DrawPatternEditor(TSubtune* p)
{
	if (!p)
		return;

	CString s;
	RECT patternblock{};

	// A Row Index that is out of bounds will be displayed with a grayed out colour
	bool isOutOfBounds;

	// Caching global variables is necessary in order to display Patterns without random "jumps" around during playback 
	// This is caused by the screen update timing, and this is the main reason why these bugs seem to happen randomly
	BYTE activeRow = m_activeRow;
	BYTE playRow = m_playRow;
	BYTE activeSongline = m_activeSongline;
	BYTE playSongline = m_playSongline;
	BYTE activeCursor = m_activeCursor;
	BYTE activeChannel = m_activeChannel;
	BYTE playSpeed = m_playSpeed;
	BYTE speedTimer = m_speedTimer;

	// Coordinates used for drawing most of the Pattern Editor block on screen
	int x = 3 * 8, y = 0;

	// The cursor position is alway centered regardless of the window size with this simple formula
	g_cursoractview = activeRow + 8 - g_line_y;

	bool active_smooth = (g_viewDoSmoothScrolling && ((m_playMode != MPLAY_STOP && m_isFollowPlay) && (playSpeed > 0 && speedTimer <= playSpeed)));
	int smooth_y = active_smooth ? speedTimer * 16 / playSpeed - 8 : 0;

	// Standard notation FIXME: create a proper method for the Note and Octave text format
	int notation = 0;

	if (g_displayflatnotes)
		notation += 1;

	if (g_usegermannotation)
		notation += 2;

	// Non-12 scales don't yet have proper display
	//if (g_notesperoctave != 12)
	//	notation = 4;

	// Process Channels 1 by 1, for all the Patterns to be drawn on screen
	for (int i = 0; i < p->channelCount; i++)
	{
		// The Y offset is always reset to 0 between Channels
		y = 0;

		// For as many Pattern Rows there are to display on screen, execute this loop
		for (int j = 0; j < g_tracklines + active_smooth; j++, y += 16)
		{
			// The first 3 Rows are used by the Channel Header, which will be drawn after everything
			if (j < 3 - active_smooth)
				continue;

			// Get the Row Index, derived from the active cursor offset and j, minus 9
			int row = g_cursoractview + j - 9;

			// This will be used for wrapping around the Pattern Row Index
			BYTE offsetSongline = activeSongline;

			// If the Row Index is out of bounds, wrap around relative to the Shortest Pattern Length itself
			if (isOutOfBounds = row < 0 || row >= g_Module.GetShortestPatternLength(offsetSongline))
			{
				// Add Rows for as many Songlines there are to offset above it
				while (row < 0)
				{
					if (--offsetSongline >= p->songLength)
						offsetSongline += p->songLength;
					row += g_Module.GetShortestPatternLength(offsetSongline);
				}

				// Subtract Rows for as many Songlines there are to offset below it
				while (row >= g_Module.GetShortestPatternLength(offsetSongline))
				{
					row -= g_Module.GetShortestPatternLength(offsetSongline);
					++offsetSongline %= p->songLength;
				}
			}

			if (row == 1)
			{
				g_mem_dc->MoveTo(x - 12, PATTERNBLOCK_Y - 16 + y + smooth_y);
				g_mem_dc->LineTo(x - 12 + (13 * 8) + (p->effectCommandCount[i] * (4 * 8)), PATTERNBLOCK_Y - 16 + y + smooth_y);
			}

			// Highlight Colour used to draw the Pattern Rows, from lowest to highest in priority
			int colour = TEXT_COLOR_WHITE;

			if (row % g_trackLineSecondaryHighlight == 0)
				colour = TEXT_COLOR_GREEN;

			if (row % g_trackLinePrimaryHighlight == 0)
				colour = TEXT_COLOR_TURQUOISE;

			if (row % g_trackLinePrimaryHighlight == 0 && row % g_trackLineSecondaryHighlight == 0)
				colour = TEXT_COLOR_CYAN;

			if (row == playRow && activeSongline == playSongline)
				colour = TEXT_COLOR_YELLOW;

			if (row == activeRow)
				colour = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;

			if (isOutOfBounds)
				colour = TEXT_COLOR_DARK_GRAY;

			// If the procedure is set to run from Channel 1, the Row Index will also be drawn on screen
			if (i == CH1)
			{
				s.Format("%02X", row);

				// The Colour Highlight for the Playing Row may be used here, regardless of being from a different Songline
				TextXY(s, PATTERNBLOCK_X, PATTERNBLOCK_Y + y + smooth_y, (row == playRow && row != activeRow && !isOutOfBounds) ? TEXT_COLOR_YELLOW : colour);
			}

			// Parse all the parameters and format the text for 1 Pattern Row
			s.Format("");

			// Conditions needed for displaying the Active Cursor Highlight
			bool isActiveCursor = g_activepart == PART_TRACKS && row == activeRow && i == activeChannel && !isOutOfBounds;

			// Get the Pattern Index from the Songline offset
			BYTE pattern = p->channel[i].songline[offsetSongline];

			// Note and Octave
			BYTE note = p->channel[i].pattern[pattern].row[row].note;

			switch (note)
			{
			case PATTERN_NOTE_EMPTY: s.AppendFormat("--- "); break;
			case PATTERN_NOTE_OFF: s.AppendFormat("OFF "); break;
			case PATTERN_NOTE_RELEASE: s.AppendFormat("=== "); break;
			case PATTERN_NOTE_RETRIGGER: s.AppendFormat("~~~ "); break;
			default:
				if (note < PATTERN_NOTE_MAX)
					s.AppendFormat("%s%1X ", notesandscales[notation][note % 12], note / 12);
				else
					s.AppendFormat("??? ");
			}

			// Instrument
			BYTE instrument = p->channel[i].pattern[pattern].row[row].instrument;

			switch (instrument)
			{
			case PATTERN_INSTRUMENT_EMPTY: s.AppendFormat("-- "); break;
			default:
				if (instrument < PATTERN_INSTRUMENT_MAX)
					s.AppendFormat("%02X ", instrument);
				else
					s.AppendFormat("?? ");
			}

			// Volume
			BYTE volume = p->channel[i].pattern[pattern].row[row].volume;

			switch (volume)
			{
			case PATTERN_VOLUME_EMPTY: s.AppendFormat("-- "); break;
			default:
				if (volume < PATTERN_VOLUME_MAX)
					s.AppendFormat("v%01X ", volume);
				else
					s.AppendFormat("?? ");
			}

			// Command(s)
			for (int k = 0; k < p->effectCommandCount[i]; k++)
			{
				BYTE command = p->channel[i].pattern[pattern].row[row].command[k].identifier;
				BYTE parameter = p->channel[i].pattern[pattern].row[row].command[k].parameter;

				switch (command)
				{
				case PATTERN_EFFECT_EMPTY: s.AppendFormat("--- "); break;
				default:
					if (command < PATTERN_EFFECT_MAX)
						s.AppendFormat("%01X%02X ", command, parameter);
					else
						s.AppendFormat("??? ");
				}
			}

			// Draw the formated Pattern Row on screen once it is ready to be output, using the cursor position for highlighted column
			TextXYCol(s, PATTERNBLOCK_X + x, PATTERNBLOCK_Y + y + smooth_y, isActiveCursor ? activeCursor : INVALID, colour);
		}

		// Update the X offset with the Channel's width, including the Active Effect Commands
		x += (10 * 8) + (p->effectCommandCount[i] * (4 * 8));
	}

	// Actual dimensions used by the Pattern Editor block, including the Channels Header
	patternblock.left = PATTERNBLOCK_X - 4;
	patternblock.top = PATTERNBLOCK_Y;
	patternblock.right = PATTERNBLOCK_X + x - 4;
	patternblock.bottom = PATTERNBLOCK_Y + (g_tracklines * 16);

	// Coordinates used for drawing lines delimiting boundaries
	x = (3 * 8) - 1;
	y = patternblock.top + (g_line_y * 16) + (1 * 16);

	// Mask out the extra Rows from smooth scroll on the top and bottom
	g_mem_dc->FillSolidRect(patternblock.left - 4, PATTERNBLOCK_Y + 1, patternblock.right - 4, 3 * 16, RGB_BACKGROUND);
	g_mem_dc->FillSolidRect(patternblock.left - 4, patternblock.bottom - 1, patternblock.right - 4, 2 * 16, RGB_BACKGROUND);

	// Row Index and Pattern Data:
	g_mem_dc->MoveTo(patternblock.left + x, patternblock.top);
	g_mem_dc->LineTo(patternblock.left + x, patternblock.bottom);

	// Active Pattern Row Highlight:
	g_mem_dc->MoveTo(patternblock.left, y);
	g_mem_dc->LineTo(patternblock.right, y);
	g_mem_dc->MoveTo(patternblock.left, y + 16);
	g_mem_dc->LineTo(patternblock.right, y + 16);

	// Channels Header and Pattern Data:
	g_mem_dc->MoveTo(patternblock.left, patternblock.top + (3 * 16));
	g_mem_dc->LineTo(patternblock.right, patternblock.top + (3 * 16));

	// Separation between each POKEY Channels, and the Channels Header on top of it
	for (int i = 0; i < p->channelCount; i++)
	{
		y = 0;

		for (int j = 0; j < 3; j++, y += 16)
		{
			switch (j)
			{
			case 0:
				s.Format(" PATTERN: %02X", p->channel[i].songline[activeSongline]);
				break;

			case 1:
			{
				BYTE offset = i >= 4 ? 0x10 : 0x00;
				BYTE audc = g_atarimem[0xd200 + offset + 2 * (i % 4) + 1];
				BYTE vol = audc & 0x0F;
				BYTE skctl = g_atarimem[0xd20F + offset];
				COLORREF acol = GetChannelOnOff(i) ? ((audc & 0x10) ? RGB_VOLUME_ONLY : RGB_NORMAL) : RGB_MUTE;

				if (GetChannelOnOff(i) && ((skctl == 0x8B && i % 4 == 0)))
					acol = RGB_TWO_TONE;

				g_mem_dc->FillSolidRect(PATTERNBLOCK_X + x + 7, PATTERNBLOCK_Y + y + 4, 15 * 6, 8, RGB(44, 60, 102));

				if (vol)
					g_mem_dc->FillSolidRect(PATTERNBLOCK_X + x + 8 + (15 - vol) * 6 / 2, PATTERNBLOCK_Y + y + 4, vol * 6, 8, acol);

				continue;
			}

			case 2:
				s.Format("      FX%i", p->effectCommandCount[i]);
				TextXYSelN("<>", -1, PATTERNBLOCK_X + x + 10 * 8, PATTERNBLOCK_Y + y);
				break;
			}

			TextXY(s, PATTERNBLOCK_X + x, PATTERNBLOCK_Y + y, GetChannelOnOff(i) == 0);
		}

		x += (10 * 8) + (p->effectCommandCount[i] * (4 * 8));
		g_mem_dc->MoveTo(patternblock.left + x, patternblock.top);
		g_mem_dc->LineTo(patternblock.left + x, patternblock.bottom);
	}

	// The Pattern Editor Block itself:
	g_mem_dc->DrawEdge(&patternblock, EDGE_BUMP, BF_RECT);
}

void CSong::DrawInstrumentEditor(TSubtune* p)
{
	if (!p)
		return;
}

void CSong::DrawDebugInfos(TSubtune* p)
{
	// Debug display at the top of the screen, this could be toggled on if needed 
	if (!g_viewDebugDisplay)
		return;

	CString s;

	CalculateDisplayFPS();

	// Don't draw further more than what could fit on screen
	for (int i = 0; i < g_width / 96; i++)
	{
		switch (i)
		{
		case 0: s.Format("FPS = %1.2F", m_averageFrameCount); break;
		case 1: s.Format("GW = %02d", g_width); break;
		case 2: s.Format("GH = %02d", g_height); break;
		case 3: s.Format("PX = %02d", g_mouseLastPointX); break;
		case 4: s.Format("PY = %02d", g_mouseLastPointY); break;
		case 5: s.Format("MB = %02d", g_mouseLastButton); break;
		case 6: s.Format("WD = %02d", g_mouseLastWheelDelta); break;
		case 7: s.Format("CA = %02d", g_cursoractview); break;
		case 8: s.Format("TA = %02d", m_playRow); break;
		case 9: s.Format("DY = %02d", g_mouseLastPointY / 16); break;
		case 10: s.Format("GTL = %02d", g_tracklines); break;
		case 11: s.Format("OL = %02d", g_tracklines / 2); break;
		case 12: s.Format("VK = %02X", g_lastKeyPressed); break;
		default: continue;
		}

		TextMiniXY(s, 12 + i * 96, 4, TEXT_MINI_COLOR_WHITE);
	}
}

// Prototype C++ RMTE Module Driver functions
// TODO: Move to a different file later

void CSong::Stop()
{
	m_playMode = MPLAY_STOP;
	ResetPlayTime();
	InitialiseSongVariables();
	//m_quantization_note = m_quantization_instr = m_quantization_vol = -1;
	//SetPlayPressedTonesSilence();
	Atari_InitRMTRoutine();
	Sleep(20);	// To ensure the Timer Routine is executed at least once here
}

void CSong::Play(int mode, BOOL follow, int special)
{
	TSubtune* p = g_Module.GetSubtuneIndex();

	if (!p)
		return;

	// Stop and reset the POKEY registers first
	Stop();

	switch (mode)
	{
	case MPLAY_START:
		m_playSongline = m_playRow = 0;
		m_playSpeed = p->songSpeed;
		break;

	case MPLAY_FROM:
		m_playSongline = m_activeSongline;
		m_playRow = m_activeRow;
		break;

	case MPLAY_PATTERN:
		m_playSongline = m_activeSongline;
		m_playRow = (special) ? m_activeRow : 0;
		break;
	}

	// Cursor following the player
	if (m_isFollowPlay = follow)
	{
		m_activeRow = m_playRow;
		m_activeSongline = m_playSongline;
	}

	// Begin playback from here
	m_playMode = mode;
	m_speedTimer = 1;
	g_PokeyStream.CallFromPlay(m_playMode, m_playRow, m_playSongline);
}

/*
void CSong::PlayRow(TSubtune* p)
{
	if (!p)
		return;

	BYTE row = m_playRow;
	BYTE songline = m_playSongline;

	// Play Row without the 6502 routines limitation, designed specifically for the RMTE Module format
	// Ultimatey aimed to become the default payback method, unless specified otherwise (eg: Legacy RMT compatibility)
	if (g_isRMTE && g_trackerDriverVersion == TRACKER_DRIVER_NONE)
	{
		// Process all channels
		for (int i = 0; i < p->channelCount; i++)
		{
			// Note
			PlayNote(p, i, songline, row);

			// Instrument
			PlayInstrument(p, i, songline, row);

			// Volume;
			PlayVolume(p, i, songline, row);

			// Command(s)
			for (int k = 0; k < p->effectCommandCount[i]; k++)
				PlayEffect(p, i, songline, row, k);
		}

		// Set the speed at the end, last Fxx Command used will take priority
		//m_speedTimer = m_playSpeed;
	}

	// Play Row with Legacy RMT compatibility in mind using the emulated 6502 routines
	// TODO: Recreate the routine behaviour entirely in software for better compatibility
	else
	{
		BYTE speed = m_playSpeed;

		// In order to simulate the Legacy 6502 RMT routines behaviour, certain compromises are deliberately introduced here
		for (int i = 0; i < p->channelCount; i++)
		{
			// Get the Pattern Index from the Songline offset
			BYTE pattern = p->channel[i].songline[songline];

			// Same as Legacy RMT routines
			BYTE note = p->channel[i].pattern[pattern].row[row].note;
			if (note >= PATTERN_NOTE_COUNT)
				note = INVALID;
			
			// Same as Legacy RMT routines
			BYTE instrument = p->channel[i].pattern[pattern].row[row].instrument;
			if (instrument >= PATTERN_INSTRUMENT_COUNT)
				instrument = INVALID;
			
			// Same as Legacy RMT routines
			BYTE volume = p->channel[i].pattern[pattern].row[row].volume;
			if (volume >= PATTERN_VOLUME_COUNT)
				volume = INVALID;

			// Compromised to only get the Speed Commands
			for (int k = 0; k < p->effectCommandCount[i]; k++)
			{
				BYTE command = p->channel[i].pattern[pattern].row[row].command[k].identifier;
				BYTE parameter = p->channel[i].pattern[pattern].row[row].command[k].parameter;

				// If a Bxx command is found, set the next Songline with it
				if (command == EFFECT_COMMAND_BXX)
				{
					m_playRow = INVALID;
					m_playSongline = parameter;
				}

				// If a Dxx command is found, the Pattern will end on the next Row
				if (command == EFFECT_COMMAND_DXX)
				{
					m_playRow = INVALID;
					m_playSongline = (songline + 1) % p->songLength;
				}

				// If a Fxx command is found, overwrite the speed with it
				if (command == EFFECT_COMMAND_FXX && parameter)
					speed = parameter;
			}

			// Compromised for compatibility with RMTE data
			//if (volume < PATTERN_VOLUME_COUNT)
			//{
			//	if (note < PATTERN_NOTE_COUNT)
			//		Atari_SetTrack_NoteInstrVolume(i, note, instrument, volume);
			//	else
			//		Atari_SetTrack_Volume(i, volume);
			//}
		}

		// Speed will be set at the end, so the last Channel with a Speed value will take priority
		m_speedTimer = m_playSpeed = speed;
	}

}
*/

// Play Module without the 6502 RMT routines limitation, designed specifically for the new RMTE Module format
// Ultimatey, this will become the default payback method, unless specified otherwise (eg: Legacy RMT compatibility)
void CSong::PlayPattern(TSubtune* p)
{
	if (!p)
		return;

	// If RMT is not playing, there is nothing to do here
	if (m_playMode == MPLAY_STOP)
		return;

	// Play next Row when it is ready
	if (--m_speedTimer == 0)
	{
		BYTE row = m_playRow;
		BYTE songline = m_playSongline;

		// Process all channels
		for (int i = 0; i < p->channelCount; i++)
		{
			// Note
			PlayNote(p, i, songline, row);

			// Instrument
			PlayInstrument(p, i, songline, row);

			// Volume;
			PlayVolume(p, i, songline, row);

			// Command(s)
			for (int k = 0; k < p->effectCommandCount[i]; k++)
				PlayEffect(p, i, songline, row, k);
		}

		// The next Songline will be played if the Pattern reached the End position
		if (++m_playRow >= p->patternLength)
			PlayNextSongline(p);

		// Set the Speed Timer to the current Play Speed parameter, the last Fxx Command used will take priority
		m_speedTimer = m_playSpeed;
	}

	// If FollowPlay is enabled, update the Active Row and Active Songline values accordingly
	if (m_isFollowPlay)
	{
		m_activeRow = m_playRow;
		m_activeSongline = m_playSongline;
	}
}

// Procedure taking care of playing Instruments, executing Effect Commands, and setting up the POKEY registers during playback
void CSong::PlayContinue(TSubtune* p)
{
	if (!p)
		return;

	// If RMT is not playing, there is nothing to do here
	if (m_playMode == MPLAY_STOP)
		return;

	TInstrumentV2* instrument;

	BYTE audctl, audctl1, audctl2, skctl, skctl1, skctl2;
	BYTE note, timbre, frame;

	for (int i = 0; i < p->channelCount; i++)
	{
		instrument = g_Module.GetInstrument(m_songVariables[i].channelInstrument);

		if (instrument)
		{
			frame = m_songVariables[i].frameCount;

			m_songVariables[i].channelDistortion = instrument->distortionEnvelope[frame];
			m_songVariables[i].channelAUDCTL = instrument->audctlEnvelope[frame];
			m_songVariables[i].instrumentVolume = instrument->volumeEnvelope[frame];

			if (++frame >= instrument->envelopeLength)
				frame = instrument->envelopeLoop;

			m_songVariables[i].frameCount = frame;
		}

	}

	// Get the combined AUDCTL and SKCTL values ahead of time, they are mendatory for generating the correct POKEY Frequencies!
	audctl1 = audctl2 = 0x00;
	skctl1 = skctl2 = 0x03;

	for (int i = 0; i < 4; i++)
	{
		audctl1 |= m_songVariables[i].channelAUDCTL;
		audctl2 |= m_songVariables[i + 4].channelAUDCTL;
	}

	// Process the Song Variables currently in memory
	for (int i = 0; i < p->channelCount; i++)
	{
		note = m_songVariables[i].channelNote;
		timbre = m_songVariables[i].channelDistortion;
		audctl = i >= 4 ? audctl2 : audctl1;

		switch (note)
		{
		case PATTERN_NOTE_EMPTY:
			break;

		case PATTERN_NOTE_OFF:
			break;

		case PATTERN_NOTE_RELEASE:
			break;

		case PATTERN_NOTE_RETRIGGER:
			break;

		default:
			if (note < PATTERN_NOTE_COUNT)
				m_songVariables[i].channelFreq = g_Tuning.GeneratePokeyFreq(g_Tuning.GetTruePitch(note, g_baseNote + 12 * (g_baseOctave - 4), g_baseTuning), i, timbre, audctl);
		}

	}

	// Update the AUDC and AUDF registers with the new data the same way
	for (int i = 0; i < p->channelCount; i++)
	{
		audctl = i >= 4 ? audctl2 : audctl1;

		// Update the AUDF using the full 16-bit Freq values, updating 2 Channels at once, if the AUDCTL is in 16-bit mode
		if ((audctl & 0x10 && audctl & 0x40 && i % 4 == 1) || (audctl & 0x08 && audctl & 0x20 && i % 4 == 3))
		{
			g_atarimem[RMTPLAYR_TRACKN_AUDF + i] = m_songVariables[i].channelFreq >> 8;
			g_atarimem[RMTPLAYR_TRACKN_AUDF + i - 1] = m_songVariables[i].channelFreq & 0xFF;
			g_atarimem[RMTPLAYR_TRACKN_AUDC + i - 1] = 0x00;
		}

		// Otherwise, a 8-bit Freq value is be assumed and used as such, no additional check is necessary in this case
		else
			g_atarimem[RMTPLAYR_TRACKN_AUDF + i] = m_songVariables[i].channelFreq & 0xFF;

		// Update the AUDC using the combined Timbre and Volume values
		g_atarimem[RMTPLAYR_TRACKN_AUDC + i] = m_songVariables[i].channelDistortion & 0xF0;
		g_atarimem[RMTPLAYR_TRACKN_AUDC + i] |= m_songVariables[i].channelVolume * m_songVariables[i].instrumentVolume / 0x0F;
	}

	// Update the AUDCTL and SKCTL using the combined values from all Channels
	g_atarimem[RMTPLAYR_V_AUDCTL] = audctl1;
	g_atarimem[RMTPLAYR_V_AUDCTL2] = audctl2;
	g_atarimem[RMTPLAYR_V_SKCTL] = skctl1;
	g_atarimem[RMTPLAYR_V_SKCTL2] = skctl2;
}

void CSong::PlayNextSongline(TSubtune* p)
{
	if (!p)
		return;

	m_playRow = 0;
	++m_playSongline %= p->songLength;

	if (g_PokeyStream.TrackSongLine(m_playSongline))
		Stop();
}

void CSong::PlayNote(TSubtune* p, BYTE channel, BYTE songline, BYTE row)
{
	if (!p)
		return;

	BYTE pattern = p->channel[channel].songline[songline];
	BYTE note = p->channel[channel].pattern[pattern].row[row].note;
	bool active = m_songVariables[channel].isNoteActive;

	switch (note)
	{
	case PATTERN_NOTE_EMPTY:
		//if (m_songVariables[channel].isNoteActive)
		//	m_songVariables[channel].isNoteSustain = true;
		break;

	case PATTERN_NOTE_OFF:
		m_songVariables[channel].channelNote = INVALID;
		m_songVariables[channel].isNoteActive = false;
		break;

	case PATTERN_NOTE_RELEASE:
		if (active)
			m_songVariables[channel].isNoteRelease = true;
		break;

	case PATTERN_NOTE_RETRIGGER:
		if (active)
			m_songVariables[channel].isNoteTrigger = true;
		break;

	default:
		if (note < PATTERN_NOTE_COUNT)
		{
			m_songVariables[channel].channelNote = note;
			m_songVariables[channel].isNoteActive = true;
			m_songVariables[channel].isNoteTrigger = true;
			m_songVariables[channel].frameCount = 0;
		}
		//else
		//	m_songVariables[channel].channelNote = INVALID;
	}

	//if (m_songVariables[channel].isNoteTrigger)
	//{
	//	m_songVariables[channel].isNoteActive = true;
	//	m_songVariables[channel].isNoteTrigger = false;
	//	m_songVariables[channel].frameCount = 0;
	//}
}

void CSong::PlayInstrument(TSubtune* p, BYTE channel, BYTE songline, BYTE row)
{
	if (!p)
		return;

	BYTE pattern = p->channel[channel].songline[songline];
	BYTE instrument = p->channel[channel].pattern[pattern].row[row].instrument;
	bool active = m_songVariables[channel].isNoteActive;

	switch (instrument)
	{
	case PATTERN_INSTRUMENT_EMPTY:
		break;

	default:
		//if (active && instrument < PATTERN_INSTRUMENT_COUNT)
		if (instrument < PATTERN_INSTRUMENT_COUNT)
			m_songVariables[channel].channelInstrument = instrument;
		//else
		//	m_songVariables[channel].channelInstrument = INVALID;
	}
}

void CSong::PlayVolume(TSubtune* p, BYTE channel, BYTE songline, BYTE row)
{
	if (!p)
		return;

	BYTE pattern = p->channel[channel].songline[songline];
	BYTE volume = p->channel[channel].pattern[pattern].row[row].volume;
	bool active = m_songVariables[channel].isNoteActive;

	switch (volume)
	{
	case PATTERN_VOLUME_EMPTY:
		break;

	default:
		//if (active && volume < PATTERN_VOLUME_COUNT)
		if (active && volume < PATTERN_VOLUME_COUNT)
			m_songVariables[channel].channelVolume = volume;
		//else
		//	m_songVariables[channel].channelVolume = 0x00;
	}

}

void CSong::PlayEffect(TSubtune* p, BYTE channel, BYTE songline, BYTE row, BYTE column)
{
	if (!p)
		return;

	BYTE pattern = p->channel[channel].songline[songline];
	BYTE command = p->channel[channel].pattern[pattern].row[row].command[column].identifier;
	BYTE parameter = p->channel[channel].pattern[pattern].row[row].command[column].parameter;
	//bool active = m_songVariables[channel].isNoteActive;

	switch (command)
	{
	case EFFECT_COMMAND_BXX:
		m_playRow = p->patternLength;	// Set the Row to equal the Pattern Length in order to force the next Songline to play (same as Dxx)
		m_playSongline = parameter - 1;	// The Songline will increment by 1 during playback, subtract 1 to the parameter to work around this
		break;

	case EFFECT_COMMAND_DXX:
		m_playRow = p->patternLength;	// Set the Row to equal the Pattern Length in order to force the next Songline to play
		// TODO(?): Add a method to set the next Row to the parameter
		break;

	case EFFECT_COMMAND_FXX:
		m_playSpeed = parameter;		// Set the Play Speed to the parameter directly, it will be applied immediately
		break;
	}
}
