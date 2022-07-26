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

/*
struct TSongTrackAndTrackData
{
	int tracknum;
	TTrack trackdata;
};
*/

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
	BOOL DrawAnalyzer(CDC* pDC);
	BOOL DrawPlaytimecounter(CDC* pDC);

	BOOL InfoKey(int vk, int shift, int control);
	BOOL InfoCursorGotoSongname(int x);
	BOOL InfoCursorGotoSpeed(int x);
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
	void SetActiveLine(int line) { m_trackactiveline = line; };

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
	BOOL TrackSetEnd() { return g_Tracks.SetEnd(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetLastLine() { return g_Tracks.GetLastLine(SongGetActiveTrack()); };
	BOOL TrackSetGo() { return g_Tracks.SetGo(SongGetActiveTrack(), m_trackactiveline); };
	int TrackGetGoLine() { return g_Tracks.GetGoLine(SongGetActiveTrack()); };

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
	int SongGetTrack(int songline, int trackcol) { return (m_songgo[songline] >= 0) ? -1 : m_song[songline][trackcol]; };
	int SongGetActiveTrackInColumn(int column) { return m_song[m_songactiveline][column]; };
	int SongGetActiveLine() { return m_songactiveline; };
	void SongSetActiveLine(int line) { m_songactiveline = line; };

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
	BOOL IsBookmark() { return (m_bookmark.speed > 0 && m_bookmark.trackline < g_Tracks.m_maxtracklen); };
	BOOL SetBookmark();

	BOOL Play(int mode, BOOL follow, int special = 0);
	BOOL Stop();
	BOOL SongPlayNextLine();

	BOOL PlayBeat();
	BOOL PlayVBI();

	BOOL PlayPressedTonesInit();
	BOOL SetPlayPressedTonesTNIV(int t, int n, int i, int v) { m_playptnote[t] = n; m_playptinstr[t] = i; m_playptvolume[t] = v; return 1; }
	BOOL SetPlayPressedTonesV(int t, int v) { m_playptvolume[t] = v; return 1; };
	BOOL SetPlayPressedTonesSilence();
	BOOL PlayPressedTones();

	void TimerRoutine();

	BOOL InitPokey();
	BOOL DeInitPokey();

	void SetRMTTitle();

	void FileOpen(const char* filename = NULL, BOOL warnunsavedchanges = 1);
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

	int SongToAta(unsigned char* dest, int max, int adr);
	BOOL AtaToSong(unsigned char* sour, int len, int adr);

	int Save(std::ofstream& ou, int iotype);
	int Load(std::ifstream& in, int iotype);
	int LoadRMT(std::ifstream& in);

	int ImportTMC(std::ifstream& in);
	int ImportMOD(std::ifstream& in);
	int Export(std::ofstream& ou, int iotype, char* filename = NULL);

	int TestBeforeFileSave();
	int GetSubsongParts(CString& resultstr);

	BOOL ComposeRMTFEATstring(CString& dest, char* filename, BYTE* instrsaved, BYTE* tracksaved, BOOL sfx, BOOL gvf, BOOL nos);

	void MarkTF_USED(BYTE* arrayTRACKSNUM);
	void MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM);

	int MakeModule(unsigned char* mem, int adr, int iotype, BYTE* instrsaved, BYTE* tracksaved);
	int MakeRMFModule(unsigned char* mem, int adr, BYTE* instrsaved, BYTE* tracksaved);
	int DecodeModule(unsigned char* mem, int adrfrom, int adrend, BYTE* instrloaded, BYTE* trackloaded);

	void TrackCopy();
	void TrackPaste();
	void TrackCut();
	void TrackDelete();
	void TrackCopyFromTo(int fromtrack, int totrack);

	void BlockPaste(int special = 0);

	void InstrCopy();
	void InstrPaste(int special = 0);
	void InstrCut();
	void InstrDelete();

	void InstrInfo(int instr, TInstrInfo* iinfo = NULL, int instrto = -1);
	int InstrChange(int instr);
	void TrackInfo(int track);

	void SongCopyLine();
	void SongPasteLine();
	void SongClearLine();

	void TracksOrderChange();
	void Songswitch4_8(int tracks4_8);
	int GetEffectiveMaxtracklen();
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

	void GetSongInfoPars(TInfo* info) { memcpy(info->songname, m_songname, SONGNAMEMAXLEN); info->speed = m_speed; info->mainspeed = m_mainspeed; info->instrspeed = m_instrspeed; info->songnamecur = m_songnamecur; };
	void SetSongInfoPars(TInfo* info) { memcpy(m_songname, info->songname, SONGNAMEMAXLEN); m_speed = info->speed; m_mainspeed = info->mainspeed; m_instrspeed = info->instrspeed; m_songnamecur = info->songnamecur; };

	BOOL volatile m_followplay;
	int volatile m_play;

	int m_playcount[256] = { 0 };	//used for keeping track of loop points, useful for SAP-R recording specifically, maybe more sometime later...

private:
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
	char m_songname[SONGNAMEMAXLEN + 1];
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
};