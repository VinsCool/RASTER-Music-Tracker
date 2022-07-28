#include "StdAfx.h"
#include <fstream>
using namespace std;

#include "GuiHelpers.h"

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
#include "Song.h"
#include "Clipboard.h"


#include "global.h"

#include "GUI_Instruments.h"

#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"


extern CSong g_Song;
extern CInstruments g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CXPokey g_Pokey;

void WaitForTimerRoutineProcessed()
{
	g_timerroutineprocessed = 0;
	while (!g_timerroutineprocessed) Sleep(1);		//waiting
}

// ----------------------------------------------------------------------------

void CALLBACK G_TimerRoutine(UINT, UINT, DWORD, DWORD, DWORD)
{
	g_Song.TimerRoutine();
}


// ----------------------------------------------------------------------------

CSong::CSong()
{
	ClearSong(8);	//the default is 8 stereo tracks

	//initialise Timer
	m_timer = 0;
	g_timerroutineprocessed = 0;

	m_quantization_note = -1; //init
	m_quantization_instr = -1;
	m_quantization_vol = -1;

	//Timer 1/50s = 20ms, 1/60s ~ 16/17ms
	//Pal or NTSC
	ChangeTimer((g_ntsc) ? 17 : 20);
}

CSong::~CSong()
{
	if (m_timer) { timeKillEvent(m_timer); m_timer = 0; }
}

void CSong::ChangeTimer(int ms)
{
	if (m_timer) { timeKillEvent(m_timer); m_timer = 0; }
	m_timer = timeSetEvent(ms, 0, G_TimerRoutine, (ULONG)(NULL), TIME_PERIODIC);
}

BOOL CSong::ClearSong(int numoftracks)
{
	g_tracks4_8 = numoftracks;	//track for 4/8 channels
	g_rmtroutine = 1;			//RMT routine execution enabled
	g_prove = 0;
	g_respectvolume = 0;
	g_rmtstripped_adr_module = 0x4000;	//default standard address for stripped RMT modules
	g_rmtstripped_sfx = 0;		//is not a standard sfx variety stripped RMT
	g_rmtstripped_gvf = 0;		//default does not use Feat GlobalVolumeFade
	g_rmtmsxtext = "";			//clear the text for MSX export
	g_expasmlabelprefix = "MUSIC";	//default label prefix for exporting simple ASM notation

	PlayPressedTonesInit();

	m_play = MPLAY_STOP;
	g_playtime = 0;
	m_followplay = 1;
	m_mainspeed = m_speed = m_speeda = 16;
	m_instrspeed = 1;

	g_activepart = g_active_ti = PART_TRACKS;	//tracks

	m_songplayline = m_songactiveline = 0;
	m_trackactiveline = m_trackplayline = 0;
	m_trackactivecol = m_trackactivecur = 0;
	m_activeinstr = 0;
	m_octave = 0;
	m_volume = MAXVOLUME;

	ClearBookmark();

	m_infoact = 0;

	memset(m_songname, ' ', SONGNAMEMAXLEN);
	strncpy(m_songname, "Noname song", 11);
	m_songname[SONGNAMEMAXLEN] = 0;

	m_songnamecur = 0;

	m_filename = "";
	m_filetype = 0;	//none
	m_exporttype = 0; //none

	m_TracksOrderChange_songlinefrom = 0x00;
	m_TracksOrderChange_songlineto = SONGLEN - 1;

	//number of lines after inserting a note/space
	g_linesafter = 1; //initial value
	CMainFrame* mf = ((CMainFrame*)AfxGetMainWnd());
	if (mf) mf->m_comboSkipLinesAfterNoteInsert.SetCurSel(g_linesafter);

	for (int i = 0; i < SONGLEN; i++)
	{
		for (int j = 0; j < SONGTRACKS; j++)
		{
			m_song[i][j] = -1;	//TRACK --
		}
		m_songgo[i] = -1;		//is not GO
	}

	//empty clipboards
	g_TrackClipboard.Empty();
	//
	m_instrclipboard.act = -1;			//according to -1 it knows that it is empty
	m_songgoclipboard = -2;				//according to -2 it knows that it is empty

	//delete all tracks and instruments
	g_Tracks.InitTracks();
	g_Instruments.InitInstruments();

	//Undo initialization
	g_Undo.Init();

	//Changes in the module
	g_changes = 0;

	return 1;
}



BOOL CSong::InitPokey() { return g_Pokey.InitSound(); };
BOOL CSong::DeInitPokey() { return g_Pokey.DeInitSound(); };










//---

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







void CSong::MarkTF_USED(BYTE* arrayTRACKSNUM)
{
	int i, tr;
	//all tracks used in the song
	for (i = 0; i < SONGLEN; i++)
	{
		if (m_songgo[i] < 0)
		{
			for (int j = 0; j < g_tracks4_8; j++)
			{
				tr = m_song[i][j];
				if (tr >= 0 && tr < TRACKSNUM) arrayTRACKSNUM[tr] = TF_USED;
			}
		}
	}
}

void CSong::MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM)
{
	for (int i = 0; i < TRACKSNUM; i++)
	{
		if (g_Tracks.CalculateNoEmpty(i)) arrayTRACKSNUM[i] |= TF_NOEMPTY;
	}
}

int CSong::MakeModule(unsigned char* mem, int adr, int iotype, BYTE* instrsaved, BYTE* tracksaved)
{
	//returns maxadr (points to the first free address after the module) and sets the instrsaved and tracksaved fields
	if (iotype == IOTYPE_RMF) return MakeRMFModule(mem, adr, instrsaved, tracksaved);

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;
	memset(instrsave, 0, INSTRSNUM);	//init
	memset(tracksave, 0, TRACKSNUM); //init

	strncpy((char*)(mem + adr), "RMT", 3);
	mem[adr + 3] = g_tracks4_8 + '0';	//4 or 8
	mem[adr + 4] = g_Tracks.m_maxtracklen & 0xff;
	mem[adr + 5] = m_mainspeed & 0xff;
	mem[adr + 6] = m_instrspeed;		//instr speed 1-4
	mem[adr + 7] = RMTFORMATVERSION;	//RMT format version number

	//in RMT all non-empty tracks and non-empty instruments will be stored in others only non-empty used tracks and used instruments in them
	int i, j;

	//all tracks used in the song
	MarkTF_USED(tracksave);

	if (iotype == IOTYPE_RMT)
	{
		//In addition to the used ones, all non-empty tracks are added to the RMT, all non-empty tracks
		MarkTF_NOEMPTY(tracksave);
	}

	//all instruments in the tracks that will be saved
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (tracksave[i] > 0)
		{
			TTrack& tr = *g_Tracks.GetTrack(i);
			for (j = 0; j < tr.len; j++)
			{
				int ins = tr.instr[j];
				if (ins >= 0 && ins < INSTRSNUM) instrsave[ins] = IF_USED;
			}
		}
	}

	if (iotype == IOTYPE_RMT)
	{
		//in addition to the instruments used in the tracks that are in the song, all non-empty instruments are stored in the RMT
		for (i = 0; i < INSTRSNUM; i++)
		{
			if (g_Instruments.CalculateNoEmpty(i)) instrsave[i] |= IF_NOEMPTY;
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
	//instruments
	//tracks
	//songlines

	int adrpinstruments = adr + 16;
	int adrptrackslbs = adrpinstruments + numinstrs * 2;
	int adrptrackshbs = adrptrackslbs + numtracks;

	int adrinstrdata = adrptrackshbs + numtracks; //behind the track byte table
	//saves instrument data and writes their beginnings to the table
	for (i = 0; i < numinstrs; i++)
	{
		if (instrsave[i])
		{
			int leninstr = g_Instruments.InstrToAta(i, mem + adrinstrdata, MAXATAINSTRLEN);
			mem[adrpinstruments + i * 2] = adrinstrdata & 0xff;	//dbyte
			mem[adrpinstruments + i * 2 + 1] = adrinstrdata >> 8;	//hbyte
			adrinstrdata += leninstr;
		}
		else
		{
			mem[adrpinstruments + i * 2] = mem[adrpinstruments + i * 2 + 1] = 0;
		}
	}

	int adrtrackdata = adrinstrdata;	//for instrument data
	//saves track data and writes their beginnings to the table
	for (i = 0; i < numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = g_Tracks.TrackToAta(i, mem + adrtrackdata, MAXATATRACKLEN);
			if (lentrack < 1)
			{	//cannot be saved to RMT
				CString msg;
				msg.Format("Fatal error in track %02X.\n\nThis track contains too many events (notes and speed commands),\nthat's why it can't be coded to RMT internal code format.", i);
				MessageBox(g_hwnd, msg, "Internal format problem.", MB_ICONERROR);
				return -1;
			}
			mem[adrptrackslbs + i] = adrtrackdata & 0xff;	//dbyte
			mem[adrptrackshbs + i] = adrtrackdata >> 8;	//hbyte
			adrtrackdata += lentrack;
		}
		else
		{
			mem[adrptrackslbs + i] = mem[adrptrackshbs + i] = 0;
		}
	}

	int adrsong = adrtrackdata;		//for track data

	//save from adrsong data song
	//int lensong = SongToAta(mem+adrsong,g_tracks4_8*SONGLEN,adrsong);  //<---COARSE WITH MAX BUFFER SIZE!
	int lensong = SongToAta(mem + adrsong, 0x10000 - adrsong, adrsong);

	int endofmodule = adrsong + lensong;

	//writes computed pointers to the header
	mem[adr + 8] = adrpinstruments & 0xff;	//dbyte
	mem[adr + 9] = adrpinstruments >> 8;		//hbyte
	//
	mem[adr + 10] = adrptrackslbs & 0xff;		//dbyte
	mem[adr + 11] = adrptrackslbs >> 8;		//hbyte
	mem[adr + 12] = adrptrackshbs & 0xff;		//dbyte
	mem[adr + 13] = adrptrackshbs >> 8;		//hbyte
	//
	mem[adr + 14] = adrsong & 0xff;		//dbyte
	mem[adr + 15] = adrsong >> 8;			//hbyte

	return endofmodule;
}

int CSong::MakeRMFModule(unsigned char* mem, int adr, BYTE* instrsaved, BYTE* tracksaved)
{
	//returns maxadr (points to the first available address behind the module) and sets the instrsaved and tracksaved fields

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;

	memset(instrsave, 0, INSTRSNUM);	//init
	memset(tracksave, 0, TRACKSNUM); //init

	mem[adr + 0] = m_instrspeed;		//instr speed 1-4
	mem[adr + 1] = m_mainspeed & 0xff;

	//all non-empty tracks and non-empty instruments will be stored in the RMT, in others only non-empty tracks and instruments used in them will be stored
	int i, j;

	//all tracks used in the song
	MarkTF_USED(tracksave);

	//all instruments in the tracks that will be saved
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (tracksave[i] > 0)
		{
			TTrack& tr = *g_Tracks.GetTrack(i);
			for (j = 0; j < tr.len; j++)
			{
				int ins = tr.instr[j];
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
		int minlen = g_Tracks.m_maxtracklen;	//init
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


int CSong::DecodeModule(unsigned char* mem, int adrfrom, int adrend, BYTE* instrloaded, BYTE* trackloaded)
{
	int adr = adrfrom;

	memset(instrloaded, 0, INSTRSNUM);
	memset(trackloaded, 0, TRACKSNUM);

	unsigned char b;
	int i, j;

	if (strncmp((char*)(mem + adr), "RMT", 3) != 0) return 0; //there is no RMT
	b = mem[adr + 3];
	if (b != '4' && b != '8') return 0;	//it is not RMT4 or RMT8
	g_tracks4_8 = b & 0x0f;
	b = mem[adr + 4];
	g_Tracks.m_maxtracklen = (b > 0) ? b : 256;	//0 => 256
	//g_cursoractview = g_Tracks.m_maxtracklen / 2;
	b = mem[adr + 5];
	m_mainspeed = b;
	if (b < 1) return 0;		//there can be no zero speed
	b = mem[adr + 6];
	if (b < 1 || b>8) return 0;		//instrument speed is less than 1 or greater than 8 (note: should be max 4, but allows up to 8 and will only display a warning)
	m_instrspeed = b;
	int version = mem[adr + 7];
	if (version > RMTFORMATVERSION)	return 0;	//the byte version is above the current one

	//Now g_tracks.m_maxtracklen is set to the value according to the header from the RMT module, 
	//so they have to re-initialize the Tracks so that this value is set to them all as the length
	g_Tracks.InitTracks();

	int adrpinstruments = mem[adr + 8] + (mem[adr + 9] << 8);
	int adrptrackslbs = mem[adr + 10] + (mem[adr + 11] << 8);
	int adrptrackshbs = mem[adr + 12] + (mem[adr + 13] << 8);
	int adrsong = mem[adr + 14] + (mem[adr + 15] << 8);

	int numinstrs = (adrptrackslbs - adrpinstruments) / 2;
	int numtracks = (adrptrackshbs - adrptrackslbs);
	int lensong = adrend - adrsong;

	//decoding of individual instruments
	for (i = 0; i < numinstrs; i++)
	{
		int instrdata = mem[adrpinstruments + i * 2] + (mem[adrpinstruments + i * 2 + 1] << 8);
		if (instrdata == 0) continue; //the omitted instruments have the pointer db, hb = 0
		//othwewise it has a non-zero pointer
		BOOL r;
		if (version == 0)
			r = g_Instruments.AtaV0ToInstr(mem + instrdata, i);
		else
			r = g_Instruments.AtaToInstr(mem + instrdata, i);
		g_Instruments.ModificationInstrument(i);	//writes to Atari ram
		if (!r) return 0; //some problem with the instrument => END
		instrloaded[i] = 1;
	}

	//decoding individual tracks
	for (i = 0; i < numtracks; i++)
	{
		int track = i;
		int trackdata = mem[adrptrackslbs + i] + (mem[adrptrackshbs + i] << 8);
		if (trackdata == 0) continue; //omitted tracks have pointer db, hb = 0

		//identify the end of the track by the address of the next track and at the last by the address of the song that follows the data of the last track
		int trackend = 0;
		for (j = i; j < numtracks; j++)
		{
			trackend = (j + 1 == numtracks) ? adrsong : mem[adrptrackslbs + j + 1] + (mem[adrptrackshbs + j + 1] << 8);
			if (trackend != 0) break;
			i++;	//continue from the next and skip the omitted one
		}
		int tracklen = trackend - trackdata;
		//
		BOOL r;
		r = g_Tracks.AtaToTrack(mem + trackdata, tracklen, track);
		if (!r) return 0; //some problem with the track => END
		trackloaded[track] = 1;
	}

	//decoded song
	BOOL r;
	r = AtaToSong(mem + adrsong, lensong, adrsong);
	if (!r) return 0; //some problem with the song => END

	return 1;
}

//---

BOOL CSong::PlayPressedTonesInit()
{
	for (int t = 0; t < SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t, -1, -1, -1);
	return 1;
}

BOOL CSong::SetPlayPressedTonesSilence()
{
	for (int t = 0; t < SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t, -1, -1, 0);
	return 1;
}

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


void CSong::ActiveInstrSet(int instr)
{
	g_Instruments.MemorizeOctaveAndVolume(m_activeinstr, m_octave, m_volume);
	m_activeinstr = instr;
	g_Instruments.RememberOctaveAndVolume(m_activeinstr, m_octave, m_volume);
}



BOOL CSong::TrackUp(int lines)
{
	if (m_play && m_followplay)	//prevents moving at all during play+follow
		return 0;
	g_Undo.Separator();
	m_trackactiveline -= lines;
	if (m_trackactiveline < 0)
	{
		if (ISBLOCKSELECTED)
		{
			m_trackactiveline = 0;	//prevent moving anywhere else
			return 1;
		}
		if (g_keyboard_updowncontinue)
		{
			BLOCKDESELECT;
			SongUp();
		}
		m_trackactiveline = m_trackactiveline + TrackGetLastLine() + 1;
		if (m_trackactiveline < 0 || TrackGetGoLine() >= 0) m_trackactiveline = m_trackactiveline + g_Tracks.m_maxtracklen;
		if (m_trackactiveline > g_Tracks.m_maxtracklen) m_trackactiveline = m_trackactiveline - TrackGetLastLine() - 1;
	}
	return 1;
}

BOOL CSong::TrackDown(int lines, BOOL stoponlastline)
{
	if (m_play && m_followplay)	//prevents moving at all during play+follow
		return 0;
	if (!g_keyboard_updowncontinue && stoponlastline && m_trackactiveline + lines > TrackGetLastLine()) return 0;
	g_Undo.Separator();
	m_trackactiveline += lines;
	if (SongGetActiveTrack() >= 0 && TrackGetGoLine() < 0)
	{
		int trlen = TrackGetLastLine() + 1;
		if (m_trackactiveline >= trlen)
		{
			if (ISBLOCKSELECTED)
			{
				m_trackactiveline = trlen - 1;	//prevent moving anywhere else
				return 1;
			}
			m_trackactiveline = m_trackactiveline % trlen;
			if (g_keyboard_updowncontinue)
			{
				BLOCKDESELECT;
				SongDown();
			}
		}
	}
	else
	{
		if (m_trackactiveline >= g_Tracks.m_maxtracklen)
		{
			if (ISBLOCKSELECTED)
			{
				m_trackactiveline = g_Tracks.m_maxtracklen - 1;	//prevent moving anywhere else
				return 1;
			}
			m_trackactiveline = m_trackactiveline % g_Tracks.m_maxtracklen;
			if (g_keyboard_updowncontinue)
			{
				BLOCKDESELECT;
				SongDown();
			}
		}
	}
	return 1;
}

BOOL CSong::TrackLeft(BOOL column)
{
	g_Undo.Separator();
	if (column || m_trackactiveline > TrackGetLastLine()) goto track_leftcolumn;
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

BOOL CSong::TrackRight(BOOL column)
{
	g_Undo.Separator();
	if (column || m_trackactiveline > TrackGetLastLine()) goto track_rightcolumn;
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

void CSong::TrackGetLoopingNoteInstrVol(int track, int& note, int& instr, int& vol)
{
	//set the current visible note to a possible goto loop
	int line, len, go;
	len = g_Tracks.GetLastLine(track) + 1;
	go = g_Tracks.GetGoLine(track);
	if (m_trackactiveline < len)
		line = m_trackactiveline;
	else
	{
		if (go >= 0)
			line = (m_trackactiveline - len) % (go - len) + go;
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

int* CSong::GetUECursor(int part)
{
	int* cursor;
	switch (part)
	{
		case PART_TRACKS:
			cursor = new int[4];
			cursor[0] = m_songactiveline;
			cursor[1] = m_trackactiveline;
			cursor[2] = m_trackactivecol;
			cursor[3] = m_trackactivecur;
			break;

		case PART_SONG:
			cursor = new int[2];
			cursor[0] = m_songactiveline;
			cursor[1] = m_trackactivecol;
			break;

		case PART_INSTRUMENTS:
		{
			cursor = new int[6];
			cursor[0] = m_activeinstr;
			TInstrument* in = &(g_Instruments.m_instr[m_activeinstr]);
			cursor[1] = in->act;
			cursor[2] = in->activeenvx;
			cursor[3] = in->activeenvy;
			cursor[4] = in->activepar;
			cursor[5] = in->activetab;
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
			m_songactiveline = cursor[0];
			m_trackactiveline = cursor[1];
			m_trackactivecol = cursor[2];
			m_trackactivecur = cursor[3];
			g_activepart = g_active_ti = PART_TRACKS;
			break;

		case PART_SONG:
			m_songactiveline = cursor[0];
			m_trackactivecol = cursor[1];
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



BOOL CSong::SongUp()
{
	BLOCKDESELECT;
	g_Undo.Separator();
	m_songactiveline--;
	if (m_songactiveline < 0) m_songactiveline = SONGLEN - 1;

	if (m_play && m_followplay)
	{
		int isgo = (m_songgo[m_songactiveline] >= 0) ? 1 : 0;
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline -= isgo;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongDown()
{
	BLOCKDESELECT;
	g_Undo.Separator();
	m_songactiveline++;
	if (m_songactiveline >= SONGLEN) m_songactiveline = 0;

	if (m_play && m_followplay)
	{
		int isgo = (m_songgo[m_songactiveline] >= 0) ? 1 : 0;
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline += isgo;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongSubsongPrev()
{
	g_Undo.Separator();
	int i;
	i = m_songactiveline - 1;
	if (m_trackactiveline == 0) i--;	//is on 0. line => search for 1 songline driv
	for (; i >= 0; i--)
	{
		if (m_songgo[i] >= 0)
		{
			m_songactiveline = i + 1;
			break;
		}
	}
	if (i < 0) m_songactiveline = 0;
	m_trackactiveline = 0;
	if (m_play && m_followplay)
	{
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongSubsongNext()
{
	g_Undo.Separator();
	int i;
	for (i = m_songactiveline; i < SONGLEN; i++)
	{
		if (m_songgo[i] >= 0)
		{
			if (i < (SONGLEN - 1))
				m_songactiveline = i + 1;
			else
				m_songactiveline = SONGLEN - 1; //Goto on the last songline (=> it is not possible to set a line below it!)
			m_trackactiveline = 0;
			break;
		}
	}
	if (m_play && m_followplay)
	{
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongTrackSet(int t)
{
	if (t >= -1 && t < TRACKSNUM)
	{
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	return 1;
}

BOOL CSong::SongTrackSetByNum(int num)
{
	int i;
	if (m_songgo[m_songactiveline] < 0) // GO ?
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
		i = m_songgo[m_songactiveline];
		if (i < 0) i = 0;
		i &= 0x0f;	//just the lower digit
		i = (i << 4) | num;
		if (i >= SONGLEN) i &= 0x0f;
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_songactiveline] = i;
		return 1;
	}
}

BOOL CSong::SongTrackDec()
{
	if (m_songgo[m_songactiveline] < 0)
	{
		int t = m_song[m_songactiveline][m_trackactivecol] - 1;
		if (t < -1) t = TRACKSNUM - 1;
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_songactiveline] - 1;
		if (g < 0) g = SONGLEN - 1;
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_songactiveline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackInc()
{
	if (m_songgo[m_songactiveline] < 0)
	{
		int t = m_song[m_songactiveline][m_trackactivecol] + 1;
		if (t >= TRACKSNUM) t = -1;
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_songactiveline] + 1;
		if (g >= SONGLEN) g = 0;
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGGO);
		m_songgo[m_songactiveline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackEmpty()
{
	g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGTRACK);
	m_song[m_songactiveline][m_trackactivecol] = -1;
	return 1;
}

BOOL CSong::SongTrackGoOnOff()
{
	//GO on/off
	g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGGO);
	m_songgo[m_songactiveline] = (m_songgo[m_songactiveline] < 0) ? 0 : -1;
	return 1;
}

BOOL CSong::SongInsertLine(int line)
{
	g_Undo.ChangeSong(line, m_trackactivecol, UETYPE_SONGDATA, 0);
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
	if (IsBookmark() && m_bookmark.songline >= line)
	{
		m_bookmark.songline++;
		if (m_bookmark.songline >= SONGLEN) ClearBookmark(); //just pushed the bookmark out of the song => cancel the bookmark
	}
	return 1;
}

BOOL CSong::SongDeleteLine(int line)
{
	g_Undo.ChangeSong(line, m_trackactivecol, UETYPE_SONGDATA, 0);
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
	if (IsBookmark() && m_bookmark.songline >= line)
	{
		m_bookmark.songline--;
		if (m_bookmark.songline < line) ClearBookmark(); //just deleted the songline with the bookmark
	}
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
						ModifyTrack(g_Tracks.GetTrack(d), 0, TRACKLEN - 1, -1, dlg.m_tuning, 0, dlg.m_volumep);
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

	g_Undo.ChangeSong(line, m_trackactivecol, UETYPE_SONGTRACK, 0);

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

	g_Undo.ChangeTrack(k, m_trackactiveline, UETYPE_TRACKDATA, 1);

	//copies source track act to k
	TrackCopyFromTo(act, k);

	m_song[line][cl] = k;

	return 1;
}


//--clipboard functions

void CSong::TrackCopy()
{
	int i = SongGetActiveTrack();
	if (i < 0 || i >= TRACKSNUM) return;

	TTrack& at = *g_Tracks.GetTrack(i);
	TTrack& tot = g_TrackClipboard.m_trackcopy;

	memcpy((void*)(&tot), (void*)(&at), sizeof(TTrack));
}

void CSong::TrackPaste()
{
	if (g_TrackClipboard.m_trackcopy.len <= 0) return;
	int i = SongGetActiveTrack();
	if (i < 0 || i >= TRACKSNUM) return;

	TTrack& fro = g_TrackClipboard.m_trackcopy;
	TTrack& at = *g_Tracks.GetTrack(i);

	memcpy((void*)(&at), (void*)(&fro), sizeof(TTrack));
}

void CSong::TrackDelete()
{
	int i = SongGetActiveTrack();
	if (i < 0 || i >= TRACKSNUM) return;

	g_Tracks.ClearTrack(i);
}

void CSong::TrackCut()
{
	TrackCopy();
	TrackDelete();
}

void CSong::TrackCopyFromTo(int fromtrack, int totrack)
{
	if (fromtrack < 0 || fromtrack >= TRACKSNUM) return;
	if (totrack < 0 || totrack >= TRACKSNUM) return;

	TTrack& at = *g_Tracks.GetTrack(fromtrack);
	TTrack& tot = *g_Tracks.GetTrack(totrack);

	memcpy((void*)(&tot), (void*)(&at), sizeof(TTrack));
}

void CSong::BlockPaste(int special)
{
	g_Undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA, 1);
	int lines = g_TrackClipboard.BlockPasteToTrack(SongGetActiveTrack(), m_trackactiveline, special);
	if (lines > 0)
	{
		int lastl = m_trackactiveline + lines - 1;
		//resets the beginning of the block to this location
		g_TrackClipboard.BlockDeselect();
		g_TrackClipboard.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
		g_TrackClipboard.BlockSetEnd(lastl);
		//moves the current line to the last bottom row of the pasted block
		m_trackactiveline = lastl;
	}
}

void CSong::InstrCopy()
{
	int i = GetActiveInstr();
	TInstrument& ai = g_Instruments.m_instr[i];
	memcpy((void*)(&m_instrclipboard), (void*)(&ai), sizeof(TInstrument));
}

void CSong::InstrPaste(int special)
{
	if (m_instrclipboard.act < 0) return;	//he has never been filled with anything

	int i = GetActiveInstr();

	g_Undo.ChangeInstrument(i, 0, UETYPE_INSTRDATA, 1);

	TInstrument& ai = g_Instruments.m_instr[i];

	Atari_InstrumentTurnOff(i); //turns off this instrument on all channels

	int x, y;
	BOOL bl = 0, br = 0, ep = 0;
	BOOL bltor = 0, brtol = 0;

	switch (special)
	{
		case 0: //normal paste
			memcpy((void*)(&ai), (void*)(&m_instrclipboard), sizeof(TInstrument));
			ai.act = ai.activenam = 0; //so that the cursor is at the beginning of the instrument name
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
			for (x = 0; x <= m_instrclipboard.par[PAR_ENVLEN]; x++)
			{
				if (br) ai.env[x][ENV_VOLUMER] = m_instrclipboard.env[x][ENV_VOLUMER];
				if (bl) ai.env[x][ENV_VOLUMEL] = m_instrclipboard.env[x][ENV_VOLUMEL];
				if (bltor) ai.env[x][ENV_VOLUMER] = m_instrclipboard.env[x][ENV_VOLUMEL];
				if (brtol) ai.env[x][ENV_VOLUMEL] = m_instrclipboard.env[x][ENV_VOLUMER];
				if (ep)
				{
					for (y = ENV_DISTORTION; y < ENVROWS; y++) ai.env[x][y] = m_instrclipboard.env[x][y];
				}
			}
			ai.par[PAR_ENVLEN] = m_instrclipboard.par[PAR_ENVLEN];
			ai.par[PAR_ENVGO] = m_instrclipboard.par[PAR_ENVGO];
			ai.activeenvx = 0;
			break;

		case 5: //TABLE
			for (x = 0; x <= m_instrclipboard.par[PAR_TABLEN]; x++) ai.tab[x] = m_instrclipboard.tab[x];
			ai.par[PAR_TABLEN] = m_instrclipboard.par[PAR_TABLEN];
			ai.par[PAR_TABGO] = m_instrclipboard.par[PAR_TABGO];
			ai.activetab = 0;
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
			int sx = m_instrclipboard.par[PAR_ENVLEN] + 1;
			if (ai.activeenvx + sx > ENVCOLS) sx = ENVCOLS - ai.activeenvx;
			for (x = ENVCOLS - 2; x >= ai.activeenvx; x--) //offset
			{
				int i = x + sx;
				if (i >= ENVCOLS) continue;
				for (y = 0; y < ENVROWS; y++) ai.env[i][y] = ai.env[x][y];
			}
			for (x = 0; x < sx; x++) //insertion
			{
				int i = ai.activeenvx + x;
				for (y = 0; y < ENVROWS; y++) ai.env[i][y] = m_instrclipboard.env[x][y];
			}
			int i = ai.par[PAR_ENVLEN] + sx;
			if (i >= ENVCOLS) i = ENVCOLS - 1;
			ai.par[PAR_ENVLEN] = i;
			if (ai.par[PAR_ENVGO] > ai.activeenvx)
			{
				i = ai.par[PAR_ENVGO] + sx;
				if (i >= ENVCOLS) i = ENVCOLS - 1;
				ai.par[PAR_ENVGO] = i;
			}
			i = ai.activeenvx + sx;
			if (i >= ENVCOLS) i = ENVCOLS - 1;
			ai.activeenvx = i;
			break;

	}
	g_Instruments.ModificationInstrument(i); //write to Atari RAM
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
	if (instr < 0 || instr >= INSTRSNUM) return;
	if (instrto < instr || instr >= INSTRSNUM) instrto = instr;

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
	for (i = 0; i < TRACKSNUM; i++)
	{
		inttrack = 0;
		TTrack& at = *g_Tracks.GetTrack(i);
		ain = -1;
		for (j = 0; j < at.len; j++)
		{
			if (at.instr[j] >= 0) ain = at.instr[j];
			if (ain >= instr && ain <= instrto)
			{
				inttrack = 1;
				if (ain > into) into = ain;
				if (ain < infrom) infrom = ain;
				int note = at.note[j];
				if (note >= 0 && note < NOTESNUM)
				{
					globallytimes++; //some note with this instrument => started
					withnote[note]++;
					if (note > maxnote) maxnote = note;
					if (note < minnote) minnote = note;
				}
				int vol = at.volume[j];
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

int CSong::InstrChange(int instr)
{
	//Change all the instrument occurences.
	if (instr < 0 || instr >= INSTRSNUM) return 0;

	CInstrumentChangeDlg dlg;
	dlg.m_song = this;
	dlg.m_combo9 = dlg.m_combo11 = instr;
	dlg.m_combo10 = dlg.m_combo12 = instr;
	dlg.m_onlytrack = SongGetActiveTrack();
	dlg.m_onlysonglinefrom = dlg.m_onlysonglineto = SongGetActiveLine();

	if (dlg.DoModal() == IDOK)
	{
		//hide all tracks and the whole song
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, -1);
		g_Undo.ChangeSong(0, 0, UETYPE_SONGDATA, 1);

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

		int i, j, t, r;

		int onlytrack = dlg.m_onlytrack;

		//!!!!!!!!!!!!!!!!!!!!!
		int onlychannels = dlg.m_onlychannels;
		int onlysonglinefrom = dlg.m_onlysonglinefrom;
		int onlysonglineto = dlg.m_onlysonglineto;

		unsigned char track_yn[TRACKSNUM];
		memset(track_yn, 0, TRACKSNUM);
		int track_column[TRACKSNUM];	//the first occurrence in the selected area of the song
		int track_line[TRACKSNUM];		//the first occurrence in the selected area of the song
		for (i = 0; i < TRACKSNUM; i++) track_column[i] = track_line[i] = -1; //init

		int onlysomething = 0;
		int trackcreated = 0; //number of newly created songs
		int songchanges = 0;	//number of changes in the song

		if (onlychannels >= 0 || (onlysonglinefrom >= 0 && onlysonglineto >= 0))
		{
			if (onlychannels <= 0) onlychannels = 0xff; //all
			if (onlysonglinefrom < 0) onlysonglinefrom = 0; //from the beginning
			if (onlysonglineto < 0) onlysonglineto = SONGLEN - 1; //to the end
			onlysomething = 1;
			unsigned char r;
			for (j = 0; j < SONGLEN; j++)
			{
				if (m_songgo[j] >= 0) continue; //there is a goto
				for (i = 0; i < g_tracks4_8; i++)
				{
					t = m_song[j][i];
					if (t < 0 || t >= TRACKSNUM) continue;
					r = (onlychannels & (1 << i)) && j >= onlysonglinefrom && j <= onlysonglineto;
					track_yn[t] |= (r) ? 1 : 2;	//1=yes, 2=no, 3=yesno (copy)
					if (r && track_column[t] < 0)
					{
						//the first occurrence in the selected area of the song
						track_column[t] = i;
						track_line[t] = j;
					}
				}
			}
		}
		else
			if (onlytrack >= 0)
			{
				track_yn[onlytrack] = 1;	//1=yes
				onlysomething = 1;
			}

		if (dnoteto >= NOTESNUM) dnoteto = dnotefrom + (snoteto - snotefrom);
		if (dvolmax >= 16) dvolmax = dvolmin + (svolmax - svolmin);
		if (dinstrto >= INSTRSNUM) dinstrto = dinstrfrom + (sinstrto - sinstrfrom);

		double notecoef;
		if (snoteto - snotefrom > 0)
			notecoef = (double)(dnoteto - dnotefrom) / (snoteto - snotefrom);
		else
			notecoef = 0;

		double volcoef;
		if (svolmax - svolmin > 0)
			volcoef = (double)(dvolmax - dvolmin) / (svolmax - svolmin);
		else
			volcoef = 0;

		double instrcoef;
		if (sinstrto - sinstrfrom > 0)
			instrcoef = (double)(dinstrto - dinstrfrom) / (sinstrto - sinstrfrom);
		else
			instrcoef = 0;

		int lasti, lastn, changes;
		int track_changeto[TRACKSNUM];

		for (i = 0; i < TRACKSNUM; i++)
		{
			track_changeto[i] = -1; //initialise
			if (onlysomething && ((track_yn[i] & 1) != 1)) continue; //it wants to change only some and this one is not

			TTrack& st = *g_Tracks.GetTrack(i);
			TTrack at; //destination track
			//make a copy
			memcpy((void*)(&at), (void*)(&st), sizeof(TTrack));
			changes = 0;
			lasti = lastn = -1;
			for (j = 0; j < at.len; j++)
			{
				if (at.instr[j] >= 0) lasti = at.instr[j];
				if (at.note[j] >= 0) lastn = at.note[j];
				if (lasti >= sinstrfrom
					&& lasti <= sinstrto
					&& lastn >= snotefrom
					&& lastn <= snoteto
					&& at.volume[j] >= svolmin
					&& at.volume[j] <= svolmax)
				{
					if (at.note[j] >= 0)
					{
						int note = dnotefrom + (int)((double)(at.note[j] - snotefrom) * notecoef + 0.5);
						while (note >= NOTESNUM) note -= 12;
						if (note != at.note[j])
						{
							at.note[j] = note;
							changes = 1;
						}
					}
					if (at.instr[j] >= 0)
					{
						int ins = dinstrfrom + (int)((double)(at.instr[j] - sinstrfrom) * instrcoef + 0.5);
						while (ins >= INSTRSNUM) ins = INSTRSNUM - 1;
						if (ins != at.instr[j])
						{
							at.instr[j] = ins;
							changes = 1;
						}
					}
					int vol = dvolmin + (int)((double)(at.volume[j] - svolmin) * volcoef + 0.5);
					if (vol > 15) vol = 15;
					if (vol != at.volume[j])
					{
						at.volume[j] = vol;
						changes = 1;
					}
				}
			}
			if (changes) //made some changes
			{
				if (track_yn[i] & 2)
				{
					//the track occurs both inside and outside the area
					//create a new track
					int k;
					BYTE tracks[TRACKSNUM];
					memset(tracks, 0, TRACKSNUM); //init
					MarkTF_USED(tracks);
					MarkTF_NOEMPTY(tracks);
					k = FindNearTrackBySongLineAndColumn(track_line[i], track_column[i], tracks);
					if (k < 0)
					{
						MessageBox(g_hwnd, "There isn't any empty unused track in song.", "Error", MB_ICONERROR);
						//UpdateShiftControlKeys();
						return 0;
					}
					//copies the changed copy (at) to the new track (nt)
					TTrack* nt = g_Tracks.GetTrack(k);
					memcpy((void*)nt, (void*)(&at), sizeof(TTrack));
					trackcreated++;
					//at least once put it in the song (due to the search in the song used tracks)
					m_song[track_line[i]][track_column[i]] = k;
					songchanges++;
					//will change all occurrences
					track_changeto[i] = k;
				}
				else
				{
					//occurs only within the area to copy, the reduced copy (data) to the original track (st)
					memcpy((void*)(&st), (void*)(&at), sizeof(TTrack));
				}
			}

		}
		//subsequent changes in the song
		if (onlysomething)
		{
			for (j = 0; j < SONGLEN; j++)
			{
				if (m_songgo[j] >= 0) continue; //there is a goto
				for (i = 0; i < g_tracks4_8; i++)
				{
					t = m_song[j][i];
					if (t < 0 || t >= TRACKSNUM) continue;
					r = (onlychannels & (1 << i)) && j >= onlysonglinefrom && j <= onlysonglineto;
					if (r && track_changeto[t] >= 0)
					{
						m_song[j][i] = track_changeto[t];
						songchanges++;
					}
				}
			}
		}
		if (trackcreated > 0 || songchanges > 0)
		{
			CString s;
			s.Format("Instrument changes implications:\nNew tracks created: %u\nChanges in song: %u", trackcreated, songchanges);
			MessageBox(g_hwnd, (LPCTSTR)s, "Instrument changes", MB_ICONINFORMATION);
		}
		return 1;
	}
	return 0;
}

void CSong::TrackInfo(int track)
{
	if (track < 0 || track >= TRACKSNUM) return;

	static char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };

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
	for (int i = 0; i < g_tracks4_8; i++) m_songlineclipboard[i] = m_song[m_songactiveline][i];
	m_songgoclipboard = m_songgo[m_songactiveline];
}

void CSong::SongPasteLine()
{
	if (m_songgoclipboard < -1) return;
	g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGDATA);
	for (int i = 0; i < g_tracks4_8; i++) m_song[m_songactiveline][i] = m_songlineclipboard[i];
	m_songgo[m_songactiveline] = m_songgoclipboard;
}

void CSong::SongClearLine()
{
	g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGDATA);
	for (int i = 0; i < g_tracks4_8; i++) m_song[m_songactiveline][i] = -1;
	m_songgo[m_songactiveline] = -1;
}

void CSong::TracksOrderChange()
{
	Stop();	//stop the sound first
	CSongTracksOrderDlg dlg;
	dlg.m_songlinefrom.Format("%02X", m_TracksOrderChange_songlinefrom);
	dlg.m_songlineto.Format("%02X", m_TracksOrderChange_songlineto);
	if (dlg.DoModal() == IDOK)
	{
		g_Undo.ChangeSong(m_songactiveline, m_trackactivecol, UETYPE_SONGDATA, 1);

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
		if (m_trackactivecol >= 4) { m_trackactivecol = 3; m_trackactivecur = 0; }
		g_tracks4_8 = 4;
		for (i = 0; i < SONGLEN; i++)
		{
			for (j = 4; j < 8; j++) m_song[i][j] = -1;
		}
	}
	else
		if (tracks4_8 == 8) g_tracks4_8 = 8;
}

int CSong::GetEffectiveMaxtracklen()
{
	//calculate the largest track length used
	int so, i, max = 1;
	for (so = 0; so < SONGLEN; so++)
	{
		if (m_songgo[so] >= 0) continue; //go to line is ignored
		int min = g_Tracks.m_maxtracklen;
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
	int min = g_Tracks.m_maxtracklen;
	int p = 0;

	if (m_songgo[so] >= 0) return 0; //go to line is ignored

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
	if (maxtracklen <= 0 || maxtracklen > TRACKLEN) return;
	//
	int i, j;
	for (i = 0; i < TRACKSNUM; i++)
	{
		TTrack& tt = *g_Tracks.GetTrack(i);
		//clear
		for (j = tt.len; j < TRACKLEN; j++)
		{
			tt.note[j] = tt.instr[j] = tt.volume[j] = tt.speed[j] = -1;
		}
		//
		if (tt.len >= maxtracklen)
		{
			tt.go = -1; //cancel GO
			tt.len = maxtracklen; //adjust length
		}
	}
	if (m_trackactiveline >= maxtracklen) m_trackactiveline = maxtracklen - 1;
	if (m_trackplayline >= maxtracklen) m_trackplayline = maxtracklen - 1;
	g_Tracks.m_maxtracklen = maxtracklen;
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
	int tracklen[TRACKSNUM];
	BOOL trackused[TRACKSNUM];
	for (i = 0; i < TRACKSNUM; i++)
	{
		//initialise
		tracklen[i] = -1;
		trackused[i] = 0;
	}

	for (int sline = 0; sline < SONGLEN; sline++)
	{
		if (m_songgo[sline] >= 0) continue;	//goto line is ignored

		int nejkratsi = g_Tracks.m_maxtracklen;
		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];
			if (n < 0 || n >= TRACKSNUM) continue;	//--
			trackused[n] = 1;
			TTrack& tr = *g_Tracks.GetTrack(n);
			if (tr.go >= 0) continue;	//there is a loop => it has a maximum length
			if (tr.len < nejkratsi) nejkratsi = tr.len;
		}

		//"nejkratsi" is the shortest track in this song line
		for (ch = 0; ch < g_tracks4_8; ch++)
		{
			int n = m_song[sline][ch];
			if (n < 0 || n >= TRACKSNUM) continue;	//--
			if (tracklen[n] < nejkratsi) tracklen[n] = nejkratsi; //if it needs a longer size, it will expand to the length it needs
		}
	}

	int ttracks = 0, tbeats = 0;
	//and now it cuts those tracks
	for (i = 0; i < TRACKSNUM; i++)
	{
		int nlen = tracklen[i];
		if (nlen < 1) continue;	//if they don't have the length of at least 1 they are skipped
		TTrack& tr = *g_Tracks.GetTrack(i);
		if (tr.go < 0)
		{
			//there is no loop
			if (nlen < tr.len)
			{
				//for what must cut, is there anything at all?
				for (j = nlen; j < tr.len; j++)
				{
					if (tr.note[j] >= 0 || tr.instr[j] >= 0 || tr.volume[j] >= 0 || tr.speed[j] >= 0)
					{
						//Yeah, there's something, so cut it
						ttracks++;
						tbeats += tr.len - nlen;
						tr.len = nlen; //cut what is not needed
						break;
					}
				}
				//there is no break;
			}
		}
		else
		{
			//there is a loop
			if (tr.len >= nlen)	//the beginning of the loop is further than the required track length
			{
				ttracks++;
				tbeats += tr.len - nlen;
				tr.len = nlen;	//short track
				tr.go = -1;		//cancel loop
			}
		}
	}

	//delete empty tracks not used in the song
	int ctracks = 0;
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

	//physical data transfer in tracks
	TTrack buft;
	for (i = 0; i < order; i++)
	{
		int n = movetrackto[i];	// swap i <--> n
		if (n == i) continue;		//they are the same, so they don't have to shuffle anything
		memcpy((void*)(&buft), (void*)(g_Tracks.GetTrack(i)), sizeof(TTrack));	// i -> buffer
		memcpy((void*)(g_Tracks.GetTrack(i)), (void*)(g_Tracks.GetTrack(n)), sizeof(TTrack)); // n -> i
		memcpy((void*)(g_Tracks.GetTrack(n)), (void*)(&buft), sizeof(TTrack));	// buffer -> n
		//
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

	for (i = 0; i < INSTRSNUM; i++) instrused[i] = 0;
	for (i = 0; i < TRACKSNUM; i++)
	{
		TTrack& tr = *g_Tracks.GetTrack(i);
		int nlen = tr.len;
		for (j = 0; j < nlen; j++)
		{
			t = tr.instr[j];
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
			if (g_Instruments.CalculateNoEmpty(i)) clearedinstruments++;	//is it empty? yes => it will be deleted
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

	for (i = 0; i < INSTRSNUM; i++) moveinstrfrom[i] = moveinstrto[i] = -1;

	int order = 0;

	//analyse all tracks
	for (i = 0; i < TRACKSNUM; i++)
	{
		TTrack& tr = *g_Tracks.GetTrack(i);
		int tlen = tr.len;
		for (j = 0; j < tlen; j++)
		{
			ins = tr.instr[j];
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
		if (moveinstrfrom[i] < 0 && g_Instruments.CalculateNoEmpty(i))
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
					memcpy((void*)(&g_Instruments.m_instr[di]), (void*)(&g_Instruments.m_instr[i]), sizeof(TInstrument));
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
				memcpy((void*)(&bufi), (void*)(&g_Instruments.m_instr[i]), sizeof(TInstrument)); // i -> buffer
				memcpy((void*)(&g_Instruments.m_instr[i]), (void*)(&g_Instruments.m_instr[n]), sizeof(TInstrument)); // n -> i
				memcpy((void*)(&g_Instruments.m_instr[n]), (void*)(&bufi), sizeof(TInstrument)); // buffer -> n
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
							memcpy((void*)(&bufi), (void*)(&g_Instruments.m_instr[j]), sizeof(TInstrument)); // j -> buffer
							memcpy((void*)(&g_Instruments.m_instr[j]), (void*)(&g_Instruments.m_instr[k]), sizeof(TInstrument)); // k -> j
							memcpy((void*)(&g_Instruments.m_instr[k]), (void*)(&bufi), sizeof(TInstrument)); // buffer -> k
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
		TTrack& tr = *g_Tracks.GetTrack(i);
		int tlen = tr.len;
		for (j = 0; j < tlen; j++)
		{
			ins = tr.instr[j];
			if (ins < 0 || ins >= INSTRSNUM) continue;
			tr.instr[j] = moveinstrfrom[ins];
		}
	}

	//and finally write all the instruments in Atari memory
	for (i = 0; i < INSTRSNUM; i++) g_Instruments.ModificationInstrument(i); //writes to Atari

	//Hooray, done
}



//
//--------------------------------------------------------------------------------------
//

BOOL CSong::SetBookmark()
{
	if (m_songactiveline >= 0 && m_songactiveline < SONGLEN
		&& m_trackactiveline >= 0 && m_trackactiveline < g_Tracks.m_maxtracklen
		&& m_speed >= 0)
	{
		m_bookmark.songline = m_songactiveline;
		m_bookmark.trackline = m_trackactiveline;
		m_bookmark.speed = m_speed;
		return 1;
	}
	return 0;
}

BOOL CSong::Play(int mode, BOOL follow, int special)
{
	g_Undo.Separator();

	if (mode == MPLAY_BOOKMARK && !IsBookmark()) return 0; //if there is no bookmark, then nothing.

	if (m_play)
	{
		if (mode != MPLAY_FROM) Stop(); //already playing and wants something other than play from edited pos.
		else
			if (!m_followplay) Stop(); //is playing and wants to play from edited pos. but not followplay
	}

	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;

	switch (mode)
	{
		case MPLAY_SONG: //whole song from the beginning including initialization (due to portamentum etc.)
			Atari_InitRMTRoutine();
			m_songplayline = 0;
			m_trackplayline = 0;
			m_speed = m_mainspeed;
			break;
		case MPLAY_FROM: //song from the current position
			if (m_play && m_followplay) //is playing with follow play
			{
				m_play = MPLAY_FROM;
				m_followplay = follow;
				//g_screenupdate=1;
				return 1;
			}
			m_songplayline = m_songactiveline;
			m_trackplayline = m_trackactiveline;
			break;
		case MPLAY_TRACK: //just the current tracks around
		Play3:
			m_songplayline = m_songactiveline;
			m_trackplayline = (special == 0) ? 0 : m_trackactiveline;
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
				m_songplayline = g_TrackClipboard.m_selsongline;
				m_trackplayline = m_trackplayblockstart = bfro;
				m_trackplayblockend = bto;
			}
			break;
		case MPLAY_BOOKMARK: //from the bookmark
			m_songplayline = m_bookmark.songline;
			m_trackplayline = m_bookmark.trackline;
			//m_speed = m_bookmark.speed; //comment out so bookmark keep the same speed in memory, won't force it to reset it each time
			break;

		case MPLAY_SEEK_NEXT: //from seeking next
			m_songactiveline++;
			if (m_songactiveline > 255) m_songactiveline = 255;
			m_songplayline = m_songactiveline;
			m_trackplayline = m_trackactiveline = 0;
			if (mode == MPLAY_SEEK_NEXT) mode = MPLAY_FROM;
			break;

		case MPLAY_SEEK_PREV: //from seeking prev
			m_songactiveline--;
			if (m_songactiveline < 0) m_songactiveline = 0;
			m_songplayline = m_songactiveline;
			m_trackplayline = m_trackactiveline = 0;
			if (mode == MPLAY_SEEK_PREV) mode = MPLAY_FROM;
			break;

	}

	if (m_songgo[m_songplayline] >= 0)	//there is a goto
	{
		m_songplayline = m_songgo[m_songplayline];	//goto where
		m_trackplayline = 0;							//from the beginning of that track
		if (m_songgo[m_songplayline] >= 0)
		{
			//goto into another goto
			MessageBox(g_hwnd, "There is recursive \"Go to line\" to other \"Go to line\" in song.", "Recursive \"Go to line\"...", MB_ICONSTOP);
			return 0;
		}
	}

	WaitForTimerRoutineProcessed();
	m_followplay = follow;
	g_screenupdate = 1;
	PlayBeat();						//sets m_speeda
	m_speeda++;						//(Original comment by Raster, April 27, 2003) adds 1 to m_speed, for what the real thing will take place in Init
	if (m_followplay)	//cursor following the player
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;
	}
	g_playtime = 0;
	m_play = mode;

	if (SAPRDUMP == 3)	//the SAP-R dumper initialisation flag was set 
	{
		for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset lines play counter first
		if (m_play == MPLAY_BLOCK)
			m_playcount[m_trackplayline] += 1;	//increment the track line play count early, so it will be detected as the selection block loop
		else
			m_playcount[m_songplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point
		SAPRDUMP = 1;	//set the SAPR dumper with the "is currently recording data" flag 
	}
	return 1;
}

BOOL CSong::Stop()
{
	g_Undo.Separator();
	m_play = MPLAY_STOP;
	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;
	SetPlayPressedTonesSilence();
	WaitForTimerRoutineProcessed();	//The Timer Routine will run at least once
	return 1;
}

BOOL CSong::SongPlayNextLine()
{
	m_trackplayline = 0;	//first track pattern line 

	//normal play, play from current position, or play from bookmark => shift to the next line  
	if (m_play == MPLAY_SONG || m_play == MPLAY_FROM || m_play == MPLAY_BOOKMARK)
	{
		m_songplayline++;	//increment the song line by 1
		if (m_songplayline > 255) m_songplayline = 0;	//above 255, roll over to 0
	}

	//when a goto line is encountered, the player will jump right to the defined line and continue playback from that position
	if (m_songgo[m_songplayline] >= 0)	//if a goto line is set here...
		m_songplayline = m_songgo[m_songplayline];	//goto line ?? 

	if (SAPRDUMP == 1)	//the SAPR dumper is running with the "is currently recording data" flag 
	{
		m_playcount[m_songplayline] += 1;	//increment the position counter by 1
		int count = m_playcount[m_songplayline];	//fetch that line play count for the next step

		if (count > 1)	//a value above 1 means a full playback loop has been completed, the line play count incremented twice
		{
			loops++;	//increment the dumper iteration count by 1
			SAPRDUMP = 2;	//set the "write SAP-R data to file" flag
			if (loops == 1)
			{
				for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset the lines play count before the next step 
				m_playcount[m_songplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 
			}
			if (loops == 2)
			{
				ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
				m_play = MPLAY_STOP;	//stop the player
			}
		}
	}
	return 1;
}

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
		tt = m_song[m_songplayline][t];
		if (tt < 0) continue; //--
		tr = g_Tracks.GetTrack(tt);
		len = tr->len;
		go = tr->go;
		if (m_trackplayline >= len)
		{
			if (go >= 0)
				xline = ((m_trackplayline - len) % (len - go)) + go;
			else
			{
				//if it is the end of the track, but it is a block play or the first PlayBeat call (when m_play = 0)
				if (m_play == MPLAY_BLOCK || m_play == MPLAY_STOP) { note[t] = -1; instr[t] = -1; vol[t] = -1; continue; }
				//otherwise a normal predecision to the next line in the song
				SongPlayNextLine();
				goto TrackLine;
			}
		}
		else
			xline = m_trackplayline;

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
			if (n >= 0 && n < NOTESNUM /*&& i>=0 && i<INSTRSNUM*/)		//adjustment for routine compatibility
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

	if (m_play == MPLAY_BLOCK)
	{	//most of this code was copied directly from the songline function, it's literally the same principle but on a trackline basis instead
		if (SAPRDUMP == 1)	//the SAPR dumper is running with the "is currently recording data" flag 
		{
			m_playcount[m_trackplayline] += 1;	//increment the position counter by 1
			int count = m_playcount[m_trackplayline];	//fetch that line play count for the next step
			if (count > 1)	//a value above 1 means a full playback loop has been completed, the line play count incremented twice
			{
				loops++;	//increment the dumper iteration count by 1
				SAPRDUMP = 2;	//set the "write SAP-R data to file" flag
				if (loops == 1)
				{
					for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset the lines play count before the next step 
					m_playcount[m_trackplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 
				}
				if (loops == 2)
				{
					ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
					m_play = MPLAY_STOP;	//stop the player
				}
			}
		}
	}

	return 1;
}

BOOL CSong::PlayVBI()
{
	if (!m_play) return 0;	//not playing

	m_speeda--;
	if (m_speeda > 0) return 0;	//too soon to update

	m_trackplayline++;

	//m_play mode 4 => only plays range in block
	if (m_play == MPLAY_BLOCK && m_trackplayline > m_trackplayblockend) m_trackplayline = m_trackplayblockstart;

	//if none of the tracks end with "end", then it will end when reaching m_maxtracklen
	if (m_trackplayline >= g_Tracks.m_maxtracklen)
		SongPlayNextLine();

	PlayBeat();	//1 pattern track line play

	if (m_speeda == m_speed && m_followplay)	//playing and following the player
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;

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

	g_screenupdate = 1;

	return 1;
}

void CSong::TimerRoutine()
{
	//things that are solved 1x for vbi
	PlayVBI();

	//play tones if there are key presses
	PlayPressedTones();

	//--- Rendered Sound ---//
	g_Pokey.RenderSound1_50(m_instrspeed);		//rendering of a piece of sample (1 / 50s = 20ms), instrspeed

	if (m_play) g_playtime++;					//if the song is currently playing, increment the timer

	//--- NTSC timing hack during playback ---//
	if (!SAPRDUMP && m_play && g_ntsc)
	{
		//the NTSC timing cannot be divided to an integer
		//the optimal timing would be 16.666666667ms, which is typically rounded to 17
		//unfortunately, things run too slow with 17, or too fast 16
		//a good enough compromise for now is to make use of a '17-17-16' miliseconds "groove"
		//this isn't proper, but at least, this makes the timing much closer to the actual thing
		//the only issue with this is that the sound will have very slight jitters during playback 
		if (g_playtime % 3 == 0) ChangeTimer(17);
		else if (g_playtime % 3 == 2) ChangeTimer(16);
	}

	//--- PICTURE DRAWING ---//
	if (g_screena > 0)
		g_screena--;
	else
	{
		if (g_screenupdate) //Does it want to redraw?
		{
			g_invalidatebytimer = 1;
			AfxGetApp()->GetMainWnd()->Invalidate();
		}
	}
	g_timerroutineprocessed = 1;	//TimerRoutine took place
}

BOOL CInstruments::AtaV0ToInstr(unsigned char* ata, int instr)
{
	//OLD INSTRUMENT VERSION
	TInstrument& ai = m_instr[instr];
	int i, j;
	//0-7 table
	for (i = 0; i <= 7; i++) ai.tab[i] = ata[i];
	//8 ;instr len  0-31 *8, table len  0-7  (iiii ittt)
	int* par = ai.par;
	int len = par[PAR_ENVLEN] = ata[8] >> 3;
	par[PAR_TABLEN] = ata[8] & 0x07;
	par[PAR_ENVGO] = ata[9] >> 3;
	par[PAR_TABGO] = ata[9] & 0x07;
	par[PAR_TABTYPE] = ata[10] >> 7;
	par[PAR_TABMODE] = (ata[10] >> 6) & 0x01;
	par[PAR_TABSPD] = ata[10] & 0x3f;
	par[PAR_VSLIDE] = ata[11];
	par[PAR_VMIN] = ata[12] >> 4;
	//par[PAR_POLY9]	= (ata[12]>>1) & 0x01;
	//par[PAR_15KHZ]	= ata[12] & 0x01;
	par[PAR_AUDCTL0] = ata[12] & 0x01;
	par[PAR_AUDCTL1] = 0;
	par[PAR_AUDCTL2] = 0;
	par[PAR_AUDCTL3] = 0;
	par[PAR_AUDCTL4] = 0;
	par[PAR_AUDCTL5] = 0;
	par[PAR_AUDCTL6] = 0;
	par[PAR_AUDCTL7] = (ata[12] >> 1) & 0x01;
	//
	par[PAR_DELAY] = ata[13];
	par[PAR_VIBRATO] = ata[14] & 0x03;
	par[PAR_FSHIFT] = ata[15];
	//
	BOOL stereo = (g_tracks4_8 > 4);
	for (i = 0, j = 16; i <= len; i++, j += 3)
	{
		int* env = (int*)&ai.env[i];
		env[ENV_VOLUMER] = (stereo) ? (ata[j] >> 4) : (ata[j] & 0x0f); //if mono, then VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER] = ata[j + 1] >> 7;
		env[ENV_COMMAND] = (ata[j + 1] >> 4) & 0x07;
		env[ENV_DISTORTION] = ata[j + 1] & 0x0e;	//even numbers 0,2,4, .., 14
		env[ENV_PORTAMENTO] = ata[j + 1] & 0x01;
		env[ENV_X] = ata[j + 2] >> 4;
		env[ENV_Y] = ata[j + 2] & 0x0f;
	}
	return 1;
}

BOOL CInstruments::AtaToInstr(unsigned char* ata, int instr)
{
	TInstrument& ai = m_instr[instr];
	int i, j;

	int tablen, tabgo, envlen, envgo;
	tablen = ata[0] - 12;
	tabgo = ata[1] - 12;
	envlen = (ata[2] - (ata[0] + 1)) / 3;
	envgo = (ata[3] - (ata[0] + 1)) / 3;

	//check the scope of the tables and envelope
	if (tablen >= TABLEN || tabgo > tablen || envlen >= ENVCOLS || envgo > envlen)
	{
		//tables exceeds boundary or table go exceeds tables or envelope length ...
		return 0;
	}

	//only if the ranges are ok, they change the content of the instrument
	int* par = ai.par;
	par[PAR_TABLEN] = tablen;
	par[PAR_TABGO] = tabgo;
	par[PAR_ENVLEN] = envlen;
	par[PAR_ENVGO] = envgo;
	//
	par[PAR_TABTYPE] = ata[4] >> 7;
	par[PAR_TABMODE] = (ata[4] >> 6) & 0x01;
	par[PAR_TABSPD] = ata[4] & 0x3f;
	//
	par[PAR_AUDCTL0] = ata[5] & 0x01;
	par[PAR_AUDCTL1] = (ata[5] >> 1) & 0x01;
	par[PAR_AUDCTL2] = (ata[5] >> 2) & 0x01;
	par[PAR_AUDCTL3] = (ata[5] >> 3) & 0x01;
	par[PAR_AUDCTL4] = (ata[5] >> 4) & 0x01;
	par[PAR_AUDCTL5] = (ata[5] >> 5) & 0x01;
	par[PAR_AUDCTL6] = (ata[5] >> 6) & 0x01;
	par[PAR_AUDCTL7] = (ata[5] >> 7) & 0x01;
	//
	par[PAR_VSLIDE] = ata[6];
	par[PAR_VMIN] = ata[7] >> 4;
	par[PAR_DELAY] = ata[8];
	par[PAR_VIBRATO] = ata[9] & 0x03;
	par[PAR_FSHIFT] = ata[10];

	//0-31 table
	for (i = 0; i <= par[PAR_TABLEN]; i++) ai.tab[i] = ata[12 + i];

	//envelope
	BOOL stereo = (g_tracks4_8 > 4);
	for (i = 0, j = ata[0] + 1; i <= par[PAR_ENVLEN]; i++, j += 3)
	{
		int* env = (int*)&ai.env[i];
		env[ENV_VOLUMER] = (stereo) ? (ata[j] >> 4) : (ata[j] & 0x0f); //if mono, then VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER] = ata[j + 1] >> 7;
		env[ENV_COMMAND] = (ata[j + 1] >> 4) & 0x07;
		env[ENV_DISTORTION] = ata[j + 1] & 0x0e;	//even numbers 0,2,4,...E
		env[ENV_PORTAMENTO] = ata[j + 1] & 0x01;
		env[ENV_X] = ata[j + 2] >> 4;
		env[ENV_Y] = ata[j + 2] & 0x0f;
	}
	return 1;
}


