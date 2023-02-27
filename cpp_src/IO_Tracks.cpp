#include "stdafx.h"
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"


#define WRITEATIDX(value) { if (idx < max) { dest[idx] = value; idx++; } else return -1; }
#define WRITEPAUSE(pause)							\
{													\
	if (pause >= 1 && pause <= 3)					\
		{ WRITEATIDX(62 | (pause << 6)); }			\
	else											\
		{ WRITEATIDX(62); WRITEATIDX(pause); }		\
}


int CTracks::SaveTrack(int track, std::ofstream& ou, int iotype)
{
	TTrack* at = GetTrack(track);
	if (!at) return 0;

	CString s;

	switch (iotype)
	{
	case IOTYPE_RMW:
		ou.write((char*)&at->len, sizeof(at->len));
		ou.write((char*)&at->go, sizeof(at->go));
		for (int i = 0; i < m_maxTrackLength; i++) ou.write((char*)&at->note[i], 1);
		for (int i = 0; i < m_maxTrackLength; i++) ou.write((char*)&at->instr[i], 1);
		for (int i = 0; i < m_maxTrackLength; i++) ou.write((char*)&at->volume[i], 1);
		for (int i = 0; i < m_maxTrackLength; i++) ou.write((char*)&at->speed[i], 1);
		return 1;

	case IOTYPE_TXT:
		s.Format("[TRACK]\n");	// Track text header
		s.AppendFormat(!IsValidTrack(track) ? "--  " : "%02X  ", track);
		s.AppendFormat(!IsValidLength(at->len) ? "--" : "%02X", at->len);
		s.AppendFormat(!IsValidGo(at->go) ? "--" : "%02X", at->go);
		s.AppendFormat("\n");
		for (int i = 0; i < at->len; i++)
		{	// Track row data
			s.AppendFormat(!IsValidNote(at->note[i]) ? "---" : notes[at->note[i]]);
			s.AppendFormat(!IsValidInstrument(at->instr[i]) ? " --" : " %02X", at->instr[i]);
			s.AppendFormat(!IsValidVolume(at->volume[i]) ? " -" : " %01X", at->volume[i]);
			s.AppendFormat(!IsValidSpeed(at->speed[i]) ? "" : "%02X", at->speed[i]);
			s.AppendFormat("\n");
		}
		ou << s << std::endl;
		return 1;
	}

	return 0;
}

int CTracks::LoadTrack(int track, std::ifstream& in, int iotype)
{
	TTrack* at;
	int idx = 0;
	int a;
	char b;
	char line[1025];

	switch (iotype)
	{
	case IOTYPE_RMW:
		ClearTrack(track);	// Clear before filling with data
		at = GetTrack(track);
		if (!at) return 0;
		in.read((char*)&at->len, sizeof(at->len));
		in.read((char*)&at->go, sizeof(at->go));
		for (int i = 0; i < m_maxTrackLength; i++) { in.read((char*)&b, 1); at->note[i] = b; }
		for (int i = 0; i < m_maxTrackLength; i++) { in.read((char*)&b, 1); at->instr[i] = b; }
		for (int i = 0; i < m_maxTrackLength; i++) { in.read((char*)&b, 1); at->volume[i] = b; }
		for (int i = 0; i < m_maxTrackLength; i++) { in.read((char*)&b, 1); at->speed[i] = b; }
		return 1;

	case IOTYPE_TXT:
		memset(line, 0, 16);
		in.getline(line, 1024); // The first line of the track

		// If the track is invalid, it is set to the value found in the text file
		if (track == -1) track = Hexstr(line, 2);

		// If the track is still invalid, it will be skipped
		if (!IsValidTrack(track))
		{
			NextSegment(in);
			return 1;
		}

		// Clear the track before it is filled with new data
		ClearTrack(track);
		at = GetTrack(track);
		if (!at) return 0;

		// Find the Track Length and Go loop first
		a = Hexstr(line + 4, 2);
		at->len = (!IsValidLength(a) || a > m_maxTrackLength) ? m_maxTrackLength : a;
		a = Hexstr(line + 6, 2);
		at->go = a > at->len ? -1 : a;

		// Now, read the Track data until the end of file is reached
		while (!in.eof())
		{
			in.read((char*)&b, 1);

			// End of track (beginning of something else)
			if (b == '[') return 1;

			// Right at the beginning of the line is EOL, or the maximal length was reached
			if (b == 10 || b == 13 || idx >= TRACKLEN)
			{
				NextSegment(in);
				return 1;
			}

			// Clear the memory, then read the next line
			memset(line, 0, 16);
			line[0] = b;
			in.getline(line + 1, 1024);

			// If the Note is Sharp, a semitone is added to the offset
			a = line[1] == '#' ? 1 : 0;

			// Find the Note index, invalid values will be ignored
			switch (b)
			{
			case 'C': a += 0; break;
			case 'D': a += 2; break;
			case 'E': a += 4; break;
			case 'F': a += 5; break;
			case 'G': a += 7; break;
			case 'A': a += 9; break;
			case 'B': a += 11; break;
			default: a = -1;
			}

			// Add the Octave to the Note for the actual index
			b = line[2];
			a = (a >= 0 && b >= '1' && b <= '6') ? a + (b - '1') * 12 : a;

			// Process the Track data for the current line 
			at->note[idx] = IsValidNote(a) ? a : -1;
			at->instr[idx] = IsValidInstrument(a = Hexstr(line + 4, 2)) ? a : -1;
			at->volume[idx] = IsValidVolume(a = Hexstr(line + 7, 1)) ? a : -1;
			at->speed[idx] = IsValidSpeed(a = Hexstr(line + 8, 2)) ? a : -1;

			if (at->note[idx] >= 0 && at->instr[idx] < 0) at->instr[idx] = 0;			// If the note is without an instrument, then instrument 0 hits there
			if (at->instr[idx] >= 0 && at->note[idx] < 0) at->instr[idx] = -1;			// If the instrument is without a note, then it cancels it
			if (at->note[idx] >= 0 && at->volume[idx] < 0) at->volume[idx] = MAXVOLUME;	// If the note is non-volume, adds the maximum volume

			idx++;	// Increment the line index for the next iteration
		}
		return 1;
	}

	return 0;
}

int CTracks::SaveAll(std::ofstream& ou, int iotype)
{
	switch (iotype)
	{
		case IOTYPE_RMW:
		{
			ou.write((char*)&m_maxTrackLength, sizeof(m_maxTrackLength));
			for (int i = 0; i < TRACKSNUM; i++)
			{
				SaveTrack(i, ou, iotype);
			}
		}
		break;

		case IOTYPE_TXT:
		{
			for (int i = 0; i < TRACKSNUM; i++)
			{
				if (!CalculateNotEmpty(i)) continue;	//saves only non-empty tracks
				SaveTrack(i, ou, iotype);
			}
		}
		break;
	}
	return 1;
}


int CTracks::LoadAll(std::ifstream& in, int iotype)
{
	InitTracks();

	in.read((char*)&m_maxTrackLength, sizeof(m_maxTrackLength));

	for (int i = 0; i < TRACKSNUM; i++)
	{
		LoadTrack(i, in, iotype);
	}
	return 1;
}

int CTracks::TrackToAta(int trackNr, unsigned char* dest, int max)
{
	// Get the data that describes the track
	TTrack* t = GetTrack(trackNr);
	if (!t) return 0;

	int note, instr, volume, speed;
	int idx = 0;
	int goidx = -1;
	int pause = 0;

	// Run over each line in the track and save its data away in the most compact form
	for (int i = 0; i < t->len; i++)
	{
		// Get the track line data
		note	= t->note[i];
		instr	= t->instr[i];
		volume	= t->volume[i];
		speed	= t->speed[i];

		// Something will be there, write empty measures first
		if (volume >= 0 || speed >= 0 || t->go == i) 
		{
			if (pause > 0)
			{
				// Emit 1 or 2 bytes for the pause
				WRITEPAUSE(pause);
				pause = 0;
			}

			// It will jump with a go loop
			if (t->go == i) goidx = idx;

			// Speed changes are stored BEFORE note data
			if (speed >= 0)
			{
				// Is speed
				WRITEATIDX(63);		// 63 = speed change
				WRITEATIDX(speed & 0xff);

				pause = 0;			// Reset the pause counter
			}
		}

		// What is changing?
		if (note >= 0 && instr >= 0 && volume >= 0)
		{
			// Note, instrument and volume is changing
			// Byte 0:	Bit 0-5 = Note
			//			Bit 6-7 = low 2 bits of the volume
			// Byte 1:	Bit 0-1 = high 2 bits of the volume
			//			Bit 2-7 = Instrument number
			WRITEATIDX(((volume & 0x03) << 6) | ((note & 0x3f)) );
			WRITEATIDX(((instr & 0x3f) << 2)  | ((volume & 0x0c) >> 2) );
			
			pause = 0;			// Reset the pause counter
		}
		else if (volume >= 0)
		{
			// Only volume change
			// Byte 0:	Bit 0-5 = 61 (Volume only indicator) 
			//			Bit 6-7 = low 2 buts of the volume
			// Byte 1:	Bit 0-1 = high 2 bits of the volume
			//			Bit 2-7 = Unused
			WRITEATIDX(((volume & 0x03) << 6) | 61);		// 61 = empty note (only the volume is set)
			WRITEATIDX((volume & 0x0c) >> 2);				// without instrument

			pause = 0;			// Reset the pause counter
		}
		else
		{
			// If nothing is being set then this is a pause and we accumulate them
			// Then write them out in a compact 1-3 beat format or a bit more verbose
			// Pause + length byte format
			pause++;
		}
	}
	// All notes have been processed

	// The track is shorter than the maximum length
	if (t->len < m_maxTrackLength)
	{
		// Is there any pause left before the end?
		if (pause > 0)
		{
			// Write the remaining pause beats
			WRITEPAUSE(pause);
		}

		// Is there a go to line loop?
		if (t->go >= 0 && goidx >= 0)
		{
			// Write the go to line loop
			WRITEATIDX(0x80 | 63);		// Go command (191)
			WRITEATIDX(goidx);
		}
		else
		{
			// Write the end marker
			WRITEATIDX(255);			// End (0xC0 | 63)
		}
	}
	else
	{	
		// The track is as long as the maximum length
		if (pause > 0)
		{
			WRITEPAUSE(pause);	// Write the remaining pause time
		}
	}
	return idx;
}

int CTracks::TrackToAtaRMF(int trackNr, unsigned char* dest, int max)
{
	TTrack* t = GetTrack(trackNr);
	if (!t) return 0;

	int note, instr, volume, speed;
	int idx = 0;
	int goidx = -1;
	int pause = 0;

	for (int i = 0; i < t->len; i++)
	{
		note = t->note[i];
		instr = t->instr[i];
		volume = t->volume[i];
		speed = t->speed[i];

		// Something will be there, write empty measures first
		if (volume >= 0 || speed >= 0 || t->go == i)
		{
			if (pause > 0)
			{
				WRITEPAUSE(pause);
				pause = 0;
			}

			// It will jump with a go loop
			if (t->go == i) goidx = idx;

			// Speed is ahead of the notes
			if (speed >= 0)
			{
				// Is speed
				WRITEATIDX(63);		// 63 = speed change
				WRITEATIDX(speed & 0xff);
				pause = 0;
			}
		}

		// What it will be
		if (note >= 0 && instr >= 0 && volume >= 0)
		{
			// Note, Instrument, Volume
			WRITEATIDX(((volume & 0x03) << 6) | ((note & 0x3f)));
			WRITEATIDX(((instr & 0x3f) << 2) | ((volume & 0x0c) >> 2));
			pause = 0;
		}
		else if (volume >= 0)
		{
			// Only volume
			WRITEATIDX(((volume & 0x03) << 6) | 61);	// 61 = empty note (only the volume is set)
			WRITEATIDX((volume & 0x0c) >> 2);	// Without instrument
			pause = 0;
		}
		else 
			pause++;
	}
	// End of loop

	// The track is shorter than the maximum length
	if (t->len < m_maxTrackLength)
	{
		// Is there a go loop?
		if (t->go >= 0 && goidx >= 0)
		{
			// Is there still a pause before the end?
			if (pause > 0)
			{
				// Write the remaining pause time
				WRITEPAUSE(pause);
				pause = 0;
			}

			// Write go loop
			WRITEATIDX(0x80 | 63);	// Go command
			WRITEATIDX(goidx);
		}
		else
		{
			// Take an endless pause
			WRITEATIDX(255);		// RMF endless pause
		}
	}
	else
	{	// The track is as long as the maximum length
		if (pause > 0)
		{
			WRITEATIDX(255);		// RMF endless pause
		}
	}
	return idx;
}

/// <summary>
/// Load a track's information into the Atari 64K of memory
/// </summary>
/// <param name="mem">Memory containing a tracks description</param>
/// <param name="trackLength">Length of the track</param>
/// <param name="trackNr">Which track number is being worked on</param>
/// <returns></returns>
BOOL CTracks::AtaToTrack(unsigned char* mem, int trackLength, int trackNr)
{
	TTrack* t = GetTrack(trackNr);
	if (!t) return 0;

	unsigned char data, count;
	int gotoIndex = -1;

	if (trackLength >= 2)
	{
		// There is a go loop at the end of the track
		if (mem[trackLength - 2] == 128 + 63) gotoIndex = mem[trackLength - 1];	// Store its index
	}

	int line = 0;
	int src = 0;

	while (src < trackLength)
	{
		// Jump to gotoIndex => set go to this line
		if (src == gotoIndex) t->go = line;

		data = mem[src] & 0x3f;

		// Have Note, Instrument and volume data on this line
		if (data >= 0 && data <= 60)
		{
			t->note[line] = data;
			t->instr[line] = ((mem[src + 1] & 0xfc) >> 2);		//11111100
			t->volume[line] = ( (mem[src + 1] & 0x03) << 2)		//00000011 -> 00001100
							| ((mem[src] & 0xc0) >> 6);			//11000000 -> 00000011
			src += 2;
			line++;
			continue;
		}

		// Have Volumne only on this line
		else if (data == 61)
		{
			t->volume[line] = ( (mem[src + 1] & 0x03) << 2)		//00000011 -> 00001100
							| ((mem[src] & 0xc0) >> 6);			//11000000 -> 00000011
			src += 2;
			line++;
			continue;
		}

		// Pause / empty line
		else if (data == 62)
		{
			// Maximum 2 bits
			count = mem[src] & 0xc0;

			if (count == 0)
			{	
				// Pause is 0 then the number of lines to skip is in the next byte
				if (mem[src + 1] == 0) break;			// Infinite pause => end
				line += mem[src + 1];	// Shift line
				src += 2;
			}
			else
			{	
				// They are non-zero
				line += (count >> 6);	// Upper 2 bits directly specify a pause 1-3
				src++;
			}
			continue;
		}

		// Speed, go loop, or end
		else if (data == 63)
		{
			count = mem[src] & 0xc0;	// 11000000

			// The highest 2 bits are 0?  (00xxxxxx)
			if (count == 0)
			{
				// Speed
				t->speed[line] = mem[src + 1];
				src += 2;

				// Without line shift
				continue;
			}

			// Highest bit = 1?   (10xxxxxx)
			else if (count == 0x80)
			{
				// Go to loop
				t->len = line;			// That's the end of the track, no more data after this
				break;
			}

			// No more than two bits = 1?  (11xxxxxx)
			else if (count == 0xc0)
			{
				// End
				t->len = line;			// That's the end of the track, no more data after this
				break;
			}
		}
	}
	return 1;
}