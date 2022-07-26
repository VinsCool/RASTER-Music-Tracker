#include "stdafx.h"
#include "resource.h"
#include <fstream>
using namespace std;

#include "Atari6502.h"

#include "importdlgs.h"

#include "Tracks.h"
#include "Song.h"
#include "Instruments.h"

#include "global.h"

extern CInstruments	g_Instruments;


struct TSourceTrack
{
	int note[64];
	int instr[64];
	int volumeL[64];
	int volumeR[64];
	int speed[64];
	int len;
	BOOL stereo;
};

struct TDestinationMark
{
	int fromtrack;
	int shift;
	BYTE leftright;
};

struct TInstrumentMark
{
	int maxvolL;
	int maxvolR;
	BOOL stereo;
};

class CConvertTracks
{
public:
	CConvertTracks();
	void SetRMTTracks(CTracks* tracks) { m_ctracks = tracks; };
	int Init();
	TSourceTrack* GetSTrack(int t) { return (t >= 0 && t < TRACKSNUM) ? &m_strack[t] : (TSourceTrack*)NULL; };
	TInstrumentMark* GetIMark(int instr) { return (instr >= 0 && instr < INSTRSNUM) ? &m_imark[instr] : (TInstrumentMark*)NULL; };

	int MakeOrFindTrackShiftLR(int from, int shift, BYTE lr);

private:
	TSourceTrack m_strack[TRACKSNUM];
	TDestinationMark m_dmark[TRACKSNUM];
	TInstrumentMark m_imark[INSTRSNUM];
	CTracks* m_ctracks;
};

CConvertTracks::CConvertTracks()
{
	m_ctracks = 0;
	Init();
}

int CConvertTracks::Init()
{
	for (int i = 0; i < TRACKSNUM; i++)
	{
		TSourceTrack* at = &m_strack[i];
		for (int j = 0; j < 64; j++)	at->note[j] = at->instr[j] = at->volumeL[j] = at->volumeR[j] = at->speed[j] = -1;
		at->len = -1;
		at->stereo = 0;

		TDestinationMark* dm = &m_dmark[i];
		dm->fromtrack = -1;
		dm->shift = 0;
		dm->leftright = 0;
	}
	for (int i = 0; i < INSTRSNUM; i++)
	{
		m_imark[i].maxvolL = 0;
		m_imark[i].maxvolR = 0;
		m_imark[i].stereo = 0;
	}
	return 1;
}

#define VOLUMES_L	1
#define VOLUMES_R	2

int CConvertTracks::MakeOrFindTrackShiftLR(int from, int shift, BYTE lr)
{
	//will try to find a matching track among the already created ones
	//or create a new one (as close as possible to the original track number).
	//Return its number, or -1

	int i;

	if (m_strack[from].len < 0) return -1;	//this source track is empty

	//will find the number where it will first create the new one
	int track = -1;

	if (m_dmark[from].fromtrack < 0)
	{
		track = from;		//the same number is free
	}
	else
	{
		//finds a free place as close as possible to the "from" track
		for (i = from + 1; i != from; i++)
		{
			if (i >= TRACKSNUM) i = 0;	//when he reaches the end, he starts from the beginning
			if (m_dmark[i].fromtrack < 0) break; //found a free track
		}
		if (i == from) return -1;		//did not find one
		track = i;	//this is empty, it will create the appropriate modification of the source track
	}

	//rewritten from source track to system track
	TSourceTrack* ts = &m_strack[from];
	if (!ts->stereo) lr = VOLUMES_L | VOLUMES_R; //if it's not a stereo track and it wants the right channel, give it the left channel anyway
	TTrack* td = m_ctracks->GetTrack(track);
	int activeinstr = -1;
	for (i = 0; i < ts->len; i++)
	{
		int note = ts->note[i];
		if (note >= 0)
		{
			note += shift;
			//while (note<0) note+=12;
			//while (note>=NOTESNUM) note-=12;
			while (note < 0) note += 64;
			while (note >= 64) note -= 64;
			if (note >= NOTESNUM) note = NOTESNUM - 1;
		}
		td->note[i] = note;

		int instr = ts->instr[i];
		td->instr[i] = instr;
		if (instr >= 0) activeinstr = instr;

		int volume = (lr & VOLUMES_L) ? ts->volumeL[i] : ts->volumeR[i];
		if (activeinstr >= 0 && volume > 0)
		{
			int maxvol = (lr & VOLUMES_L) ? m_imark[activeinstr].maxvolL : m_imark[activeinstr].maxvolR;
			if (maxvol - (15 - volume) <= 0) volume = 0;
		}
		td->volume[i] = volume;

		td->speed[i] = ts->speed[i];
	}
	td->len = ts->len;

	//search if this one does not match one that already exists
	for (i = 0; i < TRACKSNUM; i++)
	{
		if (track == i) continue;	//it will not compare itself with itself
		if (m_ctracks->CompareTracks(track, i))
		{
			//found the same
			//so the newly created one will be deleted
			m_ctracks->ClearTrack(track);
			//and return the former number
			return i; //found one
		}
	}

	//does not match the previous one

	TDestinationMark* dm = &m_dmark[track];
	dm->fromtrack = from;
	dm->shift = shift;
	dm->leftright = lr;
	return track;
}

//----------------------------------------------


int CSong::ImportTMC(ifstream& in)
{
	int originalg_tracks4_8 = g_tracks4_8;

	//delete the current song
	g_tracks4_8 = 8;					//standard TMC is 8 tracks
	g_Tracks.m_maxtracklen = 64;	//track length is 64
	ClearSong(g_tracks4_8);			//clear everything

	unsigned char mem[65536];
	memset(mem, 0, 65536);
	WORD bfrom, bto;

	int len, i, j, k;
	char a;

	len = LoadBinaryBlock(in, mem, bfrom, bto);

	if (len <= 0)
	{
		MessageBox(g_hwnd, "Corrupted TMC file or unsupported format version.", "Open error", MB_ICONERROR);
		return 0;
	}

	CConvertTracks cot;
	cot.SetRMTTracks(&g_Tracks);

	//song name
	for (j = 0; j < 30 && (a = mem[bfrom + j]); j++)
	{
		if (a < 32 || a >= 127) a = ' ';
		m_songname[j] = a;
	}
	for (k = j; k < SONGNAMEMAXLEN; k++) m_songname[k] = ' '; //fill in the gaps

	CImportTmcDlg importdlg;
	CString s = m_songname;
	s.TrimRight();
	importdlg.m_info.Format("TMC module: %s", (LPCTSTR)s);

	if (importdlg.DoModal() != IDOK) return 0;

	BOOL x_usetable = importdlg.m_check1;
	BOOL x_optimizeloops = importdlg.m_check6;
	BOOL x_truncateunusedparts = importdlg.m_check7;


	WORD instr_ptr[64], track_ptr[128];
	BOOL instr_used[64];
	WORD adr;
	BYTE c;
	int speco = 0;			//speedcorrection

	//speeds
	m_mainspeed = m_speed = mem[bfrom + 30] + 1;
	m_instrspeed = mem[bfrom + 31];
	if (m_instrspeed > 4)
	{
		speco = m_instrspeed - 4;
		m_instrspeed = 4;		//4x instrspeed maximum
	}
	else
		if (m_instrspeed < 1) m_instrspeed = 1;		//1x instrspeed minimum

		//instrument vectors
	for (i = 0; i < 64; i++)
	{
		instr_ptr[i] = mem[bfrom + 32 + i] + (mem[bfrom + 32 + 64 + i] << 8);
		instr_used[i] = 0; //find out when the track is used if it is used
	}
	//track vectors
	for (i = 0; i < 128; i++) track_ptr[i] = mem[bfrom + 32 + 128 + i] + (mem[bfrom + 32 + 128 + 128 + i] << 8);

	//tracks
	for (i = 0; i < 128; i++)
	{
		adr = track_ptr[i];
		if (mem[adr] == 0xff) continue;	//points to FF
		int line = 0;
		TSourceTrack& ts = *(cot.GetSTrack(i));
		int ains = 0, note = -1, volL = 15, volR = 15, speed = 0, space = 0;
		BOOL endt = 0;

		while (1)
		{
			if (line >= 64) { ts.len = 64; break; }

			c = mem[adr++];
			int txx = c & 0xc0;
			if (txx == 0x80)
			{
				//instrument
				ains = c & 0x3f;
				instr_used[ains] = 1;
			}
			else
				if (txx == 0x00)
				{
					//note and volume
					//note
					note = (c & 0x3f) - 1;
					if (note < 0) note = -1;
					else
					{
						ts.note[line] = note;
						ts.instr[line] = ains;
					}
					//volume
					c = mem[adr++];
					volL = 15 - ((c & 0xf0) >> 4);
					volR = 15 - (c & 0x0f);
					if (volL != volR) ts.stereo = 1;	//there is some different volume for L and R
					if (volL != 0 || volR != 0)			//in TMC there may be no note if vol L and R are both equal to 0
					{
						ts.volumeL[line] = volL;
						ts.volumeR[line] = volR;
					}
					line++;
				}
				else
					if (txx == 0x40)
					{
						//note speed  (+possible volume)
						//note
						note = (c & 0x3f) - 1;
						if (note < 0) note = -1;
						else
						{
							ts.note[line] = note;
							ts.instr[line] = ains;
						}
						//speed
						c = mem[adr++];
						speed = c & 0x0f;
						if (speed == 0)
						{
							ts.len = line + 1;
							endt = 1;
							//break;			//speed = 0 => end of this track
						}
						else
						{
							ts.speed[line] = speed + 1;
						}
						if (c & 0x80)		//behind speed is also the volume
						{
							//volume
							c = mem[adr++];
							volL = 15 - ((c & 0xf0) >> 4);
							volR = 15 - (c & 0x0f);
							if (volL != volR) ts.stereo = 1;	//there is some different volume for L and R
							if (volL != 0 || volR != 0)			//in TMC there may be no note if vol L and R are both equal to 0
							{
								ts.volumeL[line] = volL;
								ts.volumeR[line] = volR;
							}
						}
						if (endt) break;	//end of this track via speed = 0
						line++;
					}
					else
						if (txx == 0xc0)
						{
							//gap
							space = (c & 0x3f) + 1;
							if (space > 63 || line + space > 63)
							{
								if (line > 0) ts.len = 64;
								break;	//space = 64 (ff) => end of this track
							}
							line += space;
						}
		}
		//and on the next track
		line = 0;
	}

	//instruments
	int nonemptyinstruments = 0;
	for (i = 0; i < 64; i++)
	{
		TInstrument& ai = g_Instruments.m_instr[i];

		if (instr_used[i]) //this instrument is used somewhere in some track
		{
			//yes => name
			CString s;
			s.Format("TMC instrument imitation %02X", i);
			strncpy(ai.name, (LPCTSTR)s, s.GetLength());
		}

		int adr_e = instr_ptr[i];
		if (adr_e == 0) continue;	//undefined

		//defined

		nonemptyinstruments++;

		int adr_t = instr_ptr[i] + 63;
		int adr_p = instr_ptr[i] + 63 + 8;

		//audctl
		BYTE audctl1 = mem[adr_p + 1];
		BYTE audctl2 = mem[adr_p + 2];

		//ai.par[PAR_AUDCTL0...7]=audctl1;

		//envelope
		int c1, c2, c3;
		BOOL anyrightvolisntzero = 0;
		BOOL filteru = 0;					//is it using the filter?
		int lasttmccmd = -1;				//last command
		int lasttmcpar = 0;				//last parameter
		int cmd1_2 = 0;					//for command sequence 1 and 2
		int par1_2 = 0;					//parameter for adding frequency for command sequence 1 and 2
		int cmd2_2 = 0;					//sequence 2 and 2
		int par2_2 = 0;					//parameter for sequence 2 and 2
		int cmd6_6 = 0;					//sequence 6 and 6
		int par6_6 = 0;					//parameter for sequence 6 and 6
		int maxvolL = 0, maxvolR = 0, lastvol = 0;
		for (j = 0; j < 21; j++)
		{
			c1 = mem[adr_e + j * 3];
			c2 = mem[adr_e + j * 3 + 1];
			c3 = mem[adr_e + j * 3 + 2];

			int dist08 = (c1 >> 4) & 0x01;		//forced volume bit
			BYTE dist = (c1 >> 4) & 0x0e;		//distortions, in step of 2
			if (dist == 0x0e) dist = 0x0a;		//pure tones
			else
				if (dist == 0x06) dist = 0x02;		//distortion 2 sharp tones

				//now dist is 0,2,4,8,A,C (without 0x06 and 0x0e)

			int basstable = mem[adr_p + 7] & 0xc0;
			if (dist == 0x0c && (basstable == 0x80 || basstable == 0xc0)) dist = 0x0e;

			int com08 = (c2 >> 4) & 0x08;		//command 8x
			BYTE audctl = com08 ? audctl2 : audctl1;

			//16 bit bass?
			if (((audctl & 0x50) == 0x50 || (audctl & 0x28) == 0x28) && (dist == 0x0c))
				dist = 0x06;	//16bit bass

			//filter
			if (((audctl & 0x04) == 0x04 || (audctl & 0x02) == 0x02))
			{
				ai.env[j][ENV_FILTER] = 1;
				filteru = 1;
			}

			ai.env[j][ENV_DISTORTION] = dist;
			int vol = c1 & 0x0f;			//volumeL 0-F;
			ai.env[j][ENV_VOLUMEL] = lastvol = vol;			//lastvol is needed to correct the fading
			if (vol > maxvolL) maxvolL = vol;	//maximum volumeL of the whole envelope
			vol = c2 & 0x0f;			//volumeR 0-F
			ai.env[j][ENV_VOLUMER] = vol;
			if (vol > maxvolR) maxvolR = vol;	//maximum volumeR of the whole envelope
			if (vol > 0) anyrightvolisntzero = 1;	//some volumeR is> 0

			int tmccmd = (c2 >> 4) & 0x07;		//command
			int tmcpar = c3;
			int rmtcmd = tmccmd, rmtpar = tmcpar;

			//TMC to RMT command conversion
			switch (tmccmd)
			{
				case 0: //without effect
					rmtcmd = 0; rmtpar = 0;
					break;
				case 1: //P-> AUDF (same as TMC)
					cmd1_2 = 2;
					par1_2 = tmcpar;
					if (com08 || tmcpar == 0x00) //but cmd 9 or 1 with parameter 00 is volume only
					{
						rmtcmd = 7;		//Volume only
						rmtpar = 0x80;
					}
					break;
				case 2: //P+A->AUDF
					if (j > 0 && cmd1_2)
					{
						//divide 2 xy into 1 ab, where ab = value at the previous left unit + increment
						//it will work in chains for the other two, which is fine
						rmtcmd = 1;
						rmtpar = (BYTE)(par1_2 + tmcpar);
						cmd1_2 = 2;	//to make it last again for the next envelope column
						par1_2 = rmtpar;	//added frequency for next
					}
					else
						if (j > 0 && cmd2_2)
						{
							//divide 2 xy to 2 ab, where ab = value at the previous left two + increment
							rmtcmd = 2;
							rmtpar = (BYTE)(par2_2 + tmcpar);
							cmd2_2 = 2;
							par2_2 = rmtpar;
						}
						else
						{
							//will remain the same
							rmtcmd = 2;
							rmtpar = tmcpar;
							cmd2_2 = 2;
							par2_2 = rmtpar;
						}
					break;
				case 3: //P+N->AUDF
					//corresponds exactly to TMC command 2
					rmtcmd = 2; rmtpar = tmcpar;
					break;
				case 4: //P&RND->AUDF
					rmtcmd = 1;	//converts randomly selected frequencies to a fixed setting
					rmtpar = rand() & 0xff;
					break;
				case 5: //PN->AUDF  (plays a fixed note with that index)
					rmtcmd = 1;	//converts to a fixed frequency setting the corresponding note (according to distortion)
					//dist=0-e
					rmtpar = (256 - (tmcpar * 4)) & 0xff;	//JUST FOR THAT IT WILL BE ADVISORY !!!!!!!!!!!!!!!!!!!!!!!!!!!!
					break;
				case 6: //PN+A->AUDF 
					if (j > 0 && cmd6_6)
					{
						rmtcmd = 0;
						rmtpar = (BYTE)(par6_6 + tmcpar);
						cmd6_6 = 2;
						par6_6 = rmtpar;
					}
					else
					{
						rmtcmd = 0;
						rmtpar = tmcpar;
						cmd6_6 = 2;
						par6_6 = rmtpar;
					}
					break;
				case 7: //PN+N->AUDF
					rmtcmd = 0;	//in RMT, the note shift is done by command 0
					break;
			}

			//bass shift
			if (dist == 0x0c && rmtcmd == 0) rmtpar += 8;

			//forced volume
			if (dist08) { rmtcmd = 7; rmtpar = 0x80; } //volume only

			ai.env[j][ENV_COMMAND] = rmtcmd;
			ai.env[j][ENV_X] = (rmtpar >> 4) & 0x0f;
			ai.env[j][ENV_Y] = rmtpar & 0x0f;

			lasttmccmd = tmccmd;
			lasttmcpar = tmcpar;
			if (cmd1_2 > 0) cmd1_2--;		//read
			if (cmd2_2 > 0) cmd2_2--;		//read
			if (cmd6_6 > 0) cmd6_6--;		//read

		} //0-20 column envelope

		//is all right volume = 0? => copies left to right
		if (!anyrightvolisntzero)
		{
			for (j = 0; j <= 21; j++) ai.env[j][ENV_VOLUMER] = ai.env[j][ENV_VOLUMEL];
			maxvolR = maxvolL;
		}

		//envelope length
		ai.par[PAR_ENVLEN] = 20;			//the envelope is 21 columns
		ai.par[PAR_ENVGO] = 20;

		TInstrumentMark* im = cot.GetIMark(i);
		im->maxvolL = maxvolL;
		im->maxvolR = maxvolR;
		im->stereo = anyrightvolisntzero;

		//table
		BOOL tableu = 0;	//table used
		for (j = 0; j < 8; j++)
		{
			int nut = mem[adr_t + j];
			if (nut >= 0x80 && nut <= 0xc0) nut += 0x40;
			if (nut >= 0x40 && nut <= 0x7f) nut -= 0x40;
			if (nut != 0) tableu = 1; //table is used for something
			ai.tab[j] = nut;
		}

		//parameters
		//table length and speed
		int tablen = (mem[adr_p + 8] >> 4) & 0x07;
		ai.par[PAR_TABLEN] = tablen;		//0-7
		ai.par[PAR_TABSPD] = mem[adr_p + 8] & 0x0f;			//speed 0-15

		//other parameters

		//volume slide
		BYTE t1vslide = mem[adr_p + 3];
		//speco is the correction when the instrument decelerates from 5 and more to 4
		BYTE t2vslide = (t1vslide < 1) ? 0 : (BYTE)((double)15 / lastvol * (double)255 / ((double)t1vslide * (1 - ((double)speco) / 4)) + 0.5);
		if (t2vslide > 255) t2vslide = 255;
		else
			if (t2vslide < 0) t2vslide = 0;
		ai.par[PAR_VSLIDE] = t2vslide;
		ai.par[PAR_VMIN] = 0;

		//vibrato or fshift
		BYTE pvib = mem[adr_p + 5] & 0x7f;
		BYTE pvib8 = mem[adr_p + 5] & 0x80;		//highest bit
		BYTE delay = mem[adr_p + 6];
		BYTE vibspe = mem[adr_p + 7] & 0x3f;	//vibrato speed
		BYTE vib = 0, fshift = 0;
		BOOL vpt = 0;			//vibrato through the table succeeded

		int posuntable = (int)((double)delay / (vibspe + 1) + 0.5);

		BOOL nobytable = 0;	//if it tries to convert to a table
		if (!x_usetable) nobytable = 1;	//it shouldn't try
		if (filteru) nobytable = 1;

		//what if the table is used, but only to move to the 0th place
		if (!nobytable && tableu && tablen == 0 && (pvib & 0x40))		//(pvib & 0x40) <- only if vibrato uses something, otherwise it doesn't make sense to redo it
		{
			//that is, it optimizes over the shift of all notes in the envelope
			int psn = ai.tab[0];	//0th place in the table
			for (j = 0; j < 21; j++)
			{
				if (ai.env[j][ENV_COMMAND] == 0) //music shift
				{
					BYTE notenum = (ai.env[j][ENV_X] << 4) + ai.env[j][ENV_Y];
					notenum += psn; //shifts
					ai.env[j][ENV_X] = (notenum >> 4) & 0x0f;
					ai.env[j][ENV_Y] = notenum & 0x0f;
				}
			}
			ai.tab[0] = 0; //so the parameter in the table is reset
			tableu = 0;	//and thus the table is free for further use
		}

		//and now the individual values of the "vibrato" parameter:

		if (pvib > 0x10 && pvib < 0x3f)
		{
			//vibrato
			int hn = pvib >> 4;		//vibrato type 1-3
			int dn = pvib & 0x0f;	//cut out vibrato 0-f

			/*
			vib = (int) ((double) (hn+(float)dn/4)+0.5);
			if (vib<0) vib=0;
			else
			if (vib>3) vib=3;
			*/
			vib = 3;
			if (hn == 1) vib = 1 + vibspe;	//the most similar is this regardless of the magnitude of the oscillation, speed will move it to vib 2 or 3
			else
				if (hn == 2 && dn == 1) vib = 2 + vibspe; //for dn == 1 this corresponds exactly, the others are already corresponding to vib3
			if (vib > 3) vib = 3;	//if it read more than 3 after reading "vibspec"

			//if you do not use the table, try to use vibrato through the table
			if (!nobytable && !tableu)
			{
				if (hn == 1 && (dn > 2 || vibspe > 0))							//(dn>=4 || vibspe>0) )
				{
					if (posuntable > TABLEN - 4) posuntable = TABLEN - 4; //did not give what is possible according to the delay
					ai.tab[posuntable] = 0;
					ai.tab[posuntable + 1] = (pvib8) ? (BYTE)(256 - dn) : dn;
					ai.tab[posuntable + 2] = 0;
					ai.tab[posuntable + 3] = (pvib8) ? dn : (BYTE)(256 - dn);
					ai.par[PAR_TABLEN] = posuntable + 3;
					ai.par[PAR_TABGO] = posuntable;
					ai.par[PAR_TABTYPE] = 1;	//frequency table
					ai.par[PAR_TABSPD] = (vibspe < 0x3f) ? vibspe : 0x3f;
					vpt = 1; //successful
				}
				else
					if (hn == 2 && (dn > 2 || vibspe > 0))							//(dn>=4 || vibspe>0))
					{
						if (posuntable > TABLEN - 4) posuntable = TABLEN - 4; //did not give what is possible according to the delay
						ai.tab[posuntable] = (pvib8) ? (BYTE)(256 - dn) : dn;
						ai.tab[posuntable + 1] = 0;
						ai.tab[posuntable + 2] = (pvib8) ? dn : (BYTE)(256 - dn);
						ai.tab[posuntable + 3] = 0;
						ai.par[PAR_TABLEN] = posuntable + 3;
						ai.par[PAR_TABGO] = posuntable;
						ai.par[PAR_TABTYPE] = 1;	//frequency table
						int sp = dn * (vibspe + 1) - 1;
						if (sp < 0) sp = 0;	else if (sp > 0x3f) sp = 0x3f;
						ai.par[PAR_TABSPD] = sp;
						vpt = 1; //successful
					}
					else
						if (hn == 3 && (dn > 2 || vibspe > 0))							//(dn>=4 || vibspe>0))
						{
							if (posuntable > TABLEN - 4) posuntable = TABLEN - 4; //did not give what is possible according to the delay
							ai.tab[posuntable] = (pvib8) ? (BYTE)(dn * 4) : (BYTE)(256 - (dn * 4)); //for notes it is the other way around (add note = read frequency
							ai.tab[posuntable + 1] = 0;
							ai.tab[posuntable + 2] = (pvib8) ? (BYTE)(256 - (dn * 4)) : (BYTE)(dn * 4); //it is the other way around
							ai.tab[posuntable + 3] = 0;
							ai.par[PAR_TABLEN] = posuntable + 3;
							ai.par[PAR_TABGO] = posuntable;
							ai.par[PAR_TABTYPE] = 1;	//frequency table
							int sp = dn * (vibspe + 1) - 1;
							if (sp < 0) sp = 0;	else if (sp > 0x3f) sp = 0x3f;
							ai.par[PAR_TABSPD] = sp;
							vpt = 1; //successful
						}
			}
		}
		else
			if (pvib > 0x40 && pvib <= 0x4f)
			{
				//fshift down (added frq)
				fshift = pvib - 0x40;
				if (pvib8) fshift = (BYTE)(256 - fshift);

				//and now find out if it wouldn't do it through the table
				if (!nobytable && !tableu && vibspe > 0)
				{
					if (posuntable > TABLEN - 2) posuntable = TABLEN - 2; //he didn't give up
					ai.tab[posuntable] = fshift;
					ai.tab[posuntable + 1] = fshift;
					ai.par[PAR_TABLEN] = posuntable + 1;
					ai.par[PAR_TABGO] = posuntable + 1;
					ai.par[PAR_TABTYPE] = 1;	//frequency table
					ai.par[PAR_TABMODE] = 1;	//read
					ai.par[PAR_TABSPD] = (vibspe < 0x3f) ? vibspe : 0x3f;
					vpt = 1; //successful
				}
			}
			else
				if (pvib > 0x50 && pvib <= 0x5f)
				{
					//shift in notes down (left) => shift in frequency 255/61 * shift_in_notes
					fshift = (int)(((double)(255 / 61) * (pvib - 0x50)) + 0.5);
					if (pvib8) fshift = (BYTE)(256 - fshift);

					//and now find out if it wouldn't do it through the table

					if (!nobytable && !tableu)				// && vibspe>0)
					{
						if (posuntable > TABLEN - 2) posuntable = TABLEN - 2; //it didn't give up
						int nshift = pvib - 0x50;
						if (!pvib8) nshift = (BYTE)(256 - nshift);		//for notes it is the opposite (5x is <- down, Dx is up ->)
						ai.tab[posuntable] = nshift;
						ai.tab[posuntable + 1] = nshift;
						ai.par[PAR_TABLEN] = posuntable + 1;
						ai.par[PAR_TABGO] = posuntable + 1;
						ai.par[PAR_TABTYPE] = 0;	//note table
						ai.par[PAR_TABMODE] = 1;	//read
						ai.par[PAR_TABSPD] = (vibspe < 0x3f) ? vibspe : 0x3f;
						vpt = 1; //succesful
					}
				}
				else
					if (pvib > 0x60 && pvib <= 0x6f)
					{
						//tuned $ 60- $ 6f => -0 to -15 to frequency
						//we don't know
					}


		//verify whether the vibrato effect was done via table or "normal" via vibrato and fshift
		if (vpt)
		{
			//managed to do vibrato through the table, so it does not use vibrato, fshift or delay
			vib = 0;
			fshift = 0;
			delay = 0;
		}

		ai.par[PAR_VIBRATO] = vib;
		ai.par[PAR_FSHIFT] = fshift;
		if (vib == 0 && fshift == 0) delay = 0;
		else
			if ((vib > 0 || fshift > 0) && delay == 0) delay = 1;
		ai.par[PAR_DELAY] = delay;

		//optimalization
		//envelope length
		int lastnonzerovolumecol = -1;
		int lastchangecol = 0;
		for (int k = 0; k <= 20; k++)
		{
			if (ai.env[k][ENV_VOLUMEL] > 0 || ai.env[k][ENV_VOLUMER] > 0) lastnonzerovolumecol = k;
			if (k > 0)
			{
				for (int m = 0; m < ENVROWS; m++)
				{
					if (ai.env[k][m] != ai.env[k - 1][m])	//is there anything else? (volumeL, R, distortion, ..., portamento)
					{
						lastchangecol = k;	//yeah, something else.
						break;
					}
				}
				//skipping break(?)
			}
		}

		if (lastnonzerovolumecol < 20)
		{
			ai.par[PAR_ENVLEN] = ai.par[PAR_ENVGO] = lastnonzerovolumecol + 1; //shortens to the last non-zero volume
		}

		if (lastchangecol < lastnonzerovolumecol	//the last arbitrary change took place in the last change col column
			&& ai.par[PAR_VSLIDE] == 0				//only when the volume does not decrease
			)
		{
			ai.par[PAR_ENVLEN] = ai.par[PAR_ENVGO] = lastchangecol; //shorten it to the column where the last change of anything was
		}

		//table
		for (int v = 0; v <= ai.par[PAR_TABLEN]; v++)	//are there only zeros?
		{
			if (ai.tab[v] != 0) goto NoTableOptimize;
		}
		if (ai.par[PAR_TABLEN] >= 1 && ai.par[PAR_TABGO] == 0)
		{
			ai.par[PAR_TABLEN] = 0;
			ai.par[PAR_TABSPD] = 0;
		}
	NoTableOptimize:


		//projected instrument into Atari's RAM
		g_Instruments.ModificationInstrument(i);
	} //and another instrument


	//song
	int line = 0;
	BYTE lr = 0;
	BOOL stereomodul = 0;
	int numoftracks = 0;
	int adrendsong = (instr_ptr[0] > 0) ? instr_ptr[0] : track_ptr[0];

	for (adr = bfrom + 32 + 128 + 256; adr < adrendsong; adr += 16, line++)
	{
		if ((mem[adr + 15] & 0x80) == 0x80)	//goto line?
		{
			//goto
			m_songgo[line] = mem[adr + 14] & 0x7f;
			continue;
		}
		for (i = 0; i < 8; i++)
		{
			int t = mem[adr + 15 - i * 2];
			char preladeni = (char)mem[adr + 14 - i * 2];
			if (t >= 0 && t < 128)
			{
				if (i < 4)
				{
					lr = VOLUMES_L;
				}
				else
				{
					//lr = VOLUMES_R;
					lr = VOLUMES_L;
					/*
					int levy=mem[adr+15+8-i*2];
					if (levy>=0 && levy<128 && cot.GetSTrack(t)->len<0)
					{
						t = levy; //pouzije pravou variaci leveho tracku
						preladeni = (char)mem[adr+14+8-i*2];	//with the same tuning the lines have
					}
					*/
				}

				int vyslednytrack = cot.MakeOrFindTrackShiftLR(t, preladeni, lr);

				m_song[line][i] = vyslednytrack;

				if (vyslednytrack > numoftracks) numoftracks = vyslednytrack;	//total number of tracks

				if (vyslednytrack >= 0 && i >= 4) stereomodul = 1;
			}
		}
	}
	//is there a goto in the end?
	if (m_songgo[line - 1] < 0) m_songgo[line + 1] = line;	//no, so it adds an endless loop to the end
	//if (m_songgo[line-1]<0) m_songgo[line]=0; //no, so add a goto to the first line

	if (!stereomodul) g_tracks4_8 = 4;	//mono module

	//FINAL DIALOGUE AFTER IMPORT
	CImportTmcFinishedDlg imfdlg;
	imfdlg.m_info.Format("%i tracks, %i instruments, %i songlines", numoftracks, nonemptyinstruments, line);

	//OPTIMIZATIONS
	if (x_optimizeloops)
	{
		int optitracks = 0, optibeats = 0;
		TracksAllBuildLoops(optitracks, optibeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Loops in %i tracks (%i beats/lines)", optitracks, optibeats);
		imfdlg.m_info += s;
	}

	if (x_truncateunusedparts)
	{
		int clearedtracks = 0, truncatedtracks = 0, truncatedbeats = 0;
		SongClearUnusedTracksAndParts(clearedtracks, truncatedtracks, truncatedbeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Cleared %i, truncated %i tracks (%i beats/lines)", clearedtracks, truncatedtracks, truncatedbeats);
		imfdlg.m_info += s;
	}

	if (imfdlg.DoModal() != IDOK)
	{
		//did not give Ok, so it deletes
		g_tracks4_8 = originalg_tracks4_8;	//returns the original value
		ClearSong(g_tracks4_8);
		MessageBox(g_hwnd, "Module import aborted.", "Import...", MB_ICONINFORMATION);
	}

	return 1;
}


/********************************************************************************************/

//September 27, 2003 8:38 PM ... I just imported aurora.mod, released it without editing and I'm amazed !!!
//							That's absolutely AMAZING! AMAZING! ABSOLUTELY AWESOME !!!


struct TMODInstrumentMark
{
	BYTE used;			//bits determine in which column (on which channel) it is used
	int volume;
	int minnote;
	int maxnote;
	int samplen;
	int reppoint;
	int replen;
	double trackvolumeincrease;
	int trackvolumemax;
};

int AtariVolume(int volume0_64)
{
	int	avol = (int)((double)volume0_64 / 4 + 0.5);	//conversion to atari volume
	if (volume0_64 < 1) avol = 0;
	else
		if (volume0_64 == 1) avol = 1;
		else
			if (avol > 0x0f) avol = 0x0f;
	return avol;
}

int CSong::ImportMOD(ifstream& in)
{
	//deletes the current song
	int originalg_tracks4_8 = g_tracks4_8;	//keeps the original value for Abort
	g_tracks4_8 = 8;					//prepares 8 channels
	g_Tracks.m_maxtracklen = 64;	//track length 64
	ClearSong(g_tracks4_8);			//clear existing data

	int i, j;
	BYTE a;
	BYTE head[1085];

	in.read((char*)&head, 1084);
	int flen = (int)in.gcount();

	head[1084] = 0;	//finished behind the header

	if (flen != 1084)
	{
		MessageBox(g_hwnd, "Bad file format.", "Error", MB_ICONSTOP);
		return 0;
	}

	in.seekg(0, ios::end);
	int modulelength = (int)in.tellg();

	int chnls = 0;
	int song = 950;		//where the song starts at the 31 sample module
	int patstart = 1084;	//the beginning of the pattern at the 31 sample module
	int modsamples = 31;	//31 samples
	if (strncmp((char*)(head + 1080), "M.K.", 4) == 0)
		chnls = 4;					//M.K.
	else
		if (strncmp((char*)(head + 1081), "CHN", 3) == 0)
			chnls = head[1080] - '0';	//xCHN
		else
		{
			for (int i = 0; i < 4; i++)
			{
				a = head[1080 + i];
				if (a < 32 || a>90) //it's outside " " and "Z" (ie it's not a letter or a space)
				{
					//=> 15 samples MOD
					chnls = 4;
					modsamples = 15;
					song = 470;
					patstart = 600;
					break;
				}
			}
		}

	if (chnls < 4 || chnls>8)
	{
		CString es;
		es.Format("There isn't ProTracker identification header bytes.\nAllowed headers are \"M.K.\" or from \"4CHN\" to \"8CHN\",\nbut there is \"%s\".", head + 1080);
		MessageBox(g_hwnd, (LPCTSTR)es, "Error", MB_ICONSTOP);
		return 0;
	}
	int patternsize = chnls * 256;

	int songlen = head[song + 0];
	int restartpos = head[song + 1];
	if (restartpos >= songlen) restartpos = 0;

	int maxpat = 0;
	if (songlen > SONGLEN - 1) songlen = SONGLEN - 1;
	for (i = 0; i < songlen; i++)
	{
		int patnum = head[song + 2 + i];
		if (patnum > maxpat) maxpat = patnum;
	}
	int memlen = song + 130 + patternsize * (maxpat + 1);				//130 = 1 length +1 repeat + 128 song
	if (song >= 950) memlen += 4;		//in addition 4 identification letters (eg "M.K.") // song + 130 (1084)

	//now it is allocating memory
	BYTE* mem = new BYTE[memlen];
	if (!mem)
	{
		MessageBox(g_hwnd, "Can't allocate memory for module.", "Error", MB_ICONSTOP);
		return 0;
	}

	in.clear();
	in.seekg(0);		//again at the beginning
	in.read((char*)mem, memlen);	//load module
	flen = (int)in.gcount();
	if (flen != memlen)
	{
		MessageBox(g_hwnd, "Bad file.", "Error", MB_ICONSTOP);
		if (mem) delete[] mem;
		return 0;
	}

	BYTE trackorder[8] = { 0,1,2,3,4,5,6,7 };	//track layout
	int rmttype = 0;

	CImportModDlg importdlg;
	importdlg.m_info.Format("%i channels %i samples ProTracker module detected.\n(Header bytes \"%s\".)", chnls, modsamples, head + 1080);
	if (chnls == 4)
	{	//4 channels module
		importdlg.m_txtradio1 = "RMT4 with 1,2,3,4 tracks order";
		importdlg.m_txtradio2 = "RMT8 with 1,4 / 2,3 tracks order";
		if (importdlg.DoModal() == IDOK)
		{
			if (importdlg.m_txtradio1 != "")
			{	//first choice
				rmttype = 4;
			}
			else
			{	//second choice
				rmttype = 8;
				trackorder[0] = 0;
				trackorder[1] = 4;
				trackorder[2] = 5;
				trackorder[3] = 1;
			}
		}
	}
	else
	{	//5-8 channels module
		importdlg.m_txtradio1 = "RMT8 with 1,4,5,8 / 2,3,6,7 tracks order";
		importdlg.m_txtradio2 = "RMT8 with 1,2,3,4 / 5,6,7,8 tracks order";
		if (importdlg.DoModal() == IDOK)
		{
			if (importdlg.m_txtradio1 != "")
			{	//first choice
				rmttype = 8;
				trackorder[0] = 0;
				trackorder[1] = 4;
				trackorder[2] = 5;
				trackorder[3] = 1;
				trackorder[4] = 2;
				trackorder[5] = 6;
				trackorder[6] = 7;
				trackorder[7] = 3;
			}
			else
			{	//second choice
				rmttype = 8;
			}
		}
	}

	if (rmttype != 4 && rmttype != 8)
	{	//did not select the back option (cancel in the dialog)
		if (mem) delete[] mem;
		return 0;
	}

	BOOL x_shiftdownoctave = importdlg.m_check1;
	BOOL x_portamento = importdlg.m_check5;
	BOOL x_fullvolumerange = importdlg.m_check2;
	BOOL x_volumeincrease = importdlg.m_check3;
	BOOL x_decreaseinstrument = importdlg.m_check4;
	BOOL x_optimizeloops = importdlg.m_check6;
	BOOL x_truncateunusedparts = importdlg.m_check7;
	BOOL x_fourier = importdlg.m_check8;

	g_tracks4_8 = rmttype;	//produce RMT4 or RMT8

	//song name
	for (j = 0; j < 20 && (a = mem[j]); j++) m_songname[j] = a;

	//speeds
	m_mainspeed = m_speed = 6;			//default speed
	m_instrspeed = 1;

	int maxsmplen = 0;			//maximum sample length

	//instruments 1-31
	TMODInstrumentMark imark[32];
	for (i = 1; i <= modsamples; i++)
	{
		//name 
		BYTE* sdata = mem + (20 + (i - 1) * 30);	//sample header data
		TInstrument* ti = &g_Instruments.m_instr[i];
		char* dname = ti->name;
		for (j = 0; j < 22; j++)	//0-21 name
		{
			a = sdata[j];
			if (a >= 32 && a <= 126)
				dname[j] = a;
			else
				dname[j] = ' ';
		}
		for (; j < INSTRNAMEMAXLEN; j++) dname[j] = ' '; //deletes the rest of the instrument name
		int samplen = (sdata[23] | (sdata[22] << 8)) * 2;
		//BYTE finetune=sdata[24]&0x0f;		//0-15
		//if (finetune>=8) finetune+=240;		//0-7 or 248-255

		int volume = sdata[25];
		if (volume > 0x3f) volume = 0x3f;		//00-3f

		int reppoint = (sdata[27] | (sdata[26] << 8)) * 2;

		int replen = (sdata[29] | (sdata[28] << 8)) * 2;

		//ti->env[0][ENV_VOLUMEL]= (volume>>2);	//0-15
		//ti->env[0][ENV_VOLUMER]= (volume>>2);	//0-15
		//ti->env[0][ENV_DISTORTION]= 0x0a;		//clean tone
		/*if (finetune!=0)
		{
			ti->env[0][ENV_COMMAND]=0x02;		//frequency shift
			ti->env[0][ENV_X]=(finetune>>4);
			ti->env[0][ENV_Y]=(finetune&0x0f);
		}
		*/
		//if (!reppoint) ti->par[PAR_VSLIDE]=255-(samplen>>8);

		imark[i].volume = volume;
		imark[i].minnote = NOTESNUM - 1;
		imark[i].maxnote = 0;
		imark[i].used = 0;			//do not use on any channel
		imark[i].samplen = samplen;
		imark[i].reppoint = reppoint;
		imark[i].replen = replen;
		imark[i].trackvolumeincrease = 1;
		imark[i].trackvolumemax = 0;

		if (samplen > maxsmplen) maxsmplen = samplen;	//maximum sample length

		//sending into Atari memory is at the end of adding things to the instrument
	}
	imark[0].trackvolumeincrease = 1; //due to volume slide, if it is performed without specifying a sample (ie sample number 0)

	const int TABLENOTES = 73;
	const int pertable[TABLENOTES] = {															//period table
	0x06B0,0x0650,0x05F4,0x05A0,0x054C,0x0500,0x04B8,0x0474,0x0434,0x03F8,0x03C0,0x038B,	//C3-B3
	0x0358,0x0328,0x02FA,0x02D0,0x02A6,0x0280,0x025C,0x023A,0x021A,0x01FC,0x01E0,0x01C5,	//C4-B4
	0x01AC,0x0194,0x017D,0x0168,0x0153,0x0140,0x012E,0x011D,0x010D,0x00FE,0x00F0,0x00E2,	//C5-B5
	0x00D6,0x00CA,0x00BE,0x00B4,0x00AA,0x00A0,0x0097,0x008F,0x0087,0x007F,0x0078,0x0071,	//C6-B6
	0x006B,0x0065,0x005F,0x005A,0x0055,0x0050,0x004B,0x0047,0x0043,0x003F,0x003C,0x0038,	//C7-B7
	0x0035,0x0032,0x002F,0x002D,0x002A,0x0028,0x0025,0x0023,0x0021,0x001F,0x001E,0x001C,	//C8-B8
	0x001B }; //calculated value															//C9

	//make a table
	BYTE pertonote[4096];	//convert period to note
	int n1 = 0, n2 = 0, n12 = 0, lastp = 4095;
	for (i = 0; i < TABLENOTES; i++)
	{
		n1 = pertable[i];
		if (i < TABLENOTES - 1)
		{
			n2 = pertable[i + 1];
			n12 = (int)(((float)(n1 + n2)) / 2 + 0.5);
		}
		else
		{
			n12 = 0;
		}
		BYTE note = (BYTE)i;
		while (note >= NOTESNUM) note -= 12;
		for (j = lastp; j >= n12; j--) pertonote[j] = note;
		lastp = n12 - 1;
	}

	//BEGINNING OF PROCESSING THE ENTIRE SONG AND PATTERN
	int dsline;		//destination song line
	int destnum;	//destination track num

	int nofpass = (x_fullvolumerange) ? 1 : 0;
	for (int pass = 0; pass <= nofpass; pass++)
	{ //---TRANSITION 0/1---

		destnum = 0;		//destination track num
		int tnot[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };	//last edge note (used for portamento)
		int tporon[8] = { 0,0,0,0,0,0,0,0 };	//portamento yes/no
		int tporperiod[8] = { 0,0,0,0,0,0,0,0 };	//target period for portamento
		int tporspeed[8] = { 0,0,0,0,0,0,0,0 };		//portamento speed
		int tper[8] = { 0,0,0,0,0,0,0,0 };	//current period
		int tvol[8] = { 0,0,0,0,0,0,0,0 };	//volume of individual tracks (used for volume slide)
		int tvolslidedebt[8] = { 0,0,0,0,0,0,0,0 };		//debt at volume slide
		int tins[8] = { 0,0,0,0,0,0,0,0 };	//individual track instruments (used when the sample is == 0)
		int ticks = m_mainspeed;		//songspeed (for volume slide), default 6
		int beats = 125;				//default beats/min speed

		int LastRowTicks = ticks;
		int LastRowBeats = beats;

		//song
		dsline = 0;		//destination song line
		int ThisPatternFromRow = 0;	//initial pattern line (due to Dxx effect)
		int ThisPatternFromColumn = -1;	//none
		int NextPatternFromRow = 0;
		int NextPatternFromColumn = -1;
		for (i = 0; i < songlen; i++)
		{
			int patnum = mem[song + 2 + i];	//test song

			BYTE* pdata = mem + (patstart + patnum * patternsize);	//beginning of the pattern

			int songjump = -1;	//initialization for song jump
			ThisPatternFromRow = NextPatternFromRow;	//takes over from the previous pattern
			ThisPatternFromColumn = NextPatternFromColumn;	//takes over from the past
			NextPatternFromRow = 0;	//initialized to 0 for the next pattern
			NextPatternFromColumn = -1;	//initialized to -1 for the next pattern

			//pre-calculated tickrow [0..63] and speedrow [0..63]
			int tickrow[64], speedrow[64];
			int m, n;
			BOOL lrow = 0;
			ticks = LastRowTicks;		//previous ticks
			beats = LastRowBeats;		//previous beats
			LastRowTicks = -1;		//init
			LastRowBeats = -1;		//init
			for (m = ThisPatternFromRow; m < 64; m++)
			{
				for (n = 0; n < chnls; n++)
				{
					BYTE* bdata = pdata + m * chnls * 4 + n * 4;
					int effect = bdata[2] & 0x0f;
					int param = bdata[3];
					if (effect == 0x0f)
					{	//speed
						if (param <= 0x20)
							ticks = (param > 0) ? param : 1; //speed 0 is not possible
						else
							beats = param;
					}
					else
						if (effect == 0x0b || effect == 0x0d)
						{	//pattern break or song jump
							lrow = 1;	//you have to write it down and even this whole line 0..chnls
						}
				}
				//recalculation of beats and ticks on ss
				int ss = (int)(((double)(125 * ticks)) / ((double)beats) + 0.5);
				if (ss < 1) ss = 1;
				else
					if (ss > 255) ss = 255;

				tickrow[m] = ticks;
				speedrow[m] = ss;

				if ((lrow || m == 63) && LastRowTicks < 0 && LastRowBeats < 0)
				{
					LastRowTicks = ticks;	//in the next pattern, ticks start with this value
					LastRowBeats = beats;	//in the next pattern, beats start with this value
				}
			}

			//4 tracks in the pattern
			for (int ch = 0; ch < chnls; ch++)
			{
				BYTE* tdata = pdata + ch * 4;	//the beginning of the track data source
				TTrack* tr = g_Tracks.GetTrack(destnum);	//target track
				g_Tracks.ClearTrack(destnum);	//clean it first
				int dline;
				int sline;
				for (sline = ThisPatternFromRow, dline = 0; sline < 64; sline++, dline++)
				{
					BYTE* bdata = tdata + sline * chnls * 4;		//pointer to 4 bytes block

					ticks = tickrow[sline];	//common calculated value of ticks for the whole line

					int period = bdata[1] | ((bdata[0] & 0x0f) << 8);			//12 bit
					int sample = (((bdata[2] & 0xf0) >> 4) | (bdata[0] & 0x10)) & modsamples;	//0-31 / 0-15, not 0-255!
					int effect = bdata[2] & 0x0f;
					int param = bdata[3];
					int sampleorig = sample;		//original as in the track
					int pspeed;

					if (sample == 0) sample = tins[ch];	//sample number 0 means the same as last used
					else
						tins[ch] = sample;	//saves the last used sample in this "column"

					int note = -1, vol = -1;
					if (period < 1 || period >= 4096)
					{
						//empty place (there is no note)
					TonePortamento:
						if (x_portamento && tporon[ch])
						{	//the last time was portamento
							int pnote = pertonote[tper[ch]];	//what note corresponds to the period
							if (pnote != tnot[ch])
							{
								//portamento effect shifted the frequency to the level of other notes
								note = pnote;
								//the volume will be according to the current volume of what is on this channel, ie the form [ch]
								vol = tvol[ch];	//0-64
								tporon[ch] = 0;	//done for now (added note corresponds to change by portamento)
								goto NoteByPortamento;
							}
						}
					}
					else
					{
						//there is a note
						note = pertonote[period];
						if (x_portamento && (effect == 0x03 || effect == 0x05))
						{
							//tone portamento 3xx or continue tone portamento 5
							tporperiod[ch] = period;	//target portamento period
							tporon[ch] = 1;
							//addition
							int aper = tper[ch];
							int hfper = aper;
							if (effect == 0x03)
							{
								if (param)
									pspeed = tporspeed[ch] = param;	//TONE portamento speed only for parameter 3xx
								else
									pspeed = tporspeed[ch];	//if 300 => continue portamento is the last used speed
							}
							else //effect==0x05
								pspeed = tporspeed[ch];	//effect 0x05 is continued portamento

							if (aper < period)
							{
								hfper = aper + pspeed * (ticks - 1) / 2;
								if (hfper > period) hfper = period;
							}
							else
							{
								hfper = aper - pspeed * (ticks - 1) / 2;
								if (hfper < period) hfper = period;
							}
							if (pertonote[aper] != pertonote[hfper])
							{
								note = pertonote[hfper];
								vol = tvol[ch];
								tporon[ch] = 0;	//done for now (added note corresponds to change by portamento)
								goto NoteByPortamento;
							}
							//
							goto TonePortamento;	//and solve it as if it were an empty slot
						}

						tper[ch] = period;	//current period
						tporon[ch] = 0;		//no portamento
						vol = 0x40;			//the default is full volume (if you then overwrite it)
						tvolslidedebt[ch] = 0; //debt volume slide = 0
					NoteByPortamento:
						tnot[ch] = note;		//last note
						tr->note[dline] = note;
						tr->instr[dline] = sample;
						int	avol = AtariVolume(vol);		//conversion to atari volume
						tr->volume[dline] = avol;	//or it will overwrite the Cxx parameter
						imark[sample].used |= (1 << trackorder[ch]);	//sample is used on channel "ch"
						if (note > imark[sample].maxnote) imark[sample].maxnote = note;
						if (note < imark[sample].minnote) imark[sample].minnote = note;
					}

					//effect: PORTAMENTO
					pspeed = 0;
					if (effect == 0x01)	//1xx	Portamento Up
					{
						int cpor = pertable[NOTESNUM - 1];	//the highest note that RMT can play
						tporperiod[ch] = cpor;		//save the target portamento for this channel
						pspeed = param;		//portamento speed
						goto Effect3;
					}
					else
						if (effect == 0x02)	//2xx	Portamento Down
						{
							int cpor = pertable[0];	//the lowest note that RMT can play
							tporperiod[ch] = cpor;		//save the target portamento for this channel
							pspeed = param;		//portamento speed
							goto Effect3;
						}
						else
							if (effect == 0x05)	//5xx	continue toneportamento (+ simultaneous volume slide, to work low)
							{
								pspeed = tporspeed[ch]; //takes over previous speed 
								goto Effect3;	//same as effect 3, but without setting the portamento speed
							}
							else
								if (effect == 0x03)	//3xx	TonePortamento
								{
									if (param)
										pspeed = tporspeed[ch] = param;	//TONE portamento speed only for parameter 3xx
									else
										pspeed = tporspeed[ch];	//if 300 => continue portamento is the last used speed
								Effect3:
									int cpor = tporperiod[ch]; //target portamento
									int aper = tper[ch];	//current period
									if (aper > cpor)
									{
										//portamento towards smaller values, ie up to higher tones
										aper -= pspeed * (ticks - 1);
										if (aper < cpor) aper = cpor;	//if it took place, then compare
									}
									else
										if (aper < cpor)
										{
											//portamento towards higher values, ie down to lower tones
											aper += pspeed * (ticks - 1);
											if (aper > cpor) aper = cpor;	//if it took place, then compare
										}

									tper[ch] = aper;
									tporon[ch] = 1;	//in the next step, the port will be resolved
								}


					//effects: VOLUME
					if (effect == 0x0c)	//Cxx   setvolume xx=$00-$40
					{
						if (pass == 1)
							param = (int)((double)param * imark[sample].trackvolumeincrease + 0.5);
						if (param > 0x40) param = 0x40;	//maximum volume
						tvol[ch] = param;
						tvolslidedebt[ch] = 0;		//no debt
						if (param > imark[sample].trackvolumemax) imark[sample].trackvolumemax = param;

						int avol = AtariVolume(param);	//atari volume
						tr->volume[dline] = avol;
					}
					else
					{	//it is not Cxx => undefined volume
						if (note >= 0 || sampleorig > 0)
						{
							//note without volume or sample without note => default maximum volume
							int v = (vol >= 0) ? vol : 0x40;	//takes either "vol" from the portamento, or the default full
							tvol[ch] = v;
							imark[sample].trackvolumemax = v;
						}
					}

					//effect NEXT
					if (effect == 0x0a		//Axx	volumeslide xx = 0x decrease, xx = x0 increase
						|| effect == 0x05		//5xx	continue toneportamento (this has already done above) + volumeslide xx
						|| effect == 0x06		//6xx	continue vibrato + volumeslide xx => only volumeslide xx
						)
					{
						int vol = tvol[ch];
						double slidedivide = (note >= 0 || sampleorig > 0) ? 2 : 1; //first half is half
						if ((param & 0xf0) == 0)
						{
							int voldec = (int)((double)(param & 0x0f) * (ticks - 1) * imark[sample].trackvolumeincrease / slidedivide + 0.5);
							vol -= voldec;	//decrease
							if (slidedivide == 2)	tvolslidedebt[ch] = -voldec; //debt volume slide
						}
						else
						{
							int volinc = (int)((double)((param & 0xf0) >> 4) * (ticks - 1) * imark[sample].trackvolumeincrease / slidedivide + 0.5);
							vol += volinc;	//increase
							if (slidedivide == 2)	tvolslidedebt[ch] = volinc; //debt volume slide
						}

						if (vol < 0) vol = 0;
						else
							if (vol > 0x40) vol = 0x40;

						tvol[ch] = vol;
						if (vol > imark[sample].trackvolumemax) imark[sample].trackvolumemax = vol;

						int avol = AtariVolume(vol);		//atari volume
						tr->volume[dline] = avol;
					}
					else
						if (effect == 0x0f)	//Fxx   setspeed/tempo
						{
							/*
							if (param<=0x20)
								ticks=(param>0)? param : 1; //speed 0 is not possible
							else
								beats=param;

							int ss= (int)(((double)(125 * ticks)) / ((double)beats) + 0.5);
							if (ss<1) ss=1;
							else
							if (ss>255) ss=255;
							*/
							int ss = speedrow[sline];		//use a common pre-calculated value for the whole row
							tr->speed[dline] = ss;
						}
						else
							if (effect == 0x0d)	//Dxx	pattern break (xx = goto xx line in the next pattern <- hm, we do not know)
							{
								//end the track on the first occurrence of Dxx from above
								//and continue to position xx
								if (tr->len == 64)
								{
									tr->len = dline + 1;
									int nxp = ((int)(param / 16)) * 10 + (param % 16);		//it's there in the 10th system
									if (nxp >= 0 && nxp < 64 && NextPatternFromColumn == -1)
									{
										NextPatternFromRow = nxp;
										NextPatternFromColumn = ch;
									}
								}
							}
							else
								if (effect == 0x0b)	//Bxx	song jump
								{
									if (tr->len == 64) tr->len = dline + 1;	//track break
									if (songjump < 0) songjump = param;
								}

					//the debt will increment if there is any and there is a free slot
					if (tr->volume[dline] < 0)	//the volume is not specified
					{
						int mvol = tvol[ch];
						mvol += tvolslidedebt[ch]; //adjust volume by debt
						if (mvol < 0) mvol = 0;
						else
							if (mvol > 0x40) mvol = 0x40;
						int avol = AtariVolume(mvol);
						if (avol != AtariVolume(tvol[ch])) tr->volume[dline] = avol; //if it comes out differently than it was, it will add it
						tvol[ch] = mvol;
						tvolslidedebt[ch] = 0; //debt volume slide resolved
					}

				}//line 0-64

				//if the shift (effect Dxx) has started, then it must shorten the length of the respective track
				if (ThisPatternFromColumn == ch && tr->len == 64 && dline < 64) tr->len = dline;

				int cit = -1;	//track number for the song
				if (!g_Tracks.IsEmptyTrack(destnum)) //is empty
				{
					//Removes excess volume 0
					g_Tracks.TrackOptimizeVol0(destnum);

					//see if such a track already exists
					for (int k = 0; k < destnum; k++)
					{
						if (g_Tracks.CompareTracks(k, destnum))
						{
							cit = k;	//found one
							break;
						}
					}
					if (cit < 0)
					{
						cit = destnum;	//was not found
						destnum++;		//prepare for the next
					}
				}

				m_song[dsline][trackorder[ch]] = cit;
				if (destnum >= TRACKSNUM)
				{
					if (pass == 0) MessageBox(g_hwnd, "Out of RMT tracks. Tracks converting terminated.", "Warning", MB_ICONWARNING);
					goto OutOfTracks;	//the tracks have reached the end
				}

			}//4 tracks in the pattern
			dsline++; //increment the target number of songlines(?)
			if (dsline >= SONGLEN)
			{
				if (pass == 0) MessageBox(g_hwnd, "Out of song lines. Song converting terminated.", "Warning", MB_ICONWARNING);
				goto OutOfSongLines;	//ran out of songlines
			}

			if (songjump >= 0 && songjump < SONGLEN)
			{
				m_songgo[dsline] = songjump;
				dsline++;
				if (dsline >= SONGLEN)
				{
					if (pass == 0) MessageBox(g_hwnd, "Out of song lines. Song converting terminated.", "Warning", MB_ICONWARNING);
					goto OutOfSongLines;
				}
			}
		}
	OutOfTracks:
	OutOfSongLines:

		//ALL PATTERNS OF SONG ARE DONE

		//prepare a track volume increase for each sample to make it the second time it arrives
		//increased the volume in the tracks
		if (pass == 0)
		{
			for (i = 1; i <= modsamples; i++)
			{
				TMODInstrumentMark* it = &imark[i];
				if (it->trackvolumemax > 0) it->trackvolumeincrease = (double)0x40 / (it->trackvolumemax);
			}
		}

		//---END OF TRANSITION 0/1---
	} //pass=0/1

	//corrects the jumps in the song
	int nog = 0;
	for (i = 0; i < dsline; i++)
	{
		int k;
		int go = m_songgo[i];
		if (go < 0) continue;
		for (k = 0; k <= go && k < SONGLEN; k++)	//looking for how much is from the beginning of the jump and for each found moves go by 1 step
		{
			if (m_songgo[k] >= 0) go++;
		}
		m_songgo[i] = go;	//writes that shifted jump
	}
	if (m_songgo[dsline - 1] < 0) { m_songgo[dsline] = restartpos; dsline++; } //loop at the beginning or where it wants according to header[951]

	//add to the instrument in the description MIN MAX range
	//and finds globally the lowest and highest used note of all instruments and in the whole song
	int glonomin = NOTESNUM - 1;
	int glonomax = 0;
	for (i = 1; i <= modsamples; i++)
	{
		if (!imark[i].used) continue;
		int minnote = imark[i].minnote;
		int maxnote = imark[i].maxnote;
		if (minnote < glonomin) glonomin = minnote;
		if (maxnote > glonomax) glonomax = maxnote;
		//maximum volume in tracks
		int avol = AtariVolume(imark[i].trackvolumemax);	//atari volume
		CString s;
		s.Format("%s%s%X%02X", notes[minnote], notes[maxnote], avol, imark[i].used);
		strncpy(g_Instruments.m_instr[i].name + 23, s, 9);	//9 characters !! 23 + 9 = 32
	}

	if (x_shiftdownoctave) //an octave shift down for notes tuned too high (if possible)
	{
		if (glonomin >= 12 && glonomax > 36 + 5)
		{
			int noteshift = 256 - 12;	//1 octave lower
			for (i = 1; i <= modsamples; i++)
			{
				if (!imark[i].used) continue;
				g_Instruments.m_instr[i].tab[0] = (BYTE)(noteshift);
			}
		}
	}

	//imitation volume according to sample
	int smpfrom = patstart + (maxpat + 1) * patternsize;
	int samplen = 0;
	int nonemptysamples = 0;
	for (i = 1; i <= modsamples; i++, smpfrom += samplen)	//at the end of the loop, always move to the next sample
	{
		TMODInstrumentMark* im = &imark[i];
		TInstrument* rmti = &g_Instruments.m_instr[i];

		samplen = im->samplen;

		if (samplen <= 2)	continue;		//zero length => empty sample

		BYTE* smpdata = new BYTE[samplen];

		if (!smpdata) continue;		//could not be allocated
		memset(smpdata, 0, samplen);	//clear

		in.clear();		//due to reaching the end when eof is set
		in.seekg(smpfrom, ios::beg);
		if ((int)in.tellg() != smpfrom)
		{
			CString s;
			s.Format("Can't seek sample #%02X data.", i);
			MessageBox(g_hwnd, (LPCTSTR)s, "Warning", MB_ICONWARNING);
		}
		else
		{
			in.read((char*)smpdata, samplen);
			if (in.gcount() != samplen)
			{
				CString s;
				s.Format("Can't read fully sample #%02X data.", i);
				MessageBox(g_hwnd, (LPCTSTR)s, "Warning", MB_ICONWARNING);
			}
		}

		int minnote = im->minnote;
		int period = pertable[minnote];
		int parts = samplen / period;
		if (parts < 1) parts = 1;
		if (im->replen > 2 || im->reppoint > 0)
		{
			//there is a loop
			if (parts > 32) parts = 32;
		}
		else
		{
			//there is no loop
			if (parts > 31) parts = 31; //32 parts reserved for silence
		}
		int blocksize = samplen / parts;
		int blockp = blocksize - 1;		//-1 to shift the boundaries between partitions by 1 to the left
									//and the last section was counted
		long sum = 0;
		BYTE lastsd = 0;
		long maxsum = 1;	//the maximum achievable amount in a sample block (it's 1 because it is divided)

		int ix = 0;
		long sumtab[32];
		for (int k = 0; k < samplen; k++)
		{
			sum += abs((int)smpdata[k] - (int)lastsd);
			lastsd = smpdata[k];		//last state of the curve
			if (k == blockp)			//borders between divisions
			{
				sumtab[ix] = sum;
				if (sum > maxsum) maxsum = sum;	//the highest
				sum = 0;
				blockp += blocksize;	//shifting the boundaries by the length of the section
				ix++;
				if (ix >= parts) break;
			}
		}

		double sampvol = (x_decreaseinstrument) ? ((double)im->volume / 0x3f) : 1;	//decimal number 0 to 1
		if (x_volumeincrease) sampvol /= im->trackvolumeincrease;		//decreases as you increase the volume in treks
		for (int k = 0; k < ix; k++)
		{
			int avol = (int)((double)16 * ((double)sumtab[k] / maxsum) * sampvol + 0.5);
			if (avol > 15) avol = 15;
			rmti->env[k][ENV_VOLUMEL] = rmti->env[k][ENV_VOLUMER] = avol;
			rmti->env[k][ENV_DISTORTION] = 0x0a;	//pure tone
		}

		rmti->par[PAR_ENVLEN] = ix - 1;
		if (im->replen > 2 || im->reppoint > 0) //is there a loop?
		{
			//loop
			int ego = (int)((double)im->reppoint / blocksize + 0.5);
			if (ego > ix - 1) ego = ix - 1;
			rmti->par[PAR_ENVGO] = ego;
			//and divides the end of the instrument according to the length of the loop
			int lopend = (int)((double)(im->reppoint + im->replen) / blocksize + 0.5);
			if (lopend > ix - 1) lopend = ix - 1;
			rmti->par[PAR_ENVLEN];
		}
		else
		{
			//no loop (ix is max 31, so it can add a "silent loop" to the end)
			rmti->env[ix][ENV_VOLUMEL] = rmti->env[ix][ENV_VOLUMER] = 0; //silence at the end
			rmti->par[PAR_ENVLEN] = ix;	//length of 1 vic
			rmti->par[PAR_ENVGO] = ix;	//jump on the same thing
		}

		//Fourier
		/*

		if (x_fourier)
		{
#define	F_BLOCK		4096
#define F_SAMPLELEN	5*F_BLOCK
			Fft *myFFT=NULL;
			BYTE fsample[F_SAMPLELEN];	//bere prvnich 5 bloku => 20480 => cca 1 sekunda
			int sapos=0,safro=0,salen=im->samplen;
			while(sapos<F_SAMPLELEN)	// && im->replen>2
			{
				int lenb = (sapos+salen<F_SAMPLELEN)? salen : F_SAMPLELEN-sapos;
				memcpy(fsample+sapos,smpdata+safro,lenb);
				sapos+=lenb;
				if (im->replen>2 || im->reppoint>0)
				{
					safro = im->reppoint;
					salen= im->replen;
				}
			}

			int zaknota=0;
			if (rmti->par[PAR_TABLEN]==0) zaknota=rmti->tab[0];


			for(int i=0; i<flen/F_BLOCK; i++)
			{
				BYTE *bsample=fsample+i*F_BLOCK;
				if (myFFT) delete myFFT;
				myFFT = new Fft(4096,22050);
				if (!myFFT) break;
				for(int j=0; j<F_BLOCK; j++) myFFT->PutAt(j,bsample[j]-128);
				myFFT->Transform();

				int no[3];
				int p=myFFT->GetNotes(no[0],no[1],no[2]);
				if (p>0)
				{
					for(int j=0; j<p; j++) rmti->tab[j]=zaknota+no[j]%12;
					rmti->par[PAR_TABLEN]=p-1;
					rmti->par[PAR_TABGO]=0;
					rmti->par[PAR_TABTYPE]=0;	//tabulka not
					rmti->par[PAR_TABMODE]=0;	//nastavovani
					rmti->par[PAR_TABSPD]=1;
				}
				if (myFFT)
				{
					delete myFFT;
					myFFT=NULL;
				}
			}
		}
		//konec Fouriera
		*/

		//number of non-empty samples
		nonemptysamples++;

		//free memory
		if (smpdata) { delete[] smpdata; smpdata = NULL; }

	}

	//checking the end of the module with the end of the last sample
	if (smpfrom != modulelength)
	{
		//is different
		CString s;
		s.Format("Bad length of module.\n(Last sample's end is at %i, but length of module is %i.)", smpfrom, modulelength);
		MessageBox(g_hwnd, s, "Warning", MB_ICONWARNING);
	}

	//and only at the end
	for (i = 1; i <= modsamples; i++)
	{
		//send to Atari
		g_Instruments.ModificationInstrument(i);
	}

	//CLEAR MEMORY 
	if (mem) { delete[] mem;	mem = NULL; }

	//FINAL DIALOGUE AFTER IMPORT
	CImportModFinishedDlg imfdlg;
	imfdlg.m_info.Format("%i tracks, %i instruments, %i songlines", destnum, nonemptysamples, dsline);

	//OPTIMIZATIONS
	if (x_optimizeloops)
	{
		int optitracks = 0, optibeats = 0;
		TracksAllBuildLoops(optitracks, optibeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Loops in %i tracks (%i beats/lines)", optitracks, optibeats);
		imfdlg.m_info += s;
	}

	if (x_truncateunusedparts)
	{
		int clearedtracks = 0, truncatedtracks = 0, truncatedbeats = 0;
		SongClearUnusedTracksAndParts(clearedtracks, truncatedtracks, truncatedbeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Cleared %i, truncated %i tracks (%i beats/lines)", clearedtracks, truncatedtracks, truncatedbeats);
		imfdlg.m_info += s;
	}

	if (imfdlg.DoModal() != IDOK)
	{
		//did not give Ok, so it deletes
		g_tracks4_8 = originalg_tracks4_8;	//returns the original value
		ClearSong(g_tracks4_8);
		MessageBox(g_hwnd, "Module import aborted.", "Import...", MB_ICONINFORMATION);
	}

	return 1;
}