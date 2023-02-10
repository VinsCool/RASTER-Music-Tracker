#include "stdafx.h"
#include <fstream>

#include "resource.h"

#include "Clipboard.h"

#include "Song.h"

#include "MainFrm.h"
#include "EffectsDlg.h"

#include "GuiHelpers.h"


extern CSong g_Song;

CTrackClipboard::CTrackClipboard()
{
	m_trackcopy.len = -1;	//for copying a complete track with loops, etc.
	m_trackbackup.len = -1; //back up
	m_all = 1;				//1 = all events / 0 = only for events with the same instrument as currently set
	Empty();
}

void CTrackClipboard::Empty()
{
	m_selcol = m_seltrack = -1;
	ClearTrack();
}

void CTrackClipboard::ClearTrack()
{
	m_track.len = -1;
	m_track.go = -1;
	for (int i = 0; i < TRACKLEN; i++)
	{
		m_track.note[i] = -1;
		m_track.instr[i] = -1;
		m_track.volume[i] = -1;
		m_track.speed[i] = -1;
	}
}

BOOL CTrackClipboard::BlockSetBegin(int col, int track, int line)
{
	if (col < 0 || track < 0 || track >= TRACKSNUM || line < 0 || line >= TRACKLEN) return 0;
	if (m_selcol < 0 || m_selcol != col || m_seltrack != track)
	{
		//newly marked start of block
		m_selcol = col;
		m_seltrack = track;
		m_selsongline = g_Song.SongGetActiveLine();
		m_selfrom = m_selto = line;
		//keep the track as it was now
		memcpy((void*)(&m_trackbackup), (void*)(g_Tracks.GetTrack(track)), sizeof(TTrack));
		//and initializes a base track
		BlockInitBase(track);
		return 1;
	}
	return 0;
}

BOOL CTrackClipboard::BlockSetEnd(int line)
{
	if (line < 0 || line >= TRACKLEN) return 0;
	m_selto = line;
	return 1;
}

void CTrackClipboard::BlockDeselect()
{
	m_selcol = -1;
	SetStatusBarText("");
}

void CTrackClipboard::BlockInitBase(int track)
{
	if (track < 0 || track >= TRACKSNUM) return;
	memcpy((void*)(&m_trackbase), (void*)(g_Tracks.GetTrack(track)), sizeof(TTrack));
	m_changenote = 0;
	m_changeinstr = 0;
	m_changevolume = 0;
	m_instrbase = -1;
}

void CTrackClipboard::BlockAllOnOff()
{
	if (!IsBlockSelected()) return;
	m_all ^= 1;

	int bfro, bto;
	GetFromTo(bfro, bto);

	//projects the current state of the track to the base track
	BlockInitBase(m_seltrack);
}

int CTrackClipboard::BlockCopyToClipboard()
{
	if (!IsBlockSelected()) { ClearTrack(); return 0; }
	if (m_seltrack < 0) return 0;

	ClearTrack();

	int j, line, xline;
	int linefrom, lineto;
	GetFromTo(linefrom, lineto);

	TTrack& ts = *g_Tracks.GetTrack(m_seltrack);
	TTrack& td = m_track;

	int len = ts.len, go = ts.go;

	for (line = linefrom, j = 0; line <= lineto; line++, j++)
	{
		if (line < len || go < 0) xline = line;
		else
			xline = ((line - len) % (len - go)) + go;

		if (line < len || go >= 0)
		{
			td.note[j] = ts.note[xline];
			td.instr[j] = ts.instr[xline];
			td.volume[j] = ts.volume[xline];
			td.speed[j] = ts.speed[xline];
		}
		else
		{
			td.note[j] = -1;
			td.instr[j] = -1;
			td.volume[j] = -1;
			td.speed[j] = -1;
		}
	}
	td.len = j;
	return td.len;
}

int CTrackClipboard::BlockExchangeClipboard()
{
	if (!IsBlockSelected()) return 0;
	if (m_seltrack < 0) return 0;

#define EXCH(a,b)	{ int xch=a; a=b; b=xch; }

	int j, line, xline;
	int linefrom, lineto;
	GetFromTo(linefrom, lineto);

	TTrack& ts = *g_Tracks.GetTrack(m_seltrack);
	TTrack& td = m_track;

	int len = ts.len, go = ts.go;
	int cllen = td.len;

	//it has to do it backwards because of the GO loop 
	//if it first rewrites the notes from above, which are then repeated in the GO loop
	//then the freshly rewritten ones would appear in the clipboard instead of the original loops
	//for(line=linefrom,j=0; line<=lineto; line++,j++)
	for (line = lineto, j = lineto - linefrom; line >= linefrom; line--, j--)
	{
		if (line < len || go < 0) xline = line;
		else
			xline = ((line - len) % (len - go)) + go;

		if (line < len)
		{
			EXCH(td.note[j], ts.note[line]);
			EXCH(td.instr[j], ts.instr[line]);
			EXCH(td.volume[j], ts.volume[line]);
			EXCH(td.speed[j], ts.speed[line]);
			if (j >= cllen)
			{	//the block is longer than the length of the data in the clipboard, so it fills with empty lines
				ts.note[line] = -1;
				ts.instr[line] = -1;
				ts.volume[line] = -1;
				ts.speed[line] = -1;
			}
		}
		else
		{	//part of the block extending beyond the end
			if (go >= 0)
			{			//there is a loop
				td.note[j] = ts.note[xline];
				td.instr[j] = ts.instr[xline];
				td.volume[j] = ts.volume[xline];
				td.speed[j] = ts.speed[xline];
			}
			else
			{			//there is no loop
				td.note[j] = -1;
				td.instr[j] = -1;
				td.volume[j] = -1;
				td.speed[j] = -1;
			}
		}
	}
	td.len = lineto - linefrom + 1;
	return td.len;
}

int CTrackClipboard::BlockPasteToTrack(int track, int line, int special)
{
	if (track < 0 || track >= TRACKSNUM) return 0;

	int bfro = -1, bto = -1;
	if (IsBlockSelected() && m_seltrack >= 0)
	{
		//A block is selected, so the PASTE will be placed in the line position
		track = m_seltrack;
		GetFromTo(bfro, bto);
	}

	TTrack& ts = m_track;
	TTrack& td = *g_Tracks.GetTrack(track);
	int i, j;
	int linemax;

	if (bfro >= 0) //block selected (continued)
	{
		line = bfro;
		linemax = bto + 1;
	}
	else
		linemax = line + ts.len;

	int songline = g_Song.SongGetActiveLine();
	if (linemax > g_Song.GetSmallestMaxtracklen(songline)) 
		linemax = g_Song.GetSmallestMaxtracklen(songline);

	if (line > td.len)
	{
		//if it makes a paste under the --end-- line, empty the gap between --end-- and the end of the place where it pastes
		//originally i < line, but because for some it merges to the original, the number of lines will be stretched
		for (i = td.len; i < linemax; i++) td.note[i] = td.instr[i] = td.volume[i] = td.speed[i] = -1;
	}

	if (special == 0) //normal paste
	{
		for (i = line, j = 0; i < linemax; i++, j++)
		{
			td.note[i] = ts.note[j];
			td.instr[i] = ts.instr[j];
			td.volume[i] = ts.volume[j];
			td.speed[i] = ts.speed[j];
		}
	}
	else //paste special
		if (special == 1) //merge
		{
			for (i = line, j = 0; i < linemax; i++, j++)
			{
				if (ts.note[j] >= 0 && ts.instr[j] >= 0 && ts.volume[j] >= 0)
				{
					td.note[i] = ts.note[j];
					td.instr[i] = ts.instr[j];
					td.volume[i] = ts.volume[j];
				}
				if (ts.volume[j] >= 0) td.volume[i] = ts.volume[j];
				if (ts.speed[j] >= 0)	td.speed[i] = ts.speed[j];
			}
		}
		else
			if (special == 2) //volumes only
			{
				for (i = line, j = 0; i < linemax; i++, j++)
				{
					if (ts.volume[j] >= 0) td.volume[i] = ts.volume[j]; //if the source volume is non-negative, it writes it
					else //the source volume is negative, it can only be deleted where there is no note + instr
						if (td.note[i] < 0 && td.instr[i] < 0) td.volume[i] = -1; //deletes only on separate volumes
				}
			}
			else
				if (special == 3) //speeds only
				{
					for (i = line, j = 0; i < linemax; i++, j++) td.speed[i] = ts.speed[j];
				}

	//if it's beyond the end of the track, extend its length
	if (linemax > td.len) td.len = linemax;
	return (bfro >= 0) ? 0 : linemax - line;	//when it was a paste into a block, it returns 0
}

int CTrackClipboard::BlockClear()
{
	if (!IsBlockSelected() || m_seltrack < 0) return 0;

	int i, bfro, bto;
	GetFromTo(bfro, bto);

	TTrack& td = *g_Tracks.GetTrack(m_seltrack);
	for (i = bfro; i <= bto && i < td.len; i++)
	{
		td.note[i] = -1;
		td.instr[i] = -1;
		td.volume[i] = -1;
		td.speed[i] = -1;
	}
	return bto - bfro + 1;
}

int CTrackClipboard::BlockRestoreFromBackup()
{
	if (!IsBlockSelected() || m_seltrack < 0) return 0;
	memcpy((void*)(g_Tracks.GetTrack(m_seltrack)), (void*)(&m_trackbackup), sizeof(TTrack));
	BlockInitBase(m_seltrack);
	return 1;
}

void CTrackClipboard::GetFromTo(int& from, int& to)
{
	if (!IsBlockSelected()) { from = 1; to = 0; return; }
	if (m_selfrom <= m_selto)
	{
		from = m_selfrom; to = m_selto;
	}
	else
	{
		from = m_selto; to = m_selfrom;
	}
}

void CTrackClipboard::BlockNoteTransposition(int instr, int addnote)
{
	if (!IsBlockSelected() || m_seltrack < 0) return;

	if (instr != m_instrbase)
	{
		//projects the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}

	m_changenote += addnote;
	if (m_changenote >= NOTESNUM) m_changenote = NOTESNUM - 1;
	if (m_changenote <= -NOTESNUM) m_changenote = -NOTESNUM + 1;

	TTrack& td = *g_Tracks.GetTrack(m_seltrack);
	int i, j;
	int bfro, bto;
	GetFromTo(bfro, bto);

	for (i = bfro; i <= bto && i < td.len; i++)
	{
		if (td.note[i] >= 0 && (td.instr[i] == instr || m_all))
		{
			j = m_trackbase.note[i] + m_changenote;
			if (j >= NOTESNUM) j = j - NOTESNUM + 1;
			if (j < 0) j = j + NOTESNUM - 1;
			td.note[i] = j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Note transposition: %+i", m_changenote);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockInstrumentChange(int instr, int addinstr)
{
	if (!IsBlockSelected() || m_seltrack < 0) return;

	if (instr != m_instrbase)
	{
		//add up the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}

	m_changeinstr += addinstr;
	if (m_changeinstr >= INSTRSNUM) m_changeinstr = INSTRSNUM - 1;
	if (m_changeinstr <= -INSTRSNUM) m_changeinstr = -INSTRSNUM + 1;

	TTrack& td = *g_Tracks.GetTrack(m_seltrack);
	int i, j;
	int bfro, bto;
	GetFromTo(bfro, bto);

	for (i = bfro; i <= bto && i < td.len; i++)
	{
		if (m_trackbase.instr[i] >= 0 && (m_trackbase.instr[i] == instr || m_all))
		{
			j = m_trackbase.instr[i] + m_changeinstr;
			if (j >= INSTRSNUM) j = (j % INSTRSNUM);
			if (j < 0) j = (j % INSTRSNUM) + INSTRSNUM;
			td.instr[i] = j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Instrument change: %+i", m_changeinstr);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockVolumeChange(int instr, int addvol)
{
	if (!IsBlockSelected() || m_seltrack < 0) return;

	if (instr != m_instrbase)
	{
		//add up the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}

	m_changevolume += addvol;
	if (m_changevolume > MAXVOLUME) m_changevolume = MAXVOLUME;
	if (m_changevolume < -MAXVOLUME) m_changevolume = -MAXVOLUME;

	TTrack& td = *g_Tracks.GetTrack(m_seltrack);
	int i, j;
	int lasti = -1;
	int bfro, bto;
	GetFromTo(bfro, bto);

	for (i = bfro; i <= bto && i < td.len; i++)
	{
		if (td.instr[i] >= 0) lasti = td.instr[i]; //so that when the volume itself is edited, we know it belongs to the instrument above it
		if (td.volume[i] >= 0 && (lasti == instr || m_all))
		{
			j = m_trackbase.volume[i] + m_changevolume;
			if (j > MAXVOLUME) j = MAXVOLUME;
			if (j < 0) j = 0;
			td.volume[i] = j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Volume change: %+i", m_changevolume);
	SetStatusBarText((LPCTSTR)s);
}


BOOL CTrackClipboard::BlockEffect()
{
	if (!IsBlockSelected() || m_seltrack < 0) return 0;

	CEffectsDlg dlg;

	if (m_all) dlg.m_info = "Changes will be provided for all data in the block";
	else
		dlg.m_info.Format("Changes will be provided for data making use of instrument %02X only", g_Song.GetActiveInstr());

	TTrack* td = g_Tracks.GetTrack(m_seltrack);
	int ainstr = g_Song.GetActiveInstr();
	int bfro, bto;
	GetFromTo(bfro, bto);
	if (bto >= td->len) bto = td->len - 1;

	TTrack m_trackorig;
	memcpy(&m_trackorig, td, sizeof(TTrack));
	dlg.m_trackorig = &m_trackorig;
	dlg.m_trackptr = td;
	dlg.m_bfro = bfro;
	dlg.m_bto = bto;
	dlg.m_ainstr = ainstr;
	dlg.m_all = m_all;

	int r = dlg.DoModal();

	return (r == IDOK);
}