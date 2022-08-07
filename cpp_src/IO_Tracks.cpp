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

int CTracks::SaveTrack(int track, ofstream& ou, int iotype)
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
			for (j = 0; j < m_maxtracklen; j++) bf[j] = at.note[j];
			ou.write(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) bf[j] = at.instr[j];
			ou.write(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) bf[j] = at.volume[j];
			ou.write(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) bf[j] = at.speed[j];
			ou.write(bf, m_maxtracklen);
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
				ou << bf << endl;
			}
			ou << "\n"; //gap
		}
		break;
	}
	return 1;
}

int CTracks::LoadTrack(int track, ifstream& in, int iotype)
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
			in.read(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) at.note[j] = bf[j];
			in.read(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) at.instr[j] = bf[j];
			in.read(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) at.volume[j] = bf[j];
			in.read(bf, m_maxtracklen);
			for (j = 0; j < m_maxtracklen; j++) at.speed[j] = bf[j];
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

			if (tlen <= 0 || tlen > m_maxtracklen) tlen = m_maxtracklen;
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

int CTracks::SaveAll(ofstream& ou, int iotype)
{
	switch (iotype)
	{
		case IOTYPE_RMW:
		{
			ou.write((char*)&m_maxtracklen, sizeof(m_maxtracklen));
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
				if (!CalculateNoEmpty(i)) continue;	//saves only non-empty tracks
				SaveTrack(i, ou, iotype);
			}
		}
		break;
	}
	return 1;
}


int CTracks::LoadAll(ifstream& in, int iotype)
{
	InitTracks();

	in.read((char*)&m_maxtracklen, sizeof(m_maxtracklen));
	// g_cursoractview = m_maxtracklen / 2;

	for (int i = 0; i < TRACKSNUM; i++)
	{
		LoadTrack(i, in, iotype);
	}
	return 1;
}

int CTracks::TrackToAta(int track, unsigned char* dest, int max)
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
	TTrack& t = m_track[track];
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

	if (t.len < m_maxtracklen)	//the track is shorter than the maximum length
	{
		if (pause > 0)	//is there any pause left before the end?
		{
			//write the remaining pause time 
			WRITEPAUSE(pause);
			pause = 0;
		}

		if (t.go >= 0 && goidx >= 0)	//is there a go loop?
		{
			//write the go loop
			WRITEATIDX(0x80 | 63);	//go command
			WRITEATIDX(goidx);
		}
		else
		{
			//write the end
			WRITEATIDX(255);		//end
		}
	}
	else
	{	//the track is as long as the maximum length
		if (pause > 0)
		{
			WRITEPAUSE(pause);	//write the remaining pause time
		}
	}
	return idx;
}

int CTracks::TrackToAtaRMF(int track, unsigned char* dest, int max)
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
	TTrack& t = m_track[track];
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

	if (t.len < m_maxtracklen)	//the track is shorter than the maximum length
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


BOOL CTracks::AtaToTrack(unsigned char* sour, int len, int track)
{
	TTrack& t = m_track[track];
	unsigned char b, c;
	int goidx = -1;

	if (len >= 2)
	{
		//there is a go loop at the end of the track
		if (sour[len - 2] == 128 + 63) goidx = sour[len - 1];	//store its index
	}

	int line = 0;
	int i = 0;
	while (i < len)
	{
		if (i == goidx) t.go = line;		//jump to goidx => set go to this line

		b = sour[i] & 0x3f;
		if (b >= 0 && b <= 60)	//note,instr,vol
		{
			t.note[line] = b;
			t.instr[line] = ((sour[i + 1] & 0xfc) >> 2);			//11111100
			t.volume[line] = ((sour[i + 1] & 0x03) << 2)			//00000011
				| ((sour[i] & 0xc0) >> 6);			//11000000
			i += 2;
			line++;
			continue;
		}
		else
			if (b == 61)	//vol only
			{
				t.volume[line] = ((sour[i + 1] & 0x03) << 2)			//00000011
					| ((sour[i] & 0xc0) >> 6);			//11000000
				i += 2;
				line++;
				continue;
			}
			else
				if (b == 62)	//pause
				{
					c = sour[i] & 0xc0;		//maximum 2 bits
					if (c == 0)
					{	//they are zero
						if (sour[i + 1] == 0) break;			//infinite pause => end
						line += sour[i + 1];	//shift line
						i += 2;
					}
					else
					{	//they are non-zero
						line += (c >> 6);		//the upper 2 bits directly specify a pause 1-3
						i++;
					}
					continue;
				}
				else
					if (b == 63)	//speed or go loop or end
					{
						c = sour[i] & 0xc0;		//11000000
						if (c == 0)				//the highest 2 bits are 0?  (00xxxxxx)
						{
							//speed
							t.speed[line] = sour[i + 1];
							i += 2;
							//without line shift
							continue;
						}
						else
							if (c == 0x80)			//highest bit = 1?   (10xxxxxx)
							{
								//go loop
								t.len = line; //that's the end here
								break;
							}
							else
								if (c == 0xc0)			//no more than two bits = 1?  (11xxxxxx)
								{
									//end
									t.len = line;
									break;
								}
					}
	}
	return 1;
}