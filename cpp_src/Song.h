#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"

#include "Undo.h"
#include "Instruments.h"
#include "Tracks.h"


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

struct TInfo
{
	char songname[SONG_NAME_MAX_LEN + 1];
	int speed;
	int mainspeed;
	int instrspeed;
	int songnamecur; //to return the cursor to the appropriate position when undo changes in the song name
};

struct TExportMetadata
{
	char songname[SONG_NAME_MAX_LEN + 1];
	CTime currentTime;
	int instrspeed;
	bool isStereo;
	bool isNTSC;
	bool autoRegion;
	bool displayRasterbar;
	int rasterbarColour;
	char atariText[5 * 40];
};


typedef struct
{
	unsigned char mem[65536];				// default RAM size for most 800xl/xe machines

	int targetAddrOfModule;					// Start of RMT module in memory [$4000]
	int firstByteAfterModule;				// Hmm, 1st byte after the RMT module

	BYTE instrumentSavedFlags[INSTRSNUM];
	BYTE trackSavedFlags[TRACKSNUM];

} tExportDescription;

class CSong
{
public:
	CSong();
	~CSong();

	void StopTimer();
	void ChangeTimer(int ms);
	void KillTimer();

	void ClearSong(int numoftracks);

	void MidiEvent(DWORD dwParam);

	// Legacy Draw functions
	void DrawSong();				// Draw the song line info on the right
	void DrawTracks();
	void DrawInstrument();
	void DrawInfo();			//top left corner
	void DrawAnalyzer();
	void DrawPlayTimeCounter();

	// New Draw functions
	void DrawSonglines();
	void DrawSubtuneInfos();
	void DrawRegistersState();
	void DrawPatternEditor();
	void DrawInstrumentEditor();
	void DrawDebugInfos();

	BOOL InfoKey(int vk, int shift, int control);
	BOOL InfoCursorGotoSongname(int x);
	BOOL InfoCursorGotoSpeed(int x);
	BOOL InfoCursorGotoHighlight(int x);
	BOOL InfoCursorGotoOctaveSelect(int x, int y);
	BOOL InfoCursorGotoVolumeSelect(int x, int y);
	BOOL InfoCursorGotoInstrumentSelect(int x, int y);

	BOOL InstrKey(int vk, int shift, int control);
	void ActiveInstrSet(int instr);
	void ActiveInstrPrev() { g_Undo.Separator(); int instr = (m_activeinstr - 1) & 0x3f; ActiveInstrSet(instr); };
	void ActiveInstrNext() { g_Undo.Separator(); int instr = (m_activeinstr + 1) & 0x3f; ActiveInstrSet(instr); };

	int GetActiveInstr() { return m_activeinstr; };
	int GetActiveColumn() { return m_trackactivecol; };
	int GetActiveLine() { return m_trackactiveline; };
	int GetPlayLine() { return m_trackplayline; };
	void SetActiveLine(int line) { m_trackactiveline = line; };
	void SetPlayLine(int line) { m_trackplayline = line; };

	BOOL CursorToSpeedColumn();
	BOOL ProveKey(int vk, int shift, int control);
	BOOL TrackKey(int vk, int shift, int control);
	BOOL TrackCursorGoto(CPoint point);
	BOOL TrackUp(int lines);
	BOOL TrackDown(int lines, BOOL stoponlastline = 1);
	BOOL TrackLeft(BOOL column = 0);
	BOOL TrackRight(BOOL column = 0);
	BOOL TrackDelNoteInstrVolSpeed(int noteinstrvolspeed) { return g_Tracks.DelNoteInstrVolSpeed(noteinstrvolspeed, SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetNoteActualInstrVol(int note) { return g_Tracks.SetNoteInstrVol(note, m_activeinstr, m_volume, SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetNoteInstrVol(int note, int instr, int vol) { return g_Tracks.SetNoteInstrVol(note, instr, vol, SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetInstr(int instr) { return g_Tracks.SetInstr(instr, SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetVol(int vol) { return g_Tracks.SetVol(vol, SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetSpeed(int speed) { return g_Tracks.SetSpeed(speed, SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetNote() { return g_Tracks.GetNote(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetInstr() { return g_Tracks.GetInstr(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetVol() { return g_Tracks.GetVol(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetSpeed() { return g_Tracks.GetSpeed(SongGetActiveTrack(), m_trackactiveline); };
	BOOL TrackSetEnd() { return g_Tracks.SetEnd(SongGetActiveTrack(), m_trackactiveline + 1); };
	int TrackGetLastLine() { return g_Tracks.GetLastLine(SongGetActiveTrack()); };
	BOOL TrackSetGo() { return g_Tracks.SetGo(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetGoLine() { return g_Tracks.GetGoLine(SongGetActiveTrack()); };
	void RespectBoundaries();
	void TrackGetLoopingNoteInstrVol(int track, int& note, int& instr, int& vol);

	int* GetUECursor(int part);
	void SetUECursor(int part, int* cursor);
	BOOL UECursorIsEqual(int* cursor1, int* cursor2, int part);
	BOOL Undo() { return g_Undo.Undo(); };
	int	 UndoGetUndoSteps() { return g_Undo.GetUndoSteps(); };
	BOOL Redo() { return g_Undo.Redo(); };
	int  UndoGetRedoSteps() { return g_Undo.GetRedoSteps(); };

	BOOL SongKey(int vk, int shift, int control);
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
	int SongGetActiveTrack() { return (m_songgo[m_songactiveline] >= 0) ? -1 : m_song[m_songactiveline][m_trackactivecol]; };
	int SongGetTrack(int songline, int trackcol) { return IsValidSongline(songline) && !IsSongGo(songline) ? m_song[songline][trackcol] : -1; };
	int SongGetActiveTrackInColumn(int column) { return m_song[m_songactiveline][column]; };
	int SongGetActiveLine() { return m_songactiveline; };
	int SongGetPlayLine() { return m_songplayline; };
	void SongSetActiveLine(int line) { m_songactiveline = line; };
	void SongSetPlayLine(int line) { m_songplayline = line; };

	BOOL SongTrackGoOnOff();
	int SongGetGo() { return m_songgo[m_songactiveline]; };
	int SongGetGo(int songline) { return m_songgo[songline]; };
	void SongTrackGoDec() { m_songgo[m_songactiveline] = (m_songgo[m_songactiveline] - 1) & 0xff; };
	void SongTrackGoInc() { m_songgo[m_songactiveline] = (m_songgo[m_songactiveline] + 1) & 0xff; };

	BOOL SongInsertLine(int line);
	BOOL SongDeleteLine(int line);
	BOOL SongInsertCopyOrCloneOfSongLines(int& line);
	BOOL SongPrepareNewLine(int& line, int sourceline = -1, BOOL alsoemptycolumns = 1);
	int FindNearTrackBySongLineAndColumn(int songline, int column, BYTE* arrayTRACKSNUM);
	BOOL SongPutnewemptyunusedtrack();
	BOOL SongMaketracksduplicate();

	BOOL OctaveUp() { if (m_octave < 4) { m_octave++; return 1; } else return 0; };
	BOOL OctaveDown() { if (m_octave > 0) { m_octave--; return 1; } else return 0; };

	BOOL VolumeUp() { if (m_volume < MAXVOLUME) { m_volume++; return 1; } else return 0; };
	BOOL VolumeDown() { if (m_volume > 0) { m_volume--; return 1; } else return 0; };

	void ClearBookmark() { m_bookmark.songline = m_bookmark.trackline = m_bookmark.speed = -1; };
	BOOL IsBookmark() { return (m_bookmark.speed > 0 && m_bookmark.trackline < g_Tracks.GetMaxTrackLength()); };
	BOOL SetBookmark();

	BOOL Play(int mode, BOOL follow, int special = 0);
	void Stop();
	BOOL SongPlayNextLine();

	BOOL PlayBeat();
	BOOL PlayVBI();

	BOOL PlayPressedTonesInit();
	BOOL SetPlayPressedTonesTNIV(int t, int n, int i, int v) { m_playptnote[t] = n; m_playptinstr[t] = i; m_playptvolume[t] = v; return 1; }
	BOOL SetPlayPressedTonesV(int t, int v) { m_playptvolume[t] = v; return 1; };
	BOOL SetPlayPressedTonesSilence();
	BOOL PlayPressedTones();

	void TimerRoutine();

	void SetRMTTitle();

	void FileOpen(const char* filename = NULL, BOOL warnOfUnsavedChanges = TRUE);
	void FileReload();
	BOOL FileCanBeReloaded() { return (m_filename != "") /*&& (!m_fileunsaved)*/ /*&& g_changes*/; };
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

	void StrToAtariVideo(char* txt, int count);
	int SongToAta(unsigned char* dest, int max, int adr);
	BOOL AtaToSong(unsigned char* sour, int len, int adr);

	bool CreateExportMetadata(int iotype, struct TExportMetadata* metadata);
	bool WriteToXEX(struct TExportMetadata* metadata);

	bool SaveTxt(std::ofstream& ou);
	bool SaveRMW(std::ofstream& ou);

	bool LoadRMT(std::ifstream& in);
	bool LoadTxt(std::ifstream& in);
	bool LoadRMW(std::ifstream& in);

	int ImportTMC(std::ifstream& in);
	int ImportMOD(std::ifstream& in);

	bool ExportV2(std::ofstream& ou, int iotype, LPCTSTR filename = NULL);
	bool ExportAsRMT(std::ofstream& ou, tExportDescription* exportDesc);
	bool ExportAsStrippedRMT(std::ofstream& ou, tExportDescription* exportDesc, LPCTSTR filename);
	bool ExportAsAsm(std::ofstream& ou, tExportDescription* exportStrippedDesc);
	bool ExportAsRelocatableAsmForRmtPlayer(std::ofstream& ou, tExportDescription* exportStrippedDesc);

	bool ExportSAP_R(std::ofstream& ou);
	bool ExportLZSS(std::ofstream& ou, LPCTSTR filename);
	bool ExportCompactLZSS(std::ofstream& ou, LPCTSTR filename);
	bool ExportLZSS_SAP(std::ofstream& ou);
	bool ExportLZSS_XEX(std::ofstream& ou);

	bool ExportWav(std::ofstream& ou, LPCTSTR filename);

	void DumpSongToPokeyBuffer(int playmode = MPLAY_SONG, int songline = 0, int trackline = 0);
	int BruteforceOptimalLZSS(unsigned char* src, int srclen, unsigned char* dst);

	bool TestBeforeFileSave();
	int GetSubsongParts(CString& resultstr);

	void ComposeRMTFEATstring(CString& dest, const char* filename, BYTE* instrumentSavedFlags, BYTE* trackSavedFlags, BOOL sfx, BOOL gvf, BOOL nos, int assemblerFormat);

	BOOL BuildRelocatableAsm(CString& dest,
		tExportDescription* exportDesc,
		CString strAsmStartLabel,
		CString strTracksLabel,
		CString strSongLinesLabel,
		CString strInstrumentsLabel,
		int assemblerFormat,
		BOOL sfx,
		BOOL gvf,
		BOOL nos,
		bool bWantSizeInfoOnly);

	int BuildInstrumentData(
		CString& strCode,
		CString strInstrumentsLabel,
		unsigned char* buf,
		int from,
		int to,
		int* info,
		int assemblerFormat
	);

	int BuildTracksData(
		CString& strCode,
		CString strTracksLabel,
		unsigned char* buf,
		int from,
		int to,
		int* track_pos,
		int assemblerFormat);

	int BuildSongData(
		CString& strCode,
		CString strSongLinesLabel,
		unsigned char* buf,
		int offsetSong,
		int len,
		int start,
		int numTracks,
		int assemblerFormat
	);

	void MarkTF_USED(BYTE* arrayTRACKSNUM);
	void MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM);

	int MakeTuningBlock(unsigned char* mem, int addr);
	int DecodeTuningBlock(unsigned char* mem, int fromAddr, int endAddr);
	void ResetTuningVariables();

	int MakeModule(unsigned char* mem, int adr, int iotype, BYTE* instrumentSavedFlags, BYTE* trackSavedFlags);
	int MakeRMFModule(unsigned char* mem, int adr, BYTE* instrumentSavedFlags, BYTE* trackSavedFlags);
	int DecodeModule(unsigned char* mem, int adrfrom, int adrend, BYTE* instrumentLoadedFlags, BYTE* trackLoadedFlags);

	void TrackCopy();
	void TrackPaste();
	void TrackCut();
	void TrackDelete();
	void TrackCopyFromTo(int fromtrack, int totrack);
	void TrackSwapFromTo(int fromtrack, int totrack);

	void BlockPaste(int special = 0);

	void InstrCopy();
	void InstrPaste(int special = 0);
	void InstrCut();
	void InstrDelete();

	void InstrInfo(int instr, TInstrInfo* iinfo = NULL, int instrto = -1);
	void InstrChange(int instr);
	void TrackInfo(int track);

	void SongCopyLine();
	void SongPasteLine();
	void SongClearLine();

	void TracksOrderChange();
	void Songswitch4_8(int tracks4_8);
	int GetEffectiveMaxtracklen();
	int GetSmallestMaxtracklen(int songline);
	void ChangeMaxtracklen(int maxtracklen);
	void TracksAllBuildLoops(int& tracksmodified, int& beatsreduced);
	void TracksAllExpandLoops(int& tracksmodified, int& loopsexpanded);
	void SongClearUnusedTracksAndParts(int& clearedtracks, int& truncatedtracks, int& truncatedbeats);

	int SongClearDuplicatedTracks();
	int SongClearUnusedTracks();
	int ClearAllInstrumentsUnusedInAnyTrack();

	void RenumberAllTracks(int type);
	void RenumberAllInstruments(int type);

	CString GetFilename() { return m_filename; };
	int GetFiletype() { return m_filetype; };

	int(*GetSong())[SONGLEN][SONGTRACKS]{ return &m_song; };
	int(*GetSongGo())[SONGLEN] { return &m_songgo; };
	TBookmark* GetBookmark() { return &m_bookmark; };

	int GetPlayMode() { return m_play; };
	void SetPlayMode(int mode) { m_play = mode; };
	BOOL GetFollowPlayMode() { return m_followplay; };
	void SetFollowPlayMode(BOOL follow) { m_followplay = follow; };

	void GetSongInfoPars(TInfo* info) { memcpy(info->songname, m_songname, SONG_NAME_MAX_LEN); info->speed = m_speed; info->mainspeed = m_mainSpeed; info->instrspeed = m_instrumentSpeed; info->songnamecur = m_songnamecur; };
	void SetSongInfoPars(TInfo* info) { memcpy(m_songname, info->songname, SONG_NAME_MAX_LEN); m_speed = info->speed; m_mainSpeed = info->mainspeed; m_instrumentSpeed = info->instrspeed; m_songnamecur = info->songnamecur; };

	BOOL IsValidSongline(int songline) { return songline >= 0 && songline < SONGLEN; };
	BOOL IsSongGo(int songline) { return IsValidSongline(songline) ? m_songgo[songline] >= 0 : 0; };

	void SongJump(int lines);

private:
	int m_song[SONGLEN][SONGTRACKS];
	int m_songgo[SONGLEN];						// If >= 0, then GO applies

	BOOL volatile m_followplay;
	int volatile m_play;
	int m_songactiveline;
	int volatile m_songplayline;				// Which line of the song is currently being played

	int m_trackactiveline;
	int volatile m_trackplayline;				// Which line of a track is currenyly being played
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

	//POKEY EXPLORER variables, used for tests involving pitch calculations and sound debugging displayed on screen
	int e_ch_idx = 0;
	int e_modoffset = 1;
	int e_coarse_divisor = 1;
	int e_modulo = 0;
	BOOL e_valid = 1;
	double e_divisor = 1;
	double e_pitch = 0;

	int m_infoact;						// Which part of the info area is active for editing: 0 = name, 
	char m_songname[SONG_NAME_MAX_LEN + 1];
	int m_songnamecur;

	TBookmark m_bookmark;

	double m_avgspeed[8] = { 0 };		//use for calculating average BPM

	int volatile m_mainSpeed;
	int volatile m_speed;
	int volatile m_speeda;

	int volatile m_instrumentSpeed;

	int volatile m_quantization_note;
	int volatile m_quantization_instr;
	int volatile m_quantization_vol;

	int m_playptnote[SONGTRACKS];
	int m_playptinstr[SONGTRACKS];
	int m_playptvolume[SONGTRACKS];

	TInstrument m_instrclipboard;
	int m_songlineclipboard[SONGTRACKS];
	int m_songgoclipboard;

	UINT m_timerRoutine;
	const BYTE m_timerRoutineTick[3] = { 17, 17, 16 };

	CString m_filename;
	int m_filetype;
	int m_lastExportType;					// Which data format was used to export a file the last time?

	int m_TracksOrderChange_songlinefrom; //is defined as a member variable to keep in use
	int m_TracksOrderChange_songlineto;	  //the last values used remain
};