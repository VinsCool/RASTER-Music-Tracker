#include "stdafx.h"
#include <fstream>

#include "resource.h"

#include "Clipboard.h"

#include "Song.h"

#include "MainFrm.h"
#include "EffectsDlg.h"

#include "GuiHelpers.h"


#define EXCH(a, b)	{ int xch = a; a = b; b = xch; }

extern CSong g_Song;


CTrackClipboard::CTrackClipboard()
{
	m_trackcopy.len = -1;	// For copying a complete track with loops, etc.
	m_trackbackup.len = -1; // Back up
	m_all = 1;				// 1 = all events / 0 = only for events with the same instrument as currently set
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
	TTrack* tt = g_Tracks.GetTrack(track);

	// Only process further if data is valid and within boundaries
	if (tt && (m_selcol != col || m_seltrack != track) && g_Tracks.IsValidChannel(col) && g_Tracks.IsValidLine(line))
	{
		// Newly marked start of block
		m_selcol = col;
		m_seltrack = track;
		m_selsongline = g_Song.SongGetActiveLine();
		m_selfrom = m_selto = line;

		// Keep the track as it was now
		m_trackbackup = *tt;

		// And initializes a base track
		BlockInitBase(track);
		return 1;
	}

	return 0;
}

BOOL CTrackClipboard::BlockSetEnd(int line)
{
	if (g_Tracks.IsValidLine(line))
	{
		m_selto = line;
		return 1;
	}

	return 0;
}

void CTrackClipboard::BlockDeselect()
{
	if (IsBlockSelected())
	{
		m_selcol = -1;
		SetStatusBarText("");
	}
}

void CTrackClipboard::BlockInitBase(int track)
{
	TTrack* tt = g_Tracks.GetTrack(track);

	if (tt)
	{
		m_trackbase = *tt;
		m_changenote = 0;
		m_changeinstr = 0;
		m_changevolume = 0;
		m_instrbase = -1;
	}
}

void CTrackClipboard::BlockAllOnOff()
{
	if (IsBlockSelected())
	{
		// Reset the state of the selected base track upon toggle
		BlockInitBase(m_seltrack);
		m_all ^= 1;
	}
}

int CTrackClipboard::BlockCopyToClipboard()
{
	TTrack* ts = g_Tracks.GetTrack(m_seltrack), * td = &m_track;
	int i, line, xline, linefrom, lineto;

	// Process further only if these conditions are respected
	if (ts && td && IsBlockSelected() && IsTrackSelected())
	{
		// Clear the track data first 
		ClearTrack();

		// Get the selection block position and length
		GetFromTo(linefrom, lineto);

		// Copy data from selected lines to clipboard starting at the first line
		for (line = linefrom, i = 0; line <= lineto; line++, i++)
		{
			xline = (line < ts->len || ts->go < 0) ? line : ((line - ts->len) % (ts->len - ts->go)) + ts->go;
			td->note[i] = (line < ts->len || ts->go >= 0) ? ts->note[xline] : -1;
			td->instr[i] = (line < ts->len || ts->go >= 0) ? ts->instr[xline] : -1;
			td->volume[i] = (line < ts->len || ts->go >= 0) ? ts->volume[xline] : -1;
			td->speed[i] = (line < ts->len || ts->go >= 0) ? ts->speed[xline] : -1;
		}

		// Return the length of copied track data
		return td->len = i;
	}

	return 0;
}

int CTrackClipboard::BlockExchangeClipboard()
{
	TTrack* ts = g_Tracks.GetTrack(m_seltrack);
	TTrack* td = &m_track;

	int i, line, linefrom, lineto;

	if (ts && td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(linefrom, lineto);

		// To exchange data in the optimal way, wise loops must be expanded first
		g_Tracks.TrackExpandLoop(ts);
		g_Tracks.TrackExpandLoop(td);

		for (line = linefrom, i = 0; line <= lineto; line++, i++)
		{
			EXCH(td->note[i], ts->note[line]);
			EXCH(td->instr[i], ts->instr[line]);
			EXCH(td->volume[i], ts->volume[line]);
			EXCH(td->speed[i], ts->speed[line]);

			// If the block is longer than the length of the data in the clipboard, fill with empty lines
			if (i >= td->len)
			{
				ts->note[line] = -1;
				ts->instr[line] = -1;
				ts->volume[line] = -1;
				ts->speed[line] = -1;
			}

			// Likewise, if the block is shorter than the data in the track, fill with empty lines
			if (line >= ts->len)
			{
				td->note[i] = -1;
				td->instr[i] = -1;
				td->volume[i] = -1;
				td->speed[i] = -1;
			}
		}

		// Return the length of copied track data
		return td->len = i;
	}

	return 0;
}

int CTrackClipboard::BlockPasteToTrack(int track, int line, int special)
{
	TTrack* ts = &m_track;
	TTrack* td = g_Tracks.GetTrack(track);

	int i, j, linemax;
	int bfro = -1, bto = -1;
	int smallmax = g_Song.GetSmallestMaxtracklen(m_selsongline);

	// A block is selected, so the Paste will be placed in the line position
	if (IsBlockSelected() && IsTrackSelected())
	{
		td = g_Tracks.GetTrack(m_seltrack);
		GetFromTo(bfro, bto);
	}

	if (ts && td)
	{
		// To exchange data in the optimal way, wise loops must be expanded first
		g_Tracks.TrackExpandLoop(ts);
		g_Tracks.TrackExpandLoop(td);

		// Block selected (continued)
		if (bfro >= 0)
		{
			line = bfro;
			linemax = bto + 1;
		}
		else
			linemax = line + ts->len;

		if (linemax > smallmax) linemax = smallmax;

		if (line > td->len)
		{
			// If it makes a paste under the --end-- line, empty the gap between --end-- and the end of the place where it pastes
			// Originally i < line, but because for some it merges to the original, the number of lines will be stretched
			for (i = td->len; i < linemax; i++) td->note[i] = td->instr[i] = td->volume[i] = td->speed[i] = -1;
		}

		for (i = line, j = 0; i < linemax; i++, j++)
		{
			switch (special)
			{
			case 0:	// Normal paste
				td->note[i] = ts->note[j];
				td->instr[i] = ts->instr[j];
				td->volume[i] = ts->volume[j];
				td->speed[i] = ts->speed[j];
				break;

			case 1:	// Merge
				if (g_Tracks.IsValidNote(ts->note[j]) && g_Tracks.IsValidInstrument(ts->instr[j]) && g_Tracks.IsValidVolume(ts->volume[j]))
				{
					td->note[i] = ts->note[j];
					td->instr[i] = ts->instr[j];
					td->volume[i] = ts->volume[j];
				}
				if (g_Tracks.IsValidVolume(ts->volume[j])) td->volume[i] = ts->volume[j];
				if (g_Tracks.IsValidSpeed(ts->speed[j])) td->speed[i] = ts->speed[j];
				break;

			case 2:	// Volumes only
				if (g_Tracks.IsValidVolume(ts->volume[j])) td->volume[i] = ts->volume[j];	// If the source volume is non-negative, it writes it
				else if (!g_Tracks.IsValidNote(td->note[i]) && !g_Tracks.IsValidInstrument(td->instr[i])) td->volume[i] = -1; // Delete only on separate volumes
				break;

			case 3:	// Speeds only
				td->speed[i] = ts->speed[j];
				break;
			}
		}

		// If it's beyond the end of the track, extend its length
		if (linemax > td->len) td->len = linemax;

		// When it was a paste into a block, it returns 0
		return (bfro >= 0) ? 0 : linemax - line;
	}

	return 0;
}

int CTrackClipboard::BlockClear()
{
	TTrack* td = g_Tracks.GetTrack(m_seltrack);
	int i, bfro, bto;

	if (td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(bfro, bto);
		g_Tracks.TrackExpandLoop(td);

		for (i = bfro; i <= bto; i++)
		{
			td->note[i] = -1;
			td->instr[i] = -1;
			td->volume[i] = -1;
			td->speed[i] = -1;
		}

		return i;
	}

	return 0;
}

int CTrackClipboard::BlockRestoreFromBackup()
{
	TTrack* tt = g_Tracks.GetTrack(m_seltrack);

	if (tt && IsBlockSelected() && IsTrackSelected())
	{
		*tt = m_trackbackup;
		BlockInitBase(m_seltrack);
		return 1;
	}

	return 0;
}

void CTrackClipboard::GetFromTo(int& from, int& to)
{
	from = 1; to = 0;

	if (IsBlockSelected())
	{
		from = (m_selfrom <= m_selto) ? m_selfrom : m_selto;
		to = (m_selfrom <= m_selto) ? m_selto : m_selfrom;
	}
}

void CTrackClipboard::BlockNoteTransposition(int instr, int addnote)
{
	TTrack* ts = &m_trackbase, * td = g_Tracks.GetTrack(m_seltrack);
	CString s;
	int i, bfro, bto;

	if (ts && td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(bfro, bto);

		// Reset the state of the selected base track to the chosen instrument
		if (instr != m_instrbase)
		{
			BlockInitBase(m_seltrack);
			m_instrbase = instr;
		}

		m_changenote += addnote;
		m_changenote %= NOTESNUM;

		for (i = bfro; i <= bto && i < td->len; i++)
		{
			if (g_Tracks.IsValidNote(td->note[i]) && (td->instr[i] == instr || m_all))
			{
				td->note[i] = (ts->note[i] + m_changenote + NOTESNUM) % NOTESNUM;
			}
		}

		// Status bar info
		s.Format("Note transposition: %+i semitone(s)", m_changenote);
		SetStatusBarText(s);
	}
}

void CTrackClipboard::BlockInstrumentChange(int instr, int addinstr)
{
	TTrack* ts = &m_trackbase, * td = g_Tracks.GetTrack(m_seltrack);
	CString s;
	int i, bfro, bto;

	if (ts && td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(bfro, bto);

		// Reset the state of the selected base track to the chosen instrument
		if (instr != m_instrbase)
		{
			BlockInitBase(m_seltrack);
			m_instrbase = instr;
		}

		m_changeinstr += addinstr;
		m_changeinstr %= INSTRSNUM;

		for (i = bfro; i <= bto && i < td->len; i++)
		{
			if (g_Tracks.IsValidInstrument(td->instr[i]) && (td->instr[i] == instr || m_all))
			{
				td->instr[i] = (ts->instr[i] + m_changeinstr + INSTRSNUM) % INSTRSNUM;
			}
		}

		// Status bar info
		s.Format("Instrument change: %+i", m_changeinstr);
		SetStatusBarText(s);
	}
}

void CTrackClipboard::BlockVolumeChange(int instr, int addvol)
{
	TTrack* ts = &m_trackbase, * td = g_Tracks.GetTrack(m_seltrack);
	CString s;
	int i, bfro, bto;
	int lasti = -1;

	if (ts && td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(bfro, bto);

		// Reset the state of the selected base track to the chosen instrument
		if (instr != m_instrbase)
		{
			BlockInitBase(m_seltrack);
			m_instrbase = instr;
		}

		m_changevolume += addvol;
		m_changevolume %= MAXVOLUME + 1;

		for (i = bfro; i <= bto && i < td->len; i++)
		{
			// When the volume itself is edited, we know it belongs to the instrument above it
			if (g_Tracks.IsValidInstrument(td->instr[i])) lasti = td->instr[i];

			if (g_Tracks.IsValidVolume(td->volume[i]) && (lasti == instr || m_all))
			{
				td->volume[i] = ts->volume[i] + m_changevolume;

				// Unlike the Note, Instruments, Effects, etc, we want to actually cap the volume changes
				if (td->volume[i] > MAXVOLUME) td->volume[i] = MAXVOLUME;
				if (td->volume[i] < 0) td->volume[i] = 0;
			}
		}

		// Status bar info
		s.Format("Volume change: %+i", m_changevolume);
		SetStatusBarText(s);
	}
}

BOOL CTrackClipboard::BlockEffect()
{
	CEffectsDlg dlg;
	TTrack* td = g_Tracks.GetTrack(m_seltrack);
	TTrack m_trackorig;
	int bfro, bto;
	int ainstr = g_Song.GetActiveInstr();
	
	if (td && IsBlockSelected() && IsTrackSelected())
	{
		GetFromTo(bfro, bto);
		if (bto >= td->len) bto = td->len - 1;

		m_trackorig = *td;
		dlg.m_trackorig = &m_trackorig;
		dlg.m_trackptr = td;
		dlg.m_bfro = bfro;
		dlg.m_bto = bto;
		dlg.m_ainstr = ainstr;
		dlg.m_all = m_all;
		dlg.m_info.Format(m_all ? "Changes will be provided for all data in the block" : "Changes will be provided for data making use of instrument %02X only", ainstr);

		return (dlg.DoModal() == IDOK);
	}

	return 0;
}