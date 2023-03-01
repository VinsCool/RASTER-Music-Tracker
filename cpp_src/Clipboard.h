#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"

#include "Tracks.h"

class CTrackClipboard
{
public:
	CTrackClipboard();
	void Empty();
	BOOL IsBlockSelected() { return (m_selcol >= 0); };
	BOOL IsTrackSelected() { return (m_seltrack >= 0); };
	BOOL BlockSetBegin(int col, int track, int line);
	BOOL BlockSetEnd(int line);
	void BlockDeselect();

	int BlockCopyToClipboard();
	int BlockExchangeClipboard();
	int BlockPasteToTrack(int track, int line, int special = 0);
	int BlockClear();
	int BlockRestoreFromBackup();

	void GetFromTo(int& from, int& to);
	int	GetCol() { return m_selcol; };

	void BlockNoteTransposition(int instr, int addnote);
	void BlockInstrumentChange(int instr, int addinstr);
	void BlockVolumeChange(int instr, int addvol);

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
};


#define BLOCKSETBEGIN	g_TrackClipboard.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline)
#define BLOCKSETEND		g_TrackClipboard.BlockSetEnd(m_trackactiveline)
#define BLOCKDESELECT	g_TrackClipboard.BlockDeselect()
#define ISBLOCKSELECTED	g_TrackClipboard.IsBlockSelected()

