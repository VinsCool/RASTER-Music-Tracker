#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"


struct TTrack
{
	int len;				// Length of the track
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

extern const char* notes[];
extern const char* notesandscales[5][40];

class CTracks
{
public:
	CTracks();
	~CTracks();
	void InitTracks();
	void ClearTrack(int track);
	BOOL IsEmptyTrack(int track);
	void DrawTrackHeader(int x, int y, int tr, int col);
	void DrawTrackLine(int col, int x, int y, int tr, int line, int aline, int cactview, int pline, BOOL isactive, int acu, int oob);
	BOOL DelNoteInstrVolSpeed(int noteinstrvolspeed, int track, int line);
	BOOL SetNoteInstrVol(int note, int instr, int vol, int track, int line);
	BOOL SetInstr(int instr, int track, int line);
	BOOL SetVol(int vol, int track, int line);
	BOOL SetSpeed(int speed, int track, int line);

	BOOL IsValidChannel(int channel) { return channel >= 0 && channel < SONGTRACKS; };
	BOOL IsValidTrack(int track) { return track >= 0 && track < TRACKSNUM; };
	BOOL IsValidLine(int line) { return line >= 0 && line < MAXATATRACKLEN; };
	BOOL IsValidNote(int note) { return note >= 0 && note < NOTESNUM; };
	BOOL IsValidInstrument(int instr) { return instr >= 0 && instr < INSTRSNUM; };
	BOOL IsValidVolume(int vol) { return vol >= 0 && vol <= MAXVOLUME; };
	BOOL IsValidSpeed(int speed) { return speed >= 0 && speed < TRACKMAXSPEED; };
	BOOL IsValidLength(int len) { return len > 0 && len <= MAXATATRACKLEN; };
	BOOL IsValidGo(int go) { return go >= 0 && go < MAXATATRACKLEN; };

	int GetNote(int track, int line) { return IsValidTrack(track) && IsValidLine(line) ? m_track[track].note[line] : -1; };
	int GetInstr(int track, int line) { return IsValidTrack(track) && IsValidLine(line) ? m_track[track].instr[line] : -1; };
	int GetVol(int track, int line) { return IsValidTrack(track) && IsValidLine(line) ? m_track[track].volume[line] : -1; };
	int GetSpeed(int track, int line) { return IsValidTrack(track) && IsValidLine(line) ? m_track[track].speed[line] : -1; };
	void GetNoteInstrVolSpeed(int* buff, int track, int line) { if (!(IsValidTrack(track) && IsValidLine(line))) return; buff[0] = m_track[track].note[line]; buff[1] = m_track[track].instr[line]; buff[2] = m_track[track].volume[line]; buff[3] = m_track[track].speed[line]; };
	BOOL SetEnd(int track, int line);
	int GetLastLine(int track);
	int GetLength(int track);
	BOOL SetGo(int track, int line);
	int GetGoLine(int track);

	BOOL InsertLine(int track, int line);
	BOOL DeleteLine(int track, int line);

	TTrack* GetTrack(int track) { return IsValidTrack(track) ? &m_track[track] : NULL; };

	void GetTracksAll(TTracksAll* toTracks);
	void SetTracksAll(TTracksAll* fromTracks);

	int TrackToAta(int trackNr, unsigned char* dest, int max);
	int TrackToAtaRMF(int trackNr, unsigned char* dest, int max);
	BOOL AtaToTrack(unsigned char* mem, int trackLength, int trackNr);

	int SaveAll(std::ofstream& ou, int iotype);
	int LoadAll(std::ifstream& in, int iotype);

	int SaveTrack(int track, std::ofstream& ou, int iotype);
	int LoadTrack(int track, std::ifstream& in, int iotype);

	BOOL CalculateNotEmpty(int track);
	BOOL CompareTracks(int track1, int track2);

	int TrackOptimizeVol0(int track);
	int TrackBuildLoop(int track);
	int TrackExpandLoop(int track);
	int TrackExpandLoop(TTrack* ttrack);

	int GetModifiedNote(int note, int tuning);
	int GetModifiedInstr(int instr, int instradd);
	int GetModifiedVolumeP(int volume, int percentage);
	BOOL ModifyTrack(TTrack* track, int from, int to, int instrnumonly, int tuning, int instradd, int volumep);

	int m_maxTrackLength;

private:
	TTrack* m_track;
};

extern CTracks g_Tracks;
