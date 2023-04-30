#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"
#include "global.h"

#include "Undo.h"
#include "Instruments.h"
#include "Tracks.h"
#include "ModuleV2.h"

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

	void ChangeTimer(int ms);
	void KillTimer();

	void ClearSong(int numoftracks);

	void MidiEvent(DWORD dwParam);

	// Legacy Draw functions
	//void DrawSong();				// Draw the song line info on the right
	//void DrawTracks();
	//void DrawInstrument();
	//void DrawInfo();			//top left corner
	//void DrawAnalyzer();
	//void DrawPlayTimeCounter();

	// RMTE Draw functions
	void DrawSonglines(TSubtune* p);
	void DrawSubtuneInfos(TSubtune* p);
	void DrawRegistersState(TSubtune* p);
	void DrawPatternEditor(TSubtune* p);
	void DrawInstrumentEditor(TSubtune* p);
	void DrawDebugInfos(TSubtune* p);

	//BOOL InfoKey(int vk, int shift, int control);
	BOOL InfoCursorGotoSongname(int x);
	BOOL InfoCursorGotoSpeed(int x);
	BOOL InfoCursorGotoHighlight(int x);
	BOOL InfoCursorGotoOctaveSelect(int x, int y);
	BOOL InfoCursorGotoVolumeSelect(int x, int y);
	BOOL InfoCursorGotoInstrumentSelect(int x, int y);

	//BOOL InstrKey(int vk, int shift, int control);
	void ActiveInstrSet(int instr);
	void ActiveInstrPrev() { g_Undo.Separator(); int instr = (m_activeinstr - 1) & 0x3f; ActiveInstrSet(instr); };
	void ActiveInstrNext() { g_Undo.Separator(); int instr = (m_activeinstr + 1) & 0x3f; ActiveInstrSet(instr); };

	int GetActiveInstr() { return m_activeinstr; };
	int GetActiveColumn() { return m_trackactivecol; };
	int GetActiveLine() { return m_activeRow; };
	int GetPlayLine() { return m_playRow; };
	void SetActiveLine(int line) { m_activeRow = line; };
	void SetPlayLine(int line) { m_playRow = line; };

	//BOOL CursorToSpeedColumn();
	//BOOL ProveKey(int vk, int shift, int control);
	//BOOL TrackKey(int vk, int shift, int control);
	//BOOL TrackCursorGoto(CPoint point);

	// Legacy Pattern Movement functions
	//BOOL TrackUp(int lines);
	//BOOL TrackDown(int lines, BOOL stoponlastline = 1);
	//BOOL TrackLeft(BOOL column = 0);
	//BOOL TrackRight(BOOL column = 0);

	// RMTE Pattern Movement functions
	void PatternLeft();
	void PatternRight();
	void PatternUp(int rows);
	void PatternDown(int rows);
	void ChannelLeft();
	void ChannelRight();

	BOOL TrackDelNoteInstrVolSpeed(int noteinstrvolspeed) { return g_Tracks.DelNoteInstrVolSpeed(noteinstrvolspeed, SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetNoteActualInstrVol(int note) { return g_Tracks.SetNoteInstrVol(note, m_activeinstr, m_volume, SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetNoteInstrVol(int note, int instr, int vol) { return g_Tracks.SetNoteInstrVol(note, instr, vol, SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetInstr(int instr) { return g_Tracks.SetInstr(instr, SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetVol(int vol) { return g_Tracks.SetVol(vol, SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetSpeed(int speed) { return g_Tracks.SetSpeed(speed, SongGetActiveTrack(), m_activeRow); };
	int TrackGetNote() { return g_Tracks.GetNote(SongGetActiveTrack(), m_activeRow); };
	int TrackGetInstr() { return g_Tracks.GetInstr(SongGetActiveTrack(), m_activeRow); };
	int TrackGetVol() { return g_Tracks.GetVol(SongGetActiveTrack(), m_activeRow); };
	int TrackGetSpeed() { return g_Tracks.GetSpeed(SongGetActiveTrack(), m_activeRow); };
	BOOL TrackSetEnd() { return g_Tracks.SetEnd(SongGetActiveTrack(), m_activeRow + 1); };
	int TrackGetLastLine() { return g_Tracks.GetLastLine(SongGetActiveTrack()); };
	BOOL TrackSetGo() { return g_Tracks.SetGo(SongGetActiveTrack(), m_activeRow); };
	int TrackGetGoLine() { return g_Tracks.GetGoLine(SongGetActiveTrack()); };
	//void RespectBoundaries();
	void TrackGetLoopingNoteInstrVol(int track, int& note, int& instr, int& vol);

	int* GetUECursor(int part);
	void SetUECursor(int part, int* cursor);
	BOOL UECursorIsEqual(int* cursor1, int* cursor2, int part);
	BOOL Undo() { return g_Undo.Undo(); };
	int	 UndoGetUndoSteps() { return g_Undo.GetUndoSteps(); };
	BOOL Redo() { return g_Undo.Redo(); };
	int  UndoGetRedoSteps() { return g_Undo.GetRedoSteps(); };

	//BOOL SongKey(int vk, int shift, int control);
	//BOOL SongCursorGoto(CPoint point);

	// Legacy Songline Movement functions
	//BOOL SongUp();
	//BOOL SongDown();
	//BOOL SongSubsongPrev();
	//BOOL SongSubsongNext();

	// RMTE Songline Movement functions
	void SonglineUp();
	void SonglineDown();
	void SeekNextSubtune();
	void SeekPreviousSubtune();

	BOOL SongTrackSet(int t);
	BOOL SongTrackSetByNum(int num);
	BOOL SongTrackDec();
	BOOL SongTrackInc();
	BOOL SongTrackEmpty();
	int SongGetActiveTrack() { return (m_songgo[m_activeSongline] >= 0) ? -1 : m_song[m_activeSongline][m_trackactivecol]; };
	int SongGetTrack(int songline, int trackcol) { return IsValidSongline(songline) && !IsSongGo(songline) ? m_song[songline][trackcol] : -1; };
	int SongGetActiveTrackInColumn(int column) { return m_song[m_activeSongline][column]; };
	int SongGetActiveLine() { return m_activeSongline; };
	int SongGetPlayLine() { return m_playSongline; };
	void SongSetActiveLine(int line) { m_activeSongline = line; };
	void SongSetPlayLine(int line) { m_playSongline = line; };

	BOOL SongTrackGoOnOff();
	int SongGetGo() { return m_songgo[m_activeSongline]; };
	int SongGetGo(int songline) { return m_songgo[songline]; };
	void SongTrackGoDec() { m_songgo[m_activeSongline] = (m_songgo[m_activeSongline] - 1) & 0xff; };
	void SongTrackGoInc() { m_songgo[m_activeSongline] = (m_songgo[m_activeSongline] + 1) & 0xff; };

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

	//void ClearBookmark() { m_bookmark.songline = m_bookmark.trackline = m_bookmark.speed = -1; };
	//BOOL IsBookmark() { return (m_bookmark.speed > 0 && m_bookmark.trackline < g_Tracks.GetMaxTrackLength()); };
	//BOOL SetBookmark();

	void Play(int mode, BOOL follow, int special = 0);
	void Stop();

	//BOOL SongPlayNextLine();
	//BOOL PlayBeat();
	//BOOL PlayVBI();
	BOOL PlayPressedTonesInit();
	BOOL SetPlayPressedTonesTNIV(int t, int n, int i, int v) { m_playptnote[t] = n; m_playptinstr[t] = i; m_playptvolume[t] = v; return 1; };
	BOOL SetPlayPressedTonesV(int t, int v) { m_playptvolume[t] = v; return 1; };
	BOOL SetPlayPressedTonesSilence();
	BOOL PlayPressedTones();

	void TimerRoutine();

	void UpdatePlayTime() { m_playTimeFrameCount += m_playMode ? 1 : 0; };
	void ResetPlayTime() { m_playTimeFrameCount = 0; };

	void CalculatePlayTime();
	void CalculatePlayBPM();
	void CalculateDisplayFPS();

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
	//int GetSubsongParts(CString& resultstr);

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

	int GetPlayMode() { return m_playMode; };
	void SetPlayMode(int mode) { m_playMode = mode; };

	BOOL GetFollowPlayMode() { return m_isFollowPlay; };
	void SetFollowPlayMode(BOOL follow) { m_isFollowPlay = follow; };

	void GetSongInfoPars(TInfo* info) { memcpy(info->songname, m_songname, SONG_NAME_MAX_LEN); info->speed = m_speed; info->mainspeed = m_mainSpeed; info->instrspeed = m_instrumentSpeed; info->songnamecur = m_songnamecur; };
	void SetSongInfoPars(TInfo* info) { memcpy(m_songname, info->songname, SONG_NAME_MAX_LEN); m_speed = info->speed; m_mainSpeed = info->mainspeed; m_instrumentSpeed = info->instrspeed; m_songnamecur = info->songnamecur; };

	BOOL IsValidSongline(int songline) { return songline >= 0 && songline < SONGLEN; };
	BOOL IsSongGo(int songline) { return IsValidSongline(songline) ? m_songgo[songline] >= 0 : 0; };

	// Prototype C++ RMTE Module Driver functions
	// TODO: Move to a different file later

	void PlayRow(TSubtune* p);
	void PlayPattern(TSubtune* p);
	void PlaySongline(TSubtune* p);
	void PlayNote(TSubtune* p);
	void PlayInstrument(TSubtune* p);
	void PlayVolume(TSubtune* p);
	void PlayEffect(TSubtune* p);

private:
	// Legacy variables
	int m_song[SONGLEN][SONGTRACKS];	// TODO: Delete
	int m_songgo[SONGLEN];				// TODO: Delete	// If >= 0, then GO applies

	BOOL volatile m_isFollowPlay;
	int volatile m_playMode;
	int m_activeSongline;
	int volatile m_playSongline;				// Which line of the song is currently being played

	int m_activeRow;
	int volatile m_playRow;				// Which line of a track is currenyly being played
	int m_trackactivecol;						//0-7
	int m_trackactivecur;						//0-2

	int m_trackplayblockstart;
	int m_trackplayblockend;

	int m_activeinstr;
	int m_volume;
	int m_octave;

	//MIDI input variables, used for tests through MIDI CH15 
	int m_mod_wheel = 0;			// TODO: Delete
	int m_vol_slider = 0;			// TODO: Delete
	int m_heldkeys = 0;				// TODO: Delete
	int m_midi_distortion = 0;		// TODO: Delete
	BOOL m_ch_offset = 0;			// TODO: Delete

	//POKEY EXPLORER variables, used for tests involving pitch calculations and sound debugging displayed on screen
	int e_ch_idx = 0;				// TODO: Delete
	int e_modoffset = 1;			// TODO: Delete
	int e_coarse_divisor = 1;		// TODO: Delete
	int e_modulo = 0;				// TODO: Delete
	BOOL e_valid = 1;				// TODO: Delete
	double e_divisor = 1;			// TODO: Delete
	double e_pitch = 0;				// TODO: Delete

	int m_infoact;							// TODO: Delete	// Which part of the info area is active for editing: 0 = name, 
	char m_songname[SONG_NAME_MAX_LEN + 1];	// TODO: Delete
	int m_songnamecur;						// TODO: Delete
	TBookmark m_bookmark;					// TODO: Delete

	// Used for calculating Play time
	int m_playTimeFrameCount;
	int m_playTimeSecondCount;
	int m_playTimeMinuteCount;
	int m_playTimeMillisecondCount;

	// Used for calculating Average BPM
	BYTE m_rowSpeed[8];
	double m_averageBPM;
	double m_averageSpeed;
	
	// Used for calculating Average FPS
	uint64_t m_lastDeltaCount;
	uint64_t m_lastFrameCount;
	uint64_t m_lastMillisecondCount;
	uint64_t m_lastSecondCount;
	double m_averageFrameCount;

	int volatile m_mainSpeed;				// TODO: Delete
	int volatile m_speed;
	int volatile m_speeda;

	int volatile m_instrumentSpeed;

	int volatile m_quantization_note;		// TODO: Delete
	int volatile m_quantization_instr;		// TODO: Delete
	int volatile m_quantization_vol;		// TODO: Delete

	int m_playptnote[SONGTRACKS];			// TODO: Delete
	int m_playptinstr[SONGTRACKS];			// TODO: Delete
	int m_playptvolume[SONGTRACKS];			// TODO: Delete

	TInstrument m_instrclipboard;			// TODO: Delete
	int m_songlineclipboard[SONGTRACKS];	// TODO: Delete
	int m_songgoclipboard;					// TODO: Delete

	UINT m_timerRoutine;

	CString m_filename;
	int m_filetype;
	int m_lastExportType;					// Which data format was used to export a file the last time?

	int m_TracksOrderChange_songlinefrom; // TODO: Delete	//is defined as a member variable to keep in use
	int m_TracksOrderChange_songlineto;	  // TODO: Delete	//the last values used remain



	// RMTE variables
	//bool volatile m_isFollowPlay;
	//BYTE volatile m_playMode;
	//BYTE volatile m_activeSongline;
	//BYTE volatile m_playSongline;
	//BYTE volatile m_activeRow;
	//BYTE volatile m_playRow;
	BYTE volatile m_activeChannel;
	BYTE volatile m_activeCursor;
	BYTE volatile m_playBlockStart;
	BYTE volatile m_playBlockEnd;
	BYTE volatile m_activeInstrument;
	BYTE volatile m_activeVolume;
	BYTE volatile m_activeOctave;
	//BYTE volatile m_activeSpeed;
	BYTE volatile m_playSpeed;
	BYTE volatile m_speedTimer;
//	BYTE volatile m_instrumentSpeed;
//	int volatile m_playTimeFrameCount;
//	int volatile m_playTimeSecondCount;
//	int volatile m_playTimeMinuteCount;
//	int volatile m_playTimeMillisecondCount;
//	BYTE volatile m_rowSpeed[8];
//	double volatile m_averageBPM;
//	double volatile m_averageSpeed;
//	uint64_t volatile m_lastDeltaCount;
//	uint64_t volatile m_lastFrameCount;
//	uint64_t volatile m_lastMillisecondCount;
//	uint64_t volatile m_lastSecondCount;
//	double volatile m_averageFrameCount;
//	UINT m_timerRoutine;
//	const BYTE m_timerRoutineTick[3] = { 17, 17, 16 };
	CString m_fileName;
	int m_fileType;
//	int m_lastExportType;

};