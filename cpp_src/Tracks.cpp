#include "stdafx.h"
using namespace std;
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"


//TODO: Optimise the notes arrays to be more compact, there is a lot of duplicates
const char* notes[] =
{ "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","B-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","B-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","B-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","B-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","B-5",
  "C-6","???","???","???"
};

const char* notesandscales[5][40] =
{ 
	//Standard Western Notation, Sharp (#) accidentals 
	{ "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-" },

	//Standard Western Notation, Flat (b) accidentals 
	{ "C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "Bb", "B-" },

	//German Notation, Sharp (#) accidentals 
	{ "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "H-" },

	//German Notation, Flat (b) accidentals 
	{ "C-", "Db", "D-", "Eb", "E-", "F-", "Gb", "G-", "Ab", "A-", "B-", "H-" },

	//Test Notation
	{ "1-", "2-", "3-", "4-", "5-", "6-", "7-", "8-", "9-", "A-", "B-", "C-", 
	"D-", "E-", "F-", "G-", "H-", "I-", "J-", "K-", "L-", "M-", "N-", "O-", 
	"P-", "Q-", "R-", "S-", "T-", "U-", "V-", "W-", "X-", "Y-", "Z-" }
};


int GetModifiedNote(int note, int tuning)
{
	if (note < 0 || note >= NOTESNUM) return -1;
	int n = note + tuning;
	if (n < 0)
		n += ((int)((-n - 1) / 12) + 1) * 12;
	else
		if (n >= NOTESNUM)
			n -= ((int)(n - NOTESNUM) / 12 + 1) * 12;
	return n;
}

int GetModifiedInstr(int instr, int instradd)
{
	if (instr < 0 || instr >= INSTRSNUM) return -1;
	int i = instr + instradd;
	while (i < 0) { i += INSTRSNUM; }
	while (i >= INSTRSNUM) { i -= INSTRSNUM; }
	return i;
}

int GetModifiedVolumeP(int volume, int percentage)
{
	if (volume < 0) return -1;
	if (percentage <= 0) return 0;
	int v = (int)((float)percentage / 100 * volume + 0.5);
	return (v > MAXVOLUME) ? MAXVOLUME : v;
}

BOOL ModifyTrack(TTrack* track, int from, int to, int instrnumonly, int tuning, int instradd, int volumep)
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

CTracks::CTracks()
{
	m_maxTrackLength = 64;			//default value
	// g_cursoractview = m_maxtracklen / 2;
	InitTracks();
}

void CTracks::InitTracks()
{
	for (int i = 0; i < TRACKSNUM; i++)
	{
		ClearTrack(i);
	}
}

BOOL CTracks::ClearTrack(int t)
{
	if (t < 0 || t >= TRACKSNUM) return 0;

	m_track[t].len = m_maxTrackLength;	//32+(rand()&0x1f);		//0;
	m_track[t].go = -1;					//-1+(rand()&0x01);			//-1;
	for (int i = 0; i < TRACKLEN; i++)
	{
		m_track[t].note[i] = -1;		//(rand()&0xff)-192;		//-1;
		m_track[t].instr[i] = -1;		//rand()&0xff;	//-1;
		m_track[t].volume[i] = -1;		//rand()&0x0f;	//-1;
		m_track[t].speed[i] = -1;		//rand()&0xff;	//-1;
	}
	return 1;
}

BOOL CTracks::IsEmptyTrack(int track)
{
	if (track < 0 || track >= TRACKSNUM) return 0;

	if (m_track[track].len != m_maxTrackLength) return 0;
	int* tvolumes = (int*)&m_track[track].volume;
	int* tspeeds = (int*)&m_track[track].speed;

	for (int i = 0; i < m_maxTrackLength; i++)
	{
		if (*(tvolumes + i) >= 0 || *(tspeeds + i) >= 0) return 0;
	}
	return 1;
}

BOOL CTracks::DelNoteInstrVolSpeed(int noteinstrvolspeed, int track, int line)
{
	if (line >= m_track[track].len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOLSPEED);
	g_Undo.Separator();
	if (noteinstrvolspeed & 1) m_track[track].note[line] = -1;
	if (noteinstrvolspeed & 2) m_track[track].instr[line] = -1;
	if (noteinstrvolspeed & 4) m_track[track].volume[line] = -1;
	if (noteinstrvolspeed & 8) m_track[track].speed[line] = -1;
	return 1;
}

BOOL CTracks::SetNoteInstrVol(int note, int instr, int vol, int track, int line)
{
	if (note >= NOTESNUM || line >= m_track[track].len) return 0;
	if (note == -1)
	{
		instr = vol = -1;
	}
	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	g_Undo.Separator();
	m_track[track].note[line] = note;
	m_track[track].instr[line] = instr;
	if (g_respectvolume)
	{
		//overwrites volume only if it is empty or if it wants to cancel it (-1)
		if (m_track[track].volume[line] < 0 || vol < 0)
			m_track[track].volume[line] = vol;
	}
	else
		m_track[track].volume[line] = vol;	//always
	return 1;
}

BOOL CTracks::SetInstr(int instr, int track, int line)
{
	if (line >= m_track[track].len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	m_track[track].instr[line] = instr;
	return 1;
}

BOOL CTracks::SetVol(int vol, int track, int line)
{
	if (line >= m_track[track].len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_NOTEINSTRVOL);
	m_track[track].volume[line] = vol;
	return 1;
}

BOOL CTracks::SetSpeed(int speed, int track, int line)
{
	if (line >= m_track[track].len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_SPEED);
	m_track[track].speed[line] = speed;
	return 1;
}

BOOL CTracks::SetEnd(int track, int line)
{
	if (track < 0 || track >= TRACKSNUM) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_LENGO, 1);
	m_track[track].len = (line > 0 && m_track[track].len != line) ? line : m_maxTrackLength;
	if (m_track[track].go >= m_track[track].len) m_track[track].go = -1;
	return 1;
}

int CTracks::GetLastLine(int track)
{
	return (track >= 0 && track < TRACKSNUM) ? m_track[track].len - 1 : -1;	//m_maxtracklen-1; // originally there was only ...: -1
}

int CTracks::GetLength(int track)
{
	if (track < 0 || track >= TRACKSNUM) return -1;
	if (m_track[track].go >= 0) return m_maxTrackLength;
	return m_track[track].len;
}

BOOL CTracks::SetGo(int track, int line)
{
	if (track < 0 || track >= TRACKSNUM) return 0;
	if (line >= m_track[track].len) return 0;
	g_Undo.ChangeTrack(track, line, UETYPE_LENGO, 1);
	m_track[track].go = (m_track[track].go == line) ? -1 : line;
	return 1;
}

int CTracks::GetGoLine(int track)
{
	return (track >= 0) ? m_track[track].go : -1;
}

BOOL CTracks::InsertLine(int track, int line)
{
	if (track < 0 || track >= TRACKSNUM) return 0;
	TTrack& t = m_track[track];
	if (t.len < 0) return 0;
	for (int i = t.len - 2; i >= line; i--)
	{
		t.note[i + 1] = t.note[i];
		t.instr[i + 1] = t.instr[i];
		t.volume[i + 1] = t.volume[i];
		t.speed[i + 1] = t.speed[i];
	}
	t.note[line] = t.instr[line] = t.volume[line] = t.speed[line] = -1;
	return 1;
}

BOOL CTracks::DeleteLine(int track, int line)
{
	if (track < 0 || track >= TRACKSNUM) return 0;
	TTrack& t = m_track[track];
	if (t.len < 0) return 0;
	for (int i = line; i < t.len - 1; i++)
	{
		t.note[i] = t.note[i + 1];
		t.instr[i] = t.instr[i + 1];
		t.volume[i] = t.volume[i + 1];
		t.speed[i] = t.speed[i + 1];
	}
	line = t.len - 1;
	t.note[line] = t.instr[line] = t.volume[line] = t.speed[line] = -1;
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
	// Check if the track is empty
	if (trackNr < 0 || trackNr >= TRACKSNUM) return FALSE;

	// Get the track data
	TTrack& t = m_track[trackNr];

	// Check if anything has been set
	if (t.len != m_maxTrackLength)	// If the length anything but the maximum track length?
		return TRUE;					// Yes, the its NOT EMPTY

	// Check if the any note, volume or speed changes have been set
	for (int i = 0; i < t.len; i++)
	{
		if (   t.note[i] >= 0			// any note, volume or speed?
			|| t.volume[i] >= 0
			|| t.speed[i] >= 0)
		{
			return TRUE;	//not empty
		}
	}
	return FALSE;	//is empty
}

BOOL CTracks::CompareTracks(int track1, int track2)
{
	if (track1 == track2) return 1;
	if (track1 < 0 || track1 >= TRACKSNUM || track2 < 0 || track2 >= TRACKSNUM) return 0;
	TTrack& t1 = m_track[track1];
	TTrack& t2 = m_track[track2];
	if (t1.len != t2.len || t1.go != t2.go) return 0;
	for (int i = 0; i < t1.len; i++)
	{
		if (t1.note[i] != t2.note[i]
			|| t1.instr[i] != t2.instr[i]
			|| t1.volume[i] != t2.volume[i]
			|| t1.speed[i] != t2.speed[i]
			) return 0;		//found a difference => they are not the same
	}
	return 1;	//did not find a difference => they are the same
}

int CTracks::TrackOptimizeVol0(int track)
{
	//removes redundant volume 0 data
	if (track < 0 || track >= TRACKSNUM) return 0;
	TTrack& tr = m_track[track];
	int lastzline = -1;
	int kline = -1;	//candidate for deletion including note
	for (int i = 0; i < tr.len; i++)
	{
		if (tr.volume[i] == 0)
		{
			if (lastzline >= 0)
			{
				if (kline >= 0)	//any candidate to delete? (note + vol0 in the middle between zero volumes)
				{
					tr.note[kline] = tr.instr[kline] = tr.volume[kline] = -1;
				}
				if (tr.note[i] < 0 && tr.instr[i] < 0)
					tr.volume[i] = -1;	//cancel this volume
				else
					kline = i;
			}
			else
			{
				//this is currently the last line with zero volume
				lastzline = i;
			}
		}
		else
			if (tr.volume[i] > 0)
			{
				lastzline = kline = -1;
			}
	}
	return 1;
}

int CTracks::TrackBuildLoop(int track)
{
	if (track < 0 || track >= TRACKSNUM) return 0;

	TTrack& tr = m_track[track];
	if (IsEmptyTrack(track)) return 0;	//empty track
	if (tr.go >= 0) return 0;		//there is a loop
	if (tr.len != m_maxTrackLength) return 0;	//it is not full length => it cannot make a loop there

	int i, j, k, m;
	for (i = 1; i < tr.len; i++)
	{
		for (j = 0; j < i; j++)
		{
			for (k = 0; i + k < tr.len; k++)
			{
				if (tr.note[i + k] == tr.note[j + k]
					&& tr.instr[i + k] == tr.instr[j + k]
					&& tr.volume[i + k] == tr.volume[j + k]
					&& tr.speed[i + k] == tr.speed[j + k])
				{
					continue;
				}
				break;
			}
			if (k > 1 && i + k == tr.len)
			{
				//it managed to find a loop at least 2 bars long lasting until the end
				//check to see if it's not empty in that loop
				int p = 0;
				for (m = 0; i + m < tr.len; m++)
				{
					if (tr.note[j + m] >= 0
						|| tr.instr[j + m] >= 0
						|| tr.volume[j + m] >= 0
						|| tr.speed[j + m] >= 0)
					{
						p++;
						if (p > 1) //yes, it found at least two nonzero lines inside the loop
						{
							tr.len = i;
							tr.go = j;
							return k;	//returns the length of the loop found
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
	if (track < 0 || track >= TRACKSNUM) return 0;

	TTrack* tr = &m_track[track];
	if (IsEmptyTrack(track)) return 0;	//empty track
	int i = TrackExpandLoop(tr);
	return i;	//length of the expanded loop
}

int CTracks::TrackExpandLoop(TTrack* ttrack)
{
	if (!ttrack) return 0;
	TTrack& tr = *ttrack;
	if (tr.go < 0) return 0;		//there is no loop

	int i, j, k;
	for (i = 0; tr.len + i < m_maxTrackLength; i++)
	{
		j = tr.len + i;
		k = tr.go + i;
		tr.note[j] = tr.note[k];
		tr.instr[j] = tr.instr[k];
		tr.volume[j] = tr.volume[k];
		tr.speed[j] = tr.speed[k];
	}
	tr.len = m_maxTrackLength;	//full length
	tr.go = -1;				//no loop

	return i;	//length of the expanded loop
}

void CTracks::GetTracksAll(TTracksAll* dest_ta)
{
	dest_ta->maxtracklength = m_maxTrackLength;
	memcpy(dest_ta->tracks, &m_track, sizeof(m_track));
}

void CTracks::SetTracksAll(TTracksAll* src_ta)
{
	m_maxTrackLength = src_ta->maxtracklength;
	memcpy(&m_track, src_ta->tracks, sizeof(m_track));
}
