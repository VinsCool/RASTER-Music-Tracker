//
// R_MUSIC.H
// originally made by Raster, 2002-2009
// reworked by VinsCool, 2021-2022
//

#ifndef RMT_R_MUSIC_
#define RMT_R_MUSIC_

#include <fstream>
using namespace std;

#include "EffectsDlg.h"
#include "xpokey.h"
#include "Tuning.h"

extern BOOL g_changes;

//TODO: add more keys definition to simplify things
#define VK_PAGE_UP		33
#define VK_PAGE_DOWN	34

//TODO: make tuning configuration separate, also add theme configuration in the future
#define CONFIG_FILENAME "rmt.ini"

#define EOL "\x0d\x0a"			//Carriage Return (\r) and Line Feed (\n), for strings used during Exports

#define TRACKS_X 2*8
#define TRACKS_Y 8*16+8
#define	SONG_X	768
#define SONG_Y	16
#define INFO_X	2*8
#define INFO_Y	1*16

#define RMTFORMATVERSION	1	//the version number that is saved into modules, highest means more recent
#define TRACKLEN	256			//drive 128
#define TRACKSNUM	254			//0-253
#define SONGLEN		256
#define SONGTRACKS	8
#define INSTRSNUM	64
#define NOTESNUM	61			//notes 0-60 inclusive
#define MAXVOLUME	15			//maximum volume
#define PARCOUNT	24			//24 instrument parameters
#define ENVCOLS		48			//48 columns in envelope (drive 32) (48 from version 1.25)
#define ENVROWS		8			//8 line (parameter) in the envelope
#define TABLEN		32			//maximum 32 steps in the table

#define INSTRNAMEMAXLEN	32		//maximum length of instrument name
#define SONGNAMEMAXLEN	64		//maximum length of song name
#define TRACKMAXSPEED	256		//maximum speed values, highest the slowest

#define MAXATAINSTRLEN	256		//16+(ENVCOLS*3)	//atari instrument has a maximum of 16 parameters + 32 * 3 bytes envelope
#define MAXATATRACKLEN	256		//atari track has maximum 256 bytes (track index is 0-255)
#define MAXATASONGLEN	SONGTRACKS*SONGLEN	//maximum data size atari song part

#define PARTINFO	0
#define PARTTRACKS	1
#define PARTINSTRS	2
#define PARTSONG	3

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

//Undo operation (one can consume up to 3 records)
#define UNDOSTEPS		100
#define MAXUNDO			(UNDOSTEPS*3+8)	//302	//2 extra separating gap

//-----------------------------------------------------

#define POSGROUPTYPE0_63SIZE 2
#define	UETYPE_NOTEINSTRVOL			1
#define UETYPE_NOTEINSTRVOLSPEED	2
#define	UETYPE_SPEED				3
#define UETYPE_LENGO				4
#define UETYPE_TRACKDATA			5

#define UETYPE_SONGTRACK			33
#define UETYPE_SONGGO				34

#define POSGROUPTYPE64_127SIZE 1
#define UETYPE_SONGDATA				65
#define UETYPE_INSTRDATA			66
#define UETYPE_TRACKSALL			67
#define UETYPE_INSTRSALL			68
#define UETYPE_INFODATA				69

#define POSGROUPTYPE128_191SIZE 3

struct TInfo
{
	char songname[SONGNAMEMAXLEN+1];
	int speed;
	int mainspeed;
	int instrspeed;
	int songnamecur; //to return the cursor to the appropriate position when undo changes in the song name
};

struct TUndoEvent
{
	int part;		//the part in which the editing is performed
	int *cursor;	//cursor
	int type;		//type of changed data
	int *pos;		//position of changed data
	void *data;		//change data
	char separator;	//= 0 accumulate continuous changes, = 1 completed change, = -1 more events for one step
};

//-----------------------------------------------------

class CUndo
{
public:
	CUndo();
	~CUndo();

	void Init(CSong* song);
	void Clear();
	char DeleteEvent(int i);
	char PerformEvent(int i);
	void DropLast();
	
	BOOL Undo();
	BOOL Redo();
	int GetUndoSteps()		{ return m_undosteps; };
	int GetRedoSteps()		{ return m_redosteps; };

	BOOL PosIsEqual(int* pos1, int* pos2, int type);

	void Separator(int sep=1);
	void ChangeTrack(int tracknum,int trackline, int type, char separator=0);
	void ChangeSong(int songline,int trackcol,int type, char separator=0);
	void ChangeInstrument(int instrnum,int paridx,int type, char separator=0);
	void ChangeInfo(int paridx,int type, char separator=0);

private:
	void InsertEvent(TUndoEvent* ue);

	CSong *m_song;
	TUndoEvent *m_uar[MAXUNDO];
	int m_head,m_tail,m_headmax;
	int m_undosteps,m_redosteps;
};

//-----------------------------------------------------

struct TTrack
{
	int len;
	int go;
	int note[TRACKLEN];
	int instr[TRACKLEN];
	int volume[TRACKLEN];
	int speed[TRACKLEN];
};

struct TTracksAll	//for undo
{
	int maxtracklength;
	TTrack tracks[TRACKSNUM];
};

//-----------------------------------------------------

class CTracks
{
public:
	CTracks();
	void InitUndo(CUndo *undo)			{ m_undo = undo; };
	BOOL InitTracks();
	BOOL ClearTrack(int t);
	BOOL IsEmptyTrack(int track);
	BOOL DrawTrack(int col, int x, int y, int tr, int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu = 0);
	BOOL DelNoteInstrVolSpeed(int noteinstrvolspeed,int track,int line);
	BOOL SetNoteInstrVol(int note,int instr,int vol,int track,int line);
	BOOL SetInstr(int instr,int track,int line);
	BOOL SetVol(int vol,int track,int line);
	BOOL SetSpeed(int speed,int track,int line);
	int GetNote(int track,int line)		{ return m_track[track].note[line]; };
	int GetInstr(int track, int line)	{ return m_track[track].instr[line]; };
	int GetVol(int track,int line)		{ return m_track[track].volume[line]; };
	int GetSpeed(int track,int line)	{ return m_track[track].speed[line]; };
	void GetNoteInstrVolSpeed(int* buff, int track, int line) { buff[0]=m_track[track].note[line]; buff[1]=m_track[track].instr[line]; buff[2]=m_track[track].volume[line]; buff[3]=m_track[track].speed[line]; };
	BOOL SetEnd(int track, int line);
	int GetLastLine(int track);
	int GetLength(int track);
	BOOL SetGo(int track, int line);
	int GetGoLine(int track);

	BOOL InsertLine(int track, int line);
	BOOL DeleteLine(int track, int line);

	TTrack* GetTrack(int tr)	{ return &m_track[tr]; };
	void GetTracksAll(TTracksAll* dest_ta);	//{ return (TTracksAll*)&m_track; };
	void SetTracksAll(TTracksAll* src_ta);

	int TrackToAta(int track,unsigned char* dest,int max);
	int TrackToAtaRMF(int track,unsigned char* dest,int max);
	BOOL AtaToTrack(unsigned char* sour,int len,int track);

	int SaveAll(ofstream& ou,int iotype);
	int LoadAll(ifstream& in,int iotype);

	int SaveTrack(int track,ofstream& ou,int iotype);
	int LoadTrack(int track,ifstream& in,int iotype);

	BOOL CalculateNoEmpty(int track);
	BOOL CompareTracks(int track1, int track2);

	int TrackOptimizeVol0(int track);
	int TrackBuildLoop(int track);
	int TrackExpandLoop(int track);
	int TrackExpandLoop(TTrack* ttrack);

	int m_maxtracklen;

private:
	TTrack m_track[TRACKSNUM];
	CUndo *m_undo;
};

//-----------------------------------------------------

struct TInstrInfo
{
	int count;
	int usedintracks;
	int instrfrom,instrto;
	int minnote,maxnote;
	int minvol,maxvol;
};

struct TInstrument
{
	int act;
	char name[INSTRNAMEMAXLEN+1];
	int activenam;
	int par[PARCOUNT];						//[24]
	int activepar;
	int env[ENVCOLS][ENVROWS];				//[32][8]
	int activeenvx,activeenvy;
	int tab[TABLEN];
	int activetab;
	int octave;
	int volume;
};

struct TInstrumentsAll		//for undo
{
	TInstrument instruments[INSTRSNUM];
};

class CInstruments
{
public:
	CInstruments();
	BOOL InitInstruments();
	BOOL ClearInstrument(int it);
	//void RandomInstrument(int it);	

	BOOL DrawInstrument(int it);
	BOOL ModificationInstrument(int it);
	void CheckInstrumentParameters(int instr);
	BOOL RecalculateFlag(int instr);

	BYTE GetFlag(int instr)	{ return m_iflag[instr]; };

	char *GetName(int it)	{ return m_instr[it].name; };

	int SaveAll(ofstream& ou,int iotype);
	int LoadAll(ifstream& in,int iotype);

	int SaveInstrument(int instr, ofstream& ou, int iotype);
	int LoadInstrument(int instr, ifstream& in, int iotype);

	BYTE InstrToAta(int instr,unsigned char *ata,int max);
	BYTE InstrToAtaRMF(int instr,unsigned char *ata,int max);
	BOOL AtaToInstr(unsigned char *ata, int instr);

	BOOL AtaV0ToInstr(unsigned char *ata, int instr);	//Due to the loading of the old version

	BOOL CalculateNoEmpty(int instr);

	BOOL GetInstrArea(int instr, int zone, CRect& rect);
	BOOL CursorGoto(int instr, CPoint point,int pzone);

	void SetEnvVolume(int instr, BOOL right, int px, int py);

	int GetFrequency(int instr,int note);
	int GetNote(int instr,int note);

	void MemorizeOctaveAndVolume(int instr,int oct,int vol);
	void RememberOctaveAndVolume(int instr,int& oct,int& vol);

	TInstrumentsAll* GetInstrumentsAll()	{ return (TInstrumentsAll*)m_instr; };

	TInstrument m_instr[INSTRSNUM];
private:
	BOOL DrawName(int it);
	BOOL DrawPar(int p,int it);
	void DrawEnv(int e,int it);
	BOOL DrawTab(int p,int it);

	BYTE m_iflag[INSTRSNUM];
};

//-----------------------------------------------------

struct TBookmark
{
	int songline;
	int trackline;
	int speed;
};

struct TSong	//due to Undo
{
	int song[SONGLEN][SONGTRACKS];
	int songgo[SONGLEN];					//if> = 0, then GO applies
	TBookmark bookmark;
};

struct TSongTrackAndTrackData
{
	int tracknum;
	TTrack trackdata;
};

class CSong
{
public:
	CSong();
	~CSong();

	void ChangeTimer(int ms);
	BOOL ClearSong(int numoftracks);

	void MidiEvent(DWORD dwParam);

	BOOL DrawSong();
	BOOL DrawTracks();
	BOOL DrawInstrument();
	BOOL DrawInfo();			//top left corner
	BOOL DrawAnalyzer(CDC *pDC);
	BOOL DrawPlaytimecounter(CDC *pDC);

	BOOL InfoKey(int vk,int shift,int control);
	BOOL InfoCursorGotoSongname(int x);
	BOOL InfoCursorGotoSpeed(int x);
	BOOL InfoCursorGotoOctaveSelect(int x, int y);
	BOOL InfoCursorGotoVolumeSelect(int x, int y);
	BOOL InfoCursorGotoInstrumentSelect(int x, int y);

	BOOL InstrKey(int vk,int shift,int control);
	void ActiveInstrSet(int instr) { m_instrs.MemorizeOctaveAndVolume(m_activeinstr,m_octave,m_volume); m_activeinstr = instr; m_instrs.RememberOctaveAndVolume(m_activeinstr,m_octave,m_volume); };
	void ActiveInstrPrev() { m_undo.Separator(); int instr = (m_activeinstr-1) & 0x3f; ActiveInstrSet(instr); };
	void ActiveInstrNext() { m_undo.Separator(); int instr = (m_activeinstr+1) & 0x3f; ActiveInstrSet(instr); };

	int GetActiveInstr()	{ return m_activeinstr; };
	int GetActiveColumn()	{ return m_trackactivecol; };
	int GetActiveLine()		{ return m_trackactiveline; };
	void SetActiveLine(int line)	{ m_trackactiveline = line; };

	BOOL CursorToSpeedColumn();
	BOOL ProveKey(int vk,int shift,int control);
	BOOL TrackKey(int vk,int shift,int control);
	BOOL TrackCursorGoto(CPoint point);
	BOOL TrackUp(int lines);
	BOOL TrackDown(int lines, BOOL stoponlastline=1);
	BOOL TrackLeft(BOOL column=0);
	BOOL TrackRight(BOOL column=0);
	BOOL TrackDelNoteInstrVolSpeed(int noteinstrvolspeed) { return m_tracks.DelNoteInstrVolSpeed(noteinstrvolspeed,SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetNoteActualInstrVol(int note)	{ return m_tracks.SetNoteInstrVol(note,m_activeinstr,m_volume,SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetNoteInstrVol(int note,int instr,int vol)	{ return m_tracks.SetNoteInstrVol(note,instr,vol,SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetInstr(int instr) { return m_tracks.SetInstr(instr,SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetVol(int vol)	{ return m_tracks.SetVol(vol,SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetSpeed(int speed)	{ return m_tracks.SetSpeed(speed,SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetNote()			{ return m_tracks.GetNote(SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetInstr()			{ return m_tracks.GetInstr(SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetVol()			{ return m_tracks.GetVol(SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetSpeed()			{ return m_tracks.GetSpeed(SongGetActiveTrack(),m_trackactiveline); };
	BOOL TrackSetEnd()			{ return m_tracks.SetEnd(SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetLastLine()		{ return m_tracks.GetLastLine(SongGetActiveTrack()); };
	BOOL TrackSetGo()			{ return m_tracks.SetGo(SongGetActiveTrack(),m_trackactiveline); };
	int TrackGetGoLine()		{ return m_tracks.GetGoLine(SongGetActiveTrack()); };

	void TrackGetLoopingNoteInstrVol(int track,int& note,int& instr,int& vol);

	int* GetUECursor(int part);
	void SetUECursor(int part,int* cursor);
	BOOL UECursorIsEqual(int* cursor1, int* cursor2, int part);
	BOOL Undo()				{ return m_undo.Undo(); };
	int	 UndoGetUndoSteps()	{ return m_undo.GetUndoSteps(); };
	BOOL Redo()				{ return m_undo.Redo(); };
	int  UndoGetRedoSteps()	{ return m_undo.GetRedoSteps(); };

	BOOL SongKey(int vk,int shift,int control);
	BOOL SongCursorGoto(CPoint point);
	BOOL SongUp();
	BOOL SongDown();
	BOOL SongSubsongPrev();
	BOOL SongSubsongNext();
	BOOL SongTrackSet(int t);
	BOOL SongTrackSetByNum(int num);
	BOOL SongTrackDec();
	BOOL SongTrackInc();
	BOOL SongTrackEmpty();
	int SongGetActiveTrack() { return (m_songgo[m_songactiveline]>=0)? -1 : m_song[m_songactiveline][m_trackactivecol]; };
	int SongGetTrack(int songline,int trackcol) { return (m_songgo[songline]>=0)? -1 : m_song[songline][trackcol]; };
	int SongGetActiveTrackInColumn(int column) { return m_song[m_songactiveline][column]; };
	int SongGetActiveLine()	 { return m_songactiveline; };
	void SongSetActiveLine(int line) { m_songactiveline=line; };
	
	BOOL SongTrackGoOnOff();
	int SongGetGo()			{ return m_songgo[m_songactiveline]; };
	int SongGetGo(int songline) { return m_songgo[songline]; };
	void SongTrackGoDec()	{ m_songgo[m_songactiveline] = (m_songgo[m_songactiveline]-1) & 0xff; };
	void SongTrackGoInc()	{ m_songgo[m_songactiveline] = (m_songgo[m_songactiveline]+1) & 0xff; };

	BOOL SongInsertLine(int line);
	BOOL SongDeleteLine(int line);
	BOOL SongInsertCopyOrCloneOfSongLines(int& line);
	BOOL SongPrepareNewLine(int& line,int sourceline=-1,BOOL alsoemptycolumns=1);
	int FindNearTrackBySongLineAndColumn(int songline,int column, BYTE *arrayTRACKSNUM);
	BOOL SongPutnewemptyunusedtrack();
	BOOL SongMaketracksduplicate();

	BOOL OctaveUp()		{ if (m_octave<4) { m_octave++; return 1; } else return 0; };
	BOOL OctaveDown()	{ if (m_octave>0) { m_octave--; return 1; } else return 0; };

	BOOL VolumeUp()		{ if (m_volume<MAXVOLUME) { m_volume++; return 1; } else return 0; };
	BOOL VolumeDown()	{ if (m_volume>0) { m_volume--; return 1; } else return 0; };

	void ClearBookmark() { m_bookmark.songline = m_bookmark.trackline = m_bookmark.speed = -1; };
	BOOL IsBookmark()	{ return (m_bookmark.speed>0 && m_bookmark.trackline<m_tracks.m_maxtracklen); };
	BOOL SetBookmark();

	BOOL Play(int mode, BOOL follow, int special=0);
	BOOL Stop();
	BOOL SongPlayNextLine();

	BOOL PlayBeat();
	BOOL PlayVBI();

	BOOL PlayPressedTonesInit();
	BOOL SetPlayPressedTonesTNIV(int t,int n,int i,int v) { m_playptnote[t]=n; m_playptinstr[t]=i; m_playptvolume[t]=v; return 1; }
	BOOL SetPlayPressedTonesV(int t,int v) { m_playptvolume[t]=v; return 1; };
	BOOL SetPlayPressedTonesSilence();
	BOOL PlayPressedTones();

	void TimerRoutine();

	BOOL InitPokey()	{ return m_pokey.InitSound(); };
	BOOL DeInitPokey()	{ return m_pokey.DeInitSound(); };

	void SetRMTTitle();

	void FileOpen(const char *filename = NULL, BOOL warnunsavedchanges=1);
	void FileReload();
	BOOL FileCanBeReloaded()		{ return (m_filename!="") /*&& (!m_fileunsaved)*/ /*&& g_changes*/; };
	int WarnUnsavedChanges();

	void FileSave();
	void FileSaveAs();
	void FileNew();
	void FileImport();
	void FileExportAs();

	void FileInstrumentSave();
	void FileInstrumentLoad();
	void FileTrackSave();
	void FileTrackLoad();

	int SongToAta(unsigned char* dest, int max, int adr);
	BOOL AtaToSong(unsigned char* sour, int len, int adr);

	int Save(ofstream& ou,int iotype);
	int Load(ifstream& in,int iotype);
	int LoadRMT(ifstream& in);

	int ImportTMC(ifstream& in);
	int ImportMOD(ifstream& in);
	int Export(ofstream& ou,int iotype, char* filename=NULL);

	int TestBeforeFileSave();
	int GetSubsongParts(CString& resultstr);

	BOOL ComposeRMTFEATstring(CString& dest, char* filename, BYTE *instrsaved,BYTE* tracksaved, BOOL sfx, BOOL gvf, BOOL nos);

	void MarkTF_USED(BYTE* arrayTRACKSNUM);
	void MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM);

	int MakeModule(unsigned char* mem,int adr,int iotype,BYTE *instrsaved,BYTE* tracksaved);
	int MakeRMFModule(unsigned char* mem,int adr,BYTE *instrsaved,BYTE* tracksaved);
	int DecodeModule(unsigned char* mem,int adrfrom,int adrend,BYTE *instrloaded,BYTE* trackloaded);

	void TrackCopy();
	void TrackPaste();
	void TrackCut();
	void TrackDelete();
	void TrackCopyFromTo(int fromtrack,int totrack);

	void BlockPaste(int special=0);

	void InstrCopy();
	void InstrPaste(int special=0);
	void InstrCut();
	void InstrDelete()			{ m_instrs.ClearInstrument(GetActiveInstr()); };

	void InstrInfo(int instr,TInstrInfo* iinfo=NULL,int instrto=-1);
	int InstrChange(int instr);
	void TrackInfo(int track);

	void SongCopyLine();
	void SongPasteLine();
	void SongClearLine();

	void TracksOrderChange();
	void Songswitch4_8(int tracks4_8);
	int GetEffectiveMaxtracklen();
	void ChangeMaxtracklen(int maxtracklen);
	void TracksAllBuildLoops(int& tracksmodified,int& beatsreduced);
	void TracksAllExpandLoops(int& tracksmodified,int& loopsexpanded);
	void SongClearUnusedTracksAndParts(int& clearedtracks, int& truncatedtracks,int& truncatedbeats);
	
	int SongClearDuplicatedTracks();
	int SongClearUnusedTracks();
	int ClearAllInstrumentsUnusedInAnyTrack();

	void RenumberAllTracks(int type);
	void RenumberAllInstruments(int type);

	CString GetFilename()				{ return m_filename; };
	int GetFiletype()					{ return m_filetype; };

	CXPokey* GetPokey()					{ return &m_pokey; };
	CTracks* GetTracks()				{ return &m_tracks; };
	CInstruments* GetInstruments()		{ return &m_instrs; };
	int (*GetSong())[SONGLEN][SONGTRACKS]		{ return &m_song; };
	int (*GetSongGo())[SONGLEN]					{ return &m_songgo; };
	TBookmark* GetBookmark()			{ return &m_bookmark; };
	CUndo* GetUndo()					{ return &m_undo; };

	int GetPlayMode()					{ return m_play; };

	void GetSongInfoPars(TInfo* info)	{ memcpy(info->songname,m_songname,SONGNAMEMAXLEN); info->speed=m_speed; info->mainspeed=m_mainspeed; info->instrspeed=m_instrspeed; info->songnamecur=m_songnamecur; };
	void SetSongInfoPars(TInfo* info)	{ memcpy(m_songname,info->songname,SONGNAMEMAXLEN); m_speed=info->speed; m_mainspeed=info->mainspeed; m_instrspeed=info->instrspeed; m_songnamecur=info->songnamecur; };

	BOOL volatile m_followplay;
	int volatile m_play;

	int m_playcount[256] = { 0 };	//used for keeping track of loop points, useful for SAP-R recording specifically, maybe more sometime later...

private:
	CXPokey m_pokey;

	CInstruments m_instrs;
	CTracks m_tracks;

	int m_song[SONGLEN][SONGTRACKS];
	int m_songgo[SONGLEN];					//if> = 0, then GO applies

	int m_songactiveline;
	int volatile m_songplayline;

	int m_trackactiveline;
	int volatile m_trackplayline;
	int m_trackactivecol;						//0-7
	int m_trackactivecur;						//0-2

	int m_trackplayblockstart;
	int m_trackplayblockend;

	int m_activeinstr;
	int m_volume;
	int m_octave;

	//MIDI input variables, used for tests through MIDI CH15 
	int m_mod_wheel = 0;
	int m_vol_slider = 0;
	int m_heldkeys = 0;
	int m_midi_distortion = 0;
	BOOL m_ch_offset = 0;

	int m_infoact;
	char m_songname[SONGNAMEMAXLEN+1];
	int m_songnamecur;

	TBookmark m_bookmark;

	int volatile m_mainspeed;
	int volatile m_speed;
	int volatile m_speeda;

	int volatile m_instrspeed;

	int volatile m_quantization_note;
	int volatile m_quantization_instr;
	int volatile m_quantization_vol;

	int m_playptnote[SONGTRACKS];
	int m_playptinstr[SONGTRACKS];
	int m_playptvolume[SONGTRACKS];

	TInstrument m_instrclipboard;
	int m_songlineclipboard[SONGTRACKS];
	int m_songgoclipboard;

	UINT m_timer;

	CString m_filename;
	int m_filetype;
	int m_exporttype;

	int m_TracksOrderChange_songlinefrom; //is defined as a member variable to keep in use
    int m_TracksOrderChange_songlineto;	  //the last values used remain

	CUndo m_undo;
};

//-----------------------------------------------------

class CTrackClipboard
{
public:
	CTrackClipboard::CTrackClipboard();
	void Init(CSong* song);
	void Empty();
	BOOL IsBlockSelected()		{ return (m_selcol>=0); };
	BOOL BlockSetBegin(int col, int track, int line);
	BOOL BlockSetEnd(int line);
	void BlockDeselect();

	int BlockCopyToClipboard();
	int BlockExchangeClipboard();
	int BlockPasteToTrack(int track, int line, int special=0);
	int BlockClear();
	int BlockRestoreFromBackup();

	void GetFromTo(int& from, int& to);
	int	GetCol()				{ return m_selcol; };

	void BlockNoteTransposition(int instr,int addnote);
	void BlockInstrumentChange(int instr,int addinstr);
	void BlockVolumeChange(int instr,int addvol);

	//block effect
	BOOL BlockEffect();

	void BlockAllOnOff();
	void BlockInitBase(int track);

	//block
	int m_selcol;					//0-7
	int m_seltrack;
	int m_selsongline;
	int m_selfrom;
	int m_selto;
	TTrack m_track;

	//block changes
	BOOL m_all;						//TRUE = changes all / FALSE = changes only for the same instrument as the current one
	int m_instrbase;
	int m_changenote;
	int	m_changeinstr;
	int	m_changevolume;
	TTrack m_trackbase;

	TTrack m_trackbackup;

	//copying entire tracks
	TTrack m_trackcopy;

private:
	void ClearTrack();

	CSong* m_song;
};

//-----------------------------------------------------

#endif
