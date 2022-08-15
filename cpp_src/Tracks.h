#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"


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

extern const char* notes[];
extern const char* notesflat[];
extern const char* notesgerman[];
extern const char* notesgermanflat[];


extern BOOL ModifyTrack(TTrack* track, int from, int to, int instrnumonly, int tuning, int instradd, int volumep);

class CTracks
{
public:
	CTracks();
	void InitTracks();
	BOOL ClearTrack(int t);
	BOOL IsEmptyTrack(int track);
	BOOL DrawTrackHeader(int col, int x, int y, int tr);	//, int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu);
	BOOL DrawTrackLine(int col, int x, int y, int tr, int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu, int oob);
	BOOL DelNoteInstrVolSpeed(int noteinstrvolspeed, int track, int line);
	BOOL SetNoteInstrVol(int note, int instr, int vol, int track, int line);
	BOOL SetInstr(int instr, int track, int line);
	BOOL SetVol(int vol, int track, int line);
	BOOL SetSpeed(int speed, int track, int line);
	int GetNote(int track, int line) { return m_track[track].note[line]; };
	int GetInstr(int track, int line) { return m_track[track].instr[line]; };
	int GetVol(int track, int line) { return m_track[track].volume[line]; };
	int GetSpeed(int track, int line) { return m_track[track].speed[line]; };
	void GetNoteInstrVolSpeed(int* buff, int track, int line) { buff[0] = m_track[track].note[line]; buff[1] = m_track[track].instr[line]; buff[2] = m_track[track].volume[line]; buff[3] = m_track[track].speed[line]; };
	BOOL SetEnd(int track, int line);
	int GetLastLine(int track);
	int GetLength(int track);
	BOOL SetGo(int track, int line);
	int GetGoLine(int track);

	BOOL InsertLine(int track, int line);
	BOOL DeleteLine(int track, int line);

	TTrack* GetTrack(int tr) { return &m_track[tr]; };
	void GetTracksAll(TTracksAll* dest_ta);	//{ return (TTracksAll*)&m_track; };
	void SetTracksAll(TTracksAll* src_ta);

	int TrackToAta(int track, unsigned char* dest, int max);
	int TrackToAtaRMF(int track, unsigned char* dest, int max);
	BOOL AtaToTrack(unsigned char* sour, int len, int track);

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

	int m_maxTrackLength;

private:
	TTrack m_track[TRACKSNUM];
};

extern CTracks			g_Tracks;
