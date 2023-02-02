#include "stdafx.h"
#include "resource.h"
#include "General.h"

#include "Undo.h"

#include "Tracks.h"

#include "IOHelpers.h"
#include "GuiHelpers.h"

#include "ChannelControl.h"

#include "global.h"

int CTracks::SaveTrack(int track, std::ofstream& ou, int iotype)
{
	if (track < 0 || track >= TRACKSNUM) return 0;

	switch (iotype)
	{
		case IOTYPE_RMW:
		{
			int j;
			char bf[TRACKLEN];
			TTrack& at = m_track[track];
			ou.write((char*)&at.len, sizeof(at.len));
			ou.write((char*)&at.go, sizeof(at.go));
			//all
			for (j = 0; j < m_maxTrackLength; j++) bf[j] = at.note[j];
			ou.write(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) bf[j] = at.instr[j];
			ou.write(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) bf[j] = at.volume[j];
			ou.write(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) bf[j] = at.speed[j];
			ou.write(bf, m_maxTrackLength);
		}
		break;

		case IOTYPE_TXT:
		{
			CString s;
			char bf[16];
			TTrack& at = m_track[track];
			s.Format("[TRACK]\n");
			ou << s;
			strcpy(bf, "--  ----\n");
			bf[0] = CharH4(track);
			bf[1] = CharL4(track);
			if (at.len > 0)
			{
				bf[4] = CharH4(at.len & 0xff);
				bf[5] = CharL4(at.len & 0xff);
			}
			if (at.go >= 0)
			{
				bf[6] = CharH4(at.go);
				bf[7] = CharL4(at.go);
			}
			ou << bf;
			for (int j = 0; j < at.len; j++)
			{
				strcpy(bf, "--- -- -");
				int note = at.note[j];
				int instr = at.instr[j];
				int volume = at.volume[j];
				int speed = at.speed[j];
				if (note >= 0 && note < NOTESNUM) strncpy(bf, notes[note], 3);
				if (instr >= 0 && instr < INSTRSNUM)
				{
					bf[4] = CharH4(instr);
					bf[5] = CharL4(instr);
				}
				if (volume >= 0 && volume <= MAXVOLUME) bf[7] = CharL4(volume);
				if (speed >= 0 && speed <= 255)
				{
					bf[8] = CharH4(speed);
					bf[9] = CharL4(speed);
					bf[10] = 0;
				}
				ou << bf << std::endl;
			}
			ou << "\n"; //gap
		}
		break;
	}
	return 1;
}

int CTracks::LoadTrack(int track, std::ifstream& in, int iotype)
{
	switch (iotype)
	{
		case IOTYPE_RMW:
		{
			if (track < 0 || track >= TRACKSNUM) return 0;
			char bf[TRACKLEN];
			int j;
			ClearTrack(track);	//clear before filling with data
			TTrack& at = m_track[track];
			in.read((char*)&at.len, sizeof(at.len));
			in.read((char*)&at.go, sizeof(at.go));

			//everything
			in.read(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) at.note[j] = bf[j];
			in.read(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) at.instr[j] = bf[j];
			in.read(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) at.volume[j] = bf[j];
			in.read(bf, m_maxTrackLength);
			for (j = 0; j < m_maxTrackLength; j++) at.speed[j] = bf[j];
		}
		break;

		case IOTYPE_TXT:
		{
			int a;
			char b;
			char line[1025];
			memset(line, 0, 16);
			in.getline(line, 1024); //the first line of the track

			int ttr = Hexstr(line, 2);
			int tlen = Hexstr(line + 4, 2);
			int tgo = Hexstr(line + 6, 2);

			if (track == -1) track = ttr;

			if (track < 0 || track >= TRACKSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearTrack(track);	//clear before filling with data
			TTrack& at = m_track[track];

			if (tlen <= 0 || tlen > m_maxTrackLength) tlen = m_maxTrackLength;
			at.len = tlen;
			if (tgo >= tlen) tgo = -1;
			at.go = tgo;

			int idx = 0;
			while (!in.eof())
			{
				in.read((char*)&b, 1);
				if (b == '[') return 1;	//end of track (beginning of something else)
				if (b == 10 || b == 13)		//right at the beginning of the line is EOL
				{
					NextSegment(in);
					return 1;
				}

				memset(line, 0, 16);		//clear memory first
				line[0] = b;
				in.getline(line + 1, 1024);

				int note = -1, instr = -1, volume = -1, speed = -1;

				if (b == 'C') note = 0;
				else
				if (b == 'D') note = 2;
				else
				if (b == 'E') note = 4;
				else
				if (b == 'F') note = 5;
				else
				if (b == 'G') note = 7;
				else
				if (b == 'A') note = 9;
				else
				if (b == 'B') note = 11;

				a = line[1];
				if (a == '#' && note >= 0) note++;

				a = line[2];
				if (note >= 0)
				{
					if (a >= '1' && a <= '6')
						note += (a - '1') * 12;
					else
						note = -1;
				}
				instr = Hexstr(line + 4, 2);
				volume = Hexstr(line + 7, 1);
				speed = Hexstr(line + 8, 2);

				if (note >= 0 && note < NOTESNUM) at.note[idx] = note;
				if (instr >= 0 && instr < INSTRSNUM) at.instr[idx] = instr;
				if (volume >= 0 && volume <= MAXVOLUME) at.volume[idx] = volume;
				if (speed > 0 && speed <= 255) at.speed[idx] = speed;

				if (at.note[idx] >= 0 && at.instr[idx] < 0) at.instr[idx] = 0;	//if the note is without an instrument, then instrument 0 hits there
				if (at.instr[idx] >= 0 && at.note[idx] < 0) at.instr[idx] = -1;	//if the instrument is without a note, then it cancels it
				if (at.note[idx] >= 0 && at.volume[idx] < 0) at.volume[idx] = MAXVOLUME;	//if the note is non-volume, adds the maximum volume

				idx++;
				if (idx >= TRACKLEN)
				{
					NextSegment(in);
					return 1;
				}
			}
		}
		break;
	}
	return 1;
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

#define WRITEATIDX(value) { if (idx<max) { dest[idx]=value; idx++; } else return -1; }
#define WRITEPAUSE(pause)									\
{															\
	if (pause>=1 && pause<=3)								\
		{ WRITEATIDX(62 | (pause<<6)); }					\
	else													\
		{ WRITEATIDX(62); WRITEATIDX(pause); }				\
}

	int note, instr, volume, speed;
	int idx = 0;
	int goidx = -1;

	// Get the data that describes the track
	TTrack& t = m_track[trackNr];
	int pause = 0;

	// Run over each line in the track and save its data away in
	// the most compact form.
	for (int i = 0; i < t.len; i++)
	{
		// Get the track line data
		note	= t.note[i];
		instr	= t.instr[i];
		volume	= t.volume[i];
		speed	= t.speed[i];

		if (volume >= 0 || speed >= 0 || t.go == i) //something will be there, write empty measures first
		{
			if (pause > 0)
			{
				// Emit 1 or 2 bytes for the pause
				WRITEPAUSE(pause);
				pause = 0;
			}

			if (t.go == i) goidx = idx;	//it will jump with a go loop

			// Speed changes are stored BEFORE note data
			if (speed >= 0)
			{
				//is speed
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

	if (t.len < m_maxTrackLength)	//the track is shorter than the maximum length
	{
		if (pause > 0)	//is there any pause left before the end?
		{
			// write the remaining pause beats
			WRITEPAUSE(pause);
		}

		if (t.go >= 0 && goidx >= 0)	// is there a go to line loop?
		{
			// write the go to line loop
			WRITEATIDX(0x80 | 63);		// go command (191)
			WRITEATIDX(goidx);
		}
		else
		{
			// write the end marker
			WRITEATIDX(255);			// end (0xC0 | 63)
		}
	}
	else
	{	
		// the track is as long as the maximum length
		if (pause > 0)
		{
			WRITEPAUSE(pause);	//write the remaining pause time
		}
	}
	return idx;
}

int CTracks::TrackToAtaRMF(int trackNr, unsigned char* dest, int max)
{

#define WRITEATIDX(value) { if (idx<max) { dest[idx]=value; idx++; } else return -1; }
#define WRITEPAUSE(pause)									\
{															\
	if (pause>=1 && pause<=3)								\
		{ WRITEATIDX(62 | (pause<<6)); }					\
	else													\
		{ WRITEATIDX(62); WRITEATIDX(pause); }				\
}

	int note, instr, volume, speed;
	int idx = 0;
	int goidx = -1;
	TTrack& t = m_track[trackNr];
	int pause = 0;

	for (int i = 0; i < t.len; i++)
	{
		note = t.note[i];
		instr = t.instr[i];
		volume = t.volume[i];
		speed = t.speed[i];

		if (volume >= 0 || speed >= 0 || t.go == i) //something will be there, write empty measures first
		{
			if (pause > 0)
			{
				WRITEPAUSE(pause);
				pause = 0;
			}

			if (t.go == i) goidx = idx;	//it will jump with a go loop

			//speed is ahead of the notes
			if (speed >= 0)
			{
				//is speed
				WRITEATIDX(63);		//63 = speed change
				WRITEATIDX(speed & 0xff);

				pause = 0;
			}
		}

		//what it will be
		if (note >= 0 && instr >= 0 && volume >= 0)
		{
			//note,instr,vol
			WRITEATIDX(((volume & 0x03) << 6)
				| ((note & 0x3f))
			);
			WRITEATIDX(((instr & 0x3f) << 2)
				| ((volume & 0x0c) >> 2)
			);
			pause = 0;
		}
		else
			if (volume >= 0)
			{
				//only volume
				WRITEATIDX(((volume & 0x03) << 6)
					| 61		//61 = empty note (only the volume is set)
				);
				WRITEATIDX((volume & 0x0c) >> 2);	//without instrument
				pause = 0;
			}
			else
				pause++;
	}
	//end of loop

	if (t.len < m_maxTrackLength)	//the track is shorter than the maximum length
	{
		if (t.go >= 0 && goidx >= 0)	//is there a go loop?
		{
			if (pause > 0)	//is there still a pause before the end?
			{
				//write the remaining pause time
				WRITEPAUSE(pause);
				pause = 0;
			}
			//write go loop
			WRITEATIDX(0x80 | 63);	//go command
			WRITEATIDX(goidx);
		}
		else
		{
			//take an endless pause
			WRITEATIDX(255);		//RMF endless pause
		}
	}
	else
	{	//the track is as long as the maximum length
		if (pause > 0)
		{
			WRITEATIDX(255);		//RMF endless pause
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
	TTrack& t = m_track[trackNr];
	unsigned char data, count;
	int gotoIndex = -1;

	if (trackLength >= 2)
	{
		//there is a go loop at the end of the track
		if (mem[trackLength - 2] == 128 + 63) gotoIndex = mem[trackLength - 1];	//store its index
	}

	int line = 0;
	int src = 0;
	while (src < trackLength)
	{
		if (src == gotoIndex) t.go = line;		// jump to gotoIndex => set go to this line

		data = mem[src] & 0x3f;
		if (data >= 0 && data <= 60)	// Have Note, Instrument and volume data on this line
		{
			t.note[line] = data;
			t.instr[line] = ((mem[src + 1] & 0xfc) >> 2);		//11111100
			t.volume[line] = ( (mem[src + 1] & 0x03) << 2)		//00000011 -> 00001100
							| ((mem[src] & 0xc0) >> 6);			//11000000 -> 00000011
			src += 2;
			line++;
			continue;
		}
		else if (data == 61)			// Have Volumne only on this line
		{
			t.volume[line] = ( (mem[src + 1] & 0x03) << 2)		//00000011 -> 00001100
							| ((mem[src] & 0xc0) >> 6);			//11000000 -> 00000011
			src += 2;
			line++;
			continue;
		}
		else if (data == 62)			// Pause / empty line
		{
			count = mem[src] & 0xc0;	// maximum 2 bits
			if (count == 0)
			{	
				// Pause is 0 then the number of lines to skip is in the next byte
				if (mem[src + 1] == 0) break;			// infinite pause => end
				line += mem[src + 1];	// shift line
				src += 2;
			}
			else
			{	
				// they are non-zero
				line += (count >> 6);	// Upper 2 bits directly specify a pause 1-3
				src++;
			}
			continue;
		}
		else if (data == 63)			// Speed, go loop, or end
		{
			count = mem[src] & 0xc0;	// 11000000
			if (count == 0)				// the highest 2 bits are 0?  (00xxxxxx)
			{
				// Speed
				t.speed[line] = mem[src + 1];
				src += 2;
				//without line shift
				continue;
			}
			else if (count == 0x80)		// highest bit = 1?   (10xxxxxx)
			{
				// go to loop
				t.len = line;			// that's the end of the track, no more data after this
				break;
			}
			else if (count == 0xc0)		// no more than two bits = 1?  (11xxxxxx)
			{
				// end
				t.len = line;			// that's the end of the track, no more data after this
				break;
			}
		}
	}
	return 1;
}