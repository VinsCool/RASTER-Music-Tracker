#include "stdafx.h"
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"
#include "Instruments.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"

CTracks::CTracks()
{
	m_maxTrackLength = 64;			// Default value
	if (m_track) delete m_track;
	m_track = new TTrack[TRACKSNUM];
}

CTracks::~CTracks()
{
	if (m_track) delete m_track;
	m_track = NULL;
}

void CTracks::InitTracks()
{
	for (int i = 0; i < TRACKSNUM; i++)
	{
		ClearTrack(i);
	}
}

void CTracks::ClearTrack(int track)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return;

	// Clear everything, set to -1 for empty values
	memset(tr, -1, sizeof(TTrack));

	// Except for Maxtracklength, set to the last known parameter
	tr->len = m_maxTrackLength;
}

BOOL CTracks::IsEmptyTrack(int track)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	// If the track length doesn't match Maxtracklength, it is not empty
	if (tr->len != m_maxTrackLength) return 0;

	// Test for values in track, if it is equal or above 0, it is not empty
	for (int i = 0; i < m_maxTrackLength; i++)
	{
		if (tr->volume[i] >= 0 || tr->speed[i] >= 0 || tr->note[i] >= 0) return 0;
	}

	// If everything failed, the track is definitely empty
	return 1;
}

BOOL CTracks::DelNoteInstrVolSpeed(int noteinstrvolspeed, int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOLSPEED);
	g_Undo.Separator();

	// If the line on track is within boundaries, continue
	if (line >= 0 && line < tr->len)
	{
		if (noteinstrvolspeed & 1) tr->note[line] = -1;
		if (noteinstrvolspeed & 2) tr->instr[line] = -1;
		if (noteinstrvolspeed & 4) tr->volume[line] = -1;
		if (noteinstrvolspeed & 8) tr->speed[line] = -1;
		return 1;
	}

	// Else, nothing will be deleted
	return 0;
}

BOOL CTracks::SetNoteInstrVol(int note, int instr, int vol, int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	g_Undo.Separator();

	// If the line on track is within boundaries, continue
	if (line >= 0 && line < tr->len)
	{
		if (note < 0) instr = vol = -1;

		if (!g_respectvolume || (g_respectvolume && (vol < 0 || tr->volume[line] < 0))) 
			tr->volume[line] = vol;

		tr->note[line] = note;
		tr->instr[line] = instr;
		return 1;
	}

	// Else, nothing will be set
	return 0;
}

BOOL CTracks::SetInstr(int instr, int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	//g_Undo.Separator();	// Why no undo separator?

	// If the line on track is within boundaries, continue
	if (line >= 0 && line < tr->len)
	{
		tr->instr[line] = instr;
		return 1;
	}

	// Else, nothing will be set
	return 0;
}

BOOL CTracks::SetVol(int vol, int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	//g_Undo.Separator();	// Why no undo separator?

	// If the line on track is within boundaries, continue
	if (line >= 0 && line < tr->len)
	{
		tr->volume[line] = vol;
		return 1;
	}

	// Else, nothing will be set
	return 0;
}

BOOL CTracks::SetSpeed(int speed, int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_SPEED);
	//g_Undo.Separator();	// Why no undo separator?

	// If the line on track is within boundaries, continue
	if (line >= 0 && line < tr->len)
	{
		tr->speed[line] = speed;
		return 1;
	}

	// Else, nothing will be set
	return 0;
}

BOOL CTracks::SetEnd(int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	g_Undo.ChangeTrack(track, line, UETYPE_LENGO, 1);
	//g_Undo.Separator();	// Why no undo separator?

	// Set the track length to
	tr->len = (line > 0 && tr->len != line) ? line : m_maxTrackLength;
	if (tr->go >= tr->len) tr->go = -1;
	return 1;
}

int CTracks::GetLastLine(int track)
{
	TTrack* tr = GetTrack(track);
	return (tr) ? tr->len - 1 : -1;
}

int CTracks::GetLength(int track)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return -1;
	return tr->go >= 0 ? m_maxTrackLength : tr->len;
}

BOOL CTracks::SetGo(int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;
	if (line >= tr->len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_LENGO, 1);
	tr->go = tr->go == line ? -1 : line;
	return 1;
}

int CTracks::GetGoLine(int track)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;
	return (track >= 0) ? tr->go : -1;
}

BOOL CTracks::InsertLine(int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	if (tr->len < 0) return 0;

	for (int i = tr->len - 2; i >= line; i--)
	{
		tr->note[i + 1] = tr->note[i];
		tr->instr[i + 1] = tr->instr[i];
		tr->volume[i + 1] = tr->volume[i];
		tr->speed[i + 1] = tr->speed[i];
	}

	tr->note[line] = tr->instr[line] = tr->volume[line] = tr->speed[line] = -1;
	return 1;
}

BOOL CTracks::DeleteLine(int track, int line)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	if (tr->len < 0) return 0;

	for (int i = line; i < tr->len - 1; i++)
	{
		tr->note[i] = tr->note[i + 1];
		tr->instr[i] = tr->instr[i + 1];
		tr->volume[i] = tr->volume[i + 1];
		tr->speed[i] = tr->speed[i + 1];
	}

	line = tr->len - 1;
	tr->note[line] = tr->instr[line] = tr->volume[line] = tr->speed[line] = -1;
	return 1;
}

/// <summary>
/// Check if a specific track has any valid information set.
/// Check for track length, notes, and volume and speed changes
/// </summary>
/// <param name="track">Which track is being checked</param>
/// <returns>TRUE if the track is NOT empty, FALSE is there is nothing set on it</returns>
BOOL CTracks::CalculateNotEmpty(int trackNr)
{
	// Get the track data
	TTrack* tr = GetTrack(trackNr);
	if (!tr) return 0;

	// Check if anything has been set
	if (tr->len != m_maxTrackLength)	// If the length anything but the maximum track length?
		return 1;						// Yes, the its NOT EMPTY

	// Check if the any note, volume or speed changes have been set
	for (int i = 0; i < tr->len; i++)
	{
		// Any note, volume or speed?
		if (tr->note[i] >= 0 || tr->volume[i] >= 0 || tr->speed[i] >= 0)
		{
			return 1;	// Not empty
		}
	}

	return 0;	// Is empty
}

BOOL CTracks::CompareTracks(int track1, int track2)
{
	// If one of the tracks is invalid, bail out of this function
	TTrack* t1 = GetTrack(track1);
	TTrack* t2 = GetTrack(track2);
	if (!t1 || !t2) return 0;

	// If the Length or Loop isn't matching, no doubt about the difference
	if (t1->len != t2->len || t1->go != t2->go) return 0;

	// Compare the tracks and searach for a mismatched value
	for (int i = 0; i < t1->len; i++)
	{
		if (t1->note[i] != t2->note[i] || t1->instr[i] != t2->instr[i] || t1->volume[i] != t2->volume[i] || t1->speed[i] != t2->speed[i]) 
			return 0;	// Found a difference => they are not the same
	}

	return 1;	// Did not find a difference => they are the same
}

int CTracks::TrackOptimizeVol0(int track)
{
	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	int lastzline = -1;
	int kline = -1;	// Candidate for deletion including note

	for (int i = 0; i < tr->len; i++)
	{
		if (tr->volume[i] == 0)
		{
			if (lastzline >= 0)
			{
				if (kline >= 0)	// Any candidate to delete? (note + vol0 in the middle between zero volumes)
				{
					tr->note[kline] = tr->instr[kline] = tr->volume[kline] = -1;
				}
				if (tr->note[i] < 0 && tr->instr[i] < 0)
					tr->volume[i] = -1;	// Cancel this volume
				else
					kline = i;
			}
			else
			{
				// This is currently the last line with zero volume
				lastzline = i;
			}
		}
		else
			if (tr->volume[i] > 0)
			{
				lastzline = kline = -1;
			}
	}
	return 1;
}

int CTracks::TrackBuildLoop(int track)
{
	if (IsEmptyTrack(track)) return 0;	// Empty track

	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	if (tr->go >= 0) return 0;		// There is a loop
	if (tr->len != m_maxTrackLength) return 0;	// It is not full length => it cannot make a loop there

	int i, j, k, m;

	for (i = 1; i < tr->len; i++)
	{
		for (j = 0; j < i; j++)
		{
			for (k = 0; i + k < tr->len; k++)
			{
				if (tr->note[i + k] == tr->note[j + k] && tr->instr[i + k] == tr->instr[j + k] && tr->volume[i + k] == tr->volume[j + k] && tr->speed[i + k] == tr->speed[j + k])
				{
					continue;
				}
				break;
			}
			if (k > 1 && i + k == tr->len)
			{
				// It managed to find a loop at least 2 bars long lasting until the end
				// Check to see if it's not empty in that loop
				int p = 0;
				for (m = 0; i + m < tr->len; m++)
				{
					if (tr->note[j + m] >= 0 || tr->instr[j + m] >= 0 || tr->volume[j + m] >= 0 || tr->speed[j + m] >= 0)
					{
						p++;
						if (p > 1) // Yes, it found at least two nonzero lines inside the loop
						{
							tr->len = i;
							tr->go = j;
							return k;	// Returns the length of the loop found
						}
					}
				}
			}
		}
	}
	return 0;
}

int CTracks::TrackExpandLoop(int track)
{
	if (IsEmptyTrack(track)) return 0;	// Empty track

	TTrack* tr = GetTrack(track);
	if (!tr) return 0;

	// Length of the expanded loop
	return TrackExpandLoop(tr);
}

int CTracks::TrackExpandLoop(TTrack* ttrack)
{
	if (!ttrack) return 0;
	if (ttrack->go < 0) return 0;		// There is no loop

	int i, j, k;
	for (i = 0; ttrack->len + i < m_maxTrackLength; i++)
	{
		j = ttrack->len + i;
		k = ttrack->go + i;
		ttrack->note[j] = ttrack->note[k];
		ttrack->instr[j] = ttrack->instr[k];
		ttrack->volume[j] = ttrack->volume[k];
		ttrack->speed[j] = ttrack->speed[k];
	}
	ttrack->len = m_maxTrackLength;	// Full length
	ttrack->go = -1;				// No loop

	return i;	// Length of the expanded loop
}

void CTracks::GetTracksAll(TTracksAll* toTracks)
{
	toTracks->maxtracklength = m_maxTrackLength;
	for (int i = 0; i < TRACKSNUM; i++) memcpy((void*)&toTracks->tracks[i], (void*)&m_track[i], sizeof(TTrack));
}

void CTracks::SetTracksAll(TTracksAll* fromTracks)
{
	m_maxTrackLength = fromTracks->maxtracklength;
	for (int i = 0; i < TRACKSNUM; i++) memcpy((void*)&m_track[i], (void*)&fromTracks->tracks[i], sizeof(TTrack));
}

int CTracks::GetModifiedNote(int note, int tuning)
{
	if (!IsValidNote(note)) return -1;

	int n = note + tuning;

	if (n < 0) n += ((int)((-n - 1) / 12) + 1) * 12;
	else if (n >= NOTESNUM) n -= ((int)(n - NOTESNUM) / 12 + 1) * 12;
	return n;
}

int CTracks::GetModifiedInstr(int instr, int instradd)
{
	if (!IsValidInstrument(instr)) return -1;

	int i = instr + instradd;
	while (i < 0) { i += INSTRSNUM; }
	while (i >= INSTRSNUM) { i -= INSTRSNUM; }
	return i;
}

int CTracks::GetModifiedVolumeP(int volume, int percentage)
{
	if (volume < 0) return -1;
	if (percentage <= 0) return 0;
	int v = (int)((float)percentage / 100 * volume + 0.5);
	return (v > MAXVOLUME) ? MAXVOLUME : v;
}

// TODO: edit this function to remove the need for using the TTrack pointer directly
BOOL CTracks::ModifyTrack(TTrack* track, int from, int to, int instrnumonly, int tuning, int instradd, int volumep)
{
	//instruments <0 => all instruments
	//            > = 0 => only that one instrument
	if (!track) return 0;
	if (to >= TRACKLEN) to = TRACKLEN - 1;
	int i, instr;
	int ainstr = -1;
	for (i = from; i <= to; i++)
	{
		instr = track->instr[i];
		if (instr >= 0) ainstr = instr;
		if (instrnumonly >= 0 && instrnumonly != ainstr) continue;
		track->note[i] = GetModifiedNote(track->note[i], tuning);
		track->instr[i] = GetModifiedInstr(instr, instradd);
		track->volume[i] = GetModifiedVolumeP(track->volume[i], volumep);
	}
	return 1;
}