//
// OBJECTS.CPP
//

#include "stdafx.h"

#include "r_music.h"

CTracks::CTracks()
{
	for(int i=0; i<TRACKSNUM; i++)
	{
		ClearTrack(i);
	}
}

BOOL CTracks::ClearTrack(int t)
{
	m_track[t].len=0;
	m_track[t].go=-1;
	for (int i=0; i<TRACKLEN; i++)
	{
		m_track[t].note[i]=-1;
		m_track[t].instr[i]=-1;
		m_track[t].volume[i]=-1;
		m_track[t].speed[i]=-1;
	}
	return 1;
}

//----------------------------------------------

CSong::CSong()
{
	ClearSong();
}

BOOL CSong::ClearSong()
{
	m_len=2;
	m_startline = m_playline = m_viewline = 0;
	for(int i=0; i<SONGLEN; i++)
		for(int j=0; j<SONGTRACKS; j++) m_song[i][j]=255;	//TRACK --
	m_song[1][0] = 254; //GO
	m_song[1][1] = 0;	//GO LINE 0
	return 1;
}


#define	SONG_X	300
#define SONG_Y	10

BOOL CSong::ShowSong(CDC* dc)
{
	int line;
	char s[32];
	for(int i=0; i<4; i++)
	{
		line = m_viewline + i - 1; //1 radek nad
		if (line<0) continue;
		sprintf(s,"%02Xi: %02Xi %02Xi %02Xi %02Xi %02Xi %02Xi %02Xi %02Xi",
			m_song[i][0],m_song[i][1],m_song[i][2],m_song[i][3],
			m_song[i][4],m_song[i][5],m_song[i][6],m_song[i][7]);
		//TextXY(dc,s,SONG_X,SONG_Y+i*16);
	}
	return 1;
}