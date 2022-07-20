//
// R_MUSIC.CPP
//

// Original comments by Raster:
// 25.12.2002 21:45 .... Unbelievable, it saves and loads RMT in Atari object file format!!!
//
// 25.12.2002 22:22 .... Unbelievable, it exports a functional SAP module!!!
//
// 26.12.2002 10:59 .... I created a monster with montra possibilities. You can also play it as a SID from C64. ;-)
//
// 27.4.2003 .... Fixed a long bug with omitting 1/50 of the instrument at the first tones (thanks for the notification from Members from Indiana)
//
// 11.2.2004 ... Generated FEAT_VOLUMEMIN, FEAT_TABLEGO generated
//
// November 2021 -> experimental features by VinsCool... 
// Thanks again for everything. Rest in Peace, Raster.
// 
// December 2021 -> new additions, and code cleanup.
// 2022... -> a lot of experimental changes, no promise anything didn't get broken in the process!

#include "stdafx.h"
#include <fstream>
using namespace std;
#include <malloc.h>
#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"
#include "MainFrm.h"	//!
#include "r_music.h"
#include "Atari6502.h"
#include "global.h"
#include <iostream>
#include <cmath>

//-------

#define BLOCKSETBEGIN	g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline)
#define BLOCKSETEND		g_trackcl.BlockSetEnd(m_trackactiveline)
#define BLOCKDESELECT	g_trackcl.BlockDeselect()
#define ISBLOCKSELECTED	g_trackcl.IsBlockSelected()
#define SCREENUPDATE	g_screenupdate=1

CString GetFilePath(CString pathandfilename)
{
	//vstup:  c:/neco/nekde/kdovikde\nebo\taky\tohle.ext
	//vystup: c:/neco/nekde/kdovikde\nebo\taky
	//(just from the beginning to the last slash (/ or \)
	CString res;
	int pos=pathandfilename.ReverseFind('/');
	if (pos<0) pos=pathandfilename.ReverseFind('\\');
	if (pos>=0)
		res=pathandfilename.Mid(0,pos); //from 0 to pos
	else
		res="";
	return res;
}

//-------

void SetStatusBarText(const char* text)
{
	CStatusBar& sb = ((CMainFrame*)AfxGetApp()->GetMainWnd())->m_wndStatusBar;
	sb.SetWindowText(text);
}

//-------

int GetModifiedNote(int note,int tuning)
{
	if (note<0 || note>=NOTESNUM) return -1;
	int n=note+tuning;
	if (n<0) 
		n+=((int)((-n-1)/12)+1)*12;
	else
	if (n>=NOTESNUM)
		n-=((int)(n-NOTESNUM)/12+1)*12;
	return n;
}

int GetModifiedInstr(int instr,int instradd)
{
	if (instr<0 || instr>=INSTRSNUM) return -1;
	int i=instr+instradd;
	while (i<0) { i+=INSTRSNUM; }
	while (i>=INSTRSNUM) { i-=INSTRSNUM; }
	return i;
}

int GetModifiedVolumeP(int volume,int percentage)
{
	if (volume<0) return -1;
	if (percentage<=0) return 0;
	int v = (int)((float)percentage/100*volume+0.5);
	return (v>MAXVOLUME)? MAXVOLUME : v;
}

BOOL ModifyTrack(TTrack *track,int from,int to,int instrnumonly,int tuning,int instradd,int volumep)
{
	//instruments <0 => all instruments
	//            > = 0 => only that one instrument
	if (!track) return 0;
	if (to>=TRACKLEN) to=TRACKLEN-1;
	int i,instr;
	int ainstr=-1;
	for(i=from; i<=to; i++)
	{
		instr = track->instr[i];
		if (instr>=0) ainstr=instr;
		if (instrnumonly>=0 && instrnumonly!=ainstr) continue;
		track->note[i] = GetModifiedNote(track->note[i],tuning);
		track->instr[i] = GetModifiedInstr(instr,instradd);
		track->volume[i] = GetModifiedVolumeP(track->volume[i],volumep);
	}
	return 1;
}

BOOL NextSegment(std::ifstream& in)
{
	char b;
	while (!in.eof())
	{
		in.read((char*)&b, 1);
		if (b == '[') return 1;	//end of segment (beginning of something else)
	}
	return 0;
}

void Trimstr(char* txt)
{
	char a;
	for (int i = 0; (a = txt[i]); i++)
	{
		if (a == 13 || a == 10)
		{
			txt[i] = 0;
			return;
		}
	}
}

char CharH4(unsigned char b) { BYTE i = b >> 4; return ((BYTE)i + ((i < 10) ? 48 : 55)); };
char CharL4(unsigned char b) { BYTE i = b & 0x0f; return ((BYTE)i + ((i < 10) ? 48 : 55)); };


//-------

CTrackClipboard::CTrackClipboard()
{
	m_song = NULL;
	m_trackcopy.len = -1;	//for copying a complete track with loops, etc.
	m_trackbackup.len = -1; //back up
	m_all = 1;				//1 = all events / 0 = only for events with the same instrument as currently set
	Empty();
}

void CTrackClipboard::Init(CSong* song)
{
	m_song = song;
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
	for(int i=0; i<TRACKLEN; i++)
	{
		m_track.note[i]=-1;
		m_track.instr[i]=-1;
		m_track.volume[i]=-1;
		m_track.speed[i]=-1;
	}
}

BOOL CTrackClipboard::BlockSetBegin(int col, int track, int line)
{
	if (col<0 || track<0 || track>=TRACKSNUM || line<0 || line>=TRACKLEN) return 0;
	if (m_selcol<0 || m_selcol!=col || m_seltrack!=track)
	{
		//newly marked start of block
		m_selcol = col;
		m_seltrack = track;
		m_selsongline = m_song->SongGetActiveLine();
		m_selfrom = m_selto = line;
		//keep the track as it was now
		memcpy((void*)(&m_trackbackup),(void*)(m_song->GetTracks()->GetTrack(track)),sizeof(TTrack));
		//and initializes a base track
		BlockInitBase(track);
		return 1;
	}
	return 0;
}

BOOL CTrackClipboard::BlockSetEnd(int line)
{
	if (line<0 || line>=TRACKLEN) return 0;
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
	if (track<0 || track>=TRACKSNUM) return;
	memcpy((void*)(&m_trackbase),(void*)(m_song->GetTracks()->GetTrack(track)),sizeof(TTrack));
	m_changenote=0;
	m_changeinstr=0;
	m_changevolume=0;
	m_instrbase= -1;
}

void CTrackClipboard::BlockAllOnOff()
{
	if (!IsBlockSelected()) return;
	m_all ^= 1;

	int bfro,bto;
	GetFromTo(bfro,bto);

	//projects the current state of the track to the base track
	BlockInitBase(m_seltrack);
}

int CTrackClipboard::BlockCopyToClipboard()
{
	if (!IsBlockSelected()) { ClearTrack(); return 0; }
	if (m_seltrack<0) return 0;

	ClearTrack();

	int j,line,xline;
	int linefrom,lineto;
	GetFromTo(linefrom,lineto);

	TTrack& ts = *m_song->GetTracks()->GetTrack(m_seltrack);
	TTrack& td = m_track;

	int len=ts.len,go=ts.go;

	for(line=linefrom,j=0; line<=lineto; line++,j++)
	{
		if (line<len || go<0) xline=line;
		else
			xline = ((line-len) % (len-go)) + go;

		if (line<len || go>=0)
		{
			td.note[j]=ts.note[xline];
			td.instr[j]=ts.instr[xline];
			td.volume[j]=ts.volume[xline];
			td.speed[j]=ts.speed[xline];
		}
		else
		{
			td.note[j]=-1;
			td.instr[j]=-1;
			td.volume[j]=-1;
			td.speed[j]=-1;
		}
	}
	td.len = j;
	return td.len;
}

int CTrackClipboard::BlockExchangeClipboard()
{
	if (!IsBlockSelected()) return 0;
	if (m_seltrack<0) return 0;

#define EXCH(a,b)	{ int xch=a; a=b; b=xch; }

	int j,line,xline;
	int linefrom,lineto;
	GetFromTo(linefrom,lineto);

	TTrack& ts = *m_song->GetTracks()->GetTrack(m_seltrack);
	TTrack& td = m_track;

	int len=ts.len,go=ts.go;
	int cllen=td.len;

	//it has to do it backwards because of the GO loop 
	//if it first rewrites the notes from above, which are then repeated in the GO loop
	//then the freshly rewritten ones would appear in the clipboard instead of the original loops
	//for(line=linefrom,j=0; line<=lineto; line++,j++)
	for(line=lineto,j=lineto-linefrom; line>=linefrom; line--,j--)
	{
		if (line<len || go<0) xline=line;
		else
			xline = ((line-len) % (len-go)) + go;

		if (line<len)
		{
			EXCH(td.note[j],ts.note[line]);
			EXCH(td.instr[j],ts.instr[line]);
			EXCH(td.volume[j],ts.volume[line]);
			EXCH(td.speed[j],ts.speed[line]);
			if (j>=cllen)
			{	//the block is longer than the length of the data in the clipboard, so it fills with empty lines
				ts.note[line]=-1;
				ts.instr[line]=-1;
				ts.volume[line]=-1;
				ts.speed[line]=-1;
			}
		}
		else
		{	//part of the block extending beyond the end
			if (go>=0)
			{			//there is a loop
				td.note[j]=ts.note[xline];
				td.instr[j]=ts.instr[xline];
				td.volume[j]=ts.volume[xline];
				td.speed[j]=ts.speed[xline];
			}
			else
			{			//there is no loop
				td.note[j]=-1;
				td.instr[j]=-1;
				td.volume[j]=-1;
				td.speed[j]=-1;
			}
		}
	}
	td.len = lineto-linefrom+1;
	return td.len;
}

int CTrackClipboard::BlockPasteToTrack(int track, int line, int special)
{
	if (track<0 || track>=TRACKSNUM) return 0;

	int bfro=-1,bto=-1;
	if (IsBlockSelected() && m_seltrack>=0)
	{
		//A block is selected, so the PASTE will be placed in the line position
		track = m_seltrack;
		GetFromTo(bfro,bto);
	}
	
	TTrack& ts = m_track;
	TTrack& td = *m_song->GetTracks()->GetTrack(track);
	int i,j;
	int linemax;
	
	if (bfro>=0) //block selected (continued)
	{
		line = bfro;
		linemax = bto+1;
	}
	else
		linemax = line + ts.len;

	if (linemax > m_song->GetTracks()->m_maxtracklen) linemax = m_song->GetTracks()->m_maxtracklen;
	if (line>td.len)
	{
		//if it makes a paste under the --end-- line, empty the gap between --end-- and the end of the place where it pastes
		//originally i < line, but because for some it merges to the original, the number of lines will be stretched
		for(i=td.len; i<linemax; i++) td.note[i]=td.instr[i]=td.volume[i]=td.speed[i]=-1;
	}

	if (special==0) //normal paste
	{
		for(i=line,j=0; i<linemax; i++,j++)
		{
			td.note[i]=ts.note[j];
			td.instr[i]=ts.instr[j];
			td.volume[i]=ts.volume[j];
			td.speed[i]=ts.speed[j];
		}
	}
	else //paste special
	if (special==1) //merge
	{
		for(i=line,j=0; i<linemax; i++,j++)
		{
			if (ts.note[j]>=0 && ts.instr[j]>=0 && ts.volume[j]>=0)
			{
				td.note[i]=ts.note[j];
				td.instr[i]=ts.instr[j];
				td.volume[i]=ts.volume[j];
			}
			if (ts.volume[j]>=0) td.volume[i]=ts.volume[j];
			if (ts.speed[j]>=0)	td.speed[i]=ts.speed[j];
		}
	}
	else
	if (special==2) //volumes only
	{
		for(i=line,j=0; i<linemax; i++,j++)
		{
			if (ts.volume[j]>=0) td.volume[i]=ts.volume[j]; //if the source volume is non-negative, it writes it
			else //the source volume is negative, it can only be deleted where there is no note + instr
			if (td.note[i]<0 && td.instr[i]<0) td.volume[i]=-1; //deletes only on separate volumes
		}
	}
	else
	if (special==3) //speeds only
	{
		for(i=line,j=0; i<linemax; i++,j++) td.speed[i]=ts.speed[j];
	}

	//if it's beyond the end of the track, extend its length
	if (linemax>td.len) td.len=linemax;
	return (bfro>=0)? 0 : linemax-line;	//when it was a paste into a block, it returns 0
}

int CTrackClipboard::BlockClear()
{
	if (!IsBlockSelected() || m_seltrack<0) return 0;

	int i,bfro,bto;
	GetFromTo(bfro,bto);

	TTrack& td = *m_song->GetTracks()->GetTrack(m_seltrack);
	for(i=bfro; i<=bto && i<td.len; i++)
	{
		td.note[i]=-1;
		td.instr[i]=-1;
		td.volume[i]=-1;
		td.speed[i]=-1;
	}
	return bto-bfro+1;
}

int CTrackClipboard::BlockRestoreFromBackup()
{
	if (!IsBlockSelected() || m_seltrack<0) return 0;
	memcpy((void*)(m_song->GetTracks()->GetTrack(m_seltrack)),(void*)(&m_trackbackup),sizeof(TTrack));
	BlockInitBase(m_seltrack);
	return 1;
}

void CTrackClipboard::GetFromTo(int& from, int& to)
{ 
	if (!IsBlockSelected()) { from=1; to=0; return; }
	if (m_selfrom<=m_selto) 
	{ from=m_selfrom; to=m_selto; } 
	else 
	{ from=m_selto; to=m_selfrom; }
}

void CTrackClipboard::BlockNoteTransposition(int instr,int addnote)
{
	if (!IsBlockSelected() || m_seltrack<0) return;

	if (instr!=m_instrbase)
	{
		//projects the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}

	m_changenote +=addnote;
	if (m_changenote>=NOTESNUM) m_changenote = NOTESNUM-1;
	if (m_changenote<=-NOTESNUM) m_changenote = -NOTESNUM+1;

	TTrack& td = *m_song->GetTracks()->GetTrack(m_seltrack);
	int i,j;
	int bfro,bto;
	GetFromTo(bfro,bto);

	for(i=bfro; i<=bto && i<td.len; i++)
	{
		if (td.note[i]>=0 && (td.instr[i]==instr || m_all))
		{
			j=m_trackbase.note[i]+m_changenote;
			if (j>=NOTESNUM) j = j-NOTESNUM+1;
			if (j<0) j= j+NOTESNUM-1;
			td.note[i]=j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Note transposition: %+i",m_changenote);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockInstrumentChange(int instr,int addinstr)
{
	if (!IsBlockSelected() || m_seltrack<0) return;

	if (instr!=m_instrbase)
	{
		//add up the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}
	
	m_changeinstr +=addinstr;
	if (m_changeinstr>=INSTRSNUM) m_changeinstr = INSTRSNUM-1;
	if (m_changeinstr<=-INSTRSNUM) m_changeinstr = -INSTRSNUM+1;

	TTrack& td = *m_song->GetTracks()->GetTrack(m_seltrack);
	int i,j;
	int bfro,bto;
	GetFromTo(bfro,bto);

	for(i=bfro; i<=bto && i<td.len; i++)
	{
		if (m_trackbase.instr[i]>=0 && (m_trackbase.instr[i]==instr || m_all))
		{
			j=m_trackbase.instr[i]+m_changeinstr;
			if (j>=INSTRSNUM) j = (j % INSTRSNUM );
			if (j<0) j= (j % INSTRSNUM ) +INSTRSNUM;
			td.instr[i]=j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Instrument change: %+i",m_changeinstr);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockVolumeChange(int instr,int addvol)
{
	if (!IsBlockSelected() || m_seltrack<0) return;

	if (instr!=m_instrbase)
	{
		//add up the current state of the track to the base track
		BlockInitBase(m_seltrack);
		m_instrbase = instr;
	}

	m_changevolume +=addvol;
	if (m_changevolume > MAXVOLUME) m_changevolume = MAXVOLUME;
	if (m_changevolume < -MAXVOLUME) m_changevolume = -MAXVOLUME;

	TTrack& td = *m_song->GetTracks()->GetTrack(m_seltrack);
	int i,j;
	int lasti=-1;
	int bfro,bto;
	GetFromTo(bfro,bto);

	for(i=bfro; i<=bto && i<td.len; i++)
	{
		if (td.instr[i]>=0) lasti=td.instr[i]; //so that when the volume itself is edited, we know it belongs to the instrument above it
		if (td.volume[i]>=0 && (lasti==instr || m_all))
		{
			j=m_trackbase.volume[i]+m_changevolume;
			if (j>MAXVOLUME) j=MAXVOLUME;
			if (j<0) j=0;
			td.volume[i]=j;
		}
	}

	//Status bar info
	CString s;
	s.Format("Volume change: %+i",m_changevolume);
	SetStatusBarText((LPCTSTR)s);
}


BOOL CTrackClipboard::BlockEffect()
{
	if (!IsBlockSelected() || m_seltrack<0) return 0;

	CEffectsDlg dlg;

	if (m_all) dlg.m_info = "Changes will be provided for all data in the block";
	else
		dlg.m_info.Format("Changes will be provided for data making use of instrument %02X only",m_song->GetActiveInstr());

	TTrack* td = m_song->GetTracks()->GetTrack(m_seltrack);
	int ainstr = m_song->GetActiveInstr();
	int bfro,bto;
	GetFromTo(bfro,bto);
	if (bto>=td->len) bto=td->len-1;

	TTrack m_trackorig;
	memcpy(&m_trackorig,td,sizeof(TTrack));
	dlg.m_trackorig = &m_trackorig;
	dlg.m_trackptr = td;
	dlg.m_bfro = bfro;
	dlg.m_bto = bto;
	dlg.m_ainstr = ainstr;
	dlg.m_all = m_all;
	dlg.m_song = m_song;

	int r = dlg.DoModal();

	return (r==IDOK);
}

//-------

//TODO: Optimise the notes arrays to be more compact, there is a lot of duplicates
const char* notes[] =
{ "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","B-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","B-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","B-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","B-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","B-5",
  "C-6","???","???","???"
};

const char* notesflat[] =
{ "C-1","Db1","D-1","Eb1","E-1","F-1","Gb1","G-1","Ab1","A-1","Bb1","B-1",
  "C-2","Db2","D-2","Eb2","E-2","F-2","Gb2","G-2","Ab2","A-2","Bb2","B-2",
  "C-3","Db3","D-3","Eb3","E-3","F-3","Gb3","G-3","Ab3","A-3","Bb3","B-3",
  "C-4","Db4","D-4","Eb4","E-4","F-4","Gb4","G-4","Ab4","A-4","Bb4","B-4",
  "C-5","Db5","D-5","Eb5","E-5","F-5","Gb5","G-5","Ab5","A-5","Bb5","B-5",
  "C-6","???","???","???"
};

const char* notesgerman[] =
{ "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","H-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","H-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","H-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","H-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","H-5",
  "C-6","???","???","???"
};

const char* notesgermanflat[] =
{ "C-1","Db1","D-1","Eb1","E-1","F-1","Gb1","G-1","Ab1","A-1","B-1","H-1",
  "C-2","Db2","D-2","Eb2","E-2","F-2","Gb2","G-2","Ab2","A-2","B-2","H-2",
  "C-3","Db3","D-3","Eb3","E-3","F-3","Gb3","G-3","Ab3","A-3","B-3","H-3",
  "C-4","Db4","D-4","Eb4","E-4","F-4","Gb4","G-4","Ab4","A-4","B-4","H-4",
  "C-5","Db5","D-5","Eb5","E-5","F-5","Gb5","G-5","Ab5","A-5","B-5","H-5",
  "C-6","???","???","???"
};

//highlight colours on active rows in patterns
//	X	X	:		C	#	1		I	I		v	V		!	S	S	
static char csel0[15] = { 6,6,6,6,6,6,6,6,6,6,6,6,6,6,6 };	//use for active lines
static char c = COLOR_SELECTED;
static char csel1[15] = { 6,c,c,c,6,6,6,6,6,6,6,6,6,6,6 };
static char csel2[15] = { 6,6,6,6,6,c,c,6,6,6,6,6,6,6,6 };
static char csel3[15] = { 6,6,6,6,6,6,6,6,c,c,6,6,6,6,6 };
static char csel4[15] = { 6,6,6,6,6,6,6,6,6,6,6,c,c,c,6 };
static char* colac[] =
{
	csel1,csel2,csel3,csel4
};

static char cselprove0[15] = { 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13 };	//use for active lines
static char p = COLOR_SELECTED_PROVE;
static char cselprove1[15] = { 13,p,p,p,13,13,13,13,13,13,13,13,13,13,13 };
static char cselprove2[15] = { 13,13,13,13,13,p,p,13,13,13,13,13,13,13,13 };
static char cselprove3[15] = { 13,13,13,13,13,13,13,13,p,p,13,13,13,13,13 };
static char cselprove4[15] = { 13,13,13,13,13,13,13,13,13,13,13,p,p,p,13 };
static char* colacprove[] =
{
	cselprove1,cselprove2,cselprove3,cselprove4
};

//----------------------------------------------

struct Tshpar
{
	int c;
	int x,y;
	char *name;
	int pand;
	int pmax;
	int pfrom;
	int gup,gdw,gle,gri;
};

#define INSTRS_X 2*8
#define INSTRS_Y 8*16+8
#define INSTRS_PX	INSTRS_X			//parameter X
#define INSTRS_PY	INSTRS_Y+2*16		//parametry Y
#define INSTRS_EX	INSTRS_X+32*8		//envelope X  (29)
#define INSTRS_EY	INSTRS_Y+2*16		//envelope Y
#define INSTRS_TX	INSTRS_X+0*8		//table X	(16)(37)
#define INSTRS_TY	INSTRS_Y+18*16-8	//table Y
#define INSTRS_HX INSTRS_X				//active help X
#define INSTRS_HY INSTRS_Y+21*16		//active help Y
#define NUMBEROFPARS	20

const Tshpar shpar[NUMBEROFPARS]=
{
	//TABLE: LEN GO SPD TYPE MODE
	{ 0,INSTRS_PX+16*8,INSTRS_PY+ 9*16,"LENGTH:", 0x1f, 0x1f, 1, 8, 1,15,15 },
	{ 1,INSTRS_PX+18*8,INSTRS_PY+10*16,  "GOTO:", 0x1f, 0x1f, 0, 0, 2,16,16 },
	{ 2,INSTRS_PX+17*8,INSTRS_PY+11*16, "SPEED:", 0x3f, 0x3f, 1, 1, 3,17,17 },
	{ 3,INSTRS_PX+18*8,INSTRS_PY+12*16,  "TYPE:", 0x01, 0x01, 0, 2, 4,18,18 },
	{ 4,INSTRS_PX+18*8,INSTRS_PY+13*16,  "MODE:", 0x01, 0x01, 0, 3, 5,19,19 },
	//ENVELOPE: LEN GO VSLIDE VMIN
	{ 5,INSTRS_PX+16*8,INSTRS_PY+2*16, "LENGTH:"  ,0x3f, 0x2f, 1, 4, 6, 9, 9 },
	{ 6,INSTRS_PX+18*8,INSTRS_PY+3*16,   "GOTO:"   ,0x3f, 0x2f, 0, 5, 7,10,10 },
	{ 7,INSTRS_PX+15*8,INSTRS_PY+4*16,"FADEOUT:",0xff, 0xff, 0, 6, 8,11,11 },
	{ 8,INSTRS_PX+15*8,INSTRS_PY+5*16,"VOL MIN:"  ,0x0f, 0x0f, 0, 7, 0,11,11 },
	//EFFECT: DELAY VIBRATO FSHIFT
	{ 9,INSTRS_PX+ 3*8,INSTRS_PY+ 2*16,     "DELAY:",  0xff,0xff, 0,19,10, 5, 5 },
	{10,INSTRS_PX+ 1*8,INSTRS_PY+ 3*16,   "VIBRATO:",0x03,0x03, 0, 9,11, 6, 6 },
	{11,INSTRS_PX+ -1*8,INSTRS_PY+ 4*16,"FREQSHIFT:", 0xff,0xff, 0,10,12, 7, 7 },
	//AUDCTL: 00-07
	{12,INSTRS_PX+ 3*8,INSTRS_PY+ 6*16,   "15KHZ:",0x01,0x01,0,11,13, 0, 0 },
	{13,INSTRS_PX+ 1*8,INSTRS_PY+ 7*16, "HPF 2+4:",0x01,0x01,0,12,14, 0, 0 },
	{14,INSTRS_PX+ 1*8,INSTRS_PY+ 8*16, "HPF 1+3:",0x01,0x01,0,13,15, 0, 0 },
	{15,INSTRS_PX+ 0*8,INSTRS_PY+ 9*16,"JOIN 3+4:",0x01,0x01,0,14,16, 0, 0 },
	{16,INSTRS_PX+ 0*8,INSTRS_PY+10*16,"JOIN 1+2:",0x01,0x01,0,15,17, 1, 1 },
	{17,INSTRS_PX+ 0*8,INSTRS_PY+11*16,"1.79 CH3:",0x01,0x01,0,16,18, 2, 2 },
	{18,INSTRS_PX+ 0*8,INSTRS_PY+12*16,"1.79 CH1:",0x01,0x01,0,17,19, 3, 3 },
	{19,INSTRS_PX+ 3*8,INSTRS_PY+13*16,   "POLY9:",0x01,0x01,0,18, 9, 4, 4 }
};

#define PAR_TABLEN		0
#define PAR_TABGO		1
#define PAR_TABSPD		2
#define PAR_TABTYPE		3
#define PAR_TABMODE		4

#define PAR_ENVLEN		5
#define PAR_ENVGO		6
#define PAR_VSLIDE		7
#define PAR_VMIN		8
#define PAR_DELAY		9
#define PAR_VIBRATO		10
#define PAR_FSHIFT		11

#define PAR_AUDCTL0		12
#define PAR_AUDCTL1		13
#define PAR_AUDCTL2		14
#define PAR_AUDCTL3		15
#define PAR_AUDCTL4		16
#define PAR_AUDCTL5		17
#define PAR_AUDCTL6		18
#define PAR_AUDCTL7		19

struct Tshenv
{
	char ch;
	int pand;
	int padd;
	int psub;
	char *name;
	int xpos;
	int ypos;
};

const Tshenv shenv[ENVROWS]=
{
	//ENVELOPE
	{   0,0x0f,1,-1,   "VOLUME R:",INSTRS_EX+2*8,INSTRS_EY+2*16 },	//volume right
	{   0,0x0f,1,-1,   "VOLUME L:",INSTRS_EX+2*8,INSTRS_EY+8*16 },	//volume left
	{   0,0x0e,2,-2, "DISTORTION:",INSTRS_EX+0*8,INSTRS_EY+9*16 },	//distortion 0,2,4,6,...
	{   0,0x07,1,-1,    "COMMAND:",INSTRS_EX+3*8,INSTRS_EY+10*16 },	//command 0-7
	{   0,0x0f,1,-1,         "X/:",INSTRS_EX+8*8,INSTRS_EY+11*16 },	//X
	{   0,0x0f,1,-1,        "Y\\:",INSTRS_EX+8*8,INSTRS_EY+12*16 },	//Y
	{   9,0x01,1,-1, "AUTOFILTER:",INSTRS_EX+0*8,INSTRS_EY+13*16 },	//filter *
	{   9,0x01,1,-1, "PORTAMENTO:",INSTRS_EX+0*8,INSTRS_EY+14*16 }	//portamento *
};

#define	ENV_VOLUMER		0
#define	ENV_VOLUMEL		1
#define	ENV_DISTORTION	2
#define ENV_COMMAND		3
#define	ENV_X			4
#define	ENV_Y			5
#define	ENV_FILTER		6
#define	ENV_PORTAMENTO	7

CInstruments::CInstruments()
{
	InitInstruments();
}

BOOL CInstruments::InitInstruments()
{
	for(int i=0; i<INSTRSNUM; i++)
	{
		ClearInstrument(i);
	}
	return 1;
}

BOOL CInstruments::ClearInstrument(int it)
{
	Atari_InstrumentTurnOff(it); //turns off this instrument on all channels

	int i,j;
	char *s = m_instr[it].name; 
	memset(s,' ',INSTRNAMEMAXLEN);
	sprintf(s,"Instrument %02X",it);
	s[strlen(s)] = ' '; //overrides 0x00 after the end of the text
	s[INSTRNAMEMAXLEN]=0;

	//m_instr[it].act=0;				//active name
	m_instr[it].act = 2;				//active envelope, so testing instruments wouldn't cause accidental rename
	m_instr[it].activenam=0;			//0 character name
	m_instr[it].activepar=PAR_ENVLEN;	//default is ENVELOPE LEN
	m_instr[it].activeenvx=0;
	m_instr[it].activeenvy=1;			//volume left
	m_instr[it].activetab=0;			//0 element in the table

	m_instr[it].octave=0;
	m_instr[it].volume=MAXVOLUME;
	
	for(i=0; i<PARCOUNT; i++) m_instr[it].par[i]=0;
	for(i=0; i<ENVCOLS; i++)
	{
		for(j=0; j<ENVROWS; j++) m_instr[it].env[i][j]=0; //rand()&0x0f;			//0;
	}
	for(i=0; i<TABLEN; i++) m_instr[it].tab[i]=0;

	m_iflag[it] = 0;	//init instrument flag

	ModificationInstrument(it);			//apply to Atari Mem

	return 1;
}

/*
void CInstruments::RandomInstrument(int it)
{
	ClearInstrument(it);
	unsigned int rid = (unsigned)time( NULL );
	srand( rid );
	CString s;
	s.Format("Random instrument %u",rid);	//jmeno
	strncpy(m_instr[it].name,(LPCTSTR)s,s.GetLength());
	int len=rand()%ENVCOLS;
	int go=rand()%(len+1);
	int i,j;
	for(i=0; i<=len; i++)
	{
		for(j=0; j<ENVROWS-2; j++)
		{
			int ran = rand() & 0x0f & shenv[j].pand;
			if (j==ENV_COMMAND) ran = 1+ (rand() % 2);	//command 1-2
			m_instr[it].env[i][j] = ran;
		}
	}
	m_instr[it].par[PAR_ENVLEN]=len;
	m_instr[it].par[PAR_ENVGO]=go;

	ModificationInstrument(it);			//promitne do Atari mem
}
*/

int CInstruments::SaveAll(ofstream& ou,int iotype)
{
	for(int i=0; i<INSTRSNUM; i++)
	{
		if (iotype==IOINSTR_TXT && !CalculateNoEmpty(i)) continue; //to TXT only non-empty instruments
		SaveInstrument(i,ou,iotype);	//,IOINSTR_RMW);
	}
	return 1;
}

int CInstruments::LoadAll(ifstream& in,int iotype)
{
	for(int i=0; i<INSTRSNUM; i++)
	{
		LoadInstrument(i,in,iotype);	//IOINSTR_RMW);
	}

	return 1;
}

int CInstruments::SaveInstrument(int instr, ofstream& ou, int iotype)
{
	TInstrument& ai = m_instr[instr];

	int j, k;

	switch (iotype)
	{
	case IOINSTR_RTI:
		{
			//RTI file
			static char head[4] = "RTI";
			head[3] = 1;			//type 1
			ou.write(head, 4);	//4 bytes header RTI1 (binary 1)
			ou.write((const char*)ai.name, sizeof(ai.name)); //name 32 byte + 33 is a binary zero terminating string
			unsigned char ibf[MAXATAINSTRLEN];
			BYTE len = InstrToAta(instr, ibf, MAXATAINSTRLEN);
			ou.write((char*)&len, sizeof(len));				//instrument length in Atari bytes
			if (len > 0) ou.write((const char*)&ibf, len);	//instrument data
		}
		break;

	case IOINSTR_RMW:
		//instrument name
		ou.write((char*)ai.name, sizeof(ai.name));

		char bfpar[PARCOUNT], bfenv[ENVCOLS][ENVROWS], bftab[TABLEN];
		//
		for (j = 0; j < PARCOUNT; j++) bfpar[j] = ai.par[j];
		ou.write(bfpar, sizeof(bfpar));
		//
		for (k = 0; k < ENVROWS; k++)
		{
			for (j = 0; j < ENVCOLS; j++)
				bfenv[j][k] = ai.env[j][k];
		}
		ou.write((char*)bfenv, sizeof(bfenv));
		//
		for (j = 0; j < TABLEN; j++) bftab[j] = ai.tab[j];
		ou.write(bftab, sizeof(bftab));
		//
		//+editing options:
		ou.write((char*)&ai.act, sizeof(ai.act));
		ou.write((char*)&ai.activenam, sizeof(ai.activenam));
		ou.write((char*)&ai.activepar, sizeof(ai.activepar));
		ou.write((char*)&ai.activeenvx, sizeof(ai.activeenvx));
		ou.write((char*)&ai.activeenvy, sizeof(ai.activeenvy));
		ou.write((char*)&ai.activetab, sizeof(ai.activetab));
		//octaves and volumes
		ou.write((char*)&ai.octave, sizeof(ai.octave));
		ou.write((char*)&ai.volume, sizeof(ai.volume));
		break;

	case IOINSTR_TXT:
		//TXT file
		CString s, nambf;
		nambf = ai.name;
		nambf.TrimRight();
		s.Format("[INSTRUMENT]\n%02X: %s\n", instr, (LPCTSTR)nambf);
		ou << (LPCTSTR)s;
		//instrument parameters
		for (j = 0; j < NUMBEROFPARS; j++)
		{
			s.Format("%s %X\n", shpar[j].name, ai.par[j] + shpar[j].pfrom);
			ou << (LPCTSTR)s;
		}
		//table
		ou << "TABLE: ";
		for (j = 0; j <= ai.par[PAR_TABLEN]; j++)
		{
			s.Format("%02X ", ai.tab[j]);
			ou << (LPCTSTR)s;
		}
		ou << endl;
		//envelope
		for (k = 0; k < ENVROWS; k++)
		{
			char bf[ENVCOLS + 1];
			for (j = 0; j <= ai.par[PAR_ENVLEN]; j++)
			{
				bf[j] = CharL4(ai.env[j][k]);
			}
			bf[ai.par[PAR_ENVLEN] + 1] = 0; //buffer termination
			s.Format("%s %s\n", shenv[k].name, bf);
			ou << (LPCTSTR)s;
		}
		ou << "\n"; //gap
		break;
	}
	return 1;
}

int CInstruments::LoadInstrument(int instr, ifstream& in, int iotype)
{
	switch(iotype)
	{
	case IOINSTR_RTI:
	{
		//RTI
		if (instr < 0 || instr >= INSTRSNUM) return 0;
		ClearInstrument(instr);	//it will first delete it before it reads
		TInstrument& ai = m_instr[instr];
		char head[4];
		in.read(head, 4);	//4 bytes header
		if (strncmp(head, "RTI", 3) != 0) return 0;		//if there is no RTI header
		int version = head[3];
		if (version >= 2) return 0;					//it's version 2 and more (only 0 and 1 are supported)

		in.read((char*)ai.name, sizeof(ai.name));	//name 32 bytes + 33rd byte terminating zero

		BYTE len;
		in.read((char*)&len, sizeof(len));			//instrument length in Atari bytes
		if (len > 0)
		{
			unsigned char ibf[MAXATAINSTRLEN];
			in.read((char*)&ibf, len);
			BOOL r;
			if (version == 0)
				r = AtaV0ToInstr(ibf, instr);
			else
				r = AtaToInstr(ibf, instr);
			ModificationInstrument(instr);	//writes to Atari ram
			if (!r) return 0; //if there was some problem with the instrument, return 0
		}
	}
		break;

	case IOINSTR_RMW:
		{
		//RMW
		if (instr<0 || instr>=INSTRSNUM) return 0;
		ClearInstrument(instr);	//it will first delete it before it reads
		TInstrument& ai=m_instr[instr];
		//instrument name
		in.read((char*)ai.name, sizeof(ai.name));

		char bfpar[PARCOUNT],bfenv[ENVCOLS][ENVROWS],bftab[TABLEN];
		int j,k;
		//
		in.read(bfpar, sizeof(bfpar));
		for (j=0; j<PARCOUNT; j++) ai.par[j] = bfpar[j];
		//
		in.read((char*)bfenv, sizeof(bfenv));
		for (j=0; j<ENVCOLS; j++)
		{
			for(k=0; k<ENVROWS; k++)
				ai.env[j][k] = bfenv[j][k];
		}
		//
		in.read((char*)bftab, sizeof(bftab));
		for(j=0; j<TABLEN; j++) ai.tab[j] = bftab[j];
		//
		ModificationInstrument(instr);	//writes to Atari mem
		//
		//+editing options:
		in.read((char*)&ai.act,sizeof(ai.act));
		in.read((char*)&ai.activenam,sizeof(ai.activenam));
		in.read((char*)&ai.activepar,sizeof(ai.activepar));
		in.read((char*)&ai.activeenvx,sizeof(ai.activeenvx));
		in.read((char*)&ai.activeenvy,sizeof(ai.activeenvy));
		in.read((char*)&ai.activetab,sizeof(ai.activetab));
		//octaves and volumes
		in.read((char*)&ai.octave,sizeof(ai.octave));
		in.read((char*)&ai.volume,sizeof(ai.volume));
		}
		break;

	case IOINSTR_TXT:
		{
			char a;
			char b;
			char line[1025];
			in.getline(line,1024); //first row of the instrument
			int iins=Hexstr(line,2);

			if (instr==-1) instr=iins; //takes over the instrument number

			if (instr<0 || instr>=INSTRSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearInstrument(instr);	//it will first delete it before it reads
			TInstrument& ai=m_instr[instr];

			char* value = line+4;
			Trimstr(value);
			memset(ai.name,' ',INSTRNAMEMAXLEN);
			int lname=INSTRNAMEMAXLEN;
			if (strlen(value)<=INSTRNAMEMAXLEN) lname=strlen(value);
			strncpy(ai.name, value, lname);

			int v,j,k,vlen;
			while(!in.eof())
			{
				in.read((char*)&b,1);
				if (b=='[') goto InstrEnd;	//end of instrument (beginning of something else)
				line[0]=b;
				in.getline(line+1,1024);
				
				value=strstr(line,": ");
				if (value)
				{
					value[1]=0;	//close the gap by closing
					value+=2;	//the first character after the space
				}
				else
					continue;

				for(j=0; j<NUMBEROFPARS; j++)
				{
					if (strcmp(line,shpar[j].name)==0)
					{
						v = Hexstr(value,2) - shpar[j].pfrom;
						if (v<0) goto NextInstrLine;
						v &= shpar[j].pand;
						if (v>shpar[j].pmax) v=0;
						ai.par[shpar[j].c] = v;
						goto NextInstrLine;
					}
				}
				if (strcmp(line,"TABLE:")==0)
				{
					Trimstr(value);
					vlen=strlen(value);
					for(j=0; j<vlen; j+=3)
					{
						v = Hexstr(value+j,2);
						if (v<0) goto NextInstrLine;
						ai.tab[j/3]=v;
					}
					goto NextInstrLine;
				}

				for(j=0; j<ENVROWS; j++)
				{
					if (strcmp(line,shenv[j].name)==0)
					{
						for(k=0; (a=value[k]) && k<ENVCOLS; k++)
						{
							v = Hexstr(&a,1);
							if (v<0) goto NextInstrLine;
							v &= shenv[j].pand;
							ai.env[k][j] = v;
						}
						goto NextInstrLine;
					}
				}
NextInstrLine: {}
			}
		}
InstrEnd:
		ModificationInstrument(instr);	//write to Atari mem
		break;

	}

	return 1;
}


BOOL CInstruments::ModificationInstrument(int instr)
{
	unsigned char* ata = g_atarimem + instr*256 +0x4000;
	g_rmtroutine = 0;			//turn off RMT routines
	BYTE r = InstrToAta(instr,ata,MAXATAINSTRLEN);
	g_rmtroutine = 1;			//RMT routines are turned on
	RecalculateFlag(instr);
	return r;
}

void CInstruments::CheckInstrumentParameters(int instr)
{
	TInstrument& ai=m_instr[instr];
	//ENVELOPE len-go loop control
	if (ai.par[PAR_ENVGO]>ai.par[PAR_ENVLEN]) ai.par[PAR_ENVGO]=ai.par[PAR_ENVLEN];
	//TABLE len-go loop control
	if (ai.par[PAR_TABGO]>ai.par[PAR_TABLEN]) ai.par[PAR_TABGO]=ai.par[PAR_TABLEN];
	//check the cursor in the envelope
	if (ai.activeenvx>ai.par[PAR_ENVLEN]) ai.activeenvx=ai.par[PAR_ENVLEN];
	//check the cursor in the table
	if (ai.activetab>ai.par[PAR_TABLEN]) ai.activetab=ai.par[PAR_TABLEN];
	//something changed => Save instrument "to Atari"
}

BOOL CInstruments::RecalculateFlag(int instr)
{
	BYTE flag = 0;
	TInstrument& ti = m_instr[instr];
	int i;
	int envl = ti.par[PAR_ENVLEN];
	//filter?
	for(i=0; i<=envl; i++)
	{
		if (ti.env[i][ENV_FILTER]) { flag |= IF_FILTER; break; }
	}

	//bass16?
	for(i=0; i<=envl; i++)
	{
		//the filter takes priority over bass16, ie if the filter is enabled as well as bass16, bass16 does not become active
		if (ti.env[i][ENV_DISTORTION]==6 && !ti.env[i][ENV_FILTER]) { flag |= IF_BASS16; break; }
	}
	
	//portamento?
	for(i=0; i<=envl; i++)
	{
		if (ti.env[i][ENV_PORTAMENTO]) { flag |= IF_PORTAMENTO; break; }
	}
	//audctl?
	for(i=PAR_AUDCTL0; i<=PAR_AUDCTL7; i++)
	{
		if (ti.par[i]) { flag |= IF_AUDCTL; break; }
	}
	//
	m_iflag[instr] = flag;
	return 1;
}

BYTE CInstruments::InstrToAta(int instr,unsigned char *ata,int max)
{
	TInstrument& ai=m_instr[instr];
	int i,j;
	int* par = ai.par;

	/*
	  0 IDXTABLEEND
	  1 IDXTABLEGO
	  2 IDXENVEND
	  3 IDXENVGO
	  4 TABTYPEMODESPEED
	  5 AUDCTL
	  6 VSLIDE
	  7 VMIN
	  8 EFFDELAY
	  9 EFVIBRATO
	 10 FSHIFT
	 11 0 (unused)
	*/

	const int INSTRPAR = 12;			//12th byte starts the table

	int tablelast = par[PAR_TABLEN]+INSTRPAR; 
	ata[0] = tablelast;
	ata[1] = par[PAR_TABGO]+INSTRPAR;	
	ata[2] = par[PAR_ENVLEN]*3 +tablelast+1;	//behind the table is the envelope
	ata[3] = par[PAR_ENVGO]*3 +tablelast+1;
	//
	ata[4] = (par[PAR_TABTYPE]<<7)
		   | (par[PAR_TABMODE]<<6)
		   | (par[PAR_TABSPD]);
	//
	ata[5] = par[PAR_AUDCTL0]
		   | (par[PAR_AUDCTL1]<<1)
		   | (par[PAR_AUDCTL2]<<2)
		   | (par[PAR_AUDCTL3]<<3)
		   | (par[PAR_AUDCTL4]<<4)
		   | (par[PAR_AUDCTL5]<<5)
		   | (par[PAR_AUDCTL6]<<6)		   
		   | (par[PAR_AUDCTL7]<<7);
	ata[6] = par[PAR_VSLIDE];
	ata[7] = par[PAR_VMIN]<<4;
	ata[8] = par[PAR_DELAY];
	ata[9] = par[PAR_VIBRATO] & 0x03;
	ata[10]= par[PAR_FSHIFT];
	ata[11]= 0; //unused, for now

	//the entire table length gets the data copied
	for(i=0; i<=par[PAR_TABLEN]; i++) ata[INSTRPAR+i]=ai.tab[i];

	//envelope is behind the table
	BOOL stereo = (g_tracks4_8>4);
	int len= par[PAR_ENVLEN];
	for(i=0,j=tablelast+1; i<=len; i++,j+=3)
	{
		int* env = (int*) &ai.env[i];
		ata[j] = (stereo)?
			(env[ENV_VOLUMER]<<4) | (env[ENV_VOLUMEL])	//stereo
		:
			(env[ENV_VOLUMEL]<<4) | (env[ENV_VOLUMEL]); //mono, VOLUME R = VOLUME L

		ata[j+1]=(env[ENV_FILTER]<<7)
			   | (env[ENV_COMMAND]<<4)	//0-7
			   | (env[ENV_DISTORTION])	//0,2,4,6,8,A,C,E
			   | (env[ENV_PORTAMENTO]);
		ata[j+2]=(env[ENV_X]<<4)
			   | (env[ENV_Y]);
	}
	return tablelast + 1 + (len+1)*3;	//returns the data length of the instrument
}

BYTE CInstruments::InstrToAtaRMF(int instr,unsigned char *ata,int max)
{
	TInstrument& ai=m_instr[instr];
	int i,j;
	int* par = ai.par;

	/*							RMF
	  0 IDXTABLEEND				
	  1 IDXTABLEGO
	  2 IDXENVEND				+1
	  3 IDXENVGO
	  4 TABTYPEMODESPEED
	  5 AUDCTL
	  6 VSLIDE
	  7 VMIN
	  8 EFFDELAY
	  9 EFVIBRATO
	 10 FSHIFT
	 11 0 (unused)				omitted
	*/

	const int INSTRPAR = 11;			//RMF (default is 12)

	int tablelast = par[PAR_TABLEN]+INSTRPAR;	 //+12	//12th byte starts the table
	ata[0] = tablelast;
	ata[1] = par[PAR_TABGO]+INSTRPAR;				//12th byte starts the table
	ata[2] = par[PAR_ENVLEN]*3 +tablelast+1+1;	//behind the table is the envelope // RMF +1
	ata[3] = par[PAR_ENVGO]*3 +tablelast+1;
	//
	ata[4] = (par[PAR_TABTYPE]<<7)
		   | (par[PAR_TABMODE]<<6)
		   | (par[PAR_TABSPD]);
	//
	ata[5] = par[PAR_AUDCTL0]
		   | (par[PAR_AUDCTL1]<<1)
		   | (par[PAR_AUDCTL2]<<2)
		   | (par[PAR_AUDCTL3]<<3)
		   | (par[PAR_AUDCTL4]<<4)
		   | (par[PAR_AUDCTL5]<<5)
		   | (par[PAR_AUDCTL6]<<6)		   
		   | (par[PAR_AUDCTL7]<<7);
	ata[6] = par[PAR_VSLIDE];
	ata[7] = par[PAR_VMIN]<<4;
	ata[8] = par[PAR_DELAY];
	ata[9] = par[PAR_VIBRATO] & 0x03;
	ata[10]= par[PAR_FSHIFT];
	ata[11]= 0; //unused

	//write for the entire length of the table
	for(i=0; i<=par[PAR_TABLEN]; i++) ata[INSTRPAR+i]=ai.tab[i];

	//envelope is behind the table
	BOOL stereo = (g_tracks4_8>4);
	int len= par[PAR_ENVLEN];
	for(i=0,j=tablelast+1; i<=len; i++,j+=3)
	{
		int* env = (int*) &ai.env[i];
		ata[j] = (stereo)?
			(env[ENV_VOLUMER]<<4) | (env[ENV_VOLUMEL]) //stereo
		:
			(env[ENV_VOLUMEL]<<4) | (env[ENV_VOLUMEL]); //mono, VOLUME R = VOLUME L

		ata[j+1]=(env[ENV_FILTER]<<7)
			   | (env[ENV_COMMAND]<<4)	//0-7
			   | (env[ENV_DISTORTION])	//0,2,4,..14
			   | (env[ENV_PORTAMENTO]);
		ata[j+2]=(env[ENV_X]<<4)
			   | (env[ENV_Y]);
	}
	return tablelast + 1 + (len+1)*3;	//returns the data length of the instrument
}

BOOL CInstruments::AtaV0ToInstr(unsigned char *ata, int instr)
{
	//OLD INSTRUMENT VERSION
	TInstrument& ai=m_instr[instr];
	int i,j;
	//0-7 table
	for(i=0; i<=7; i++) ai.tab[i] = ata[i];
	//8 ;instr len  0-31 *8, table len  0-7  (iiii ittt)
	int* par = ai.par;
	int len = par[PAR_ENVLEN] = ata[8]>>3;
	par[PAR_TABLEN] = ata[8] & 0x07;
	par[PAR_ENVGO]  = ata[9]>>3;
	par[PAR_TABGO]  = ata[9] & 0x07;
	par[PAR_TABTYPE]= ata[10]>>7;
	par[PAR_TABMODE]= (ata[10]>>6) & 0x01;
	par[PAR_TABSPD]	= ata[10] & 0x3f;
	par[PAR_VSLIDE] = ata[11];
	par[PAR_VMIN]	= ata[12]>>4;
	//par[PAR_POLY9]	= (ata[12]>>1) & 0x01;
	//par[PAR_15KHZ]	= ata[12] & 0x01;
	par[PAR_AUDCTL0] = ata[12] & 0x01;
	par[PAR_AUDCTL1] = 0;
	par[PAR_AUDCTL2] = 0;
	par[PAR_AUDCTL3] = 0;
	par[PAR_AUDCTL4] = 0;
	par[PAR_AUDCTL5] = 0;
	par[PAR_AUDCTL6] = 0;
	par[PAR_AUDCTL7] = (ata[12]>>1) & 0x01;
	//
	par[PAR_DELAY]	= ata[13];
	par[PAR_VIBRATO]= ata[14] & 0x03;
	par[PAR_FSHIFT]	= ata[15];
	//
	BOOL stereo = (g_tracks4_8>4);
	for(i=0,j=16; i<=len; i++,j+=3)
	{
		int* env = (int*) &ai.env[i];
		env[ENV_VOLUMER] = (stereo)? (ata[j]>>4) : (ata[j] & 0x0f); //if mono, then VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER]  = ata[j+1]>>7;
		env[ENV_COMMAND] = (ata[j+1]>>4) & 0x07;
		env[ENV_DISTORTION] = ata[j+1] & 0x0e;	//even numbers 0,2,4, .., 14
		env[ENV_PORTAMENTO] = ata[j+1] & 0x01;
		env[ENV_X]		 = ata[j+2]>>4;
		env[ENV_Y]		 = ata[j+2] & 0x0f;
	}
	return 1;
}

BOOL CInstruments::AtaToInstr(unsigned char *ata, int instr)
{
	TInstrument& ai=m_instr[instr];
	int i,j;

	int tablen,tabgo,envlen,envgo;
	tablen = ata[0] -12;
	tabgo  = ata[1] -12;
	envlen = (ata[2] - (ata[0]+1))/3;
	envgo  = (ata[3] - (ata[0]+1))/3;

	//check the scope of the tables and envelope
	if (tablen>=TABLEN || tabgo>tablen || envlen>=ENVCOLS || envgo>envlen)
	{
		//tables exceeds boundary or table go exceeds tables or envelope length ...
		return 0; 
	}

	//only if the ranges are ok, they change the content of the instrument
	int* par = ai.par;
	par[PAR_TABLEN] = tablen;
	par[PAR_TABGO]  = tabgo;
	par[PAR_ENVLEN] = envlen;
	par[PAR_ENVGO]  = envgo;
	//
	par[PAR_TABTYPE]= ata[4]>>7;
	par[PAR_TABMODE]= (ata[4]>>6) & 0x01;
	par[PAR_TABSPD]	= ata[4] & 0x3f;
	//
	par[PAR_AUDCTL0] = ata[5] & 0x01;
	par[PAR_AUDCTL1] = (ata[5]>>1) & 0x01;
	par[PAR_AUDCTL2] = (ata[5]>>2) & 0x01;
	par[PAR_AUDCTL3] = (ata[5]>>3) & 0x01;
	par[PAR_AUDCTL4] = (ata[5]>>4) & 0x01;
	par[PAR_AUDCTL5] = (ata[5]>>5) & 0x01;
	par[PAR_AUDCTL6] = (ata[5]>>6) & 0x01;
	par[PAR_AUDCTL7] = (ata[5]>>7) & 0x01;
	//
	par[PAR_VSLIDE] = ata[6];
	par[PAR_VMIN]	= ata[7]>>4;
	par[PAR_DELAY]	= ata[8];
	par[PAR_VIBRATO]= ata[9] & 0x03;
	par[PAR_FSHIFT]	= ata[10];

	//0-31 table
	for(i=0; i<=par[PAR_TABLEN]; i++) ai.tab[i] = ata[12+i];

	//envelope
	BOOL stereo = (g_tracks4_8>4);
	for(i=0,j=ata[0]+1; i<=par[PAR_ENVLEN]; i++,j+=3)
	{
		int* env = (int*) &ai.env[i];
		env[ENV_VOLUMER] = (stereo)? (ata[j]>>4) : (ata[j] & 0x0f); //if mono, then VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER]  = ata[j+1]>>7;
		env[ENV_COMMAND] = (ata[j+1]>>4) & 0x07;
		env[ENV_DISTORTION] = ata[j+1] & 0x0e;	//even numbers 0,2,4,...E
		env[ENV_PORTAMENTO] = ata[j+1] & 0x01;
		env[ENV_X]		 = ata[j+2]>>4;
		env[ENV_Y]		 = ata[j+2] & 0x0f;
	}
	return 1;
}

BOOL CInstruments::CalculateNoEmpty(int instr)
{
	TInstrument& it = m_instr[instr];
	int i,j;
	int len=it.par[PAR_ENVLEN];
	for(i=0; i<=len; i++)
	{
		for (j=0; j<ENVROWS; j++) 
		{
			if (it.env[i][j]!=0) return 1;
		}
	}
	for(i=0; i<PARCOUNT; i++) 
	{
		if (it.par[i]!=0) return 1;
	}
	return 0; //is empty
}


BOOL CInstruments::DrawInstrument(int it)
{
	int i;
	char s[128];
	int sp = g_scaling_percentage;

	TInstrument& t = m_instr[it];

	sprintf(s,"INSTRUMENT %02X",it);
	TextXY(s, INSTRS_X, INSTRS_Y, TEXT_COLOR_WHITE);
	int size=(t.par[PAR_ENVLEN]+1)*3+(t.par[PAR_TABLEN]+1)+12;
	sprintf(s,"(SIZE %u BYTES)",size);
	TextMiniXY(s,INSTRS_X+14*8,INSTRS_Y+5);
	DrawName(it);

	TextMiniXY("EFFECT",INSTRS_PX,INSTRS_PY+1*16+8);
	TextMiniXY("AUDCTL",INSTRS_PX+0*8,INSTRS_PY+5*16+8);
	TextMiniXY("ENVELOPE",INSTRS_PX+15*8,INSTRS_PY+16+8);
	TextMiniXY("TABLE",INSTRS_PX+15*8,INSTRS_PY+8*16+8);
	//
	TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX + 11 * 8 - 1, INSTRS_EY + 3 * 16, TEXT_COLOR_GRAY);
	
	if (t.act==2)	//only when the cursor is on the envelope edit
	{
		sprintf(s,"POS %02X",t.activeenvx);
		TextXY(s, INSTRS_EX + 2 * 8, INSTRS_EY + 5 * 16, TEXT_COLOR_GRAY);
	}
	//
	for(i=1; i<ENVROWS; i++) //omits "VOLUME R:"
	{
		TextXY(shenv[i].name, shenv[i].xpos, shenv[i].ypos, TEXT_COLOR_WHITE);
	}

	if (g_tracks4_8>4)
	{
		TextXY(shenv[0].name, shenv[0].xpos, shenv[0].ypos, TEXT_COLOR_WHITE); //"VOLUME R:"
		TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX + 11 * 8 - 1, INSTRS_EY - 2 * 16, TEXT_COLOR_GRAY);
		g_mem_dc->MoveTo(((INSTRS_EX + 12 * 8 - 1) * sp) / 100, ((INSTRS_EY + 2 * 16 - 1) * sp) / 100);
		g_mem_dc->LineTo(((INSTRS_EX + 12 * 8 + ENVCOLS * 8) * sp) / 100, ((INSTRS_EY + 2 * 16 - 1) * sp) / 100);
	}

	for(i=0; i<NUMBEROFPARS; i++) DrawPar(i,it);
	
	//TABLE TYPE icon
	i = (t.par[PAR_TABTYPE]==0) ? 1 : 2;
	IconMiniXY(i,shpar[PAR_TABTYPE].x+8*8+2,shpar[PAR_TABTYPE].y+7);
	//inscription at the bottom of TABLE
	TextMiniXY((i==1)? "TABLE OF NOTES":"TABLE OF FREQS",INSTRS_TX,INSTRS_TY-8);

	//TABLE MODE icon
	i = (t.par[PAR_TABMODE]==0) ? 3 : 4;
	IconMiniXY(i,shpar[PAR_TABMODE].x+8*8+2,shpar[PAR_TABMODE].y+7);

	s[1]=0;

	//ENVELOPE
	int len = t.par[PAR_ENVLEN];	//par 13 is the length of the envelope
	for(i=0; i<=len; i++) DrawEnv(i,it);

	//ENVELOPE LOOP ARROWS
	int go = t.par[PAR_ENVGO];		//par 14 is the GO loop envelope
	if (go<len)
	{
		s[0]='\x07';	//Go from here
		TextXY(s, INSTRS_EX + 12 * 8 + len * 8, INSTRS_EY + 7 * 16, TEXT_COLOR_WHITE);
		s[0]='\x06';	//Go here

		int lengo = len-go;
		if (lengo>3) NumberMiniXY(lengo+1,INSTRS_EX+11*8+4+go*8+lengo*4,INSTRS_EY+7*16+4); //len-go number
	}
	else
		s[0]='\x16';	//GO from here to here
	TextXY(s, INSTRS_EX + 12 * 8 + go * 8, INSTRS_EY + 7 * 16, TEXT_COLOR_WHITE);
	if (go>2) NumberMiniXY(go,INSTRS_EX+11*8+go*4,INSTRS_EY+7*16+4); //GO number

	//TABLE
	len = t.par[PAR_TABLEN];	//length table
	for(i=0; i<=len; i++) DrawTab(i,it);

	//TABLE LOOP ARROWS
	go = t.par[PAR_TABGO];		//table GO loop
	if (len==0)
	{
		TextXY("\x18", INSTRS_TX + 4, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
	}
	else
	{
		TextXY("\x19", INSTRS_TX + go * 8 * 3, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
		TextXY("\x1a", INSTRS_TX + 8 + len * 8 * 3, INSTRS_TY + 8 + 16, TEXT_COLOR_WHITE);
	}

	//delimitation of space for Envelope VOLUME
	g_mem_dc->MoveTo(((INSTRS_EX + 12 * 8 - 1)*sp)/100, ((INSTRS_EY + 7 * 16 - 1)*sp)/100);
	g_mem_dc->LineTo(((INSTRS_EX + 12 * 8 + ENVCOLS * 8)*sp)/100, ((INSTRS_EY + 7 * 16 - 1)*sp)/100);

	//boundaries of all parts of the instrument
	/*
	CBrush br(RGB(112,112,112));
	g_mem_dc->FrameRect(CRect(INSTRS_X-2,INSTRS_Y-2,INSTRS_X+38*8+4,INSTRS_Y+2*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_PX-2,INSTRS_PY+16-2,INSTRS_PX+29*8,INSTRS_PY+15*16+4),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_EX,INSTRS_EY-2*16-2,INSTRS_EX+48*8,INSTRS_EY+15*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_TX-2,INSTRS_TY-2,INSTRS_TX+54*8+4,INSTRS_TY+2*16+8),&br);
	*/

	if (!g_viewinstractivehelp) return 1; //does not want help => end
	//want help => continue

//separating line
#define HORIZONTALLINE {	g_mem_dc->MoveTo(INSTRS_HX,INSTRS_HY-1); g_mem_dc->LineTo(INSTRS_HX+93*8,INSTRS_HY-1); }

	if (t.act == 0)	//is the cursor on the instrument name?
		is_editing_instr = 1;

	if (t.act==2)	//is the cursor on the envelope?
	{
		is_editing_instr = 0;
		switch(t.activeenvy)
		{
		case ENV_DISTORTION:
			{
			int d = t.env[t.activeenvx][ENV_DISTORTION];
			static char* distor_help[8] = {
				"Distortion 0, white noise. (AUDC $0v, Poly5+17/9)",
				"Distortion 2, square-ish tones. (AUDC $2v, Poly5)",
				"Distortion 4, no note table yet, Pure Table by default. (AUDC $4v, Poly4+5)",
				"16-Bit tones in valid channels, use command 6 to set the Distortion. (Distortion A by default)",
				"Distortion 8, white noise. (AUDC $8v, Poly17/9)",
				"Distortion A, pure tones. Special mode: CH1+CH3 1.79mhz + AUTOFILTER = Sawtooth (AUDC $Av)",
				"Distortion C, buzzy bass tones. (AUDC $Cv, Poly4)",
				"Distortion C, gritty bass tones. (AUDC $Cv, Poly4)" };
			char* hs = distor_help[(d>>1)&0x07];
			TextXY(hs, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
			//HORIZONTALLINE;
			}
			break;

		case ENV_COMMAND:
			{
			int c = t.env[t.activeenvx][ENV_COMMAND];
			static char* comm_help[8] = {
				"Play BASE_NOTE + $XY semitones.",
				"Play frequency $XY.",
				"Play BASE_NOTE + frequency $XY.",
				"Set BASE_NOTE += $XY semitones. Play BASE_NOTE.",
				"Set FSHIFT += frequency $XY. Play BASE_NOTE.",
				"Set portamento speed $X, step $Y. Play BASE_NOTE.",
				"Set FILTER_SHFRQ += $XY. $0Y = BASS16 Distortion. $FF/$01 = Sawtooth inversion (Distortion A).",
				"Set instrument AUDCTL. $FF = VOLUME ONLY mode. $FE/$FD = enable/disable Two-Tone Filter." };
			char* hs = comm_help[c & 0x07];
			TextXY(hs, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
			//HORIZONTALLINE;
			}
			break;

		case ENV_X:
		case ENV_Y:
			{
			char i = (t.env[t.activeenvx][ENV_X]<<4) | t.env[t.activeenvx][ENV_Y];
			sprintf(s,"XY: $%02X = %i = %+i",(unsigned char)i,(unsigned char)i,i);
			TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
			//HORIZONTALLINE;
			}
			break;
		}
	}
	else
	if (t.act==1)	//the cursor is on the main parameters
	{
		is_editing_instr = 0;
		switch(t.activepar)
		{
		case PAR_DELAY:
			{
			unsigned char i = (t.par[t.activepar]);
			if (i>0)
				sprintf(s,"$%02X = %i",i,i);
			else
				sprintf(s,"$00 = no effects.");
			TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
			//HORIZONTALLINE;
			}
			break;

		case PAR_VSLIDE:
			{
			unsigned char i = (t.par[t.activepar]);
			double f;
			if (i==0) f=0;
			else
			if (i==0xff) f=1;
			else
				f = (double)i/256 + 0.0005;
			sprintf(s,"$%02X = -%.3f / vbi",(unsigned char)i,f);
			TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
			//HORIZONTALLINE;
			}
			break;
		}
	}
	if (t.act==3)	//the cursor is on the table
	{
		is_editing_instr = 0;
		char i = (t.tab[t.activetab]);
		sprintf(s,"$%02X = %+i",(unsigned char)i,i);
		TextXY(s, INSTRS_HX, INSTRS_HY, TEXT_COLOR_GRAY);
		//HORIZONTALLINE;
	}
	return 1;
}

BOOL CInstruments::DrawName(int it)
{
	char* s=GetName(it);
	int n = -1, color = TEXT_COLOR_WHITE;

	if (g_activepart==PARTINSTRS && m_instr[it].act==0)  //is an active change of instrument name
	{
		n=m_instr[it].activenam;
		if (g_prove) color = TEXT_COLOR_BLUE;
		else color = TEXT_COLOR_RED;
		is_editing_instr = 1;
	}
	else color = TEXT_COLOR_LIGHT_GRAY;

	TextXY("NAME:", INSTRS_X, INSTRS_Y + 16, TEXT_COLOR_WHITE);
	TextXYSelN(s,n,INSTRS_X+6*8,INSTRS_Y+16,color);
	return 1;
}

BOOL CInstruments::DrawPar(int p,int it)
{
	int sp = g_scaling_percentage;
	char s[2];
	s[1]=0;
	char *txt=shpar[p].name;
	int x=shpar[p].x;
	int y=shpar[p].y;

	char a;
	int color = TEXT_COLOR_WHITE;

	for (int i = 0; a = (txt[i]); i++, x += 8)
	{
		//necessary! this is a custom textxy function for displaying instruments...
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, color, 8, 16, SRCCOPY);
	}

	//if the cursor is on the main parameters
	if (g_activepart==PARTINSTRS && m_instr[it].act==1 && m_instr[it].activepar==p)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	x+=8;
	int showpar = m_instr[it].par[p] + shpar[p].pfrom;	//some parameters are 0..x but 1..x + 1 is displayed

	if (shpar[p].pmax+shpar[p].pfrom>0x0f)
	{
		s[0]=CharH4(showpar);
		TextXY(s,x,y,color);
	}

	x+=8;
	s[0]=CharL4(showpar);
	TextXY(s,x,y,color);	

	return 1;
}

BOOL CInstruments::CursorGoto(int instr,CPoint point,int pzone)
{
	if (instr<0 || instr>=INSTRSNUM) return 0;
	TInstrument& tt=m_instr[instr];
	int x,y;

	is_editing_instr = 0;	//when it is not edited, it shouldn't allow playing notes

	switch(pzone) 
	{
	case 0:
		//envelope large table
		g_activepart=PARTINSTRS;
		tt.act=2;	//the envelope is active
		x=point.x/8;
		if (x>=0 && x<=tt.par[PAR_ENVLEN]) tt.activeenvx=x;
		y=point.y/16+1;
		if (y>=1 && y<ENVROWS) tt.activeenvy=y;
		return 1;
	case 1:
		//envelope line volume number of the right channel
		g_activepart=PARTINSTRS;
		tt.act=2;	//the envelope is active
		x=point.x/8;
		if (x>=0 && x<=tt.par[PAR_ENVLEN]) tt.activeenvx=x;
		tt.activeenvy=0;
		return 1;
	case 2:
		//TABLE
		g_activepart=PARTINSTRS;
		tt.act=3;	//the table is active
		x=(point.x+4)/(3*8);
		if (x>=0 && x<=tt.par[PAR_TABLEN]) tt.activetab=x;
		return 1;
	case 3:
		//INSTRUMENT NAME
		g_activepart=PARTINSTRS;
		tt.act=0;	//the name is active 
		is_editing_instr = 1;	//instrument name is being edited
		x=point.x/8-6;
		if (x>=0 && x<=INSTRNAMEMAXLEN) tt.activenam=x;
		if (x<0) tt.activenam=0;
		return 1;
	case 4:
		//INSTRUMENT PARAMETERS
	{
		x=point.x/8;
		y=point.y/16;
		if (x>11 && x<15) return 0; //middle empty part
		if (y<0 || y>12) return 0; //just in case
		const int xytopar[2][12]=
		{
			{ PAR_DELAY,PAR_VIBRATO,PAR_FSHIFT,-1,PAR_AUDCTL0,PAR_AUDCTL1,PAR_AUDCTL2,PAR_AUDCTL3,PAR_AUDCTL4,PAR_AUDCTL5,PAR_AUDCTL6,PAR_AUDCTL7 },
			{ PAR_ENVLEN,PAR_ENVGO,PAR_VSLIDE,PAR_VMIN,-1,-1,-1,PAR_TABLEN,PAR_TABGO,PAR_TABSPD,PAR_TABTYPE,PAR_TABMODE}
		};
		int p;
		p=xytopar[(x>11)][y];
		if (p>=0 && p<NUMBEROFPARS)
		{
			tt.activepar=p;
			g_activepart=PARTINSTRS;
			tt.act=1;	//parameters are active
			return 1;
		}
	}
		return 0;

	case 5:
		//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
		//left mouse button
		//changes GO and moves LEN if necessary
		x=point.x/8;
		if (x<0) x=0; else if (x>=ENVCOLS) x=ENVCOLS-1;
		tt.par[PAR_ENVGO]=x;
		if (tt.par[PAR_ENVLEN]<x) tt.par[PAR_ENVLEN]=x;
CG_InstrumentParametersChanged:
		//because there has been some change in the instrument parameter => this instrument will stop on all channels
		Atari_InstrumentTurnOff(instr);
		CheckInstrumentParameters(instr);
		//something changed => Save instrument "to Atari"
		ModificationInstrument(instr);
		return 1;

	case 6:
		//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
		//right mouse button
		//changes LEN and moves GO if necessary
		x=point.x/8;
		if (x<0) x=0; else if (x>=ENVCOLS) x=ENVCOLS-1;
		tt.par[PAR_ENVLEN]=x;
		if (tt.par[PAR_ENVGO]>x) tt.par[PAR_ENVGO]=x;
		goto CG_InstrumentParametersChanged;
	case 7:
		//TABLE SET LEN/GO PARAMETER by MOUSE
		//left mouse button
		//changes GO and moves LEN if necessary
		x=(point.x+4)/(3*8);
		if (x<0) x=0; else if (x>=TABLEN) x=TABLEN-1;
		tt.par[PAR_TABGO]=x;
		if (tt.par[PAR_TABLEN]<x) tt.par[PAR_TABLEN]=x;
		goto CG_InstrumentParametersChanged;
	case 8:
		//TABLE SET LEN/GO PARAMETER by MOUSE
		//right mouse button
		//changes LEN and moves GO if necessary
		x=(point.x+4)/(3*8);
		if (x<0) x=0; else if (x>=TABLEN) x=TABLEN-1;
		tt.par[PAR_TABLEN]=x;
		if (tt.par[PAR_TABGO]>x) tt.par[PAR_TABGO]=x;
		goto CG_InstrumentParametersChanged;
	}
	return 0;
}

void CInstruments::SetEnvVolume(int instr, BOOL right, int px, int py)
{
	int len = m_instr[instr].par[PAR_ENVLEN]+1;
	if (px<0 || px>=len) return;
	if (py<0 || py>15) return;
	int ep = (right && g_tracks4_8>4)? ENV_VOLUMER : ENV_VOLUMEL;
	m_instr[instr].env[px][ep] = py;
	ModificationInstrument(instr);
}

int CInstruments::GetFrequency(int instr,int note)
{
	if (instr<0 || instr>=INSTRSNUM || note<0 || note>=NOTESNUM) return -1;
	TInstrument& tt=m_instr[instr];
	if (tt.par[PAR_TABTYPE]==0 )  //only for TABTYPE NOTES
	{
		int nsh=tt.tab[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note<0 || note>=NOTESNUM) return -1;
	}
	int frq=-1;
	int dis=tt.env[0][ENV_DISTORTION];
	if (dis==0x0c) frq=g_atarimem[RMT_FRQTABLES+64+note];
	else
	if (dis==0x0e || dis==0x06) frq=g_atarimem[RMT_FRQTABLES+128+note];
	else
		frq=g_atarimem[RMT_FRQTABLES+192+note];
	return frq;
}

int CInstruments::GetNote(int instr,int note)
{
	if (instr<0 || instr>=INSTRSNUM || note<0 || note>=NOTESNUM) return -1;
	TInstrument& tt=m_instr[instr];
	if (tt.par[PAR_TABTYPE]==0 )  //only for TABTYPE NOTES
	{
		int nsh=tt.tab[0];	//shift notes according to table 0
		note = (note + nsh) & 0xff;
		if (note<0 || note>=NOTESNUM) return -1;
	}
	return note;
}

void CInstruments::MemorizeOctaveAndVolume(int instr,int oct,int vol)
{
	if (g_keyboard_rememberoctavesandvolumes)
	{
		if (oct>=0) m_instr[instr].octave=oct;
		if (vol>=0) m_instr[instr].volume=vol;
	}
}

void CInstruments::RememberOctaveAndVolume(int instr,int& oct,int& vol)
{
	if (g_keyboard_rememberoctavesandvolumes)
	{
		oct=m_instr[instr].octave;
		vol=m_instr[instr].volume;
	}
}

BOOL CInstruments::GetInstrArea(int instr, int zone, CRect& rect)
{
	int len = m_instr[instr].par[PAR_ENVLEN]+1;
	switch(zone) 
	{
	case 0:
		//left channel volume curve (lower)
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+4,INSTRS_EX+12*8+len*8,INSTRS_EY+3*16+4+4*16);
		return 1;
	case 1:
		//right channel volume curve (upper)
		if (g_tracks4_8<=4) return 0;
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY-2*16+4,INSTRS_EX+12*8+len*8,INSTRS_EY-2*16+4+4*16);
		return 1;
	case 2:
		//envelope area large table
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+0+5*16,INSTRS_EX+12*8+len*8,INSTRS_EY+3*16+0+5*16+7*16);
		return 1;
	case 3:
		//envelope area of volume numbers for right channel
		if (g_tracks4_8<=4) return 0;
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY-2*16+0+4*16,INSTRS_EX+12*8+len*8,INSTRS_EY-2*16+0+4*16+16);
		return 1;
	case 4:
		//instrument table line
		{
		int tabl = m_instr[instr].par[PAR_TABLEN]+1;
		rect.SetRect(INSTRS_TX,INSTRS_TY+8,INSTRS_TX+tabl*24-8,INSTRS_TY+8+16);
		return 1;
		}
	case 5:
		//instrument name
		rect.SetRect(INSTRS_PX,INSTRS_PY-16,INSTRS_PX+6*8+INSTRNAMEMAXLEN*8,INSTRS_PY+0);
		return 1;
	case 6:
		//instrument parameters
		rect.SetRect(INSTRS_PX,INSTRS_PY+32,INSTRS_PX+26*8,INSTRS_PY+32+12*16);
		return 1;
	case 7:
		//instrument number
		rect.SetRect(INSTRS_X,INSTRS_Y,INSTRS_X+13*8,INSTRS_Y+16);
		return 1;
	case 8:
		//envelope area under the left (lower) volume curve
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+0+4*16,INSTRS_EX+12*8+ENVCOLS*8,INSTRS_EY+3*16+0+4*16+16);
		return 1;
	case 9:
		//instrument table + 1 line below parameter table 
		rect.SetRect(INSTRS_TX,INSTRS_TY+8+1*16,INSTRS_TX+TABLEN*24-8,INSTRS_TY+8+2*16);
		return 1;
	}
	return 0;
}

void CInstruments::DrawEnv(int e, int it)
{
	int sp = g_scaling_percentage;
	TInstrument& in = m_instr[it];
	int volR= in.env[e][ENV_VOLUMER] & 0x0f; //volume right
	int volL= in.env[e][ENV_VOLUMEL] & 0x0f; //volume left/mono
	int color;
	int x=INSTRS_EX+12*8+e*8;
	char s[2],a;
	s[1]=0;
	int ay= (in.act==2 && in.activeenvx==e)? in.activeenvy : -1;

	//Volume Only mode uses Command 7 with $XY == $FF
	COLORREF fillColor = (in.env[e][ENV_COMMAND]==0x07 && in.env[e][ENV_X]==0x0f && in.env[e][ENV_Y]==0x0f) ?
		RGB(128,255,255) : RGB(255,255,255);

	//volume column
	if (volL) g_mem_dc->FillSolidRect((x * sp) / 100, ((INSTRS_EY + 3 * 16 + 4 + 4 * (15 - volL)) * sp) / 100, (8 * sp) / 100, ((volL * 4) * sp) / 100, fillColor);

	if (g_tracks4_8 > 4 && volR) g_mem_dc->FillSolidRect((x * sp) / 100, ((INSTRS_EY - 2 * 16 + 4 + 4 * (15 - volR)) * sp) / 100, (8 * sp) / 100, ((volR * 4) * sp) / 100, fillColor);

	for(int j=0; j<8; j++)
	{
		if ( (a=shenv[j].ch)!=0 )
		{
			if (m_instr[it].env[e][j])
				s[0]=a;
			else
				s[0]=8;	//character in the envelope
		}
		else
			s[0]=CharL4(m_instr[it].env[e][j]);

		if (j==ay && g_activepart==PARTINSTRS)
		{
			color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
		}
		else color = TEXT_COLOR_WHITE;

		if (j==0)
		{
			if (g_tracks4_8>4) TextXY(s,x,INSTRS_EY+2*16,color);		 //volume R is out of the box
		}
		else
			TextXY(s,x,INSTRS_EY+7*16+j*16,color);
	}
}

BOOL CInstruments::DrawTab(int p,int it)
{
	TInstrument& in = m_instr[it];
	char s[4];

	//small number
	s[0]=CharH4(p);
	s[1]=CharL4(p);
	s[2]=0;
	TextMiniXY(s,INSTRS_TX+p*24,INSTRS_TY);

	//table parameter
	sprintf(s,"%02X",in.tab[p]);

	int color = TEXT_COLOR_WHITE;

	if (in.act==3 && in.activetab==p && g_activepart==PARTINSTRS)
	{
		color = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;
	}

	TextXY(s, INSTRS_TX + p * 24, INSTRS_TY + 8, color);
	return 1;
}

//----------------------------------------------

CTracks::CTracks()
{
	m_maxtracklen = 64;			//default value
	g_cursoractview = m_maxtracklen/2;
	InitTracks();
}

BOOL CTracks::InitTracks()
{
	for(int i=0; i<TRACKSNUM; i++)
	{
		ClearTrack(i);
	}
	return 1;
}

BOOL CTracks::ClearTrack(int t)
{
	if (t<0 || t>=TRACKSNUM) return 0;

	m_track[t].len=m_maxtracklen;	//32+(rand()&0x1f);		//0;
	m_track[t].go=-1;	//-1+(rand()&0x01);			//-1;
	for (int i=0; i<TRACKLEN; i++)
	{
		m_track[t].note[i]=-1;	//(rand()&0xff)-192;		//-1;
		m_track[t].instr[i]=-1;	//rand()&0xff;	//-1;
		m_track[t].volume[i]=-1;//rand()&0x0f;	//-1;
		m_track[t].speed[i]=-1; //rand()&0xff;	//-1;
	}
	return 1;
}

BOOL CTracks::IsEmptyTrack(int track)
{
	if (track<0 || track>=TRACKSNUM) return 0;

	if (m_track[track].len!=m_maxtracklen) return 0;
	int* tvolumes = (int*)&m_track[track].volume;
	int* tspeeds = (int*)&m_track[track].speed;

	for(int i=0; i<m_maxtracklen; i++)
	{
		if ( *(tvolumes+i) >=0 || *(tspeeds+i)>=0) return 0;
	}
	return 1;
}

int CTracks::SaveTrack(int track,ofstream& ou,int iotype)
{
	if (track<0 || track>=TRACKSNUM) return 0;

	switch(iotype)
	{
	case IOTYPE_RMW:
		{
			int j;
			char bf[TRACKLEN];
			TTrack& at=m_track[track];
			ou.write((char*)&at.len, sizeof(at.len));
			ou.write((char*)&at.go, sizeof(at.go));
			//all
			for(j=0; j<m_maxtracklen; j++) bf[j]=at.note[j];
			ou.write(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) bf[j]=at.instr[j];
			ou.write(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) bf[j]=at.volume[j];
			ou.write(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) bf[j]=at.speed[j];
			ou.write(bf,m_maxtracklen);
		}
		break;
	
	case IOTYPE_TXT:
		{
			CString s;
			char bf[16];
			TTrack& at=m_track[track];
			s.Format("[TRACK]\n");
			ou << s;
			strcpy(bf,"--  ----\n");
			bf[0]=CharH4(track);
			bf[1]=CharL4(track);
			if (at.len>0)
			{
				bf[4]=CharH4(at.len&0xff);
				bf[5]=CharL4(at.len&0xff);
			}
			if (at.go>=0)
			{
				bf[6]=CharH4(at.go);
				bf[7]=CharL4(at.go);
			}
			ou << bf;
			for(int j=0; j<at.len; j++)
			{
				strcpy(bf,"--- -- -");
				int note = at.note[j];
				int instr= at.instr[j];
				int volume = at.volume[j];
				int speed = at.speed[j];
				if (note>=0 && note<NOTESNUM) strncpy(bf,notes[note],3);
				if (instr>=0 && instr<INSTRSNUM)
				{
					bf[4]=CharH4(instr);
					bf[5]=CharL4(instr);
				}
				if (volume>=0 && volume<=MAXVOLUME) bf[7]=CharL4(volume);
				if (speed>=0 && speed<=255)
				{
					bf[8]=CharH4(speed);
					bf[9]=CharL4(speed);
					bf[10]=0;
				}
				ou << bf << endl;
			}
			ou << "\n"; //gap
		}
		break;
	}
	return 1;
}

int CTracks::LoadTrack(int track,ifstream& in,int iotype)
{
	switch (iotype)
	{
	case IOTYPE_RMW:
		{
			if (track<0 || track>=TRACKSNUM) return 0;
			char bf[TRACKLEN];
			int j;
			ClearTrack(track);	//clear before filling with data
			TTrack& at=m_track[track];
			in.read((char*)&at.len, sizeof(at.len));
			in.read((char*)&at.go, sizeof(at.go));
			
			//everything
			in.read(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) at.note[j]=bf[j];
			in.read(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) at.instr[j]=bf[j];
			in.read(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) at.volume[j]=bf[j];
			in.read(bf,m_maxtracklen);
			for(j=0; j<m_maxtracklen; j++) at.speed[j]=bf[j];
		}
		break;
		
	case IOTYPE_TXT:
		{
			int a;
			char b;
			char line[1025];
			memset(line,0,16);
			in.getline(line,1024); //the first line of the track

			int ttr=Hexstr(line,2);
			int tlen=Hexstr(line+4,2);
			int tgo=Hexstr(line+6,2);

			if (track==-1) track = ttr;

			if (track<0 || track>=TRACKSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearTrack(track);	//clear before filling with data
			TTrack& at=m_track[track];

			if (tlen<=0 || tlen>m_maxtracklen) tlen=m_maxtracklen;
			at.len=tlen;
			if (tgo>=tlen) tgo=-1;
			at.go=tgo;

			int idx=0;
			while(!in.eof())
			{
				in.read((char*)&b,1);
				if (b=='[') return 1;	//end of track (beginning of something else)
				if (b==10 || b==13)		//right at the beginning of the line is EOL
				{
					NextSegment(in);
					return 1;
				}

				memset(line,0,16);		//clear memory first
				line[0]=b;
				in.getline(line+1,1024);

				int note=-1,instr=-1,volume=-1,speed=-1;

				if (b=='C') note=0;
				else
				if (b=='D') note=2;
				else
				if (b=='E') note=4;
				else
				if (b=='F') note=5;
				else
				if (b=='G') note=7;
				else
				if (b=='A') note=9;
				else
				if (b=='H') note=11;

				a=line[1];
				if (a=='#' && note>=0) note++;

				a=line[2];
				if (note>=0)
				{
					if (a>='1' && a<='6') 
						note+=(a-'1')*12;
					else
						note=-1;
				}
				instr=Hexstr(line+4,2);
				volume=Hexstr(line+7,1);
				speed=Hexstr(line+8,2);

				if (note>=0 && note<NOTESNUM) at.note[idx]=note;
				if (instr>=0 && instr<INSTRSNUM) at.instr[idx]=instr;
				if (volume>=0 && volume<=MAXVOLUME) at.volume[idx]=volume;
				if (speed>0 && speed<=255) at.speed[idx]=speed;

				if (at.note[idx]>=0 && at.instr[idx]<0) at.instr[idx]=0;	//if the note is without an instrument, then instrument 0 hits there
				if (at.instr[idx]>=0 && at.note[idx]<0) at.instr[idx]=-1;	//if the instrument is without a note, then it cancels it
				if (at.note[idx]>=0 && at.volume[idx]<0) at.volume[idx]=MAXVOLUME;	//if the note is non-volume, adds the maximum volume

				idx++;
				if (idx>=TRACKLEN)
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

int CTracks::SaveAll(ofstream& ou,int iotype)
{
	switch(iotype)
	{
	case IOTYPE_RMW:
		{
		ou.write((char*)&m_maxtracklen, sizeof(m_maxtracklen));
		for(int i=0; i<TRACKSNUM; i++)
			{
				SaveTrack(i,ou,iotype);
			}
		}
		break;
	
	case IOTYPE_TXT:
		{
			for(int i=0; i<TRACKSNUM; i++)
			{
				if (!CalculateNoEmpty(i)) continue;	//saves only non-empty tracks
				SaveTrack(i,ou,iotype);
			}
		}
		break;
	}
	return 1;
}


int CTracks::LoadAll(ifstream& in,int iotype)
{
	InitTracks();

	in.read((char*)&m_maxtracklen, sizeof(m_maxtracklen));
	g_cursoractview = m_maxtracklen/2;

	for(int i=0; i<TRACKSNUM; i++)
	{
		LoadTrack(i,in,iotype);
	}
	return 1;
}

int CTracks::TrackToAta(int track,unsigned char* dest,int max)
{

#define WRITEATIDX(value) { if (idx<max) { dest[idx]=value; idx++; } else return -1; }
#define WRITEPAUSE(pause)									\
{															\
	if (pause>=1 && pause<=3)								\
		{ WRITEATIDX(62 | (pause<<6)); }					\
	else													\
		{ WRITEATIDX(62); WRITEATIDX(pause); }				\
}

	int note,instr,volume,speed;
	int idx=0;
	int goidx=-1;
	TTrack& t=m_track[track];
	int pause=0;

	for(int i=0; i<t.len; i++)
	{
		note = t.note[i];
		instr = t.instr[i];
		volume = t.volume[i];
		speed = t.speed[i];

		if (volume>=0 || speed>=0 || t.go==i) //something will be there, write empty measures first
		{
			if (pause>0)
			{
				WRITEPAUSE(pause);
				pause=0;
			}
			
			if (t.go==i) goidx=idx;	//it will jump with a go loop

			//speed is ahead of the notes
			if (speed>=0)
			{
				//is speed
				WRITEATIDX(63);		//63 = speed change
				WRITEATIDX(speed & 0xff);
				pause=0;
			}
		}
		
		//what it will be
		if (note>=0 && instr>=0 && volume>=0)
		{
			//note,instr,vol
			WRITEATIDX( ((volume & 0x03)<<6)
					|  ((note & 0x3f))
					 );
			WRITEATIDX( ((instr & 0x3f)<<2)
					|  ((volume & 0x0c)>>2)
					 );
			pause=0;
		}
		else
		if (volume>=0)
		{
			//only volume
			WRITEATIDX( ((volume & 0x03)<<6)
					|	61		//61 = empty note (only the volume is set)
					 );
			WRITEATIDX( (volume & 0x0c)>>2 );	//without instrument
			pause=0;
		}
		else
			pause++;
	}
	//end of loop

	if (t.len<m_maxtracklen)	//the track is shorter than the maximum length
	{
		if (pause>0)	//is there any pause left before the end?
		{
			//write the remaining pause time 
			WRITEPAUSE(pause);
			pause=0;
		}
		
		if (t.go>=0 && goidx>=0)	//is there a go loop?
		{
			//write the go loop
			WRITEATIDX( 0x80 | 63 );	//go command
			WRITEATIDX( goidx );
		}
		else
		{
			//write the end
			WRITEATIDX( 255 );		//end
		}
	}
	else
	{	//the track is as long as the maximum length
		if (pause>0)
		{
			WRITEPAUSE(pause);	//write the remaining pause time
		}
	}
	return idx;
}

int CTracks::TrackToAtaRMF(int track,unsigned char* dest,int max)
{

#define WRITEATIDX(value) { if (idx<max) { dest[idx]=value; idx++; } else return -1; }
#define WRITEPAUSE(pause)									\
{															\
	if (pause>=1 && pause<=3)								\
		{ WRITEATIDX(62 | (pause<<6)); }					\
	else													\
		{ WRITEATIDX(62); WRITEATIDX(pause); }				\
}

	int note,instr,volume,speed;
	int idx=0;
	int goidx=-1;
	TTrack& t=m_track[track];
	int pause=0;

	for(int i=0; i<t.len; i++)
	{
		note = t.note[i];
		instr = t.instr[i];
		volume = t.volume[i];
		speed = t.speed[i];

		if (volume>=0 || speed>=0 || t.go==i) //something will be there, write empty measures first
		{
			if (pause>0)
			{
				WRITEPAUSE(pause);
				pause=0;
			}
			
			if (t.go==i) goidx=idx;	//it will jump with a go loop

			//speed is ahead of the notes
			if (speed>=0)
			{
				//is speed
				WRITEATIDX(63);		//63 = speed change
				WRITEATIDX(speed & 0xff);

				pause=0;
			}
		}
		
		//what it will be
		if (note>=0 && instr>=0 && volume>=0)
		{
			//note,instr,vol
			WRITEATIDX( ((volume & 0x03)<<6)
					|  ((note & 0x3f))
					 );
			WRITEATIDX( ((instr & 0x3f)<<2)
					|  ((volume & 0x0c)>>2)
					 );
			pause=0;
		}
		else
		if (volume>=0)
		{
			//only volume
			WRITEATIDX( ((volume & 0x03)<<6)
					|	61		//61 = empty note (only the volume is set)
					 );
			WRITEATIDX( (volume & 0x0c)>>2 );	//without instrument
			pause=0;
		}
		else
			pause++;
	}
	//end of loop

	if (t.len<m_maxtracklen)	//the track is shorter than the maximum length
	{
		if (t.go>=0 && goidx>=0)	//is there a go loop?
		{
			if (pause>0)	//is there still a pause before the end?
			{
				//write the remaining pause time
				WRITEPAUSE(pause);
				pause=0;
			}
			//write go loop
			WRITEATIDX( 0x80 | 63 );	//go command
			WRITEATIDX( goidx );
		}
		else
		{
			//take an endless pause
			WRITEATIDX( 255 );		//RMF endless pause
		}
	}
	else
	{	//the track is as long as the maximum length
		if (pause>0)
		{
			WRITEATIDX( 255 );		//RMF endless pause
		}
	}
	return idx;
}


BOOL CTracks::AtaToTrack(unsigned char* sour,int len,int track)
{
	TTrack& t = m_track[track];
	unsigned char b,c;
	int goidx=-1;

	if (len>=2)
	{
		//there is a go loop at the end of the track
		if (sour[len-2]==128+63) goidx=sour[len-1];	//store its index
	}

	int line=0;
	int i=0;
	while(i<len)
	{
		if (i==goidx) t.go = line;		//jump to goidx => set go to this line

		b = sour[i] & 0x3f;
		if (b>=0 && b<=60)	//note,instr,vol
		{
			t.note[line]=b;
			t.instr[line]= ((sour[i+1] & 0xfc)>>2);			//11111100
			t.volume[line]=((sour[i+1] & 0x03)<<2)			//00000011
						|  ((sour[i]   & 0xc0)>>6);			//11000000
			i+=2;
			line++;
			continue;
		}
		else
		if (b==61)	//vol only
		{
			t.volume[line]=((sour[i+1] & 0x03)<<2)			//00000011
						|  ((sour[i] & 0xc0)>>6);			//11000000
			i+=2;
			line++;
			continue;
		}
		else
		if (b==62)	//pause
		{
			c = sour[i] & 0xc0;		//maximum 2 bits
			if (c==0)
			{	//they are zero
				if (sour[i+1]==0) break;			//infinite pause => end
				line += sour[i+1];	//shift line
				i+=2;
			}
			else
			{	//they are non-zero
				line += (c>>6);		//the upper 2 bits directly specify a pause 1-3
				i++;
			}
			continue;
		}
		else
		if (b==63)	//speed or go loop or end
		{
			c = sour[i] & 0xc0;		//11000000
			if (c==0)				//the highest 2 bits are 0?  (00xxxxxx)
			{
				//speed
				t.speed[line]=sour[i+1];
				i+=2;
				//without line shift
				continue;
			}
			else
			if (c==0x80)			//highest bit = 1?   (10xxxxxx)
			{
				//go loop
				t.len = line; //that's the end here
				break;
			}
			else
			if (c==0xc0)			//no more than two bits = 1?  (11xxxxxx)
			{
				//end
				t.len = line;
				break;
			}
		}
	}
	return 1;
}

BOOL CTracks::DrawTrack(int col, int x, int y, int tr, int line_cnt, int aline, int cactview, int pline, BOOL isactive, int acu)
{
	//cactview = cursor active view line
	char s[16];
	int i,line,xline,n,color,len,last,go;

	len=last=0;
	TTrack *tt=NULL;
	strcpy(s, "  --  -----  ");
	if (tr>=0)
	{
		tt=&m_track[tr];

		s[2] = CharH4(tr);
		s[3] = CharL4(tr);
		s[4] = ':';

		len=last=tt->len;	//len a last
		go=tt->go;	//go
		if (IsEmptyTrack(tr))
		{
			strncpy(s+6, "EMPTY", 5);
		}
		else
		{
			//non-empty track
			if (len>=0)
			{
				s[6] = CharH4(len);
				s[7] = CharL4(len);
			}
			if (go>=0)
			{
				s[9] = CharH4(len);
				s[10] = CharL4(len);
				last=m_maxtracklen;
			}
		}
	}
	color = (GetChannelOnOff(col))? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY;
	TextXY(s,x+8,y,color);
	y+=32;

	for (i = 0; i < line_cnt; i++, y += 16)
	{
		line = cactview + i - 8;		//8 lines from above
		if (line<0 || line>=m_maxtracklen) continue;	//if line is below 0 or above maximal length, ignore it

		if (line>=last)
		{
			if (line == aline && isactive)
				color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
			else if (line == pline)
				color = TEXT_COLOR_YELLOW;
			else
				color = TEXT_COLOR_GRAY;
			
			if (line==last && len>0)
			{
				if (color != TEXT_COLOR_BLUE && color != TEXT_COLOR_RED) color = TEXT_COLOR_WHITE;	//The end is white unless the active colour is used
				TextXY("\x12\x12\x12\x12\x12\x13\x14\x15\x12\x12\x12\x12\x12", x + 1 * 8, y, color);
			}
			else
				//empty track line
				TextXY(" \x8\x8\x8 \x8\x8 \x8\x8 \x8\x8\x8", x, y, color);
			continue;
		}
		strcpy(s, " --- -- -- ---");
		if (tt)
		{
			if (line < len || go < 0)
				xline = line;
			else
				xline = ((line - len) % (len - go)) + go;

			if ((n = tt->note[xline]) >= 0)
			{
				//notes
				//TODO: optimise this to not have redundant data
				if (g_displayflatnotes && g_usegermannotation)
				{
					s[1] = notesgermanflat[n][0];		// B
					s[2] = notesgermanflat[n][1];		// -
					s[3] = notesgermanflat[n][2];		// 1
				}
				else
				if (g_displayflatnotes && !g_usegermannotation)
				{
					s[1] = notesflat[n][0];		// D
					s[2] = notesflat[n][1];		// b
					s[3] = notesflat[n][2];		// 1
				}
				else
				if (!g_displayflatnotes && g_usegermannotation)
				{
					s[1] = notesgerman[n][0];		// H
					s[2] = notesgerman[n][1];		// -
					s[3] = notesgerman[n][2];		// 1
				}
				else
				{
					s[1] = notes[n][0];		// C
					s[2] = notes[n][1];		// #
					s[3] = notes[n][2];		// 1
				}	
			}
			if ( (n=tt->instr[xline])>=0 )
			{
				//instrument
				s[5] = CharH4(n);
				s[6] = CharL4(n);
			}
			if ( (n=tt->volume[xline])>=0 )
			{
				//volume
				s[8] = 'v';
				s[9] = CharL4(n);
			}
			if ( (n=tt->speed[xline])>=0 )
			{
				//speed
				s[11] = 'F';			//Fxx is the Famitracker speed command, because one day, RMT *will* have speed commands support
				s[12] = CharH4(n);
				s[13] = CharL4(n);
			}
		}

		if (line==go)
		{
			if (line==len-1)
				s[0]='\x11';			//left-up-right arrow
			else
				s[0]='\x0f';			//up-right arrow
		}
		else
		if (line==len-1 && go>=0) s[0]='\x10'; //left-up arrow

		//colours
		if (line == aline && isactive)
			color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
		else if (line == pline)
			color = TEXT_COLOR_YELLOW;
		else if (line >= len)
			color = TEXT_COLOR_GRAY;
		else if ((line % g_tracklinehighlight) == 0)
			color = TEXT_COLOR_CYAN;
		else
			color = TEXT_COLOR_WHITE;

		if (g_activepart==PARTTRACKS && line<len && (line == aline && isactive))
		{
			if (g_prove) TextXYCol(s,x,y,colacprove[acu]);
			else TextXYCol(s,x,y,colac[acu]);
		}
		else
			TextXY(s,x,y,color);
	}
	return 1;
}

BOOL CTracks::DelNoteInstrVolSpeed(int noteinstrvolspeed,int track,int line)
{
	if (line>=m_track[track].len) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_NOTEINSTRVOLSPEED);
	m_undo->Separator();
	if (noteinstrvolspeed&1) m_track[track].note[line]=-1;
	if (noteinstrvolspeed&2) m_track[track].instr[line]=-1;
	if (noteinstrvolspeed&4) m_track[track].volume[line]=-1;
	if (noteinstrvolspeed&8) m_track[track].speed[line]=-1;
	return 1;
}

BOOL CTracks::SetNoteInstrVol(int note,int instr,int vol,int track,int line)
{
	if (note>=NOTESNUM || line>=m_track[track].len) return 0;
	if (note==-1)
	{
		instr=vol=-1;
	}
	m_undo->ChangeTrack(track,line,UETYPE_NOTEINSTRVOL);
	m_undo->Separator();
	m_track[track].note[line]=note;
	m_track[track].instr[line]=instr;
	if (g_respectvolume)
	{
		//overwrites volume only if it is empty or if it wants to cancel it (-1)
		if (m_track[track].volume[line]<0 || vol<0) 
				m_track[track].volume[line]=vol;
	}
	else
		m_track[track].volume[line]=vol;	//always
	return 1;	
}

BOOL CTracks::SetInstr(int instr,int track,int line)
{
	if (line>=m_track[track].len) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_NOTEINSTRVOL);
	m_track[track].instr[line]=instr;
	return 1;
}

BOOL CTracks::SetVol(int vol,int track,int line)
{
	if (line>=m_track[track].len) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_NOTEINSTRVOL);
	m_track[track].volume[line]=vol;
	return 1;
}

BOOL CTracks::SetSpeed(int speed,int track,int line)
{
	if (line>=m_track[track].len) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_SPEED);
	m_track[track].speed[line]=speed;
	return 1;
}

BOOL CTracks::SetEnd(int track, int line)
{
	if (track<0 || track>=TRACKSNUM) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_LENGO,1);
	m_track[track].len= (line>0 && m_track[track].len!=line)? line : m_maxtracklen;
	if (m_track[track].go>=m_track[track].len) m_track[track].go=-1;
	return 1;
}

int CTracks::GetLastLine(int track)
{
	return (track>=0 && track<TRACKSNUM)? m_track[track].len-1 : -1;	//m_maxtracklen-1; // originally there was only ...: -1
}

int CTracks::GetLength(int track)
{
	if (track<0 || track>=TRACKSNUM) return -1;
	if (m_track[track].go>=0) return m_maxtracklen;
	return m_track[track].len;
}

BOOL CTracks::SetGo(int track, int line)
{
	if (track<0 || track>=TRACKSNUM) return 0;
	if (line>=m_track[track].len) return 0;
	m_undo->ChangeTrack(track,line,UETYPE_LENGO,1);
	m_track[track].go= ( m_track[track].go == line)? -1: line;
	return 1;
}

int CTracks::GetGoLine(int track)
{
	return (track>=0)? m_track[track].go : -1;
}

BOOL CTracks::InsertLine(int track, int line)
{
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& t=m_track[track];
	if (t.len<0) return 0;
	for(int i=t.len-2; i>=line; i--)
	{
		t.note[i+1]=t.note[i];
		t.instr[i+1]=t.instr[i];
		t.volume[i+1]=t.volume[i];
		t.speed[i+1]=t.speed[i];
	}
	t.note[line]=t.instr[line]=t.volume[line]=t.speed[line]=-1;
	return 1;
}

BOOL CTracks::DeleteLine(int track, int line)
{
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& t=m_track[track];
	if (t.len<0) return 0;
	for(int i=line; i<t.len-1; i++)
	{
		t.note[i]=t.note[i+1];
		t.instr[i]=t.instr[i+1];
		t.volume[i]=t.volume[i+1];
		t.speed[i]=t.speed[i+1];
	}
	line = t.len-1;
	t.note[line]=t.instr[line]=t.volume[line]=t.speed[line]=-1;
	return 1;
}

BOOL CTracks::CalculateNoEmpty(int track)
{
	//check if the track is empty
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& t = m_track[track];
	if (t.len != m_maxtracklen)	//has a length other than the maximum
		return 1;				//not empty
	else
	{
		for(int i=0; i<t.len; i++)
		{
			if (	t.note[i]>=0			//any note, volume or speed?
				||	t.volume[i]>=0
				||	t.speed[i]>=0 )
			{
				return 1;	//not empty
			}
		}
	}
	return 0;	//is empty
}

BOOL CTracks::CompareTracks(int track1, int track2)
{
	if (track1==track2) return 1;
	if (track1<0 || track1>=TRACKSNUM || track2<0 || track2>=TRACKSNUM) return 0;
	TTrack& t1=m_track[track1];
	TTrack& t2=m_track[track2];
	if (t1.len!=t2.len || t1.go!=t2.go) return 0;
	for(int i=0; i<t1.len; i++)
	{
		if (	t1.note[i]!=t2.note[i]
			||	t1.instr[i]!=t2.instr[i]
			||	t1.volume[i]!=t2.volume[i]
			||	t1.speed[i]!=t2.speed[i]
			) return 0;		//found a difference => they are not the same
	}
	return 1;	//did not find a difference => they are the same
}

int CTracks::TrackOptimizeVol0(int track)
{
	//removes redundant volume 0 data
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& tr = m_track[track];
	int lastzline=-1;
	int kline=-1;	//candidate for deletion including note
	for(int i=0; i<tr.len; i++)
	{
		if (tr.volume[i]==0)
		{
			if (lastzline>=0)
			{
				if (kline>=0)	//any candidate to delete? (note + vol0 in the middle between zero volumes)
				{
					tr.note[kline]=tr.instr[kline]=tr.volume[kline]=-1;
				}
				if (tr.note[i]<0 && tr.instr[i]<0) 
					tr.volume[i]=-1;	//cancel this volume
				else
					kline=i;
			}
			else
			{
				//this is currently the last line with zero volume
				lastzline=i;
			}
		}
		else
		if (tr.volume[i]>0)
		{
			lastzline=kline=-1;
		}
	}
	return 1;
}

int CTracks::TrackBuildLoop(int track)
{
	if (track<0 || track>=TRACKSNUM) return 0;

	TTrack& tr = m_track[track];
	if (IsEmptyTrack(track)) return 0;	//empty track
	if (tr.go>=0) return 0;		//there is a loop
	if (tr.len!=m_maxtracklen) return 0;	//it is not full length => it cannot make a loop there

	int i,j,k,m;
	for(i=1; i<tr.len; i++)
	{
		for(j=0; j<i; j++)
		{
			for(k=0; i+k<tr.len; k++)
			{
				if (   tr.note[i+k]==tr.note[j+k]
					&& tr.instr[i+k]==tr.instr[j+k]
					&& tr.volume[i+k]==tr.volume[j+k]
					&& tr.speed[i+k]==tr.speed[j+k] )
				{
					continue;
				}
				break;
			}
			if (k>1 && i+k==tr.len)
			{
				//it managed to find a loop at least 2 bars long lasting until the end
				//check to see if it's not empty in that loop
				int p=0;
				for(m=0; i+m<tr.len; m++)
				{
					if (   tr.note[j+m]>=0
						|| tr.instr[j+m]>=0
						|| tr.volume[j+m]>=0
						|| tr.speed[j+m]>=0 )
					{
						p++;
						if (p>1) //yes, it found at least two nonzero lines inside the loop
						{
							tr.len=i;
							tr.go=j;
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
	if (track<0 || track>=TRACKSNUM) return 0;

	TTrack* tr = &m_track[track];
	if (IsEmptyTrack(track)) return 0;	//empty track
	int i = TrackExpandLoop(tr);
	return i;	//length of the expanded loop
}

int CTracks::TrackExpandLoop(TTrack* ttrack)
{
	if (!ttrack) return 0;
	TTrack& tr = *ttrack;
	if (tr.go<0) return 0;		//there is no loop

	int i,j,k;	
	for(i=0; tr.len+i<m_maxtracklen; i++)
	{
		j=tr.len+i;
		k=tr.go+i;
		tr.note[j]=tr.note[k];
		tr.instr[j]=tr.instr[k];
		tr.volume[j]=tr.volume[k];
		tr.speed[j]=tr.speed[k];
	}
	tr.len=m_maxtracklen;	//full length
	tr.go=-1;				//no loop

	return i;	//length of the expanded loop
}

void CTracks::GetTracksAll(TTracksAll *dest_ta)
{
	dest_ta->maxtracklength=m_maxtracklen;
	memcpy(dest_ta->tracks,&m_track,sizeof(m_track));
}

void CTracks::SetTracksAll(TTracksAll* src_ta)
{
	m_maxtracklen=src_ta->maxtracklength;
	memcpy(&m_track,src_ta->tracks,sizeof(m_track));
}

//----------------------------------------------

CSong* g_song;
void CALLBACK G_TimerRoutine(UINT, UINT, DWORD, DWORD, DWORD)
{
	g_song->TimerRoutine();
}


CSong::CSong()
{
	m_tracks.InitUndo(&m_undo);

	ClearSong(8);	//the default is 8 stereo tracks

	//initialise Timer
	m_timer=0;
	g_timerroutineprocessed=0;
	g_song = this;

	m_quantization_note=-1; //init
	m_quantization_instr=-1;
	m_quantization_vol=-1;

	//Timer 1/50s = 20ms, 1/60s ~ 16/17ms
	//Pal or NTSC
	ChangeTimer((g_ntsc)? 17 : 20);
}

CSong::~CSong()
{
	if (m_timer) { timeKillEvent(m_timer); m_timer=0; }
}

void CSong::ChangeTimer(int ms)
{
	if (m_timer) { timeKillEvent(m_timer); m_timer=0; }
	m_timer = timeSetEvent(ms, 0, G_TimerRoutine,(ULONG) (NULL), TIME_PERIODIC);
}

BOOL CSong::ClearSong(int numoftracks)
{
	g_tracks4_8 = numoftracks;	//track for 4/8 channels
	g_rmtroutine = 1;			//RMT routine execution enabled
	g_prove=0;
	g_respectvolume=0;
	g_rmtstripped_adr_module = 0x4000;	//default standard address for stripped RMT modules
	g_rmtstripped_sfx = 0;		//is not a standard sfx variety stripped RMT
	g_rmtstripped_gvf = 0;		//default does not use Feat GlobalVolumeFade
	g_rmtmsxtext = "";			//clear the text for MSX export
	g_expasmlabelprefix = "MUSIC";	//default label prefix for exporting simple ASM notation

	PlayPressedTonesInit();

	m_play=MPLAY_STOP;
	g_playtime=0;
	m_followplay=1;
	m_mainspeed=m_speed=m_speeda=16;
	m_instrspeed=1;
	
	g_activepart=g_active_ti=PARTTRACKS;	//tracks
	
	m_songplayline = m_songactiveline = 0;
	m_trackactiveline = m_trackplayline = 0;
	m_trackactivecol = m_trackactivecur = 0;
	m_activeinstr = 0;
	m_octave = 0;
	m_volume = MAXVOLUME;
	
	ClearBookmark();
	
	m_infoact=0;

	memset(m_songname,' ',SONGNAMEMAXLEN);
	strncpy(m_songname,"Noname song",11);
	m_songname[SONGNAMEMAXLEN]=0;

	m_songnamecur=0;
	
	m_filename = "";
	m_filetype = 0;	//none
	m_exporttype = 0; //none
	
	m_TracksOrderChange_songlinefrom = 0x00;
	m_TracksOrderChange_songlineto   = SONGLEN-1;

	//number of lines after inserting a note/space
	g_linesafter=1; //initial value
	CMainFrame *mf = ((CMainFrame*)AfxGetMainWnd());
	if (mf) mf->m_c_linesafter.SetCurSel(g_linesafter);

	for(int i=0; i<SONGLEN; i++)
	{
		for(int j=0; j<SONGTRACKS; j++)
		{
			m_song[i][j]=-1;	//TRACK --
		}
		m_songgo[i]=-1;		//is not GO
	}

	//empty clipboards
	g_trackcl.Init(this);
	g_trackcl.Empty();
	//
	m_instrclipboard.act = -1;			//according to -1 it knows that it is empty
	m_songgoclipboard = -2;				//according to -2 it knows that it is empty

	//delete all tracks and instruments
	m_tracks.InitTracks();
	m_instrs.InitInstruments();

	//Undo initialization
	m_undo.Init(this);

	//Changes in the module
	g_changes=0;

	return 1;
}

void CSong::MidiEvent(DWORD dwParam)
{
	unsigned char chn,cmd, pr1, pr2;
	unsigned char *mv = (unsigned char*)&dwParam;
	cmd =  mv[0] & 0xf0;
	chn =  mv[0] & 0x0f;
	pr1 =  mv[1];
	pr2 =  mv[2];
	if (cmd==0x80 && chn != 15 && chn != 9) { cmd=0x90; pr2=0; }	//key off, as long as the MIDI channel isn't 15 or 10 to avoid conflicts
	else
	if (cmd==0xf0)
	{
		if (mv[0]==0xff)
		{
			//System Reset
MIDISystemReset:
			Atari_InitRMTRoutine(); //reinit RMT routines
			for(int i=1; i<16; i++)	//from 1, because it is MULTITIMBRAL 2-16
			{
				g_midi_notech[i]=-1;	//last pressed keys on each channel
				g_midi_voluch[i]=0;		//volume
				g_midi_instch[i]=0;		//instrument numbers
			}
		}
		return; //END
	}

	if (chn>0 && chn<9)
	{
		//2-10 channels (chn = 1-9) are used for multitimbral L1-L4, R1-R4
		int atc=(chn-1)%g_tracks4_8;	//atari track 0-7 (resp. 0-3 in mono)
		int note,vol;
		if (cmd==0x90)
		{
			if (chn==9)
			{
				//channel 10 (chn=9) ...drums channel
				if (pr2>0) //"note on" any non-zero volume
				{
					// planned
					// make a record of all g_midi _.... ch [0 to g_tracks4_8] into the track and move the line one step lower
				}
			}
			else
			{
				//channel 2-9 (chn=1-8)
				note = pr1-36;
				if (note>=0 && note<NOTESNUM)
				{
					if (pr2!=0 || (pr2==0 && note==g_midi_notech[1+atc]))
					{
						vol=pr2/8;
NoteOFF:
						g_midi_notech[1+atc]=note;
						g_midi_voluch[1+atc]=vol;
						int ins=g_midi_instch[chn];
						SetPlayPressedTonesTNIV(atc,note,ins,vol);
					}
				}
			}
		}
		else
		if (cmd==0xc0)
		{
			if (pr1>=0 && pr1<INSTRSNUM)
			{
				g_midi_instch[1+atc]=pr1;
			}
		}
		else
		if (cmd==0xb0)
		{
			if (pr1==123)
			{
				//All notes OFF
				note = -1;
				vol = 0;
				goto NoteOFF;
			}
			else
			if (pr1==121)
			{
				//Reset All Controls
				goto MIDISystemReset;
			}
		}
		return; //END
	}

	if (!g_focus && !g_prove) return;	//when it has no focus and is not in prove mode, the MIDI input will be ignored, to avoid overwriting patterns accidentally

	//test input from my own MIDI controller. CH15 for most events input, and CH9 specifically for the drumpad buttons, used for certain shortcuts triggered with MIDI NOTE ON events
	if (chn == 15 || chn == 9)
	{
		//command buttons, while this would technically work from any MIDI channel, it is specifically mapped for CH15 in order to avoid conflicing code, as a temporary workaround
		if (cmd == 0xB0 && chn == 15)	//control change and key pressed
		{
			int o = (m_ch_offset) ? 2 : 0;
			switch (pr1)
			{
			case 1:		//Modulation wheel
				m_mod_wheel = (pr2 - 64) / 8;
				break;

			case 7:		//volume slider
				m_vol_slider = pr2 / 8;
				if (m_vol_slider == 0) m_vol_slider++;
				if (m_vol_slider > 15) m_vol_slider = 15;
				m_volume = m_vol_slider;
				SCREENUPDATE;
				break;

			case 115:	//LOOP key
				if (!pr2) break;	//no key press
				Play(MPLAY_TRACK, m_followplay, 0);
				break;

			case 116:	//STOP key
				if (!pr2) break;	//no key press
				Stop();
				break;

			case 117:	//PLAY key
				if (!pr2) break;	//no key press
				Play(MPLAY_SONG, m_followplay, 0);
				break;

			case 118:	//REC key
				if (!pr2) break;	//no key press
				//todo: call CRMTView Class functions directly instead of redundancy copypasta
				if (g_prove == 0) g_prove = 1;
				else if (g_prove == 3) g_prove = 0;			//disable the special MIDI test mode immediately
				else
				{
					if (g_prove == 1 && g_tracks4_8 > 4)	//PROVE 2 only works for 8 tracks
						g_prove = 2;
					else
						g_prove = 3;						//special mode exclusive to MIDI CH15
				}
				SCREENUPDATE;
				break;

			case 123:
				if (!pr2) break;	//no key press
				Stop();
				//Atari_InitRMTRoutine();
				goto MIDISystemReset;
				break;

			//SPECIAL MIDI CH15 MODE
				if (g_prove == 3)
				{
			case 71: //Knob C1, AUDF0/AUDF2 upper 4 bits
				//g_atarimem[0xD200] &= 0x0F;
				//g_atarimem[0xD200] |= pr2 << 4;
				g_atarimem[0x3178 + o] &= 0x0F;
				g_atarimem[0x3178 + o] |= pr2 << 4;
				
				//GetPokey()->MemToPokey(g_tracks4_8);
				SCREENUPDATE;
				break;

			case 72: //Knob C2, AUDF1/AUDF3 upper 4 bits
				//g_atarimem[0xD202] &= 0x0F;
				//g_atarimem[0xD202] |= pr2 << 4;
				g_atarimem[0x3179 + o] &= 0x0F;
				g_atarimem[0x3179 + o] |= pr2 << 4;
				SCREENUPDATE;
				break;

			case 73: //Knob C3, AUDC0/AUDC2 volume
				//g_atarimem[0xD201] &= 0xF0;
				//g_atarimem[0xD201] |= pr2;
				g_atarimem[0x3180 + o] &= 0xF0;
				g_atarimem[0x3180 + o] |= pr2;
				SCREENUPDATE;
				break;

			case 74: //Knob C4, AUDC1/AUDC3 volume
				//g_atarimem[0xD203] &= 0xF0;
				//g_atarimem[0xD203] |= pr2;
				g_atarimem[0x3181 + o] &= 0xF0;
				g_atarimem[0x3181 + o] |= pr2;
				SCREENUPDATE;
				break;

			case 75: //Knob C5, AUDF0/AUDF2 lower 4 bits
				//g_atarimem[0xD200] &= 0xF0;
				//g_atarimem[0xD200] |= pr2;
				g_atarimem[0x3178 + o] &= 0xF0;
				g_atarimem[0x3178 + o] |= pr2;
				SCREENUPDATE;
				break;

			case 76: //Knob C6, AUDF1/AUDF3 lower 4 bits
				//g_atarimem[0xD202] &= 0xF0;
				//g_atarimem[0xD202] |= pr2;
				g_atarimem[0x3179 + o] &= 0xF0;
				g_atarimem[0x3179 + o] |= pr2;
				SCREENUPDATE;
				break;

			case 77: //Knob C7, AUDC0/AUDC2 distortion
				//g_atarimem[0xD201] &= 0x0F;
				//g_atarimem[0xD201] |= (pr2 * 2) << 4;
				g_atarimem[0x3180 + o] &= 0x0F;
				g_atarimem[0x3180 + o] |= (pr2 * 2) << 4;
				SCREENUPDATE;
				break;

			case 78: //Knob C8, AUDC1/AUDC3 distortion
				//g_atarimem[0xD203] &= 0x0F;
				//g_atarimem[0xD203] |= (pr2 * 2) << 4;
				g_atarimem[0x3181 + o] &= 0x0F;
				g_atarimem[0x3181 + o] |= (pr2 * 2) << 4;
				SCREENUPDATE;
				break;
				}

			default:
				//do nothing
				break;
			}
			return;	//finished, everything else will be ignored, unless it's using a different MIDI channel
		}

		if (cmd == 0x90 && chn == 9 && g_prove == 3)
		{	//drumpads used to control the POKEY registers
			switch (pr1)
			{
			case 60:	//drumpad 1, toggle High Pass Filter in ch1+3
				if (!pr2) break;	//no key press
				g_atarimem[0x3C69] ^= 0x04;
				SCREENUPDATE;
				break;

			case 62:	//drumpad 2, toggle High Pass Filter in ch2+4
				if (!pr2) break;	//no key press
				g_atarimem[0x3C69] ^= 0x02;
				SCREENUPDATE;
				break;

			case 66:	//drumpad 3, toggle 1.79mHz mode in the respective channels
				if (!pr2) break;	//no key press
				g_atarimem[0x3C69] ^= (m_ch_offset) ? 0x20 : 0x40;
				SCREENUPDATE;
				break;

			case 70:	//drumpad 4, toggle Join 16-bit mode in the respective channels
				if (!pr2) break;	//no key press
				g_atarimem[0x3C69] ^= (m_ch_offset) ? 0x08 : 0x10;
				SCREENUPDATE;
				break;

			case 74:	//drumpad 5, select the POKEY channels 1 and 2 or 3 and 4
				if (!pr2) break;	//no key press
				if (m_ch_offset) m_ch_offset = 0;
				else  m_ch_offset = 1;
				break;

			case 69:	//drumpad 6, reset all AUDCTL and SKCTL bits
				if (!pr2) break;	//no key press
				g_atarimem[0x3CD3] = 0x03;	//SKCTL
				g_atarimem[0x3C69] = 0x00;	//AUDCTL
				SCREENUPDATE;
				break;

			case 75:	//drumpad 7, toggle Two-Tone filter
				if (!pr2) break;	//no key press
				if (g_atarimem[0x3CD3] == 0x03) g_atarimem[0x3CD3] = 0x8B;
				else g_atarimem[0x3CD3] = 0x03;
				SCREENUPDATE;
				break;

			case 73:	//drumpad 8, toggle 15kHz mode
				if (!pr2) break;	//no key press
				g_atarimem[0x3C69] ^= 0x01;
				SCREENUPDATE;
				break;

			default:
				//do nothing
				break;
			}
			return;
		}

		if (chn == 9) return;	//we do not want any of those MIDI events outside of the drumpads!!!

		//default notes input event, which is mostly copied from the CH0 code. This is a very terrible approach, and will eventually be replaced (see above)
		if (chn == 15 && g_prove != 3)
		{

			int atc = m_heldkeys % g_tracks4_8;	//atari track 0-7 (resp. 0-3 in mono)

			if (cmd == 0x80) //key off
			{
				//key off
				int note = pr1 - 36 + m_mod_wheel;		//from the 3rd octave + modulation wheel offset
				//if (g_midi_notech[atc] == note) //last key pressed on this midi channel
				//{
					m_heldkeys--;
					g_midi_voluch[atc] = 0;		//volume
					g_midi_notech[atc] = -1;
					g_midi_instch[atc] = m_activeinstr;		//instrument numbers
				//}
				if (m_heldkeys < 0) m_heldkeys = 0;
				return;
			}

			if (cmd == 0x90)
			{
				//key on
				int note = pr1 - 36 + m_mod_wheel;		//from the 3rd octave + modulation wheel offset
				int vol;

				//if (g_midi_notech[atc] == note) //last key pressed on this midi channel
					m_heldkeys++;

				if (pr2 == 0)
				{
					if (!g_midi_noteoff) return;	//note off is not recognized
					vol = 0;			//keyoff
				}
				else
					if (g_midi_tr)
					{
						vol = g_midi_volumeoffset + pr2 / 8;	//dynamics
						if (vol == 0) vol++;		//vol=1
						else
							if (vol > 15) vol = 15;

						if (m_volume != vol) g_screenupdate = 1;
						m_volume = vol;
					}
					else
					{
						vol = m_volume;
					}

				if (note >= 0 && note < NOTESNUM)		//only within this range
				{
					if (g_activepart != PARTTRACKS || g_prove || g_shiftkey || g_controlkey) goto Prove_midi_test;	//play notes but do not record them if the active screen is not TRACKS, or if any other PROVE combo is detected

					if (vol > 0)
					{
						//volume > 0 => write note
						//Quantization
						if (m_play && m_followplay && (m_speeda < (m_speed / 2)))
						{
							m_quantization_note = note;
							m_quantization_instr = m_activeinstr;
							m_quantization_vol = vol;
							g_midi_notech[atc] = note; //see below
							g_midi_voluch[atc] = vol;		//volume
							g_midi_instch[atc] = m_activeinstr;		//instrument numbers

						}	//end Q
						else
							if (TrackSetNoteInstrVol(note, m_activeinstr, vol))
							{
								BLOCKDESELECT;
								g_midi_notech[atc] = note; //last key pressed on this midi channel
								g_midi_voluch[atc] = vol;		//volume
								g_midi_instch[atc] = m_activeinstr;		//instrument numbers
								if (g_respectvolume)
								{
									int v = TrackGetVol();
									if (v >= 0 && v <= MAXVOLUME) vol = v;
								}
								goto NextLine_midi_test;
							}
					}
					else
					{
						//volume = 0 => noteOff => delete note and write only volume 0
						if (g_midi_notech[atc] == note) //is it really the last one pressed?
						{
							if (m_play && m_followplay && (m_speed < (m_speed / 2)))
							{
								m_quantization_note = -2;
							}
							else
								if (TrackSetNoteActualInstrVol(-1) && TrackSetVol(0))
									goto NextLine_midi_test;
						}
					}

					if (0) //inside jumps only through goto
					{
					NextLine_midi_test:
						if (!(m_play && m_followplay)) TrackDown(g_linesafter);	//scrolls only when there is no followplay
						g_screenupdate = 1;
					Prove_midi_test:
						//SetPlayPressedTonesTNIV(m_trackactivecol, note, m_activeinstr, vol);
						SetPlayPressedTonesTNIV(atc, note, m_activeinstr, vol);
						//	if ((g_prove == 2 || g_controlkey) && g_tracks4_8 > 4)
						//	{	//with control or in prove2 => stereo test
						//		SetPlayPressedTonesTNIV((m_trackactivecol + 4) & 0x07, note, m_activeinstr, vol);
						//	}
					}
				}
			}

		} ////
		else //notes (soon...)
		{
			int note = pr1 /* - 36 + m_mod_wheel */;			//direct MIDI note mapping, for easier tests, else the older comment applies -> //from the 3rd octave + modulation wheel offset
			int vol = m_volume;									//direct volume value taken from the one of active instrument in memory, controlled by the volume slider
			int track = 0;										//m_trackactivecol is the active channel to map, so for tests simply moving the cursor should do the trick

			char midi_audctl = 0x00;							//AUDCTL without any special effect, default 64khz clock
			char midi_audc = 0x00;								//AUDC, for the Distortion and Volume
			char midi_audf = 0x00;								//AUDF, for the frequency 

			if (note > 63) return;								//crossing the boundary of the older table, so let's ignore it for now

			if (cmd == 0xc0)
			{
				m_midi_distortion = (pr1 % 8) * 2;
				return;
			}

			//MIDI NOTE OFF events
			if (cmd == 0x80)
			{
				m_heldkeys--;
				if (m_heldkeys < 0) m_heldkeys = 0;				//if by any mean the count is desynced, force it to be 0
				track = (m_trackactivecol + m_heldkeys) % 4;	//offset to the previous channel
				for (int i = 0; i < 4; i++)
				{
					if (note == g_midi_notech[i])
					{
						track = i;	//if there is a match the correct channel will be used 
						g_midi_notech[track] = -1;						//note
						g_midi_voluch[track] = 0;						//volume
						g_midi_instch[track] = 0;						//instrument numbers
						break;
					}
				}
				SCREENUPDATE;
			}

			//MIDI NOTE ON events
			if (cmd == 0x90)
			{
				track = (m_trackactivecol + m_heldkeys) % 4;
				m_heldkeys++;
				for (int i = 0; i < 4; i++)
				{
					if (g_midi_notech[i] == -1)
					{
						track = i;	//if there is a match the first empty channel found will be used
						g_midi_notech[track] = note;					//note
						g_midi_voluch[track] = vol;						//volume
						g_midi_instch[track] = m_midi_distortion;		//instrument numbers to set the Distortion lol
						break;
					}
				}
				SCREENUPDATE;
			}

			//TESTING HARDCODED DATA, THIS MUST NOT BE THE WAY TO GO!
			//COMMENT THIS ENTIRE BLOCK OUT ONCE A PROPER INPUT HANDLER IS ADDED TO TAKE ALL THE PARAMETERS INTO ACCOUNT
			//

			//midi_audf = g_atarimem[0xB100 + note];		//Distortion A 64khz frequency directly loaded from the generated table in memory
			midi_audc |= g_midi_instch[track] << 4;			//force Distortion based on instrument to AUDC
			midi_audctl = g_atarimem[0x3C69];

			bool CLOCK_15 = midi_audctl & 0x01;
			bool HPF_CH24 = midi_audctl & 0x02;
			bool HPF_CH13 = midi_audctl & 0x04;
			bool JOIN_34 = midi_audctl & 0x08;
			bool JOIN_12 = midi_audctl & 0x10;
			bool CH3_179 = midi_audctl & 0x20;
			bool CH1_179 = midi_audctl & 0x40;
			bool POLY9 = midi_audctl & 0x80;
			//bool TWO_TONE = (skctl == 0x8B) ? 1 : 0;

			//combined modes for some special output...
			bool JOIN_16BIT = ((JOIN_12 && CH1_179 && (track == 1 || track == 5)) || (JOIN_34 && CH3_179 && (track == 3 || track == 7))) ? 1 : 0;
			bool CLOCK_179 = ((CH1_179 && (track == 0 || track == 4)) || (CH3_179 && (track == 2 || track == 6))) ? 1 : 0;
			if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode

			if (CH1_179 && CH3_179)
			{
				//force only valid 1.79mhz channels even if the current track doesn't support it, if both are enabled but not in the right channel
				if (track > 0 && track < 2) track = 2;
				else if (track > 2) track = 0;
				CLOCK_179 = 1;
			}

			//what is the distortion? must be known to set the right note table
			switch (midi_audc & 0xF0) 
			{
			case 0x00:
				goto case_default;
				break;

			case 0x20:
			case 0x60:
				if (CLOCK_179)
				{
					midi_audf = g_atarimem[0xB040 + note];
				}
				else if (CLOCK_15)
					goto case_default;
				else
					midi_audf = g_atarimem[0xB000 + note];
				break;

			case 0x40:
				goto case_default;
				break;

			case 0x80:
				goto case_default;
				break;

			case 0xC0:
				if (CLOCK_179)
					midi_audf = g_atarimem[0xB240 + note];
				else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
					midi_audf = g_atarimem[0xB4C0 + note];
				else
					midi_audf = g_atarimem[0xB200 + note];
				break;

			case 0xE0:
				midi_audc = (char)0xC0;	//Distortion C bass E
				if (CLOCK_179)
					midi_audf = g_atarimem[0xB340 + note];
				else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
					midi_audf = g_atarimem[0xB4C0 + note];
				else
					midi_audf = g_atarimem[0xB300 + note];
				break;

			case 0xA0:
			default:
case_default:
				if (CLOCK_179)
					midi_audf = g_atarimem[0xB140 + note];
				else if (CLOCK_15)	//PAGE_EXTRA_0 => Address 0xB400, 0xB480 for 15khz Pure and 0xB4C0 for 15khz Buzzy
					midi_audf = g_atarimem[0xB480 + note];
				else
					midi_audf = g_atarimem[0xB100 + note];
				break;
			}

			midi_audc |= g_midi_voluch[track];				//also merge the volume into it

			//DIRECT MEMORY WRITE
			//g_atarimem[0x3C69] = midi_audctl;				//AUDCTL address used by SetPokey
			g_atarimem[0x3178 + track] = midi_audf;			//AUDF address + offset used by SetPokey
			g_atarimem[0x3180 + track] = midi_audc;			//AUDC address + offset used by SetPokey

			//
			//END OF HARDCODED TEST
		}
	}

	//The following is performed only on channel 0 (chn = 0), and is the defacto notes input in tracks. 
	//This is also the only mode that is specifically using the old code, and is technically legacy for compatibility reasons.
	if (chn == 0)
	{

		if (cmd == 0x90)
		{
			//key on/off
			int note = pr1 - 36;		//from the 3rd octave
			int vol;
			if (pr2 == 0)
			{
				if (!g_midi_noteoff) return;	//note off is not recognized
				vol = 0;			//keyoff
			}
			else
				if (g_midi_tr)
				{
					vol = g_midi_volumeoffset + pr2 / 8;	//dynamics
					if (vol == 0) vol++;		//vol=1
					else
						if (vol > 15) vol = 15;

					if (m_volume != vol) g_screenupdate = 1;
					m_volume = vol;
				}
				else
				{
					vol = m_volume;
				}

			if (note >= 0 && note < NOTESNUM)		//only within this range
			{
				if (g_activepart != PARTTRACKS || g_prove || g_shiftkey || g_controlkey) goto Prove_midi;	//play notes but do not record them if the active screen is not TRACKS, or if any other PROVE combo is detected

				if (vol > 0)
				{
					//volume > 0 => write note
					//Quantization
					if (m_play && m_followplay && (m_speeda < (m_speed / 2)))
					{
						m_quantization_note = note;
						m_quantization_instr = m_activeinstr;
						m_quantization_vol = vol;
						g_midi_notech[chn] = note; //see below
					}	//end Q
					else
						if (TrackSetNoteInstrVol(note, m_activeinstr, vol))
						{
							BLOCKDESELECT;
							g_midi_notech[chn] = note; //last key pressed on this midi channel
							if (g_respectvolume)
							{
								int v = TrackGetVol();
								if (v >= 0 && v <= MAXVOLUME) vol = v;
							}
							goto NextLine_midi;
						}
				}
				else
				{
					//volume = 0 => noteOff => delete note and write only volume 0
					if (g_midi_notech[chn] == note) //is it really the last one pressed?
					{
						if (m_play && m_followplay && (m_speed < (m_speed / 2)))
						{
							m_quantization_note = -2;
						}
						else
							if (TrackSetNoteActualInstrVol(-1) && TrackSetVol(0))
								goto NextLine_midi;
					}
				}

				if (0) //inside jumps only through goto
				{
				NextLine_midi:
					if (!(m_play && m_followplay)) TrackDown(g_linesafter);	//scrolls only when there is no followplay
					g_screenupdate = 1;
				Prove_midi:
					SetPlayPressedTonesTNIV(m_trackactivecol, note, m_activeinstr, vol);
					if ((g_prove == 2 || g_controlkey) && g_tracks4_8 > 4)
					{	//with control or in prove2 => stereo test
						SetPlayPressedTonesTNIV((m_trackactivecol + 4) & 0x07, note, m_activeinstr, vol);
					}
				}
			}
		}

	}
	else
	if (cmd==0xc0)
	{
		//prg change
		if (pr1>=0 && pr1<INSTRSNUM)
		{
			ActiveInstrSet(pr1);
			g_screenupdate = 1;
		}
	}
}

int CSong::SongToAta(unsigned char* dest, int max, int adr)
{
	int j;
	int apos=0,len=0,go=-1;;
	for(int sline=0; sline<SONGLEN; sline++)
	{
		apos=sline*g_tracks4_8;
		if (apos+g_tracks4_8>max) return len;		//if it had a buffer overflow

		if ( (go=m_songgo[sline])>=0)
		{
			//there is a goto line
			dest[apos]= 254;		//go command
			dest[apos+1] = go;		//number where to jump
			WORD goadr = adr + (go*g_tracks4_8);
			dest[apos+2] = goadr & 0xff;	//low byte
			dest[apos+3] = (goadr>>8);		//high byte
			if (g_tracks4_8>4)
			{
				for (int j = 4; j < g_tracks4_8; j++) dest[apos + j] = 255; //to make sure this is the correct line
			}
			len = sline*g_tracks4_8 +4; //this is the end for now (goto has 4 bytes for 8 tracks)
		}
		else
		{
			//there are track numbers
			for(int i=0; i<g_tracks4_8; i++)
			{
				j = m_song[sline][i];
				if (j>=0 && j<TRACKSNUM) 
				{ 
					dest[apos+i]=j;
					len = (sline+1) * g_tracks4_8;		//this is the end for now
				}
				else
					dest[apos+i]=255; //--
			}
		}
	}
	return len;
}

BOOL CSong::AtaToSong(unsigned char* sour, int len, int adr)
{
	int i=0;
	int col=0,line=0;
	unsigned char b;
	while(i<len)
	{
		b=sour[i];
		if (b>=0 && b<TRACKSNUM)
		{
			m_song[line][col]=b;
		}
		else
		if (b==254 && col==0)		//go command only in 0 track
		{
			//m_songgo[line]=sour[i+1];  //the driver took it by the number in channel 1
			//but more importantly, it's a vector, so it's better done that way
			int ptr=sour[i+2]|(sour[i+3]<<8); //goto vector
			int go=(ptr-adr)/g_tracks4_8;
			if (go>=0 && go<(len/g_tracks4_8) && go<SONGLEN)
				m_songgo[line]=go;
			else
				m_songgo[line]=0;	//place of invalid jump and jump to line 0
			i+= g_tracks4_8;
			if (i>=len)	return 1;		//this is the end of goto 
			line++;
			if (line>=SONGLEN) return 1;
			continue;
		}
		else
			m_song[line][col]=-1;

		col++;
		if (col>=g_tracks4_8)
		{
			line++;
			if (line>=SONGLEN) return 1;	//so that it does not overflow
			col=0;
		}
		i++;
	}
	return 1;
}


void CSong::SetRMTTitle()
{
	CString s,s1,s2;
	if (m_filename=="")
	{
		if (g_changes)
		{
			s="Noname *";
		}
		else
		{	//RMT version number and author 
			s1.LoadString(IDS_RMTVERSION);
			s2.LoadString(IDS_RMTAUTHOR);
			s.Format("%s, %s",s1,s2);
		}
	}
	else
	{
		s=m_filename;
		if (g_changes) s+=" *";
	}
	AfxGetApp()->GetMainWnd()->SetWindowText(s);
}

int CSong::WarnUnsavedChanges()
{
	//returns 1 upon cancelation
	if (!g_changes) return 0;
	int r=MessageBox(g_hwnd,"Save current changes?","Current song has been changed",MB_YESNOCANCEL|MB_ICONQUESTION);
	if (r==IDCANCEL) return 1;
	if (r==IDYES)
	{
		FileSave();
		SetRMTTitle();
		if (g_changes) return 1; //failed to save or canceled
	}
	return 0;
}

void CSong::FileReload()
{
	if (!FileCanBeReloaded()) return;
	Stop();
	int r=MessageBox(g_hwnd,"Discard all changes since your last save?\n\nWarning: Undo operation won't be possible!!!","Reload",MB_YESNOCANCEL|MB_ICONQUESTION);
	if (r==IDYES)
	{
		CString filename = m_filename;
		FileOpen((LPCTSTR)filename,0); //without Warning for changes
	}
}

void CSong::FileOpen(const char *filename, BOOL warnunsavedchanges)
{
	//stop the music first
	Stop();

	if (warnunsavedchanges && WarnUnsavedChanges()) return;

	CFileDialog fid(TRUE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY,
					"RMT song files (*.rmt)|*.rmt|TXT song files (*.txt)|*.txt|RMW song work files (*.rmw)|*.rmw||");
	fid.m_ofn.lpstrTitle = "Load song file";
	if (g_lastloadpath_songs!="")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fid.m_ofn.lpstrInitialDir = g_path_songs;

	if (m_filetype==IOTYPE_RMT) fid.m_ofn.nFilterIndex=1;
	if (m_filetype==IOTYPE_TXT) fid.m_ofn.nFilterIndex=2;
	if (m_filetype==IOTYPE_RMW) fid.m_ofn.nFilterIndex=3;

	CString fn="";
	int type=0;
	if (filename)
	{
		fn = filename;
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext==".rmt") type=IOTYPE_RMT;
		else
		if (ext==".txt") type=IOTYPE_TXT;
		else
		if (ext==".rmw") type=IOTYPE_RMW;
	}
	else
	{
		//if not ok, it's over
		if ( fid.DoModal() == IDOK )
		{
			fn = fid.GetPathName();
			type = fid.m_ofn.nFilterIndex;
		}
	}

	if ( (fn!="") && type) //only when a file was selected in the FileDialog or specified at startup
	{
		//uses fn what was selected in the FileDialog or what was specified when running //fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn); //direct way

		if (type<1 || type>3) return;

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}
		//
		if (type == 2)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			MessageBox(g_hwnd, "TXT format is currently broken, this will be fixed in a future RMT version.\nSorry for the inconvenience...", "Open error", MB_ICONERROR);
			return;
		}
		//
		//deletes the current song
		ClearSong(g_tracks4_8);

		int result;
		switch (type)
		{
		case 1: //first choice in Dialog (RMT)
			result=LoadRMT(in);
			m_filetype = IOTYPE_RMT;
			break;
		case 2: //second choice in Dialog (TXT)
			result=Load(in,IOTYPE_TXT);
			m_filetype = IOTYPE_TXT;
			break;
		case 3: //third choice in Dialog (RMW)
			result=Load(in,IOTYPE_RMW);
			m_filetype = IOTYPE_RMW;
			break;
		}
		if (!result)
		{
			//something in the Load function failed
			//MessageBoxA(g_hwnd,"Failed to open file", "ERROR",MB_ICONERROR);
			ClearSong(g_tracks4_8);		//erases everything
			SetRMTTitle();
			g_screenupdate = 1;	//must refresh
			return;
		}

		in.close();
		m_filename = fn;
		
		//init speed
		m_speed = m_mainspeed;

		//window name
		SetRMTTitle();

		//all channels ON (unmute all)
		SetChannelOnOff(-1,1);		//-1 = all, 1 = on

		if (m_instrspeed>0x04)
		{
			//Allow RMT to support instrument speed up to 8, but warn when it's above 4. Pressing "No" resets the value to 1.
			int r=MessageBox(g_hwnd,"Instrument speed values above 4 are not officially supported by RMT.\nThis may cause compatibility issues.\nDo you want keep this nonstandard speed anyway?","Warning",MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
			if (r!=IDYES) 
			{
				MessageBox(g_hwnd,"Instrument speed has been reset to the value of 1.","Warning",MB_ICONEXCLAMATION);
				m_instrspeed=0x01;
			}
		}

		g_screenupdate = 1;
	}
}

void CSong::FileSave()
{
	//stop the music first
	Stop();

	//if the file does not yet exist, prompt the "save as" dialog first
	if (m_filename=="" || m_filetype==0) 
	{
		FileSaveAs();
		return;
	}

	//if the RMT module hasn't met the conditions required to be valid, it won't be saved/overwritten
	if (m_filetype==IOTYPE_RMT && !TestBeforeFileSave())
	{
		MessageBox(g_hwnd,"Warning!\nNo data has been saved!","Warning",MB_ICONEXCLAMATION);
		SetRMTTitle();
		return;
	}
	
	//Allow saving files with speed values above 4, up to 8, which will also trigger a warning message, but it will save with no problem.
	if (m_instrspeed>0x04)
	{
		int r=MessageBox(g_hwnd,"Instrument speed values above 4 are not officially supported by RMT.\nThis may cause compatibility issues.\nDo you want to save anyway?","Warning",MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
		if (r!=IDYES) 
		{
			MessageBox(g_hwnd,"Warning!\nNo data has been saved!","Warning",MB_ICONEXCLAMATION);
			return;
		}

	}
	
	//create the file to save, iso::binary will be assumed if the format isn't TXT
	ofstream out(m_filename, (m_filetype == IOTYPE_TXT) ? ios::out : ios::binary);
	if (!out)
	{
		MessageBox(g_hwnd,"Can't create this file","Write error",MB_ICONERROR);
		return;
	}

	int r=0;
	switch (m_filetype)
	{
	case IOTYPE_RMT: //RMT
		r = Export(out,IOTYPE_RMT);
		break;
	case IOTYPE_TXT: //TXT
		r = Save(out,IOTYPE_TXT);
		break;
	case IOTYPE_RMW: //RMW
		//remembers the current octave and volume for the active instrument (for saving to RMW) 
		//because it is only saved when the instrument is changed and could change the octave or volume before saving without subsequently changing the current instrument
		m_instrs.MemorizeOctaveAndVolume(m_activeinstr,m_octave,m_volume);
		//and now saves:
		r = Save(out,IOTYPE_RMW);
		break;
	}

	//TODO: add a method to prevent deleting a valid .rmt by accident when a stripped .rmt export was aborted

	if (!r) //failed to save
	{
		out.close();
		DeleteFile(m_filename);
		MessageBox(g_hwnd,"RMT save aborted.\nFile was deleted, beware of data loss!","Save aborted",MB_ICONEXCLAMATION);
	}
	else	//saved successfully
		g_changes=0;	//changes have been saved

	SetRMTTitle();

	//closing only when "out" is open (because with IOTYPE_RMT it can be closed earlier)
	if (out.is_open()) out.close();
}

void CSong::FileSaveAs()
{
	//stop the music first
	Stop();
	
	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"RMT song file (*.rmt)|*.rmt|TXT song files (*.txt)|*.txt|RMW song work file (*.rmw)|*.rmw||");
	fod.m_ofn.lpstrTitle = "Save song as...";

	if (g_lastloadpath_songs!="")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fod.m_ofn.lpstrInitialDir = g_path_songs;

	//specifies the name of the file according to the last saved one
	char filenamebuff[1024];
	if (m_filename!="")
	{
		int pos=m_filename.ReverseFind('\\');
		if (pos<0) pos=m_filename.ReverseFind('/');
		if (pos>=0)
		{
			CString s = m_filename.Mid(pos+1);
			memset(filenamebuff,0,1024);
			strcpy(filenamebuff,(char*)(LPCTSTR)s);
			fod.m_ofn.lpstrFile = filenamebuff;
			fod.m_ofn.nMaxFile = 1020;	//4 bytes less, just to make sure ;-)
		}
	}

	//prefers the type according to the last saved
	if (m_filetype==IOTYPE_RMT) fod.m_ofn.nFilterIndex = 1;
	if (m_filetype==IOTYPE_TXT) fod.m_ofn.nFilterIndex = 2;
	if (m_filetype==IOTYPE_RMW) fod.m_ofn.nFilterIndex = 3;
	
	//if not ok, nothing will be saved
	if ( fod.DoModal() == IDOK )
	{
		int type = fod.m_ofn.nFilterIndex;
		if (type<1 || type>3) return;

		m_filename = fod.GetPathName();
		const char* exttype[]={".rmt",".txt",".rmw"};
		CString ext=m_filename.Right(4);
		ext.MakeLower();
		if (ext!=exttype[type-1]) m_filename += exttype[type-1];

		g_lastloadpath_songs = GetFilePath(m_filename); //direct way

		//TODO: fix saving the TXT format in a future version
		if (type == 2)
		{
			MessageBox(g_hwnd, "Can't save this file: " + m_filename, "Save error", MB_ICONERROR);
			MessageBox(g_hwnd, "TXT format is currently broken, this will be fixed in a future RMT version.\nSorry for the inconvenience...", "Save error", MB_ICONERROR);
			return;
		}

		switch (type)
		{
		case 1: //first choice
			m_filetype = IOTYPE_RMT;
			break;
		case 2: //second choice
			m_filetype = IOTYPE_TXT;
			break;
		case 3: //third choice
			m_filetype = IOTYPE_RMW;
			break;
		default:
			return;	//nothing will be saved if neither option was chosen
		}

		//if everything went well, the file will now be saved
		FileSave();
	}
}

void CSong::FileNew()
{
	//stop the music first
	Stop();

	//if the last changes were not saved, nothing will be created
	if (WarnUnsavedChanges()) return;

	CFileNewDlg dlg;
	if (dlg.DoModal() == IDOK )
	{
		m_tracks.m_maxtracklen = dlg.m_maxtracklen;
		g_cursoractview = m_tracks.m_maxtracklen/2;

		int i = dlg.m_combotype;
		g_tracks4_8 = (i==0)? 4 : 8;
		ClearSong(g_tracks4_8);
		SetRMTTitle();

		//automatically create 1 songline of empty patterns
		for(i=0; i<g_tracks4_8; i++) m_song[0][i] = i;

		//set the goto to the first line 
		m_songgo[1] = 0; 

		//all channels ON (unmute all)
		SetChannelOnOff(-1,1);		//-1 = all, 1 = on

		//delete undo history
		m_undo.Clear();

		//refresh the screen 
		g_screenupdate = 1;
	}
}

int l_lastimporttypeidx=-1;		//so that during the next import it has the pre-selected type that it imported last

void CSong::FileImport()
{
	//stop the music first
	Stop();

	if (WarnUnsavedChanges()) return;

	CFileDialog fid(TRUE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY,
					"ProTracker Modules (*.mod)|*.mod|TMC song files (*.tmc,*.tm8)|*.tmc;*.tm8||");
	fid.m_ofn.lpstrTitle = "Import song file";
	if (g_lastloadpath_songs!="")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fid.m_ofn.lpstrInitialDir = g_path_songs;

	if (l_lastimporttypeidx>=0) fid.m_ofn.nFilterIndex = l_lastimporttypeidx;

	//if not ok, nothing will be imported
	if ( fid.DoModal() == IDOK )
	{
		Stop();

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn);	//direct way

		int type = fid.m_ofn.nFilterIndex;
		if (type<1 || type>2) return;

		l_lastimporttypeidx = type;

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		int r=0;

		switch (type)
		{
		case 1: //first choice in Dialog (MOD)

			r = ImportMOD(in);

			break;
		case 2: //second choice in Dialog (TMC)

			r = ImportTMC(in);

			break;
		case 3: //third choice in Dialog (nothing)
			break;
		}

		in.close();
		m_filename = "";
		
		if (!r)	//import failed?
			ClearSong(g_tracks4_8);			//delete everything
		else
		{
			//init speed
			m_speed = m_mainspeed;

			//window name
			AfxGetApp()->GetMainWnd()->SetWindowText("Imported "+fn);
			//SetRMTTitle();
		}
		//all channels ON (unmute all)
		SetChannelOnOff(-1,1);		//-1 = all, 1 = on

		g_screenupdate = 1;
	}
}


void CSong::FileExportAs()
{
	//stop the music first
	Stop();

	//verify the integrity of the .rmt module to save first, so it won't be saved if it's not meeting the conditions for it
	if (!TestBeforeFileSave())
	{
		MessageBox(g_hwnd,"Warning!\nNo data has been saved!","Warning",MB_ICONEXCLAMATION);
		return;
	}

	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"RMT stripped song file (*.rmt)|*.rmt|"
					"ASM simple notation source (*.asm)|*.asm|"
					"SAP-R data stream (*.sapr)|*.sapr|"
					"Compressed SAP-R data stream (*.lzss)|*.lzss|"
					"SAP file + LZSS driver (*.sap)|*.sap|"
					"XEX Atari executable + LZSS driver (*.xex)|*.xex|");

	fod.m_ofn.lpstrTitle = "Export song as...";

	if (g_lastloadpath_songs!="")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fod.m_ofn.lpstrInitialDir = g_path_songs;

	if (m_exporttype == IOTYPE_RMTSTRIPPED) fod.m_ofn.nFilterIndex = 1;
	if (m_exporttype == IOTYPE_ASM) fod.m_ofn.nFilterIndex = 2;
	if (m_exporttype == IOTYPE_SAPR) fod.m_ofn.nFilterIndex = 3;
	if (m_exporttype == IOTYPE_LZSS) fod.m_ofn.nFilterIndex = 4;
	if (m_exporttype == IOTYPE_LZSS_SAP) fod.m_ofn.nFilterIndex = 5;
	if (m_exporttype == IOTYPE_LZSS_XEX) fod.m_ofn.nFilterIndex = 6;

	//if not ok, nothing will be saved
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		int type = fod.m_ofn.nFilterIndex;

		if (type < 1 || type > 6) return;

		const char* exttype[] = { ".rmt",".asm",".sapr",".lzss",".sap",".xex" };
		int extoff = (type - 1 == 2 || type - 1 == 3) ? 5 : 4;	//fixes the "duplicate extention" bug for 4 characters extention

		CString ext=fn.Right(extoff);
		ext.MakeLower();
		if (ext!=exttype[type-1]) fn += exttype[type-1];

		g_lastloadpath_songs = GetFilePath(fn); //direct way

		ofstream out(fn,ios::binary);
		if (!out)
		{
			MessageBox(g_hwnd,"Can't create this file: " +fn,"Write error",MB_ICONERROR);
			return;
		}

		int r;
		switch (type)
		{
		case 1: //RMT Stripped
			r = Export(out, IOTYPE_RMTSTRIPPED, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_RMTSTRIPPED;
			break;

		case 2: //ASM
			r = Export(out, IOTYPE_ASM, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_ASM;
			break;

		case 3:	//SAP-R
			r = Export(out, IOTYPE_SAPR, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_SAPR;
			break;

		case 4:	//LZSS
			r = Export(out, IOTYPE_LZSS, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_LZSS;
			break;

		case 5:	//LZSS SAP
			r = Export(out, IOTYPE_LZSS_SAP, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_LZSS_SAP;
			break;

		case 6:	//LZSS XEX
			r = Export(out, IOTYPE_LZSS_XEX, (char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_LZSS_XEX;
			break;
		}

		//file should have been successfully saved, make sure to close it
		out.close();

		//TODO: add a method to prevent accidental deletion of valid files
		if (!r)
		{
			DeleteFile(fn);
			MessageBox(g_hwnd,"Export aborted.\nFile was deleted, beware of data loss!","Export aborted",MB_ICONEXCLAMATION);
		}
		
	}
}

void CSong::FileInstrumentSave()
{
	//stop the music first
	Stop();

	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"RMT instrument file (*.rti)|*.rti||");
	fod.m_ofn.lpstrTitle = "Save RMT instrument file";
	
	if (g_lastloadpath_instruments!="")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_instruments;
	else
	if (g_path_instruments) fod.m_ofn.lpstrInitialDir = g_path_instruments;

	//if it's not ok, nothing is saved
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext!=".rti") fn += ".rti";

		g_lastloadpath_instruments=GetFilePath(fn);	//direct way

		ofstream ou(fn,ios::binary);
		if (!ou)
		{
			MessageBox(g_hwnd,"Can't create this file: " +fn,"Write error",MB_ICONERROR);
			return;
		}

		m_instrs.SaveInstrument(m_activeinstr,ou,IOINSTR_RTI);

		ou.close();
	}
}

void CSong::FileInstrumentLoad()
{
	//stop the music first
	Stop();

	CFileDialog fid(TRUE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY,
					"RMT instrument files (*.rti)|*.rti||");
	fid.m_ofn.lpstrTitle = "Load RMT instrument file";
	if (g_lastloadpath_instruments!="")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_instruments;
	else
	if (g_path_instruments) fid.m_ofn.lpstrInitialDir = g_path_instruments;
	
	//if it's not ok, nothing will be loaded
	if ( fid.DoModal() == IDOK )
	{
		m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA,1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_instruments=GetFilePath(fn);	//direct way

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		int r = m_instrs.LoadInstrument(m_activeinstr,in,IOINSTR_RTI);
		in.close();
		g_screenupdate = 1;

		if (!r)
		{
			MessageBox(g_hwnd,"It isn't RTI format (standard version 0)","Data error",MB_ICONERROR);
			return;
		}
	}
}

void CSong::FileTrackSave()
{
	int track=SongGetActiveTrack();
	if (track<0 || track>=TRACKSNUM) return;

	//stop the music first
	Stop();

	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"TXT track file (*.txt)|*.txt||");
	fod.m_ofn.lpstrTitle = "Save TXT track file";
	
	if (g_lastloadpath_tracks!="") 
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_tracks;
	else
	if (g_path_tracks!="")
		fod.m_ofn.lpstrInitialDir = g_path_tracks;

	//if not ok, nothing will be saved
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext!=".txt") fn += ".txt";

		g_lastloadpath_tracks=GetFilePath(fn);	//direct way

		ofstream ou(fn);	//text mode by default
		if (!ou)
		{
			MessageBox(g_hwnd,"Can't create this file: " +fn,"Write error",MB_ICONERROR);
			return;
		}

		m_tracks.SaveTrack(track,ou,IOTYPE_TXT);

		ou.close();
	}
}

void CSong::FileTrackLoad()
{
	int track=SongGetActiveTrack();
	if (track<0 || track>=TRACKSNUM) return;

	//stop the music first
	Stop();

	CFileDialog fid(TRUE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY,
					"TXT track files (*.txt)|*.txt||");
	fid.m_ofn.lpstrTitle = "Load TXT track file";
	if (g_lastloadpath_tracks!="") 
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_tracks;
	else
	if (g_path_tracks!="")
		fid.m_ofn.lpstrInitialDir = g_path_tracks;
	
	//if not ok, nothing will be loaded
	if ( fid.DoModal() == IDOK )
	{
		m_undo.ChangeTrack(0,0,UETYPE_TRACKSALL,1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_tracks=GetFilePath(fn);	//direct way
		ifstream in(fn);	//text mode by default
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		char line[1025];
		int nt=0;		//number of tracks
		int type=0;		//type when loading multiple tracks
		while(NextSegment(in)) //will therefore look for the beginning of the next segment "["
		{
			in.getline(line,1024);
			Trimstr(line);
			if (strcmp(line,"TRACK]")==0) nt++;
		}

		if (nt==0)
		{
			MessageBox(g_hwnd,"Sorry, this file doesn't contain any track in TXT format","Data error",MB_ICONERROR);
			return;
		}
		else
		if (nt>1)
		{
			CTracksLoadDlg dlg;
			dlg.m_trackfrom=track;
			dlg.m_tracknum=nt;
			if (dlg.DoModal()!=IDOK) return;
			type=dlg.m_radio;
		}

		in.seekg(0);	//again at the beginning
		in.clear();		//reset the flag from the end
		int nr=0;
		NextSegment(in);	//move after the first "["
		while(!in.eof())
		{
			in.getline(line,1024);
			Trimstr(line);
			if (strcmp(line,"TRACK]")==0)
			{
				int tt = (type==0)? track : -1;
				if ( m_tracks.LoadTrack(tt,in,IOTYPE_TXT) )
				{
					nr++;	//number of tracks loaded
					if (type==0)
					{
						track++;	//shift by 1 to load the next track
						if (track>=TRACKSNUM)
						{
							MessageBox(g_hwnd,"Track's maximum number reached.\nLoading aborted.","Error",MB_ICONERROR);
							break;
						}
					}
					g_screenupdate = 1;
				}
			}
			else
				NextSegment(in);	//move to the next "["
		}
		in.close();

		if (nr==0 || nr>1) //if it has not found any or more than 1
		{
			CString s;
			s.Format("%i track(s) loaded.",nr);
			MessageBox(g_hwnd,s,"Track(s) loading finished.",MB_ICONINFORMATION);
		}
	}
}

#define RMWMAINPARAMSCOUNT		31		//
#define DEFINE_MAINPARAMS int* mainparams[RMWMAINPARAMSCOUNT]= {		\
	&g_tracks4_8,												\
	(int*)&m_speed,(int*)&m_mainspeed,(int*)&m_instrspeed,		\
	(int*)&m_songactiveline,(int*)&m_songplayline,				\
	(int*)&m_trackactiveline,(int*)&m_trackplayline,			\
	(int*)&g_activepart,(int*)&g_active_ti,						\
	(int*)&g_prove,(int*)&g_respectvolume,						\
	&g_tracklinehighlight,										\
	&g_tracklinealtnumbering,									\
	&g_displayflatnotes,										\
	&g_usegermannotation,										\
	&g_cursoractview,											\
	&g_keyboard_layout,											\
	&g_keyboard_escresetatarisound,								\
	&g_keyboard_swapenter,										\
	&g_keyboard_playautofollow,									\
	&g_keyboard_updowncontinue,									\
	&g_keyboard_rememberoctavesandvolumes,						\
	&g_keyboard_escresetatarisound,								\
	&m_trackactivecol,&m_trackactivecur,						\
	&m_activeinstr,&m_volume,&m_octave,							\
	&m_infoact,&m_songnamecur									\
}

int CSong::Save(ofstream& ou,int iotype)
{
	switch(iotype)
	{
	case IOTYPE_RMW:
		{
		CString version;
		version.LoadString(IDS_RMTVERSION);
		ou << (unsigned char*)(LPCSTR)version << endl;
		//
		ou.write((char*)m_songname, sizeof(m_songname));
		//
		DEFINE_MAINPARAMS;
		
		int p = RMWMAINPARAMSCOUNT; //number of stored parameters
		ou.write((char*)&p, sizeof(p));		//write the number of main parameters
		for(int i=0; i<p; i++)
			ou.write((char*)mainparams[i], sizeof(mainparams[0]));
		
		//write a complete song and songgo
		ou.write((char*)m_song, sizeof(m_song));
		ou.write((char*)m_songgo, sizeof(m_songgo));
	}
		break;

	case IOTYPE_TXT:
		{
		CString s,nambf;
		char bf[16];
		nambf=m_songname;
		nambf.TrimRight();
		s.Format("[MODULE]\nRMT: %X\nNAME: %s\nMAXTRACKLEN: %02X\nMAINSPEED: %02X\nINSTRSPEED: %X\nVERSION: %02X\n",g_tracks4_8,(LPCTSTR)nambf,m_tracks.m_maxtracklen,m_mainspeed,m_instrspeed,RMTFORMATVERSION);
		ou << s << "\n"; //gap
		ou << "[SONG]\n";
		int i,j;
		//looking for the length of the song
		int lens=-1;
		for(i=0; i<SONGLEN; i++)
		{
			if (m_songgo[i]>=0) { lens=i; continue; }
			for(j=0; j<g_tracks4_8; j++)
			{
				if (m_song[i][j]>=0 && m_song[i][j]<TRACKSNUM) { lens=i; break; }
			}
		}

		//write the song
		for(i=0; i<=lens; i++)
		{
			if (m_songgo[i]>=0)
			{
				s.Format("Go to line %02X\n",m_songgo[i]);
				ou << s;
				continue;
			}
			for(j=0; j<g_tracks4_8; j++)
			{
				int t=m_song[i][j];
				if (t>=0 && t<TRACKSNUM)
				{
					bf[0]=CharH4(t);
					bf[1]=CharL4(t);
				}
				else
				{
					bf[0]=bf[1]='-';
				}
				bf[2]=0;
				ou << bf;
				if (j+1==g_tracks4_8) 
					ou << "\n";			//for the last end of the line
				else
					ou << " ";			//between them
			}
		}

		ou << "\n"; //gap
		}
		break;
	}

	m_instrs.SaveAll(ou,iotype);
	m_tracks.SaveAll(ou,iotype);

	return 1;
}


int CSong::Load(ifstream& in,int iotype)
{
	ClearSong(8);	//always clear 8 tracks 

	if (iotype==IOTYPE_RMW)
	{
	//LOAD RMW
	CString version;
	version.LoadString(IDS_RMTVERSION);
	char filever[256];
	in.getline(filever,255);
	if (strcmp((char*)(LPCTSTR)version,filever)!=0)
	{
		MessageBox(g_hwnd,CString("Incorrect version: ")+filever,"Load error",MB_ICONERROR);
		return 0;
	}
	//
	in.read((char*)m_songname, sizeof(m_songname));
	//
	DEFINE_MAINPARAMS;
	int p = 0;
	in.read((char*)&p, sizeof(p));	//read the number of main parameters
	for (int i = 0; i < p; i++)
		in.read((char*)mainparams[i], sizeof(mainparams[0]));
	//

	//read the complete song and songgo
	in.read((char*)m_song, sizeof(m_song));
	in.read((char*)m_songgo, sizeof(m_songgo));

	m_instrs.LoadAll(in,iotype);
	m_tracks.LoadAll(in,iotype);
	}
	else
	if (iotype==IOTYPE_TXT)
	{
		//LOAD TXT
		m_tracks.InitTracks();

		char b;
		char line[1025];
		//read after the first "[" (inclusive)
		NextSegment(in);

		while (!in.eof())
		{
			in.getline(line,1024);
			Trimstr(line);
			if (strcmp(line,"MODULE]")==0)
			{
				//MODULE
				while(!in.eof())
				{
					in.read((char*)&b,1);
					if (b=='[') break;
					line[0]=b;
					in.getline(line+1,1024);
					char *value=strstr(line,": ");
					if (value)
					{
						value[1]=0;	//gap
						value+=2;	//move to the first character after the space
					}
					else
						continue;

					if (strcmp(line,"RMT:")==0) 
					{
						int v = Hexstr(value,2);
						if (v<=4) 
							v=4;
						else
							v=8;
						g_tracks4_8 = v;
					}
					else
					if (strcmp(line,"NAME:")==0)
					{
						Trimstr(value);
						memset(m_songname,' ',SONGNAMEMAXLEN);
						int lname=SONGNAMEMAXLEN;
						if (strlen(value)<=SONGNAMEMAXLEN) lname=strlen(value);
						strncpy(m_songname,value,lname);
					}
					else
					if (strcmp(line,"MAXTRACKLEN:")==0) 
					{
						int v = Hexstr(value,2);
						if (v==0) v=256;
						m_tracks.m_maxtracklen= v;
						g_cursoractview = m_tracks.m_maxtracklen/2;
						m_tracks.InitTracks();	//reinitialise
					}
					else
					if (strcmp(line,"MAINSPEED:")==0) 
					{
						int v = Hexstr(value,2);
						if (v>0) m_mainspeed=v;
					}
					else
					if (strcmp(line,"INSTRSPEED:")==0) 
					{
						int v = Hexstr(value,1);
						if (v>0) m_instrspeed=v;
					}
					else
					if (strcmp(line,"VERSION:")==0) 
					{
						//int v = Hexstr(value,2);
						//the version number is not needed for TXT yet, because it only selects the parameters it knows
					}
				}
			}
			else
			if (strcmp(line,"SONG]")==0)
			{
				//SONG 
				int idx,i;
				for(idx=0; !in.eof() && idx<SONGLEN; idx++)
				{
					memset(line,0,32);
					in.read((char*)&b,1);
					if (b=='[') break;
					line[0]=b;
					in.getline(line+1,1024);
					if (strncmp(line,"Go to line ",11)==0)
					{
						int go=Hexstr(line+11,2);
						if (go>=0 && go<SONGLEN) m_songgo[idx]=go;
						continue;
					}
					for(i=0; i<g_tracks4_8; i++)
					{
						int track=Hexstr(line+i*3,2);
						if (track>=0 && track<TRACKSNUM) m_song[idx][i]=track;
					}
				}
			}
			else
			if (strcmp(line,"INSTRUMENT]")==0)
			{
				m_instrs.LoadInstrument(-1,in,iotype); //-1 => retrieve the instrument number from the TXT source
			}
			else
			if (strcmp(line,"TRACK]")==0)
			{
				m_tracks.LoadTrack(-1,in,iotype);	//-1 => retrieve the track number from TXT source
			}
			else
				NextSegment(in); //will therefore look for the beginning of the next segment
		}
	}

	return 1;
}

int CSong::TestBeforeFileSave()
{
	//it is performed on Export (everything except RMW) before the target file is overwritten
	//so if it returns 0, the export is terminated and the file is not overwritten

	//try to create a module
	unsigned char mem[65536];
	int adr_module=0x4000;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];
	int maxadr;
	
	//try to make a blank RMT module
	maxadr = MakeModule(mem,adr_module,IOTYPE_RMT,instrsaved,tracksaved);
	if (maxadr<0) return 0;	//if the module could not be created

	//and now it will be checked whether the song ends with GOTO line and if there is no GOTO on GOTO line
	CString errmsg,wrnmsg,s;
	int trx[SONGLEN];
	int i,j,r,go,last=-1,tr=0,empty=0;

	for(i=0; i<SONGLEN; i++)
	{
		if (m_songgo[i]>=0) 
		{
			trx[i]=2;
			last=i;
		}
		else
		{
			trx[i]=0;
			for(j=0; j<g_tracks4_8; j++)
			{
				if (m_song[i][j]>=0 && m_song[i][j]<TRACKSNUM)
				{
					trx[i]=1;
					last=i; //tracks
					tr++;
					break;
				}
			}
		}
	}

	if (last<0)
	{
		errmsg += "Error: Song is empty.\n";
	}

	for(i=0; i<=last; i++)
	{
		if (m_songgo[i]>=0)
		{
			//there is a goto line
			go = m_songgo[i];	//where is goto set to?
			if (go>last)
			{
				s.Format("Error: Song line [%02X]: Go to line over last used song line.\n",i);
				errmsg += s;
			}
			if (m_songgo[go]>=0)
			{
				s.Format("Error: Song line [%02X]: Recursive \"go to line\" to \"go to line\".\n",i);
				errmsg += s;
			}
			if (i>0 && m_songgo[i-1]>=0)
			{
				s.Format("Warning: Song line [%02X]: More \"go to line\" on subsequent lines.\n",i);
				wrnmsg += s;
			}
			goto TestTooManyEmptyLines;
		}
		else
		{
			//are there tracks or empty lines?
			if (trx[i]==0) 
				empty++; 
			else 
			{
TestTooManyEmptyLines:
				if (empty>1)
				{
					s.Format("Warning: Song lines [%02X-%02X]: Too many empty song lines (%i) waste memory.\n",i-empty,i-1,empty);
					wrnmsg += s;
				}
				empty=0;
			}
		}
	}

	if (trx[last]==1)
	{
		char gotoline[140];
		sprintf(gotoline,"Song line[%02X]: Unexpected end of song.\nYou have to use \"go to line\" at the end of song.\n\nSong line [00] will be used by default.",last+1);
		MessageBox(g_hwnd, gotoline,"Warning",MB_ICONINFORMATION);
		m_songgo[last+1] = 0;	//force a goto line to the first track line
	}

	//if the warning or error messages aren't empty, something did happen
	if (errmsg!="" || wrnmsg!="")
	{
		//if there are warnings without errors, the choice is left to ignore them
		if (errmsg=="")
		{
			wrnmsg += "\nIgnore warnings and save anyway?";
			r = MessageBox(g_hwnd, wrnmsg, "Warnings", MB_YESNO | MB_ICONQUESTION);
			if (r==IDYES) return 1;
			return 0;
		}
		//else, if there are any errors, always return 0
		MessageBox(g_hwnd, errmsg + wrnmsg, "Errors", MB_ICONERROR);
		return 0;
	}

	return 1;	
}

//---

int CSong::GetSubsongParts(CString& resultstr)
{
	CString s;
	int songp[SONGLEN];
	int i,j,n,lastgo,apos,asub;
	BOOL ok;
	lastgo=-1;
	for(i=0; i<SONGLEN; i++)
	{
		songp[i]=-1;
		if (m_songgo[i]>=0) lastgo=i;
	}

	resultstr="";
	apos=0;
	asub=0;
	ok=0;	//if it found any non-zero tracks in the given subsong (different from --)

	for(i=0; i<=lastgo; i++)
	{
		if (songp[i]<0)
		{
			apos=i;
			while(songp[apos]<0)
			{
				n = m_songgo[apos];
				songp[apos] = asub;
				if (n>=0) //jump to another line
					apos=n;
				else
				{
					if (!ok)
					{	//has not found any tracks in this subsong yet
						for(j=0; j<g_tracks4_8; j++)
						{
							if (m_song[apos][j]>=0)
							{	//if then found, this will be the beginning of the subsong
								s.Format("%02X ",apos);
								resultstr+=s;
								ok=1;		//the beginning of this subsong is already written
								break;
							}
						}
					}
					apos++;
					if (apos>=SONGLEN) break;
				}
			}
			if (ok) asub++;	//will move to the next if the subsong contains anything at all
			ok=0; //initialization for further search
		}
	}
	return asub;
}

int CSong::Export(ofstream& ou,int iotype, char* filename)
{
	//TODO: manage memory in a more dynamic way 
	unsigned char mem[65536];					//default RAM size for most 800xl/xe machines
	unsigned char buff1[65536];					//LZSS buffers for each ones of the tune parts being reconstructed
	unsigned char buff2[65536];					//they are used for parts labeled: full, intro, and loop 
	unsigned char buff3[65536];					//a LZSS export will typically make use of intro and loop only, unless specified otherwise

	//SAP-R and LZSS variables used for export binaries and/or data dumped
	int firstcount, secondcount, thirdcount, fullcount;	//4 frames counter, for complete playback, looped section, intro section, and total frames counted
	int adr_lzss_pointer = 0x3000;				//all the LZSS subtunes index will occupy this memory page
	int adr_loop_flag = 0x1C22;					//VUPlayer's address for the Loop flag
	int adr_stereo_flag = 0x1DC3;				//VUPlayer's address for the Stereo flag 
	int adr_song_speed = 0x2029;				//VUPlayer's address for setting the song speed
	int adr_region = 0x202D;					//VUPlayer's address for the region initialisation
	int adr_colour = 0x2132;					//VUPlayer's address for the rasterbar colour
	int adr_rasterbar = 0x216B;					//VUPlayer's address for the rasterbar display 
	int adr_do_play = 0x2174;					//VUPlayer's address for Play, for SAP exports bypassing the mainloop code
	int adr_rts_nop = 0x21B1;					//VUPlayer's address for JMP loop being patched to RTS NOP NOP with the SAP format
	int adr_shuffle = 0x2308;					//VUPlayer's address for the rasterbar colour shuffle (incomplete feature)
	int adr_init_sap = 0x3080;					//VUPlayer SAP initialisation hack
	int bytenum = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number
	int lzss_offset, lzss_end;

	//RMT module addresses
	int adr_module = 0x4000;					//standard RMT modules are set to $4000
	int maxadr = adr_module;

	WORD adrfrom,adrto;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];
	BOOL head_ffff=1;							//FFFF header at the beginning of the file, it only needs to be defined once

	//create a module
	memset(mem,0,65536);
	maxadr = MakeModule(mem,adr_module,iotype,instrsaved,tracksaved);
	if (maxadr<0) return 0;						//if the module could not be created, the export process is immediately aborted
	CString s;

	//first, we must dump the current module as SAP-R before LZSS conversion
	//TODO: turn this into a proper SAP-R dumper function. That way, conversions in batches could be done for multiple tunes
	if (iotype == IOTYPE_SAPR || iotype == IOTYPE_LZSS || iotype == IOTYPE_LZSS_SAP || iotype == IOTYPE_LZSS_XEX)
	{
		fullcount = firstcount = secondcount = thirdcount = 0;	//initialise them all to 0 for the first part 
		ChangeTimer((g_ntsc) ? 17 : 20);	//this helps avoiding corruption if things are running too fast
		Atari_InitRMTRoutine();	//reset the RMT routines 
		SetChannelOnOff(-1, 0);	//switch all channels off 
		SAPRDUMP = 3;	//set the SAP-r dumper initialisation flag 
		Play(MPLAY_SONG, m_followplay);	//play song from start, start before the timer changes again
		ChangeTimer(1);	//set the timer to be as fast as possible for the recording process

		while (m_play)	//the SAP-R dumper is running during that time...
		{
			if (SAPRDUMP == 2)	//ready to write data when the flag is set to 2
			{	
				if (loops == 1)
				{	//first loop completed		
					SAPRDUMP = 1;								//set the flag back to dump for the looped part
					firstcount = framecount;					//from start to loop point
				}
				if (loops == 2)
				{	//second loop completed
					SAPRDUMP = 0;								//the dumper has reached its end
					secondcount = framecount - firstcount;		//from loop point to end
				}
			}
			//TODO: display progress instead of making the program look like it isn't responding 
			//it is basically writing the SAP-R data during the time the program appears frozen
			//unfortunately, I haven't figured out a good way to display the dump/compression process
			//so this is a very small annoyance, but usually it only takes about 30 seconds to export, which is definitely not the worst thing ever
		}

		ChangeTimer((g_ntsc) ? 17 : 20);			//reset the timer again, to avoid corruption from running too fast
		Stop();										//end playback now, the SAP-R data should have been dumped successfully!

		//the difference defines the length of the intro section. a size of 0 means it is virtually identical to the looped section, with few exceptions
		thirdcount = firstcount - secondcount;

		//total frames counted, from start to the end of second loop
		fullcount = framecount;
	}

	switch (iotype)
	{
	case IOTYPE_RMT:
	{
		//save the first RMT module block
		SaveBinaryBlock(ou, mem, adr_module, maxadr - 1, head_ffff);

		//the individual names are truncated by spaces and terminated by a zero
		int adrsongname = maxadr;
		s = m_songname;
		s.TrimRight();
		int lens = s.GetLength() + 1;	//including 0 after the string
		strncpy((char*)(mem + adrsongname), (LPCSTR)s, lens);
		int adrinstrnames = adrsongname + lens;
		for (int i = 0; i < INSTRSNUM; i++)
		{
			if (instrsaved[i])
			{
				s = m_instrs.GetName(i);
				s.TrimRight();
				lens = s.GetLength() + 1;	//including 0 after the string
				strncpy((char*)(mem + adrinstrnames), s, lens);
				adrinstrnames += lens;
			}
		}
		//and now, save the 2nd block
		SaveBinaryBlock(ou, mem, adrsongname, adrinstrnames - 1, 0);
	}
		break;

	case IOTYPE_RMTSTRIPPED:
	{
		//create a variant for SFX (ie. including unused instruments and tracks)
		BYTE instrsaved2[INSTRSNUM];
		BYTE tracksaved2[TRACKSNUM];
		int maxadr2 = MakeModule(mem, adr_module, IOTYPE_RMT, instrsaved2, tracksaved2);
		if (maxadr2 < 0) return 0;	//if the module could not be created

		//Dialog for specifying the address of the RMT module in memory
		CExpRMTDlg dlg;
		dlg.m_len = maxadr - adr_module;
		dlg.m_len2 = maxadr2 - adr_module;
		dlg.m_adr = g_rmtstripped_adr_module;	//global, so that it remains the same on repeated export
		dlg.m_sfx = g_rmtstripped_sfx;
		dlg.m_gvf = g_rmtstripped_gvf;
		dlg.m_nos = g_rmtstripped_nos;
		dlg.m_song = this;
		dlg.m_filename = filename;
		dlg.m_instrsaved = instrsaved;
		dlg.m_instrsaved2 = instrsaved2;
		dlg.m_tracksaved = tracksaved;
		dlg.m_tracksaved2 = tracksaved2;
		if (dlg.DoModal() != IDOK) return 0;
		
		g_rmtstripped_adr_module = adr_module = dlg.m_adr;
		g_rmtstripped_sfx = dlg.m_sfx;
		g_rmtstripped_gvf = dlg.m_gvf;
		g_rmtstripped_nos = dlg.m_nos;

		//regenerates the module according to the entered address "adr"
		memset(mem, 0, 65536);
		if (!g_rmtstripped_sfx) //either without unused instruments and tracks
			maxadr = MakeModule(mem, adr_module, iotype, instrsaved, tracksaved);
		else					//or with them
			maxadr = MakeModule(mem, adr_module, IOTYPE_RMT, instrsaved, tracksaved);
		if (maxadr < 0) return 0; //if the module could not be created
		//and now save the RMT module block
		SaveBinaryBlock(ou, mem, adr_module, maxadr - 1, head_ffff);
	}
	break;
	
	//TODO: cleanup
	case IOTYPE_ASM:
	{
		CExpASMDlg dlg;
		CString s, snot;
		int maxova = 16;				//maximal amount of data per line
		dlg.m_labelsprefix = g_expasmlabelprefix;
		if (dlg.DoModal() != IDOK) return 0;
		//
		g_expasmlabelprefix = dlg.m_labelsprefix;
		CString lprefix = g_expasmlabelprefix;
		BYTE tracks[TRACKSNUM];
		memset(tracks, 0, TRACKSNUM); //init
		MarkTF_USED(tracks);
		ou << ";ASM notation source";
		ou << EOL << "XXX\tequ $FF\t;empty note value";
		if (!lprefix.IsEmpty())
		{
			s.Format("%s_data", lprefix);
			ou << EOL << s;
		}
		//
		if (dlg.m_type == 1)
		{
			//tracks
			int t, i, j, not, dur, ins;
			for (t = 0; t < TRACKSNUM; t++)
			{
				if (!(tracks[t] & TF_USED)) continue;
				s.Format(";Track $%02X", t);
				ou << EOL << s;
				if (!lprefix.IsEmpty())
				{
					s.Format("%s_track%02X", lprefix, t);
					ou << EOL << s;
				}
				TTrack& origtt = *m_tracks.GetTrack(t);
				TTrack tt;	//temporary track
				memcpy((void*)&tt, (void*)&origtt, sizeof(TTrack)); //make a copy of origtt to tt
				m_tracks.TrackExpandLoop(&tt); //expands tt due to GO loops
				int ova = maxova;
				for (i = 0; i < tt.len; i++)
				{
					if (ova >= maxova)
					{
						ou << EOL << "\tdta ";
						ova = 0;
					}
					not= tt.note[i];
					if (not>= 0)
					{
						ins = tt.instr[i];
						if (dlg.m_notes == 1) //notes
							not= m_instrs.GetNote(ins, not);
						else				//frequencies
							not= m_instrs.GetFrequency(ins, not);
					}
					if (not>= 0) snot.Format("$%02X", not); else snot = "XXX";
					for (dur = 1; i + dur < tt.len && tt.note[i + dur] < 0; dur++);
					if (dlg.m_durations == 1)
					{
						if (ova > 0) ou << ",";
						ou << snot; ova++;
						for (j = 1; j < dur; j++, ova++)
						{
							if (ova >= maxova)
							{
								ova = 0;
								ou << EOL << "\tdta XXX";
							}
							else
								ou << ",XXX";
						}
					}
					else
						if (dlg.m_durations == 2)
						{
							if (ova > 0) ou << ",";
							ou << snot;
							ou << "," << dur;
							ova += 2;
						}
						else
							if (dlg.m_durations == 3)
							{
								if (ova > 0) ou << ",";
								ou << dur << ",";
								ou << snot;
								ova += 2;
							}
					i += dur - 1;
				}
			}
		}
		else
			if (dlg.m_type == 2)
			{
				//song columns
				int clm;
				for (clm = 0; clm < g_tracks4_8; clm++)
				{
					BYTE finished[SONGLEN];
					memset(finished, 0, SONGLEN);
					int sline = 0;
					static char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };
					s.Format(";Song column %s", cnames[clm]);
					ou << EOL << s;
					if (!lprefix.IsEmpty())
					{
						s.Format("%s_column%s", lprefix, cnames[clm]);
						ou << EOL << s;
					}
					while (sline >= 0 && sline < SONGLEN && !finished[sline])
					{
						finished[sline] = 1;
						s.Format(";Song line $%02X", sline);
						ou << EOL << s;
						if (m_songgo[sline] >= 0)
						{
							sline = m_songgo[sline]; //GOTO line
							s.Format(" Go to line $%02X", sline);
							ou << s;
							continue;
						}
						int trackslen = m_tracks.m_maxtracklen;
						for (int i = 0; i < g_tracks4_8; i++)
						{
							int at = m_song[sline][i];
							if (at < 0 || at >= TRACKSNUM) continue;
							if (m_tracks.GetGoLine(at) >= 0) continue;
							int al = m_tracks.GetLastLine(at) + 1;
							if (al < trackslen) trackslen = al;
						}
						int t, i, j, not, dur, ins;
						int ova = maxova;
						t = m_song[sline][clm];
						if (t < 0)
						{
							ou << " Track --";
							if (!lprefix.IsEmpty())
							{
								s.Format("%s_column%s_line%02X", lprefix, cnames[clm], sline);
								ou << EOL << s;
							}
							if (dlg.m_durations == 1)
							{
								for (i = 0; i < trackslen; i++, ova++)
								{
									if (ova >= maxova)
									{
										ova = 0;
										ou << EOL << "\tdta XXX";
									}
									else
										ou << ",XXX";
								}
							}
							else
								if (dlg.m_durations == 2)
								{
									ou << EOL << "\tdta XXX,";
									ou << trackslen;
									ova += 2;
								}
								else
									if (dlg.m_durations == 3)
									{
										ou << EOL << "\tdta ";
										ou << trackslen << ",XXX";
										ova += 2;
									}
							sline++;
							continue;
						}

						s.Format(" Track $%02X", t);
						ou << s;
						if (!lprefix.IsEmpty())
						{
							s.Format("%s_column%s_line%02X", lprefix, cnames[clm], sline);
							ou << EOL << s;
						}

						TTrack& origtt = *m_tracks.GetTrack(t);
						TTrack tt; //temporary track
						memcpy((void*)&tt, (void*)&origtt, sizeof(TTrack)); //make a copy of origtt to tt
						m_tracks.TrackExpandLoop(&tt); //expands tt due to GO loops
						for (i = 0; i < trackslen; i++)
						{
							if (ova >= maxova)
							{
								ova = 0;
								ou << EOL << "\tdta ";
							}

							not= tt.note[i];
							if (not>= 0)
							{
								ins = tt.instr[i];
								if (dlg.m_notes == 1) //notes
									not= m_instrs.GetNote(ins, not);
								else				//frequencies
									not= m_instrs.GetFrequency(ins, not);
							}
							if (not>= 0) snot.Format("$%02X", not); else snot = "XXX";
							for (dur = 1; i + dur < trackslen && tt.note[i + dur] < 0; dur++);
							if (dlg.m_durations == 1)
							{
								if (ova > 0) ou << ",";
								ou << snot; ova++;
								for (j = 1; j < dur; j++, ova++)
								{
									if (ova >= maxova)
									{
										ova = 0;
										ou << EOL << "\tdta XXX";
									}
									else
										ou << ",XXX";
								}
							}
							else
								if (dlg.m_durations == 2)
								{
									if (ova > 0) ou << ",";
									ou << snot;
									ou << "," << dur;
									ova += 2;
								}
								else
									if (dlg.m_durations == 3)
									{
										if (ova > 0) ou << ",";
										ou << dur << ",";
										ou << snot;
										ova += 2;
									}
							i += dur - 1;
						}
						sline++;
					}
				}
			}
		ou << EOL;
	}
	break;

	//TODO: clean this up better, and allow multiple export choices later
	//for now, this will simply be a full raw dump of all the bytes dumped at once, which is a full tune playback before loop
	case IOTYPE_SAPR:
	{
		CExpSAPDlg dlg;
		s = m_songname;
		s.TrimRight();			//cuts spaces after the name
		s.Replace('"', '\'');	//replaces quotation marks with an apostrophe
		dlg.m_name = s;

		dlg.m_author = "???";

		CTime time = CTime::GetCurrentTime();
		dlg.m_date = time.Format("%d/%m/%Y");

		if (dlg.DoModal() != IDOK)
		{	//clear the SAP-R dumper memory and reset RMT routines
			GetPokey()->DoneSAPR();
			return 0;
		}

		ou << "SAP" << EOL;

		s = dlg.m_author;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "AUTHOR \"" << s << "\"" << EOL;

		s = dlg.m_name;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "NAME \"" << s << " (" << firstcount << " frames)" << "\"" << EOL;	//display the total frames recorded

		s = dlg.m_date;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "DATE \"" << s << "\"" << EOL;

		s.MakeUpper();

		ou << "TYPE R" << EOL;

		if (g_tracks4_8 > 4)
		{	//stereo module
			ou << "STEREO" << EOL;
		}

		if (g_ntsc)
		{	//NTSC module
			ou << "NTSC" << EOL;
		}

		if (m_instrspeed > 1)
		{
			ou << "FASTPLAY ";
			switch (m_instrspeed)
			{
			case 2:
				ou << ((g_ntsc) ? "131" : "156");
				break;

			case 3:
				ou << ((g_ntsc) ? "87" : "104");
				break;

			case 4:
				ou << ((g_ntsc) ? "66" : "78");
				break;

			default:
				ou << ((g_ntsc) ? "262" : "312");
				break;
			}
			ou << EOL;
		}
		//a double EOL is necessary for making the SAP-R export functional
		ou << EOL;

		//write the SAP-R stream to the output file defined in the path dialog with the data specified above
		GetPokey()->WriteFileToSAPR(ou, firstcount, 0);

		//clear the memory and reset the dumper to its initial setup for the next time it will be called
		GetPokey()->DoneSAPR();
	}
		break;

	//TODO: allow more LZSS choices, for now it will simply write the full tune with no loop
	case IOTYPE_LZSS:
	{
		//Just in case
		memset(buff1, 0, 65536);

		//Now, create LZSS files using the SAP-R dump created earlier
		ifstream in;

		//full tune playback up to its loop point
		int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
		if (full)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff1, sizeof(buff1))))
				full = (int)in.gcount();
			if (full < 16) full = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		GetPokey()->DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

		ou.write((char*)buff1, full);	//write the buffer 1 contents to the export file
	}
	break;

	//TODO: add more options through addional parameters from dialog box
	case IOTYPE_LZSS_SAP:
	{
		//Just in case
		memset(buff1, 0, 65536);
		memset(buff2, 0, 65536);
		memset(buff3, 0, 65536);

		//Now, create LZSS files using the SAP-R dump created earlier
		ifstream in;

		//full tune playback up to its loop point
		int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
		if (full)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff1, sizeof(buff1))))
				full = (int)in.gcount();
			if (full < 16) full = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		//intro section playback, up to the start of the detected loop point
		int intro = LZSS_SAP((char*)SAPRSTREAM, thirdcount * bytenum);
		if (intro)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff2, sizeof(buff2))))
				intro = (int)in.gcount();
			if (intro < 16) intro = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		//looped section playback, this part is virtually seamless to itself
		int loop = LZSS_SAP((char*)SAPRSTREAM + (firstcount * bytenum), secondcount * bytenum);
		if (loop)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff3, sizeof(buff3))))
				loop = (int)in.gcount();
			if (loop < 16) loop = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		GetPokey()->DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

		//some additional variables that will be used below
		adr_module = 0x3100;												//all the LZSS data will be written starting from this address
		lzss_offset = (intro) ? adr_module + intro : adr_module + full;		//calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
		lzss_end = lzss_offset + loop;										//this sets the address that defines where the data stream has reached its end

		//if the size is too big, abort the process and show an error message
		if (lzss_end > 0xBFFF)
		{
			MessageBox(g_hwnd,
				"Error, LZSS data is too big to fit in memory!\n\n"
				"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
				"Error, Buffer Overflow!", MB_ICONERROR);
			return 0;
		}

		int r;
		r = LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, adrfrom, adrto);
		if (!r)
		{
			MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
			return 0;
		}

		CExpSAPDlg dlg;

		s = m_songname;
		s.TrimRight();			//cuts spaces after the name
		s.Replace('"', '\'');	//replaces quotation marks with an apostrophe
		dlg.m_name = s;

		dlg.m_author = "???";
		GetSubsongParts(dlg.m_subsongs);

		CTime time = CTime::GetCurrentTime();
		dlg.m_date = time.Format("%d/%m/%Y");

		if (dlg.DoModal() != IDOK)
			return 0;

		ou << "SAP" << EOL;

		s = dlg.m_author;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "AUTHOR \"" << s << "\"" << EOL;

		s = dlg.m_name;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "NAME \"" << s << " (" << firstcount << " frames)" << "\"" << EOL;	//display the total frames recorded

		s = dlg.m_date;
		s.TrimRight();
		s.Replace('"', '\'');

		ou << "DATE \"" << s << "\"" << EOL;

		s = dlg.m_subsongs + " ";		//space after the last character due to parsing

		s.MakeUpper();
		int subsongs = 0;
		BYTE subpos[MAXSUBSONGS];
		subpos[0] = 0;					//start at songline 0 by default
		BYTE a, n, isn = 0;

		//parses the "Subsongs" line from the ExportSAP dialog
		for (int i = 0; i < s.GetLength(); i++)
		{
			a = s.GetAt(i);
			if (a >= '0' && a <= '9') { n = (n << 4) + (a - '0'); isn = 1; }
			else
				if (a >= 'A' && a <= 'F') { n = (n << 4) + (a - 'A' + 10); isn = 1; }
				else
				{
					if (isn)
					{
						subpos[subsongs] = n;
						subsongs++;
						if (subsongs >= MAXSUBSONGS) break;
						isn = 0;
					}
				}
		}
		if (subsongs > 1)
			ou << "SONGS " << subsongs << EOL;

		ou << "TYPE B" << EOL;
		s.Format("INIT %04X", adr_init_sap);
		ou << s << EOL;
		s.Format("PLAYER %04X", adr_do_play);
		ou << s << EOL;

		if (g_tracks4_8 > 4)
		{	//stereo module
			ou << "STEREO" << EOL;
		}

		if (g_ntsc)
		{	//NTSC module
			ou << "NTSC" << EOL;
		}

		if (m_instrspeed > 1)
		{
			ou << "FASTPLAY ";
			switch (m_instrspeed)
			{
			case 2:
				ou << ((g_ntsc) ? "131" : "156");
				break;

			case 3:
				ou << ((g_ntsc) ? "87" : "104");
				break;

			case 4:
				ou << ((g_ntsc) ? "66" : "78");
				break;

			default:
				ou << ((g_ntsc) ? "262" : "312");
				break;
			}
			ou << EOL;
		}

		//a double EOL is necessary for making the SAP export functional
		ou << EOL;

		//patch: change a JMP [label] to a RTS with 2 NOPs
		unsigned char saprtsnop[3] = { 0x60,0xEA,0xEA };
		for (int i = 0; i < 3; i++) mem[adr_rts_nop + i] = saprtsnop[i];

		//patch: change a $00 to $FF to force the LOOP flag to be infinite
		mem[adr_loop_flag] = 0xFF;

		//SAP initialisation patch, running from address 0x3080 in Atari executable 
		unsigned char sapbytes[14] =
		{
			0x8D,0xE7,0x22,		// STA SongIdx
			0xA2,0x00,			// LDX #0
			0x8E,0x93,0x1B,		// STX is_fadeing_out
			0x8E,0x03,0x1C,		// STX stop_on_fade_end
			0x4C,0x39,0x1C		// JMP SetNewSongPtrsLoopsOnly
		};
		for (int i = 0; i < 14; i++) mem[adr_init_sap + i] = sapbytes[i];

		mem[adr_song_speed] = m_instrspeed;							//song speed
		mem[adr_stereo_flag] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		//is the song stereo?

		//reconstruct the export binary 
		SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	//LZSS Driver, and some free bytes for later if needed
		SaveBinaryBlock(ou, mem, 0x2000, 0x27FF, 0);	//VUPlayer only

		//songstart pointers
		mem[0x3000] = adr_module >> 8;
		mem[0x3001] = lzss_offset >> 8;
		mem[0x3002] = adr_module & 0xFF;
		mem[0x3003] = lzss_offset & 0xFF;

		//songend pointers
		mem[0x3004] = lzss_offset >> 8;
		mem[0x3005] = lzss_end >> 8;
		mem[0x3006] = lzss_offset & 0xFF;
		mem[0x3007] = lzss_end & 0xFF;

		if (intro)
			for (int i = 0; i < intro; i++) { mem[adr_module + i] = buff2[i]; }
		else
			for (int i = 0; i < full; i++) { mem[adr_module + i] = buff1[i]; }
		for (int i = 0; i < loop; i++) { mem[lzss_offset + i] = buff3[i]; }

		//overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
		SaveBinaryBlock(ou, mem, adr_lzss_pointer, lzss_end, 0);
	}
	break;

	//TODO: add more options through dialog box parameters
	case IOTYPE_LZSS_XEX:
	{
		//Just in case
		memset(buff1, 0, 65536);
		memset(buff2, 0, 65536);
		memset(buff3, 0, 65536);

		//Now, create LZSS files using the SAP-R dump created earlier
		ifstream in;

		//full tune playback up to its loop point
		int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
		if (full)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff1, sizeof(buff1))))
				full = (int)in.gcount();
			if (full < 16) full = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		//intro section playback, up to the start of the detected loop point
		int intro = LZSS_SAP((char*)SAPRSTREAM, thirdcount * bytenum);
		if (intro)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff2, sizeof(buff2))))
				intro = (int)in.gcount();
			if (intro < 16) intro = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		//looped section playback, this part is virtually seamless to itself
		int loop = LZSS_SAP((char*)SAPRSTREAM + (firstcount * bytenum), secondcount * bytenum);
		if (loop)
		{
			//load tmp.lzss in destination buffer 
			in.open(g_prgpath + "tmp.lzss", ifstream::binary);
			if (!(in.read((char*)buff3, sizeof(buff3))))
				loop = (int)in.gcount();
			if (loop < 16) loop = 1;
			in.close(); in.clear();
			DeleteFile(g_prgpath + "tmp.lzss");
		}

		GetPokey()->DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

		//some additional variables that will be used below
		adr_module = 0x3100;												//all the LZSS data will be written starting from this address
		lzss_offset = (intro) ? adr_module + intro : adr_module + full;		//calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
		lzss_end = lzss_offset + loop;										//this sets the address that defines where the data stream has reached its end

		//if the size is too big, abort the process and show an error message
		if (lzss_end > 0xBFFF)
		{
			MessageBox(g_hwnd,
				"Error, LZSS data is too big to fit in memory!\n\n"
				"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
				"Error, Buffer Overflow!", MB_ICONERROR);
			return 0;
		}

		int r;
		r = LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, adrfrom, adrto);
		if (!r)
		{
			MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
			return 0;
		}

		CExpMSXDlg dlg;
		s = m_songname;
		s.TrimRight();
		CTime time = CTime::GetCurrentTime();
		if (g_rmtmsxtext != "")
		{
			dlg.m_txt = g_rmtmsxtext;	//same from last time, making repeated exports faster
		}
		else
		{
			dlg.m_txt = s + "\x0d\x0a";
			if (g_tracks4_8 > 4) dlg.m_txt += "STEREO";
			dlg.m_txt += "\x0d\x0a" + time.Format("%d/%m/%Y");
			dlg.m_txt += "\x0d\x0a";
			dlg.m_txt += "Author: (press SHIFT key)\x0d\x0a";
			dlg.m_txt += "Author: ???";
		}
		s = "Playback speed will be adjusted to ";
		s += g_ntsc ? "60" : "50";
		s += "Hz on both PAL and NTSC systems.";
		dlg.m_speedinfo = s;

		if (dlg.DoModal() != IDOK)
		{
			return 0;
		}
		g_rmtmsxtext = dlg.m_txt;
		g_rmtmsxtext.Replace("\x0d\x0d", "\x0d");	//13, 13 => 13

		//this block of code will handle all the user input text that will be inserted in the binary during the export process
		memset(mem + 0x2EBC, 32, 40 * 5);	//5 lines of 40 characters at the user text address
		int p = 0, q = 0;
		char a;
		for (int i = 0; i < dlg.m_txt.GetLength(); i++)
		{
			a = dlg.m_txt.GetAt(i);
			if (a == '\n') { p += 40; q = 0; }
			else
			{
				mem[0x2EBC + p + q] = a;
				q++;
			}
			if (p + q >= 5 * 40) break;
		}
		StrToAtariVideo((char*)mem + 0x2EBC, 200);

		memset(mem + 0x2C0B, 32, 28);	//28 characters on the top line, next to the Region and VBI speed
		char framesdisplay[28] = { 0 };
		sprintf(framesdisplay, "(%i frames)", firstcount);	//total recorded frames
		for (int i = 0; i < 28; i++)
		{
			mem[0x2C0B + i] = framesdisplay[i];
		}
		StrToAtariVideo((char*)mem + 0x2C0B, 28);

		//I know the binary I have is currently set to NTSC, so I'll just convert to PAL and keep this going for now...
		if (!g_ntsc)
		{
			unsigned char regionbytes[18] =
			{
				0xB9,0xE6,0x26,	//LDA tabppPAL-1,y
				0x8D,0x56,0x21,	//STA acpapx2
				0xE0,0x9B,		//CPX #$9B
				0x30,0x05,		//BMI set_ntsc
				0xB9,0xF6,0x26,	//LDA tabppPALfix-1,y
				0xD0,0x03,		//BNE region_done
				0xB9,0x16,0x27	//LDA tabppNTSCfix-1,y
			};
			for (int i = 0; i < 18; i++) mem[adr_region + i] = regionbytes[i];
		}

		//additional patches from the Export Dialog...
		mem[adr_song_speed] = m_instrspeed;										//song speed
		mem[adr_rasterbar] = (dlg.m_meter) ? 0x80 : 0x00;						//display the rasterbar for CPU level
		mem[adr_colour] = dlg.m_metercolor;										//rasterbar colour 
		mem[adr_shuffle] = 0x00;	// = (dlg.m_msx_shuffle) ? 0x10 : 0x00;		//rasterbar colour shuffle, incomplete feature so it is disabled
		mem[adr_stereo_flag] = (g_tracks4_8 > 4) ? 0xFF : 0x00;					//is the song stereo?
		if (!dlg.m_region_auto)													//automatically adjust speed between regions?
			for (int i = 0; i < 4; i++) mem[adr_region + 6 + i] = 0xEA;			//set the 4 bytes to NOPs to disable it
		
		//reconstruct the export binary 
		SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	//LZSS Driver, and some free bytes for later if needed
		SaveBinaryBlock(ou, mem, 0x2000, 0x2FFF, 0);	//VUPlayer + Font + Data + Display Lists

		//set the run address to VUPlayer 
		mem[0x2e0] = 0x2000 & 0xff;
		mem[0x2e1] = 0x2000 >> 8;
		SaveBinaryBlock(ou, mem, 0x2e0, 0x2e1, 0);

		//songstart pointers
		mem[0x3000] = adr_module >> 8;
		mem[0x3001] = lzss_offset >> 8;
		mem[0x3002] = adr_module & 0xFF;
		mem[0x3003] = lzss_offset & 0xFF;

		//songend pointers
		mem[0x3004] = lzss_offset >> 8;
		mem[0x3005] = lzss_end >> 8;
		mem[0x3006] = lzss_offset & 0xFF;
		mem[0x3007] = lzss_end & 0xFF;

		if (intro)
			for (int i = 0; i < intro; i++) { mem[adr_module + i] = buff2[i]; }
		else
			for (int i = 0; i < full; i++) { mem[adr_module + i] = buff1[i]; }
		for (int i = 0; i < loop; i++) { mem[lzss_offset + i] = buff3[i]; }

		//overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
		SaveBinaryBlock(ou, mem, adr_lzss_pointer, lzss_end, 0);
	}
		break;

	default:
		return 0;
	}
	return 1;
}

//******************************************
//IMPORT is in a separate import.cpp file
#include "import.cpp"
//Contains methods:
//int CSong::ImportTMC(ifstream& in)
//int CSong::ImportMOD(ifstream& in);
//
//******************************************


int CSong::LoadRMT(ifstream& in)
{
	unsigned char mem[65536];
	memset(mem,0,65536);
	WORD bfrom,bto;
	WORD bto_mainblock;

	BYTE instrloaded[INSTRSNUM];
	BYTE trackloaded[TRACKSNUM];

	int len,i,j,k;
	
	//RMT header is the first main block of an RMT song

	len = LoadBinaryBlock(in,mem,bfrom,bto);

	if (len>0)
	{
		int r = DecodeModule(mem,bfrom,bto+1,instrloaded,trackloaded);
		if (!r)
		{
			MessageBox(g_hwnd,"Bad RMT data format or old tracker version.","Open error",MB_ICONERROR);
			return 0;
		}
		//the main block of the module is OK => take its boot address
		g_rmtstripped_adr_module = bfrom;
		bto_mainblock = bto;
	}
	else
	{
		MessageBox(g_hwnd,"Corrupted file or unsupported format version.","Open error",MB_ICONERROR);
		return 0;	//did not retrieve any data in the first block
	}

	//RMT - now read the second block with names)
	len = LoadBinaryBlock(in,mem,bfrom,bto);
	if (len<1)
	{
		CString s;
		s.Format("This file appears to be a stripped RMT module.\nThe song and instruments names are missing.\n\nMemory addresses: $%04X - $%04X.",g_rmtstripped_adr_module,bto_mainblock);
		MessageBox(g_hwnd,(LPCTSTR)s,"Info",MB_ICONINFORMATION);
		return 1;
	}

	char a;
	for(j=0; j<SONGNAMEMAXLEN && (a=mem[bfrom+j]); j++)
		m_songname[j]=a;
	
	for(k=j;k<SONGNAMEMAXLEN; k++) m_songname[k]=' '; //fill in the gaps

	int adrinames=bfrom+j+1; //+1 that's the zero behind the name
	for(i=0; i<INSTRSNUM; i++)
	{
		if (instrloaded[i])
		{
			for(j=0; j<INSTRNAMEMAXLEN && (a=mem[adrinames+j]); j++)
				m_instrs.m_instr[i].name[j] = a;

			for(k=j; k<INSTRNAMEMAXLEN; k++) m_instrs.m_instr[i].name[k]=' '; //fill in the gaps
				adrinames += j+1; //+1 is zero behind the name
		}
	}
	return 1;
}


BOOL CSong::ComposeRMTFEATstring(CString& dest, char* filename, BYTE *instrsaved,BYTE* tracksaved, BOOL sfx, BOOL gvf, BOOL nos)
{

#define DEST(var,str)	s.Format("%s\t\tequ %i\t\t;(%i times)\n",str,(var>0),var); dest+=s;

	dest.Format(";* --------BEGIN--------\n;* %s\n",filename);
	//
	int usedcmd[8]={0,0,0,0,0,0,0,0};
	int usedcmd7volumeonly=0;
	int usedcmd7volumeonlyongx[8]={0,0,0,0,0,0,0,0};
	int usedcmd7setnote=0;
	int usedportamento=0;
	int usedfilter=0;
	int usedfilterongx[8]={0,0,0,0,0,0,0,0};
	int usedbass16=0;
	int usedbass16ongx[8]={0,0,0,0,0,0,0,0};
	int usedtabtype=0;
	int usedtabmode=0;
	int usedtablego=0;
	int usedaudctlmanualset=0;
	int usedvolumemin=0;
	int usedeffectvibrato=0;
	int usedeffectfshift=0;
	int speedchanges=0;
	int i,j,g,tr;
	CString s;

	//analysis of which instruments are used on which channels and if there is a change in speed
	int instrongx[INSTRSNUM][SONGTRACKS];
	memset(&instrongx,0,INSTRSNUM*SONGTRACKS*sizeof(instrongx[0][0]));
	//test the song
	for(int sl=0; sl<SONGLEN; sl++)
	{
		if (m_songgo[sl]>=0) continue;	//goto line
		for(g=0; g<g_tracks4_8; g++)
		{
			tr=m_song[sl][g];
			if (tr<0 || tr>=TRACKSNUM) continue;
			TTrack* tt=m_tracks.GetTrack(tr);
			for(i=0; i<tt->len; i++)
			{
				int inum = tt->instr[i];
				if (inum>=0 && inum<INSTRSNUM) instrongx[inum][g]++;
				int chsp = tt->speed[i];
				if (chsp>=0) speedchanges++;
			}
		}
	}

	//analyse the individual instruments and what they use
	for(i=0; i<INSTRSNUM; i++)
	{
		if (instrsaved[i])
		{
			TInstrument& ai=m_instrs.m_instr[i];
			//commands
			for(j=0; j<=ai.par[PAR_ENVLEN]; j++)
			{
				int cmd = ai.env[j][ENV_COMMAND] & 0x07;
				usedcmd[cmd]++;
				if (cmd==7)
				{
					if (ai.env[j][ENV_X]==0x08 && ai.env[j][ENV_Y]==0x00)
					{
						usedcmd7volumeonly++;
						for (g=0; g<g_tracks4_8; g++) { if (instrongx[i][g]) usedcmd7volumeonlyongx[g]++; }
					}
					else
						usedcmd7setnote++;
				}
				//portamento
				if (ai.env[j][ENV_PORTAMENTO]) usedportamento++;
				//filter
				if (ai.env[j][ENV_FILTER])
				{
					usedfilter++;
					for (g=0; g<g_tracks4_8; g++) { if (instrongx[i][g]) usedfilterongx[g]++; }
				}
			
				//bass16
				if (ai.env[j][ENV_DISTORTION]==6)
				{
					usedbass16++;
					for (g=0; g<g_tracks4_8; g++) { if (instrongx[i][g]) usedbass16ongx[g]++; }
				}
				
			}
			//table type
			if (ai.par[PAR_TABTYPE]) usedtabtype++;
			//table mode
			if (ai.par[PAR_TABMODE]) usedtabmode++;
			//table go
			if (ai.par[PAR_TABGO]) usedtablego++;	//non-zero table go
			//audctl manual set
			if (   ai.par[PAR_AUDCTL0]
				|| ai.par[PAR_AUDCTL1]
				|| ai.par[PAR_AUDCTL2]
				|| ai.par[PAR_AUDCTL3]
				|| ai.par[PAR_AUDCTL4]
				|| ai.par[PAR_AUDCTL5]
				|| ai.par[PAR_AUDCTL6]
				|| ai.par[PAR_AUDCTL7] ) usedaudctlmanualset++;
			//volume mininum
			if (ai.par[PAR_VMIN]) usedvolumemin++;
			//effect vibrato and fshift
			if (ai.par[PAR_DELAY]) //only when the effect delay is nonzero
			{
				if (ai.par[PAR_VIBRATO]) usedeffectvibrato++;
				if (ai.par[PAR_FSHIFT]) usedeffectfshift++;
			}
		}
	}

	//generate strings

	s.Format("FEAT_SFX\t\tequ %u\n",sfx); dest+=s;

	s.Format("FEAT_GLOBALVOLUMEFADE\tequ %u\t\t;RMTGLOBALVOLUMEFADE variable\n",gvf); dest+=s;

	s.Format("FEAT_NOSTARTINGSONGLINE\tequ %u\n",nos); dest+=s;

	s.Format("FEAT_INSTRSPEED\t\tequ %i\n",m_instrspeed); dest+=s;

	s.Format("FEAT_CONSTANTSPEED\t\tequ %i\t\t;(%i times)\n",(speedchanges==0)? m_mainspeed : 0,speedchanges); dest+=s;

	//commands 1-6
	for(i=1; i<=6; i++)
	{
		s.Format("FEAT_COMMAND%i\t\tequ %i\t\t;(%i times)\n",i,(usedcmd[i]>0),usedcmd[i]);
		dest+=s;
	}

	//command 7
	DEST(usedcmd7setnote,"FEAT_COMMAND7SETNOTE");
	DEST(usedcmd7volumeonly,"FEAT_COMMAND7VOLUMEONLY");

	//
	DEST(usedportamento,"FEAT_PORTAMENTO");
	//
	DEST(usedfilter,"FEAT_FILTER");
	DEST(usedfilterongx[0],"FEAT_FILTERG0L");
	DEST(usedfilterongx[1],"FEAT_FILTERG1L");
	DEST(usedfilterongx[0+4],"FEAT_FILTERG0R");
	DEST(usedfilterongx[1+4],"FEAT_FILTERG1R");
	//
	DEST(usedbass16,"FEAT_BASS16");
	DEST(usedbass16ongx[1],"FEAT_BASS16G1L");
	DEST(usedbass16ongx[3],"FEAT_BASS16G3L");
	DEST(usedbass16ongx[1+4],"FEAT_BASS16G1R");
	DEST(usedbass16ongx[3+4],"FEAT_BASS16G3R");
	//
	DEST(usedcmd7volumeonlyongx[0],"FEAT_VOLUMEONLYG0L");
	DEST(usedcmd7volumeonlyongx[2],"FEAT_VOLUMEONLYG2L");
	DEST(usedcmd7volumeonlyongx[3],"FEAT_VOLUMEONLYG3L");
	DEST(usedcmd7volumeonlyongx[0+4],"FEAT_VOLUMEONLYG0R");
	DEST(usedcmd7volumeonlyongx[2+4],"FEAT_VOLUMEONLYG2R");
	DEST(usedcmd7volumeonlyongx[3+4],"FEAT_VOLUMEONLYG3R");
	//
	DEST(usedtabtype,"FEAT_TABLETYPE");
	DEST(usedtabmode,"FEAT_TABLEMODE");
	DEST(usedtablego,"FEAT_TABLEGO");
	DEST(usedaudctlmanualset,"FEAT_AUDCTLMANUALSET");
	DEST(usedvolumemin,"FEAT_VOLUMEMIN");

	DEST(usedeffectvibrato,"FEAT_EFFECTVIBRATO");
	DEST(usedeffectfshift,"FEAT_EFFECTFSHIFT");

	dest+= ";* --------END--------\n";
	dest.Replace("\n","\x0d\x0a");
	return 1;
}


void CSong::MarkTF_USED(BYTE* arrayTRACKSNUM)
{
	int i,tr;
	//all tracks used in the song
	for(i=0; i<SONGLEN; i++)
	{
		if (m_songgo[i]<0)
		{
			for(int j=0; j<g_tracks4_8; j++)
			{
				tr = m_song[i][j];
				if (tr>=0 && tr<TRACKSNUM) arrayTRACKSNUM[tr] =TF_USED;
			}
		}
	}
}

void CSong::MarkTF_NOEMPTY(BYTE* arrayTRACKSNUM)
{
	for(int i=0; i<TRACKSNUM; i++)
	{
		if (m_tracks.CalculateNoEmpty(i)) arrayTRACKSNUM[i] |= TF_NOEMPTY;
	}
}

int CSong::MakeModule(unsigned char* mem,int adr,int iotype,BYTE *instrsaved,BYTE* tracksaved)
{
	//returns maxadr (points to the first free address after the module) and sets the instrsaved and tracksaved fields
	if (iotype==IOTYPE_RMF) return MakeRMFModule(mem,adr,instrsaved,tracksaved);

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;
	memset(instrsave,0,INSTRSNUM);	//init
	memset(tracksave,0,TRACKSNUM); //init

	strncpy((char*)(mem+adr),"RMT",3);
	mem[adr+3]=g_tracks4_8+'0';	//4 or 8
	mem[adr+4]=m_tracks.m_maxtracklen & 0xff;
	mem[adr+5]=m_mainspeed & 0xff;
	mem[adr+6]=m_instrspeed;		//instr speed 1-4
	mem[adr+7]=RMTFORMATVERSION;	//RMT format version number

	//in RMT all non-empty tracks and non-empty instruments will be stored in others only non-empty used tracks and used instruments in them
	int i,j;

	//all tracks used in the song
	MarkTF_USED(tracksave);

	if (iotype==IOTYPE_RMT)
	{
		//In addition to the used ones, all non-empty tracks are added to the RMT, all non-empty tracks
		MarkTF_NOEMPTY(tracksave);
	}

	//all instruments in the tracks that will be saved
	for(i=0; i<TRACKSNUM; i++)
	{
		if (tracksave[i]>0)
		{
			TTrack& tr = *m_tracks.GetTrack(i);
			for(j=0; j<tr.len; j++)
			{
				int ins = tr.instr[j];
				if (ins>=0 && ins<INSTRSNUM) instrsave[ins] = IF_USED;
			}
		}
	}

	if (iotype==IOTYPE_RMT)
	{
		//in addition to the instruments used in the tracks that are in the song, all non-empty instruments are stored in the RMT
		for(i=0; i<INSTRSNUM; i++)
		{
			if (m_instrs.CalculateNoEmpty(i)) instrsave[i] |= IF_NOEMPTY;
		}
	}

	//---

	int numtracks=0;
	for (i=TRACKSNUM-1; i>=0; i--)
	{
		if (tracksave[i]>0) { numtracks=i+1; break; }
	}

	int numinstrs=0;
	for (i=INSTRSNUM-1; i>=0; i--)
	{
		if (instrsave[i]>0) { numinstrs=i+1; break; }
	}

	//and now save:
	//instruments
	//tracks
	//songlines

	int adrpinstruments = adr + 16;
	int adrptrackslbs = adrpinstruments + numinstrs*2;
	int adrptrackshbs = adrptrackslbs + numtracks;

	int adrinstrdata = adrptrackshbs + numtracks; //behind the track byte table
	//saves instrument data and writes their beginnings to the table
	for(i=0; i<numinstrs; i++)
	{
		if (instrsave[i])
		{
			int leninstr = m_instrs.InstrToAta(i,mem+adrinstrdata,MAXATAINSTRLEN);
			mem[adrpinstruments+i*2] = adrinstrdata & 0xff;	//dbyte
			mem[adrpinstruments+i*2+1] = adrinstrdata >> 8;	//hbyte
			adrinstrdata +=leninstr;
		}
		else
		{
			mem[adrpinstruments+i*2] = mem[adrpinstruments+i*2+1] = 0;
		}
	}

	int adrtrackdata = adrinstrdata;	//for instrument data
	//saves track data and writes their beginnings to the table
	for(i=0; i<numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = m_tracks.TrackToAta(i,mem+adrtrackdata,MAXATATRACKLEN);
			if (lentrack<1)
			{	//cannot be saved to RMT
				CString msg;
				msg.Format("Fatal error in track %02X.\n\nThis track contains too many events (notes and speed commands),\nthat's why it can't be coded to RMT internal code format.",i);
				MessageBox(g_hwnd,msg,"Internal format problem.",MB_ICONERROR);
				return -1;
			}
			mem[adrptrackslbs+i] = adrtrackdata & 0xff;	//dbyte
			mem[adrptrackshbs+i] = adrtrackdata >> 8;	//hbyte
			adrtrackdata +=lentrack;
		}
		else
		{
			mem[adrptrackslbs+i] = mem[adrptrackshbs+i] = 0;
		}
	}

	int adrsong = adrtrackdata;		//for track data

	//save from adrsong data song
	//int lensong = SongToAta(mem+adrsong,g_tracks4_8*SONGLEN,adrsong);  //<---COARSE WITH MAX BUFFER SIZE!
	int lensong = SongToAta(mem+adrsong,0x10000-adrsong,adrsong);

	int endofmodule = adrsong+lensong;

	//writes computed pointers to the header
	mem[adr+8] = adrpinstruments & 0xff;	//dbyte
	mem[adr+9] = adrpinstruments >> 8;		//hbyte
	//
	mem[adr+10] = adrptrackslbs & 0xff;		//dbyte
	mem[adr+11] = adrptrackslbs >> 8;		//hbyte
	mem[adr+12] = adrptrackshbs & 0xff;		//dbyte
	mem[adr+13] = adrptrackshbs >> 8;		//hbyte
	//
	mem[adr+14] = adrsong & 0xff;		//dbyte
	mem[adr+15] = adrsong >> 8;			//hbyte

	return endofmodule;
}

int CSong::MakeRMFModule(unsigned char* mem,int adr,BYTE *instrsaved,BYTE* tracksaved)
{
	//returns maxadr (points to the first available address behind the module) and sets the instrsaved and tracksaved fields

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;

	memset(instrsave,0,INSTRSNUM);	//init
	memset(tracksave,0,TRACKSNUM); //init

	mem[adr+0]=m_instrspeed;		//instr speed 1-4
	mem[adr+1]=m_mainspeed & 0xff;

	//all non-empty tracks and non-empty instruments will be stored in the RMT, in others only non-empty tracks and instruments used in them will be stored
	int i,j;

	//all tracks used in the song
	MarkTF_USED(tracksave);

	//all instruments in the tracks that will be saved
	for(i=0; i<TRACKSNUM; i++)
	{
		if (tracksave[i]>0)
		{
			TTrack& tr = *m_tracks.GetTrack(i);
			for(j=0; j<tr.len; j++)
			{
				int ins = tr.instr[j];
				if (ins>=0 && ins<INSTRSNUM) instrsave[ins] = IF_USED;
			}
		}
	}

	//---

	int numtracks=0;
	for (i=TRACKSNUM-1; i>=0; i--)
	{
		if (tracksave[i]>0) { numtracks=i+1; break; }
	}

	int numinstrs=0;
	for (i=INSTRSNUM-1; i>=0; i--)
	{
		if (instrsave[i]>0) { numinstrs=i+1; break; }
	}

	//and now save:
	//songlines
	//instruments
	//tracks 

	//Find out the length of a song and the shortest lengths of individual tracks in songlines
	int songlines=0,a;
	int emptytrackusedinline=-1;
	BOOL emptytrackused=0;
	int songline_trackslen[SONGLEN];
	for(i=0; i<SONGLEN; i++)
	{
		int minlen = m_tracks.m_maxtracklen;	//init
		if (m_songgo[i]>=0)  //Go to line 
		{
			songlines=i+1;	//temporary end
			if (emptytrackusedinline>=0) emptytrackused=1;
		}
		else
		{
			for(j=0; j<g_tracks4_8; j++)
			{
				a = m_song[i][j];
				if (a>=0 && a<TRACKSNUM)
				{
					songlines=i+1;	//temporary end
					int tl = m_tracks.GetLength(a);
					if (tl<minlen) minlen = tl; //is less than the shortest
				}
				else
					emptytrackusedinline=i;
			}
		}
		songline_trackslen[i]= minlen;
	}
	int lensong = (songlines-1) * (g_tracks4_8*2+3);	//songlines-1, because the last GOTO does not have to be put there

	//stores instrument data and writes their beginnings to the table in the temporary memory meminstruments with respect to the beginning 0
	unsigned char meminstruments[65536];
	memset(meminstruments,0,65536);
	int adrinstrdata=numinstrs*2;
	for(i=0; i<numinstrs; i++)
	{
		if (instrsave[i])
		{
			int leninstr = m_instrs.InstrToAtaRMF(i,meminstruments+adrinstrdata,MAXATAINSTRLEN);
			meminstruments[i*2] = adrinstrdata & 0xff;	//dbyte
			meminstruments[i*2+1] = adrinstrdata >> 8;	//hbyte
			adrinstrdata +=leninstr;
		}
	}
	int leninstruments = adrinstrdata;

	//saves track data and writes their beginnings to a table in the memtracks temporary memory due to the beginning 0
	unsigned char memtracks[65536];
	WORD trackpointers[TRACKSNUM];
	memset(memtracks,0,65536);
	memset(trackpointers,0,TRACKSNUM*2);
	int adrtrackdata = 0;	//from the beginning
	//empty track
	int adremptytrack=0;
	if (emptytrackused)
	{
		//memtracks[0]=62;	//pause
		//memtracks[1]=0;		//infinite
		memtracks[0]=255;	//infinite pause (FASTER modification)
		adrtrackdata += 1; //for empty track
	}
	for(i=0; i<numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = m_tracks.TrackToAtaRMF(i,memtracks+adrtrackdata,MAXATATRACKLEN);
			if (lentrack<1)
			{	//cannot be saved to RMT
				CString msg;
				msg.Format("Fatal error in track %02X.\n\nThis track contains too many events (notes and speed commands),\nthat's why it can't be coded to RMT internal code format.",i);
				MessageBox(g_hwnd,msg,"Internal format problem.",MB_ICONERROR);
				return -1;
			}
			trackpointers[i] = adrtrackdata;
			adrtrackdata +=lentrack;
		}
	}
	int lentracks = adrtrackdata;

	int adrsong = adr+6;
	int adrinstruments = adrsong + lensong;
	int adrtracks = adrinstruments + leninstruments;
	int endofmodule = adrtracks + lentracks;

	//Construct the Song now
	int apos,go;
	for(int sline=0; sline<songlines; sline++)
	{
		apos=adrsong+sline*(g_tracks4_8*2+3);
		if ( (go=m_songgo[sline])>=0)
		{
			//thee is a goto line
			//=> to songline by 1 above, this changes GO nextline to GO somewhere else
			//
			if (apos>=0) //GOTO songline 0
			{
				WORD goadr = adrsong + (go*(g_tracks4_8*2+3));
				mem[apos-2] = goadr & 0xff;		//low byte
				mem[apos-1] = (goadr>>8);		//high byte
			}
			for(int j=0; j<g_tracks4_8*2+3; j++) mem[apos+j]=255; //just to make sure
			mem[apos+g_tracks4_8*2+2]=go;
		}
		else
		{
			//there are track numbers
			for(int i=0; i<g_tracks4_8; i++)
			{
				j = m_song[sline][i];
				WORD at=0;
				if (j>=0 && j<TRACKSNUM)
					at = trackpointers[j] + adrtracks;
				else
					at = adremptytrack + adrtracks;	//empty track --
				mem[apos+i]= at & 0xff;					//db
				mem[apos+i+g_tracks4_8] = (at >> 8);	//hb

				mem[apos+g_tracks4_8*2] = songline_trackslen[sline]; //maxtracklen for this songline
				WORD nextsongline = apos+ g_tracks4_8*2+3;
				mem[apos+g_tracks4_8*2+1] = nextsongline & 0xff;	//db
				mem[apos+g_tracks4_8*2+2] = (nextsongline >> 8);	//hb
			}
		}
	}

	//INSTRUMENTS cast (instrument pointers table and instruments data) add an offset in the pointer table
	for(i=0; i<numinstrs; i++)
	{
		WORD ai = meminstruments[i*2] + (meminstruments[i*2+1]<<8) + adrinstruments;
		meminstruments[i*2] = ai & 0xff;
		meminstruments[i*2+1] = (ai >> 8);
	}
	//write the instrument table pointers and instrument data
	memcpy(mem+adrinstruments,meminstruments,leninstruments);

	//TRACKS cast
	memcpy(mem+adrtracks,memtracks,lentracks);

	//writes the calculated pointers to the header
	mem[adr+2] = adrsong & 0xff;	//dbyte
	mem[adr+3] = (adrsong >> 8);		//hbyte
	//
	mem[adr+4] = adrinstruments & 0xff;		//dbyte
	mem[adr+5] = (adrinstruments >> 8);		//hbyte

	return endofmodule;
}


int CSong::DecodeModule(unsigned char* mem,int adrfrom,int adrend,BYTE *instrloaded,BYTE* trackloaded)
{
	int adr=adrfrom;

	memset(instrloaded,0,INSTRSNUM);
	memset(trackloaded,0,TRACKSNUM);

	unsigned char b;
	int i,j;

	if (strncmp((char*)(mem+adr),"RMT",3)!=0) return 0; //there is no RMT
	b = mem[adr+3];
	if (b!='4' && b!='8') return 0;	//it is not RMT4 or RMT8
	g_tracks4_8 = b & 0x0f;
	b = mem[adr+4];
	m_tracks.m_maxtracklen = (b>0)? b:256;	//0 => 256
	g_cursoractview = m_tracks.m_maxtracklen/2;
	b = mem[adr+5];
	m_mainspeed = b;
	if (b<1) return 0;		//there can be no zero speed
	b = mem[adr+6];
	if (b<1 || b>8) return 0;		//instrument speed is less than 1 or greater than 8 (note: should be max 4, but allows up to 8 and will only display a warning)
	m_instrspeed = b;
	int version = mem[adr+7];
	if (version > RMTFORMATVERSION)	return 0;	//the byte version is above the current one

	//Now m_tracks.m_maxtracklen is set to the value according to the header from the RMT module, 
	//so they have to re-initialize the Tracks so that this value is set to them all as the length
	m_tracks.InitTracks();

	int adrpinstruments = mem[adr+ 8] + (mem[adr+ 9]<<8);
	int adrptrackslbs   = mem[adr+10] + (mem[adr+11]<<8);
	int adrptrackshbs   = mem[adr+12] + (mem[adr+13]<<8);
	int adrsong		    = mem[adr+14] + (mem[adr+15]<<8);

	int numinstrs = (adrptrackslbs-adrpinstruments)/2;
	int numtracks = (adrptrackshbs-adrptrackslbs);
	int lensong = adrend - adrsong;

	//decoding of individual instruments
	for(i=0; i<numinstrs; i++)
	{
		int instrdata = mem[adrpinstruments+i*2] + (mem[adrpinstruments+i*2+1]<<8);
		if (instrdata==0) continue; //the omitted instruments have the pointer db, hb = 0
		//othwewise it has a non-zero pointer
		BOOL r;
		if (version==0)
			r=m_instrs.AtaV0ToInstr(mem+instrdata,i);
		else
			r=m_instrs.AtaToInstr(mem+instrdata,i);
		m_instrs.ModificationInstrument(i);	//writes to Atari ram
		if (!r) return 0; //some problem with the instrument => END
		instrloaded[i]=1;
	}

	//decoding individual tracks
	for(i=0; i<numtracks; i++)
	{
		int track=i;
		int trackdata = mem[adrptrackslbs+i] + (mem[adrptrackshbs+i]<<8);
		if (trackdata==0) continue; //omitted tracks have pointer db, hb = 0

		//identify the end of the track by the address of the next track and at the last by the address of the song that follows the data of the last track
		int trackend=0;
		for(j=i; j<numtracks; j++)
		{
			trackend = (j+1==numtracks)? adrsong : mem[adrptrackslbs+j+1] + (mem[adrptrackshbs+j+1]<<8);
			if (trackend!=0) break;
			i++;	//continue from the next and skip the omitted one
		}
		int tracklen = trackend - trackdata;
		//
		BOOL r;
		r=m_tracks.AtaToTrack(mem+trackdata,tracklen,track);
		if (!r) return 0; //some problem with the track => END
		trackloaded[track]=1;
	}

	//decoded song
	BOOL r;
	r=AtaToSong(mem+adrsong,lensong,adrsong);
	if (!r) return 0; //some problem with the song => END

	return 1;
}

//---

BOOL CSong::PlayPressedTonesInit()
{
	for(int t=0; t<SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t,-1,-1,-1);
	return 1;
}

BOOL CSong::SetPlayPressedTonesSilence()
{
	for(int t=0; t<SONGTRACKS; t++)
		SetPlayPressedTonesTNIV(t,-1,-1,0);
	return 1;
}

BOOL CSong::PlayPressedTones()
{
	int t,n,i,v;
	for(t=0; t<SONGTRACKS; t++)
	{
		if ( (v=m_playptvolume[t])>=0) //volume is set last
		{
			n=m_playptnote[t];
			i=m_playptinstr[t];
			if (n>=0 && i>=0)
				Atari_SetTrack_NoteInstrVolume(t,n,i,v);
			else
				Atari_SetTrack_Volume(t,v);
			SetPlayPressedTonesTNIV(t,-1,-1,-1);
		}
	}
	return 1;
}

BOOL CSong::DrawSong()
{
	int line,i,j,k,y,t;
	char s[32],color;
	int sp = g_scaling_percentage;

	int MINIMAL_WIDTH_TRACKS = (g_tracks4_8 > 4 && g_active_ti == 1) ? 1420 : 960;
	int MINIMAL_WIDTH_INSTRUMENTS = (g_tracks4_8 > 4 && g_active_ti == 2) ? 1220 : 1220;
	int WINDOW_OFFSET = (g_width < 1320 && g_tracks4_8 > 4 && g_active_ti == 1) ? -250 : 0;	//test displacement with the window size
	int INSTRUMENT_OFFSET = (g_active_ti == 2 && g_tracks4_8 > 4) ? -250 : 0;
	if (g_tracks4_8 == 4 && g_active_ti == 2 && g_width > MINIMAL_WIDTH_INSTRUMENTS - 220) INSTRUMENT_OFFSET = 260;
	int SONG_OFFSET = SONG_X + WINDOW_OFFSET + INSTRUMENT_OFFSET + ((g_tracks4_8 == 4) ? -200 : 310);	//displace the SONG block depending on certain parameters

	TextXY("SONG", SONG_OFFSET + 8, SONG_Y, TEXT_COLOR_WHITE);

	//print L1 .. L4 R1 .. R4 with highlighted current track
	k=SONG_OFFSET+6*8;
	s[0]='L';
	s[2]=0;
	for(i=0; i<4; i++,k+=24)
	{
		s[1]=i+49;	//character 1-4
		if (GetChannelOnOff(i))
		{
			if (m_trackactivecol == i) color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;	//active channel highlight
			else color = TEXT_COLOR_WHITE; //normal channel
		}
		else color = TEXT_COLOR_GRAY; //switched off channels are in gray
		TextXY(s, k, SONG_Y, color);
	}
	s[0]='R';
	for(i=4; i<g_tracks4_8; i++,k+=24)
	{
		s[1]=i+49-4;	//character 1-4
		if (GetChannelOnOff(i))
		{
			if (m_trackactivecol==i) color = (g_prove) ? 13 : 6;	//active channel highlight
			else color=0; //normal channel
		}
		else color=1; //switched off channels are in gray
		TextXY(s, k, SONG_Y, color);
	}

	y=SONG_Y+16;
	int linescount = (WINDOW_OFFSET) ? 5 : 9;

	for (i = 0; i < linescount; i++, y += 16)
	{
		int linesoffset = (WINDOW_OFFSET) ? -2 : -4;
		line = m_songactiveline + i + linesoffset;

		if (line<0 || line>255) continue;
		strcpy(s,"XX:");
		s[0]= CharH4(line);
		s[1]= CharL4(line);
		color = (line==m_songplayline)? 2:0;
		TextXY(s, SONG_OFFSET + 16, y, color);

		if ((j=m_songgo[line])>=0)	//there is a GO to line
		{
			s[0]=CharH4(j);
			s[1]=CharL4(j);
			s[2]=0;
			TextXY("Go\x1fto\x1fline", SONG_OFFSET + 16, y, TEXT_COLOR_LIGHT_GRAY);	//turquoise text, blank tiles to mask text if needed
			color = TEXT_COLOR_WHITE;	//white, for the number used
			if (line == m_songactiveline)
			{
				if (g_prove) color = (g_activepart == PARTSONG) ? COLOR_SELECTED_PROVE : TEXT_COLOR_BLUE;
				else color = (g_activepart == PARTSONG) ? COLOR_SELECTED : TEXT_COLOR_RED;
			}
			TextXY(s, SONG_OFFSET + 16 + 11 * 8, y, color);
		}
		else	//there are track numbers
		{
			s[2]=0;
			for (j=0,k=32; j<g_tracks4_8; j++,k+=24)
			{
				if ( (t=m_song[line][j]) >=0 )
				{
					s[0]= CharH4(t);
					s[1]= CharL4(t);
				}
				else s[0]=s[1]='-';	//--
				if (line==m_songactiveline && j==m_trackactivecol)
				{
					if (g_prove) color = (g_activepart == PARTSONG) ? COLOR_SELECTED_PROVE : TEXT_COLOR_BLUE;
					else color = (g_activepart == PARTSONG) ? COLOR_SELECTED : TEXT_COLOR_RED;
				}
				else color = (line == m_songplayline) ? TEXT_COLOR_YELLOW : TEXT_COLOR_WHITE;
				TextXY(s, SONG_OFFSET + 16 + k, y, color);
			}
		}
	}
	color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;
	int arrowpos = (WINDOW_OFFSET) ? SONG_Y + 48 : SONG_Y + 80;
	TextXY("\x04\x05", SONG_OFFSET, arrowpos, color);	//arrow on current song line

	if (g_tracks4_8 > 4)	//a line delimiting the boundary between left/right
	{
		int fl, tl;
		int LINE = m_songactiveline;
		int SEPARATION = SONG_Y + 80 + 5 * 8 + 3;
		int INITIAL_LINE = (WINDOW_OFFSET) ? 64 : 96;
		int TOTAL_LINES = (WINDOW_OFFSET) ? 253 : 251;
		fl = INITIAL_LINE - m_songactiveline * 16;
		if (fl < 32) fl = 32;
		tl = 32 + linescount * 16; 
		if (LINE > TOTAL_LINES) tl = 32 + linescount * 16 - (LINE - TOTAL_LINES) * 16;
		g_mem_dc->MoveTo(((SONG_OFFSET + SEPARATION) * sp) / 100, (fl * sp) / 100);
		g_mem_dc->LineTo(((SONG_OFFSET + SEPARATION) * sp) / 100, (tl * sp) / 100);
	}
	return 1;
}

BOOL CSong::DrawTracks()
{
	static char* tnames="L1L2L3L4R1R2R3R4";
	char s[16],stmp[16];
	int i,x,y,tr,line,color;
	int t;
	int sp = g_scaling_percentage;

	if (SongGetGo()>=0)		//it's a GOTO line, it won't draw tracks
	{
		int TRACKS_OFFSET = (g_tracks4_8 == 8) ? 62 : 30;
		TextXY("Go to line ", TRACKS_X + TRACKS_OFFSET * 8, TRACKS_Y + 8 * 16, TEXT_COLOR_LIGHT_GRAY);
		if (g_prove) color = (g_activepart == PARTTRACKS) ? COLOR_SELECTED_PROVE : TEXT_COLOR_BLUE;
		else color = (g_activepart == PARTTRACKS) ? COLOR_SELECTED : TEXT_COLOR_RED;
		sprintf(s,"%02X",SongGetGo());
		TextXY(s, TRACKS_X + TRACKS_OFFSET * 8 + 11 * 8, TRACKS_Y + 8 * 16, color);
		return 1;
	}

	y = TRACKS_Y + 3 * 16;

	//the cursor position is alway centered regardless of the window size with this simple formula
	g_cursoractview = ((m_trackactiveline + 8) - (g_tracklines / 2));

	strcpy(s,"--‚");

	for (i = 0; i < g_tracklines; i++, y += 16)
	{
		line = g_cursoractview + i - 8;		//8 lines from above
		if (line<0 || line>=m_tracks.m_maxtracklen)
		{
			continue;
		}
		if (g_tracklinealtnumbering)
		{
			GetTracklineText(stmp,line);
			s[0]=(stmp[1]=='1')?stmp[0]:' ';
			s[1]=stmp[1];
		}
		else
		{
			s[0]=CharH4(line);
			s[1]=CharL4(line);
		}
		if (line == m_trackactiveline) color = (g_prove) ? TEXT_COLOR_BLUE : TEXT_COLOR_RED;	//red or blue
		else if (line == m_trackplayline) color = TEXT_COLOR_YELLOW;
		else if ((line % g_tracklinehighlight) == 0) color = TEXT_COLOR_CYAN;
		else color = TEXT_COLOR_WHITE;
		TextXY(s, TRACKS_X, y, color);
	}

	//tracks
	strcpy(s, "  TRACK XX   ");
	x = TRACKS_X+5*8;
	for (i = 0; i < g_tracks4_8; i++, x += 16 * 8)
	{
		s[8] = tnames[i * 2];
		s[9] = tnames[i * 2 + 1];

		color = (GetChannelOnOff(i)) ? TEXT_COLOR_WHITE : TEXT_COLOR_GRAY;	//channels off are in gray
		TextXY(s, x + 12, TRACKS_Y, color);
		//track in the current line of the song
		tr = m_song[m_songactiveline][i];
		
		//is it playing?
		if (m_songplayline == m_songactiveline) t = m_trackplayline; else t = -1;
		m_tracks.DrawTrack(i, x, TRACKS_Y + 16, tr, g_tracklines, m_trackactiveline, g_cursoractview, t, (m_trackactivecol == i), m_trackactivecur);
	}

	//selected block
	if (g_trackcl.IsBlockSelected())
	{
		x = TRACKS_X + 6 * 8 + g_trackcl.m_selcol * 16 * 8 - 8;
		int xt = x + 14 * 8 + 8;

		y = TRACKS_Y+16*3;
		int bfro,bto;
		g_trackcl.GetFromTo(bfro,bto);

		int yf = bfro-g_cursoractview+8;
		int yt = bto-g_cursoractview+8+1;
		int p1=1,p2=1;
		if (yf<0)  { yf=0; p1=0; }

		if (yt > g_tracklines) { yt = g_tracklines; p2 = 0; }
		if (yf < g_tracklines && yt>0

			&& g_trackcl.m_seltrack==SongGetActiveTrackInColumn(g_trackcl.m_selcol)
			&& g_trackcl.m_selsongline==SongGetActiveLine())
		{
			//a rectangle delimiting the selected block
			CPen redpen(PS_SOLID,1,RGB(255,255,255));
			CPen* origpen = g_mem_dc->SelectObject(&redpen);

			g_mem_dc->MoveTo((x * sp) / 100, ((y + yf * 16) * sp) / 100);
			g_mem_dc->LineTo((x * sp) / 100, ((y + yt * 16) * sp) / 100);
			g_mem_dc->MoveTo((xt* sp) / 100, ((y + yf * 16)* sp) / 100);
			g_mem_dc->LineTo((xt* sp) / 100, ((y + yt * 16)* sp) / 100);

			if (p1) { g_mem_dc->MoveTo((x * sp) / 100, ((y + yf * 16) * sp) / 100); g_mem_dc->LineTo((xt * sp) / 100, ((y + yf * 16) * sp) / 100); }
			if (p2) { g_mem_dc->MoveTo((x * sp) / 100, ((y + yt * 16) * sp) / 100); g_mem_dc->LineTo(((xt + 1) * sp) / 100, ((y + yt * 16) * sp) / 100); }

			g_mem_dc->SelectObject(origpen);
		}
		char tx[96];
		char s1[4],s2[4];
		GetTracklineText(s1,bfro);
		GetTracklineText(s2,bto);
		sprintf(tx,"%i line(s) [%s-%s] selected in the pattern track %02X",bto-bfro+1,s1,s2,g_trackcl.m_seltrack);
		TextXY(tx, TRACKS_X + 4 * 8, TRACKS_Y + (4 + g_tracklines) * 16, TEXT_COLOR_WHITE);
		x = TRACKS_X+4*8 + strlen(tx)*8 +8;
		if (g_trackcl.m_all)
			strcpy(tx,"[edit ALL data]");
		else
			sprintf(tx,"[edit data ONLY for instrument %02X]",m_activeinstr);
		TextXY(tx, x, TRACKS_Y + (4 + g_tracklines) * 16, TEXT_COLOR_RED);
	}

	//lines delimiting the current line
	x = (g_tracks4_8 == 4) ? TRACKS_X + (93 - 4 * 11) * 11 - 4 : TRACKS_X + (93 + 3) * 11 - 8;
	y = TRACKS_Y + 3 * 16 - 2 + (m_trackactiveline - g_cursoractview + 8) * 16;

	g_mem_dc->MoveTo((TRACKS_X * sp) / 100, (y * sp) / 100);
	g_mem_dc->LineTo((x * sp) / 100, (y * sp) / 100);
	g_mem_dc->MoveTo((TRACKS_X * sp) / 100, ((y + 19) * sp) / 100);
	g_mem_dc->LineTo((x * sp) / 100, ((y + 19) * sp) / 100);

	//a line delimiting the boundary between left/right
	if (g_tracks4_8>4)
	{
		int fl,tl;
		fl=8-g_cursoractview; if (fl<0) fl=0;
		 		
		tl = 8 - g_cursoractview + m_tracks.m_maxtracklen; if (tl > g_tracklines) tl = g_tracklines;

		g_mem_dc->MoveTo(((TRACKS_X + 50 * 11 - 3)* sp) / 100, ((TRACKS_Y + 3 * 16 - 2 + fl * 16)* sp) / 100);
		g_mem_dc->LineTo(((TRACKS_X + 50 * 11 - 3)* sp) / 100, ((TRACKS_Y + 3 * 16 + 2 + tl * 16)* sp) / 100);
	}
	return 1;
}

BOOL CSong::DrawInstrument()
{
	m_instrs.DrawInstrument(m_activeinstr);
	return 1;
}

BOOL CSong::DrawInfo()
{
	char s[80];
	int i,color;
	is_editing_infos = 0;

	if (g_prove == 3)	//test mode exclusive from MIDI CH15 inputs, this cannot be set by accident unless I did something stupid
		TextXY("EXPLORER MODE (MIDI CH15)", INFO_X, INFO_Y + 2 * 16, TEXT_COLOR_LIGHT_GRAY);
	else if (g_prove > 0)
		TextXY((g_prove == 1) ? "JAM MODE (MONO)" : "JAM MODE (STEREO)", INFO_X, INFO_Y + 2 * 16, TEXT_COLOR_BLUE);
	else
		TextXY("EDIT MODE", INFO_X, INFO_Y + 2 * 16, TEXT_COLOR_RED);

	if (g_activepart==PARTINFO && m_infoact==0) //info? && edit name?
	{
		is_editing_infos = 1;
		i=m_songnamecur;
		if (g_prove) color = TEXT_COLOR_BLUE;
		else color = TEXT_COLOR_RED;
	}
	else
	{
		i=-1;
		color = TEXT_COLOR_LIGHT_GRAY;
	}
	TextXYSelN(m_songname, i, INFO_X, INFO_Y, color);

	sprintf(s,"MUSIC SPEED: %02X/%02X/%X  MAXTRACKLENGTH: %02X  %s  %s",
		m_speed,m_mainspeed,m_instrspeed,
		m_tracks.m_maxtracklen,
		(g_tracks4_8==4)? "MONO-4-TRACKS" : "STEREO-8-TRACKS",
		(g_ntsc)? "NTSC" : "PAL"
		);

	TextXY(s, INFO_X, INFO_Y + 1 * 16, TEXT_COLOR_WHITE);

	s[15]=0; //current speed
	TextXY(s + 13, INFO_X + 13 * 8, INFO_Y + 1 * 16, TEXT_COLOR_LIGHT_GRAY);
	s[18]=0; //main speed
	TextXY(s + 16, INFO_X + 16 * 8, INFO_Y + 1 * 16, TEXT_COLOR_LIGHT_GRAY);
	s[20]=0; //instrspeed
	TextXY(s + 19, INFO_X + 19 * 8, INFO_Y + 1 * 16, TEXT_COLOR_LIGHT_GRAY);
	s[64]=0; //MAXTRACKLENGTH value and everything after
	TextXY(s + 38, INFO_X + 38 * 8, INFO_Y + 1 * 16, TEXT_COLOR_LIGHT_GRAY);

	if (g_activepart==PARTINFO)
	{
		if (g_prove) color = COLOR_SELECTED_PROVE;
		else color = COLOR_SELECTED;
		switch (m_infoact)
		{
		case 1:	//speed
			s[15]=0; 
			TextXY(s + 13, INFO_X + 13 * 8, INFO_Y + 1 * 16, color);	//selected
			break;
		case 2: //mainspeed
			s[18]=0; 
			TextXY(s + 16, INFO_X + 16 * 8, INFO_Y + 1 * 16, color);	//selected
			break;
		case 3: //instrspeed
			s[20]=0; 
			TextXY(s + 19, INFO_X + 19 * 8, INFO_Y + 1 * 16, color);	//selected
			break;
		}
	}

	sprintf(s,"%02X: %s",m_activeinstr,m_instrs.GetName(m_activeinstr));
	TextXY(s, INFO_X, INFO_Y + 3 * 16, TEXT_COLOR_WHITE);
	s[40]=0;
	TextXY(s + 4, INFO_X + 4 * 8, INFO_Y + 3 * 16, TEXT_COLOR_LIGHT_GRAY);

	sprintf(s,"OCTAVE %i-%i",m_octave+1,m_octave+2);
	TextXY(s, INFO_X + 47 * 8, INFO_Y + 3 * 16, TEXT_COLOR_WHITE);
	s[40]=0;
	TextXY(s + 7, INFO_X + 54 * 8, INFO_Y + 3 * 16, TEXT_COLOR_LIGHT_GRAY);

	sprintf(s,"VOLUME %X",m_volume);
	TextXY(s, INFO_X + 49 * 8, INFO_Y + 4 * 16, TEXT_COLOR_WHITE);
	s[40]=0;
	TextXY(s + 7, INFO_X + 56 * 8, INFO_Y + 4 * 16, TEXT_COLOR_LIGHT_GRAY);

	if (g_respectvolume) TextXY("\x17", INFO_X + 57 * 8, INFO_Y + 4 * 16, TEXT_COLOR_WHITE);	//respect volume mode

	//over instrument line with flags
	BYTE flag = m_instrs.GetFlag(m_activeinstr);

	int x=INFO_X;	//+4*8;
	const int y = INFO_Y + 4 * 16;
	int g=(m_trackactivecol%4) +1;		//channel 1 to 4

	if (flag&IF_FILTER)
	{
		if (g>2) 
		{
			TextMiniXY("NO FILTER",x,y, TEXT_MINI_COLOR_GRAY);
			x += 10*8;
		}
		else
		{
			if (g == 1)
				TextMiniXY("AUTOFILTER(1+3)", x, y, TEXT_MINI_COLOR_BLUE);
			else
				TextMiniXY("AUTOFILTER(2+4)", x, y, TEXT_MINI_COLOR_BLUE);
			x += 16*8;
		}
	}

	if (flag&IF_BASS16)
	{
		if (g==2)
		{
			TextMiniXY("BASS16(2+1)", x, y, TEXT_MINI_COLOR_BLUE);
			x += 12*8;
		}
		else
		if (g==4)
		{
			TextMiniXY("BASS16(4+3)", x, y, TEXT_MINI_COLOR_BLUE);
			x += 12*8;
		}
		else
		{
			TextMiniXY("NO BASS16", x, y, TEXT_MINI_COLOR_GRAY);;
			x += 10*8;
		}
	}

	if (flag&IF_PORTAMENTO)
	{
		TextMiniXY("PORTAMENTO", x, y, TEXT_MINI_COLOR_BLUE);
		x += 11*8;
	}
	if (flag&IF_AUDCTL)
	{
		TInstrument& ti=m_instrs.m_instr[m_activeinstr];
		int audctl = ti.par[PAR_AUDCTL0]
				  | (ti.par[PAR_AUDCTL1]<<1)
				  | (ti.par[PAR_AUDCTL2]<<2)
				  | (ti.par[PAR_AUDCTL3]<<3)
				  | (ti.par[PAR_AUDCTL4]<<4)
				  | (ti.par[PAR_AUDCTL5]<<5)
				  | (ti.par[PAR_AUDCTL6]<<6)
				  | (ti.par[PAR_AUDCTL7]<<7);
		sprintf(s,"AUDCTL:%02X",audctl);
		TextMiniXY(s, x, y, TEXT_MINI_COLOR_BLUE);
		x += 6*8;
	}

	return 1;
}

BOOL CSong::DrawPlaytimecounter(CDC *pDC = NULL)
{
	if (!g_viewplaytimecounter) return 0;	//the timer won't be displayed without the setting enabled first

	int sp = g_scaling_percentage;

	int MINIMAL_WIDTH_TRACKS = (g_tracks4_8 > 4 && g_active_ti == 1) ? 1420 : 960;
	int MINIMAL_WIDTH_INSTRUMENTS = (g_tracks4_8 > 4 && g_active_ti == 2) ? 1220 : 1220;
	int WINDOW_OFFSET = (g_width < 1320 && g_tracks4_8 > 4 && g_active_ti == 1) ? -250 : 0;	//test displacement with the window size
	int INSTRUMENT_OFFSET = (g_active_ti == 2 && g_tracks4_8 > 4) ? -250 : 0;
	if (g_tracks4_8 == 4 && g_active_ti == 2 && g_width > MINIMAL_WIDTH_INSTRUMENTS - 220) INSTRUMENT_OFFSET = 260;
	int SONG_OFFSET = SONG_X + WINDOW_OFFSET + INSTRUMENT_OFFSET + ((g_tracks4_8 == 4) ? -200 : 310);	//displace the SONG block depending on certain parameters

#define PLAYTC_X	(SONG_OFFSET+7)
#define PLAYTC_Y	(SONG_Y-8) 
#define PLAYTC_S	(4*8)  
#define PLAYTC_H	8  

	int fps = (g_ntsc) ? 60 : 50;
	int time10 = (g_playtime % fps) * 10 / fps;
	int time100 = ((g_playtime % fps) * 100 / fps) % 10;
	int ts = g_playtime / fps;	//total time in seconds
	int timesec = ts % 60;		//seconds 0 to 59
	int timemin = ts / 60;		//minutes 0 to ...
	char timstr[6];

	if (!timemin)	//less than a minute
	{
		//time from 00.0 to 59.9
		timstr[0] = (timesec / 10) | '0';
		timstr[1] = (timesec % 10) | '0';
		timstr[2] = (timesec % 2 == 0) ? '.' : ' ';
		timstr[3] = (time10) | '0';
		timstr[4] = (time100) | '0';
	}
	else if (!(timemin / 10))	//less than 10 minutes
	{
		//time from 1:00 to 9:99
		timstr[0] = (timemin % 10) | '0';
		timstr[1] = (timesec % 2 == 0) ? ':' : ' ';
		timstr[2] = (timesec / 10) | '0';
		timstr[3] = (timesec % 10) | '0';
		timstr[4] = 0;	//null
	}
	else
	{
		//time from 00:00 to 99:99
		timstr[0] = (timemin / 10) | '0';
		timstr[1] = (timemin % 10) | '0';
		timstr[2] = (timesec % 2 == 0) ? ':' : ' ';
		timstr[3] = (timesec / 10) | '0';
		timstr[4] = (timesec % 10) | '0';
	}

	timstr[5] = 0; //null
	TextMiniXY(timstr, PLAYTC_X, PLAYTC_Y, (m_play) ? TEXT_MINI_COLOR_WHITE : TEXT_MINI_COLOR_GRAY);
	if (pDC) pDC->BitBlt((PLAYTC_X*sp)/100, (PLAYTC_Y*sp)/100, (PLAYTC_S*sp)/100, (PLAYTC_H*sp)/100, g_mem_dc, (PLAYTC_X*sp)/100, (PLAYTC_Y*sp)/100, SRCCOPY);
	return 1;
}

BOOL CSong::DrawAnalyzer(CDC* pDC = NULL)
{
	if (!g_viewanalyzer) return 0;	//the analyser won't be displayed without the setting enabled first
	int sp = g_scaling_percentage;

	int MINIMAL_WIDTH_TRACKS = (g_tracks4_8 > 4 && g_active_ti == 1) ? 1420 : 960;
	int MINIMAL_WIDTH_INSTRUMENTS = (g_tracks4_8 > 4 && g_active_ti == 2) ? 1220 : 1220;
	int WINDOW_OFFSET = (g_width < 1320 && g_tracks4_8 > 4 && g_active_ti == 1) ? -250 : 0;	//test displacement with the window size
	int INSTRUMENT_OFFSET = (g_active_ti == 2 && g_tracks4_8 > 4) ? -250 : 0;
	if (g_tracks4_8 == 4 && g_active_ti == 2 && g_width > MINIMAL_WIDTH_INSTRUMENTS - 220) INSTRUMENT_OFFSET = 260;
	int SONG_OFFSET = SONG_X + WINDOW_OFFSET + INSTRUMENT_OFFSET + ((g_tracks4_8 == 4) ? -200 : 310);	//displace the SONG block depending on certain parameters

	BOOL DEBUG_POKEY = 1;	//registers debug display
	BOOL DEBUG_MEMORY = 0;	//memory debug display

	if (g_width < MINIMAL_WIDTH_TRACKS && g_active_ti == 1) DEBUG_POKEY = DEBUG_MEMORY = 0;
	if (g_width < MINIMAL_WIDTH_INSTRUMENTS && g_active_ti == 2) DEBUG_POKEY = DEBUG_MEMORY = 0;

#define ANALYZER_X	(TRACKS_X+6*8+4) 
#define ANALYZER_Y	(TRACKS_Y-8) 
#define ANALYZER_S	6 
#define ANALYZER_H	5 
#define ANALYZER_HP	8 
//
#define ANALYZER2_X	(SONG_OFFSET+6*8) 
#define ANALYZER2_Y	(TRACKS_Y-128) 
#define ANALYZER2_S	1 
#define ANALYZER2_H	4 
#define ANALYZER2_HP 8 
//
#define ANALYZER3_X	(SONG_OFFSET+6*8-32) 
#define ANALYZER3_Y	(TRACKS_Y+50) 
#define ANALYZER3_S	6 
#define ANALYZER3_H	5 
#define ANALYZER3_HP 8 
//
#define COL_BLOCK		56
#define RGBMUTE			RGB(120,160,240)
#define RGBNORMAL		RGB(255,255,255)
#define RGBVOLUMEONLY	RGB(128,255,255)
#define RGBTWOTONE		RGB(128,255,0) 
#define RGBBACKGROUND	RGB(34,50,80)

#define Hook1(g1,g2)																						\
	{																										\
		g_mem_dc->MoveTo(((ANALYZER_X+2+ANALYZER_S*15/2+16*8*(g1))*sp)/100,((ANALYZER_Y-1)*sp)/100);		\
		g_mem_dc->LineTo(((ANALYZER_X+2+ANALYZER_S*15/2+16*8*(g1))*sp)/100,((ANALYZER_Y-yu)*sp)/100); 		\
		g_mem_dc->LineTo(((ANALYZER_X+2+ANALYZER_S*15/2+16*8*(g2))*sp)/100,((ANALYZER_Y-yu)*sp)/100);		\
		g_mem_dc->LineTo(((ANALYZER_X+2+ANALYZER_S*15/2+16*8*(g2))*sp)/100,(ANALYZER_Y*sp)/100);			\
	}
#define Hook2(g1,g2)																						\
	{																										\
		g_mem_dc->MoveTo(((ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g1))*sp)/100,((ANALYZER_Y-120-1)*sp)/100);		\
		g_mem_dc->LineTo(((ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g1))*sp)/100,((ANALYZER_Y-120-yu)*sp)/100);	\
		g_mem_dc->LineTo(((ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g2))*sp)/100,((ANALYZER_Y-120-yu)*sp)/100);	\
		g_mem_dc->LineTo(((ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g2))*sp)/100,((ANALYZER_Y-120)*sp)/100);		\
	}

	int audf, audf2, audf3, audf16, audc, audc2, audctl, skctl, pitch, dist, vol, vol2;
	static int idx[8] = { 0xd200,0xd202,0xd204,0xd206,0xd210,0xd212,0xd214,0xd216 };	//AUDF and AUDC
	static int idx2[2] = { 0xd208,0xd218 };	//AUDCTL and SKCTL
	int col[8];
	int R[8];
	int G[8];
	int yu = 7;
	for (int i = 0; i < g_tracks4_8; i++) { col[i] = 102; R[i] = 44; G[i] = 60; }
	int a;
	int b;
	COLORREF acol;

	if (g_active_ti == PARTTRACKS) //bigger look for track edit mode
	{
		g_mem_dc->FillSolidRect((ANALYZER_X * sp) / 100, ((ANALYZER_Y - ANALYZER_HP) * sp) / 100, ((g_tracks4_8 * 16 * 8 - 34) * sp) / 100, ((ANALYZER_H + ANALYZER_HP) * sp) / 100, RGBBACKGROUND);

		a = g_atarimem[0xd208]; //audctl1
		if (a & 0x04) { col[2] = COL_BLOCK; Hook1(0, 2); yu -= 2; }
		if (a & 0x02) { col[3] = COL_BLOCK;	Hook1(1, 3); yu -= 2; }
		if (a & 0x10) { col[0] = COL_BLOCK;	Hook1(0, 1); yu -= 2; }
		if (a & 0x08) { col[2] = COL_BLOCK;	Hook1(2, 3); yu -= 2; }

		b = g_atarimem[0xd20f]; //skctl1
		if (b == 0x8b) { col[1] = COL_BLOCK; Hook1(0, 1); yu -= 2; }
		yu = 7;

		a = g_atarimem[0xd218]; //audctl2
		if (a & 0x04) { col[2 + 4] = COL_BLOCK; Hook1(0 + 4, 2 + 4); yu -= 2; }
		if (a & 0x02) { col[3 + 4] = COL_BLOCK; Hook1(1 + 4, 3 + 4); yu -= 2; }
		if (a & 0x10) { col[0 + 4] = COL_BLOCK; Hook1(0 + 4, 1 + 4); yu -= 2; }
		if (a & 0x08) { col[2 + 4] = COL_BLOCK; Hook1(2 + 4, 3 + 4); yu -= 2; }

		b = g_atarimem[0xd21f]; //skctl2
		if (b == 0x8b) { col[1 + 4] = COL_BLOCK; Hook1(0 + 4, 1 + 4); yu -= 2; }

		for (int i = 0; i < g_tracks4_8; i++)
		{
			audf = g_atarimem[idx[i]];
			audc = g_atarimem[idx[i] + 1];
			int skctl1 = g_atarimem[0xd20f];
			int skctl2 = g_atarimem[0xd21f];
			vol = audc & 0x0f;
			a = i * 16 * 8;

			g_mem_dc->FillSolidRect(((ANALYZER_X + a + 2) * sp) / 100, (ANALYZER_Y * sp) / 100, ((15 * ANALYZER_S) * sp) / 100, (ANALYZER_H * sp) / 100, RGB(R[i], G[i], col[i]));

			acol = GetChannelOnOff(i) ? ((audc & 0x10) ? RGBVOLUMEONLY : RGBNORMAL) : RGBMUTE;

			if (GetChannelOnOff(i) && ((skctl1 == 0x8b && i == 0) || (skctl2 == 0x8b && i == 4))) acol = RGBTWOTONE;
			if (vol) g_mem_dc->FillSolidRect(((ANALYZER_X + a + 3 + (15 - vol) * ANALYZER_S / 2) * sp) / 100, (ANALYZER_Y * sp) / 100, ((vol * ANALYZER_S) * sp) / 100, (ANALYZER_H * sp) / 100, acol);

			if (g_viewpokeyregs)
			{
				NumberMiniXY(audf, ANALYZER_X + 10 + a + 17, ANALYZER_Y - 8, TEXT_MINI_COLOR_GRAY);
				NumberMiniXY(audc, ANALYZER_X + 36 + a + 17, ANALYZER_Y - 8, TEXT_MINI_COLOR_GRAY);
			}
		}
		if (g_viewpokeyregs)
		{
			NumberMiniXY(g_atarimem[0xd208], ANALYZER_X + 23 + 1 * 8 * 16 + 80, ANALYZER_Y - 8);
			if (g_tracks4_8 > 4) NumberMiniXY(g_atarimem[0xd218], ANALYZER_X + 23 + 5 * 8 * 16 + 80, ANALYZER_Y - 8);
			NumberMiniXY(g_atarimem[0xd20f], ANALYZER_X + 23 + 1 * 8 * 16 + 80, ANALYZER_Y - 0);
			if (g_tracks4_8 > 4) NumberMiniXY(g_atarimem[0xd21f], ANALYZER_X + 23 + 5 * 8 * 16 + 80, ANALYZER_Y - 0);
		}
		if (pDC) pDC->BitBlt((ANALYZER_X*sp)/100, ((ANALYZER_Y - ANALYZER_HP)*sp)/100, ((g_tracks4_8 * 16 * 8 - 8)*sp)/100, ((ANALYZER_H + ANALYZER_HP + 4)*sp)/100, g_mem_dc, (ANALYZER_X*sp)/100, ((ANALYZER_Y - ANALYZER_HP)*sp)/100, SRCCOPY);
	}
	else
	if (g_active_ti == PARTINSTRS) //smaller appearance for instrument edit mode
	{
		g_mem_dc->FillSolidRect((ANALYZER2_X * sp) / 100, ((ANALYZER2_Y - ANALYZER2_HP) * sp) / 100, ((g_tracks4_8 * 3 * 8 - 8) * sp) / 100, ((ANALYZER2_H + ANALYZER2_HP) * sp) / 100, RGBBACKGROUND);

		a = g_atarimem[0xd208]; //audctl1
		if (a & 0x04) { col[2] = COL_BLOCK; Hook2(0, 2); yu -= 2; }
		if (a & 0x02) { col[3] = COL_BLOCK;	Hook2(1, 3); yu -= 2; }
		if (a & 0x10) { col[0] = COL_BLOCK;	Hook2(0, 1); yu -= 2; }
		if (a & 0x08) { col[2] = COL_BLOCK;	Hook2(2, 3); yu -= 2; }

		b = g_atarimem[0xd20f]; //skctl1
		if (b == 0x8b) { col[1] = COL_BLOCK; Hook2(0, 1); yu -= 2; }
		yu = 7;

		a = g_atarimem[0xd218]; //audctl2
		if (a & 0x04) { col[2 + 4] = COL_BLOCK; Hook2(0 + 4, 2 + 4); yu -= 2; }
		if (a & 0x02) { col[3 + 4] = COL_BLOCK; Hook2(1 + 4, 3 + 4); yu -= 2; }
		if (a & 0x10) { col[0 + 4] = COL_BLOCK; Hook2(0 + 4, 1 + 4); yu -= 2; }
		if (a & 0x08) { col[2 + 4] = COL_BLOCK; Hook2(2 + 4, 3 + 4); yu -= 2; }

		b = g_atarimem[0xd21f]; //skctl2
		if (b == 0x8b) { col[1 + 4] = COL_BLOCK; Hook2(0 + 4, 1 + 4); yu -= 2; }

		for (int i = 0; i < g_tracks4_8; i++)
		{
			audc = g_atarimem[idx[i] + 1];
			int skctl1 = g_atarimem[0xd20f];
			int skctl2 = g_atarimem[0xd21f];
			vol = audc & 0x0f;

			g_mem_dc->FillSolidRect(((ANALYZER2_X + i * 3 * 8) * sp) / 100, (ANALYZER2_Y * sp) / 100, ((15 * ANALYZER2_S) * sp) / 100, (ANALYZER2_H * sp) / 100, RGB(R[i], G[i], col[i]));

			acol = GetChannelOnOff(i) ? ((audc & 0x10) ? RGBVOLUMEONLY : RGBNORMAL) : RGBMUTE;

			if (GetChannelOnOff(i) && ((skctl1 == 0x8b && i == 0) || (skctl2 == 0x8b && i == 4))) acol = RGBTWOTONE;
			if (vol) g_mem_dc->FillSolidRect(((ANALYZER2_X + i * 3 * 8 + (15 - vol) * ANALYZER2_S / 2) * sp) / 100, (ANALYZER2_Y * sp) / 100, ((vol * ANALYZER2_S) * sp) / 100, (ANALYZER2_H * sp) / 100, acol);
		}
		if (pDC) pDC->BitBlt((ANALYZER2_X*sp)/100, ((ANALYZER2_Y - ANALYZER2_HP)*sp)/100, ((g_tracks4_8 * 3 * 8 - 8)*sp)/100, ((ANALYZER2_H + ANALYZER2_HP)*sp)/100, g_mem_dc, (ANALYZER2_X*sp)/100, ((ANALYZER2_Y - ANALYZER2_HP)*sp)/100, SRCCOPY);
	}
	if (DEBUG_POKEY)	//detailed registers viewer
	{
		//AUDCTL bits
		BOOL CLOCK_15 = 0;	//0x01
		BOOL HPF_CH24 = 0;	//0x02
		BOOL HPF_CH13 = 0;	//0x04
		BOOL JOIN_34 = 0;	//0x08
		BOOL JOIN_12 = 0;	//0x10
		BOOL CH3_179 = 0;	//0x20
		BOOL CH1_179 = 0;	//0x40
		BOOL POLY9 = 0;	//0x80
		BOOL TWO_TONE = 0;	//0x8B

		BOOL JOIN_16BIT = 0;
		BOOL JOIN_64KHZ = 0;
		BOOL JOIN_15KHZ = 0;
		BOOL JOIN_WRONG = 0;
		BOOL REVERSE_16 = 0;
		BOOL SAWTOOTH = 0;
		BOOL SAWTOOTH_INVERTED = 0;
		BOOL CLOCK_179 = 0;

		g_mem_dc->FillSolidRect((ANALYZER3_X* sp) / 100, (ANALYZER3_Y* sp) / 100, (680 * sp) / 100, (192 * sp) / 100, RGBBACKGROUND);

		for (int i = 0; i < g_tracks4_8; i++)
		{
			BOOL IS_RIGHT_POKEY = (i >= 4) ? 1 : 0;

			audctl = g_atarimem[idx2[IS_RIGHT_POKEY]];
			skctl = g_atarimem[idx2[IS_RIGHT_POKEY] + 7];
			audf = g_atarimem[idx[i]];
			audc = g_atarimem[idx[i] + 1];

			vol = audc & 0x0f;
			dist = audc & 0xf0;
			pitch = audf;

			if (i % 4 == 0)								//only in valid sawtooth channels
				audf3 = g_atarimem[idx[i + 2]];

			if (i % 2 == 1)								//only in valid 16-bit channels
			{ 
				audf2 = g_atarimem[idx[i - 1]];
				audc2 = g_atarimem[idx[i - 1] + 1];
				vol2 = audc2 & 0x0f;
				audf16 = audf;
				audf16 <<= 8;
				audf16 += audf2;
				cout << audf16;
			}

			int gap = (IS_RIGHT_POKEY) ? 64 : 0;
			int gap2 = (IS_RIGHT_POKEY) ? 96 : 0;
			a = i * 8 + gap + 16;
			int minus = (IS_RIGHT_POKEY) ? -8 : 0;
			int audnum = (i * 2) + minus;
			char s[2];
			char p[12];
			char n[4];
			double PITCH = 0;

			CLOCK_15 = audctl & 0x01;
			HPF_CH24 = audctl & 0x02;
			HPF_CH13 = audctl & 0x04;
			JOIN_34 = audctl & 0x08;
			JOIN_12 = audctl & 0x10;
			CH3_179 = audctl & 0x20;
			CH1_179 = audctl & 0x40;
			POLY9 = audctl & 0x80;
			TWO_TONE = (skctl == 0x8B) ? 1 : 0;

			//combined modes for some special output...
			SAWTOOTH = (CH1_179 && CH3_179 && HPF_CH13 && (dist == 0xA0|| dist == 0xE0) && (i == 0 || i == 4)) ? 1 : 0;
			SAWTOOTH_INVERTED = 0;
			JOIN_16BIT = ((JOIN_12 && CH1_179 && (i == 1 || i == 5)) || (JOIN_34 && CH3_179 && (i == 3 || i == 7))) ? 1 : 0;
			JOIN_64KHZ = ((JOIN_12 && !CH1_179 && !CLOCK_15 && (i == 1 || i == 5)) || (JOIN_34 && !CH3_179 && !CLOCK_15 && (i == 3 || i == 7))) ? 1 : 0;
			JOIN_15KHZ = ((JOIN_12 && !CH1_179 && CLOCK_15 && (i == 1 || i == 5)) || (JOIN_34 && !CH3_179 && CLOCK_15 && (i == 3 || i == 7))) ? 1 : 0;
			JOIN_WRONG = (((JOIN_12 && (i == 0 || i == 4)) || (JOIN_34 && (i == 2 || i == 6))) && (vol == 0x00));	//16-bit, invalid channel, no volume
			REVERSE_16 = (((JOIN_12 && (i == 0 || i == 4)) || (JOIN_34 && (i == 2 || i == 6))) && (vol > 0x00));	//16-bit, invalid channel, with volume (Reverse-16)
			CLOCK_179 = ((CH1_179 && (i == 0 || i == 4)) || (CH3_179 && (i == 2 || i == 6))) ? 1 : 0;
			if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode

			int modoffset = 1;
			int coarse_divisor = 1;
			double divisor = 1;
			int v_modulo = 0;
			bool IS_VALID = 0;

			if (JOIN_16BIT) modoffset = 7;
			else if (CLOCK_179) modoffset = 4;
			else coarse_divisor = (CLOCK_15) ? 114 : 28;

			int i_audf = (JOIN_16BIT || JOIN_64KHZ || JOIN_15KHZ) ? audf16 : audf;
			PITCH = generate_freq(audc, i_audf, audctl, i);
			snprintf(p, 10, "%9.2f", PITCH);

			if (g_viewpokeyregs)
			{
				TextMiniXY("$D200: $   $     PITCH = $     (         HZ ---  +  ), VOL = $ , DIST = $ ,", ANALYZER3_X, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);
				TextMiniXY("$D208: $  ", ANALYZER3_X, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_GRAY);
				TextMiniXY("$D20F: $  ", ANALYZER3_X, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_GRAY);

				if (CLOCK_15)	//15khz
					TextMiniXY("15KHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
				else
					TextMiniXY("64KHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);

				if (CLOCK_179)
					TextMiniXY("1.79MHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);

				if (JOIN_16BIT || JOIN_64KHZ || JOIN_15KHZ)
					TextMiniXY("16-BIT", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);

				/*
				if (JOIN_16BIT)
					TextMiniXY("16-BIT, 1.79MHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
				else if (JOIN_64KHZ)
					TextMiniXY("16-BIT, 64KHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
				else if (JOIN_15KHZ)
					TextMiniXY("16-BIT, 15KHZ", ANALYZER3_X + 8 * 76, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
				*/

				/*
				if (dist == 0xC0)
				{
					int v_modulo = (CLOCK_15) ? 5 : 15;
					BOOL IS_UNSTABLE_DIST_C = ((audf + modoffset) % 5 == 0) ? 1 : 0;
					BOOL IS_BUZZY_DIST_C = ((audf + modoffset) % 3 == 0 || CLOCK_15) ? 1 : 0;
					IS_VALID = ((audf + modoffset) % v_modulo == 0) ? 0 : 1;
					if (IS_VALID)
					{
						if (IS_BUZZY_DIST_C) TextMiniXY("BUZZY", ANALYZER3_X + 8 * 84, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
						else if (IS_UNSTABLE_DIST_C) TextMiniXY("UNSTABLE", ANALYZER3_X + 8 * 84, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
						else TextMiniXY("GRITTY", ANALYZER3_X + 8 * 84, ANALYZER3_Y + a, TEXT_MINI_COLOR_BLUE);
					}
				}
				*/

				if (HPF_CH13)
				{
					if (SAWTOOTH && !SAWTOOTH_INVERTED)
						TextMiniXY("CH1: HIGH PASS FILTER, SAWTOOTH", ANALYZER3_X + 8 * 32, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_BLUE);
					else
						if (SAWTOOTH && SAWTOOTH_INVERTED)
							TextMiniXY("CH1: HIGH PASS FILTER, SAWTOOTH (INVERTED)", ANALYZER3_X + 8 * 32, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_BLUE);
						else
							TextMiniXY("CH1: HIGH PASS FILTER", ANALYZER3_X + 8 * 32, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_BLUE);
				}

				if (HPF_CH24)
					TextMiniXY("CH2: HIGH PASS FILTER", ANALYZER3_X + 8 * 32, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_BLUE);

				if (POLY9)
					TextMiniXY("POLY9 ENABLED", ANALYZER3_X + 8 * 11, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_BLUE);

				if (TWO_TONE)
					TextMiniXY("CH1: TWO TONE FILTER", ANALYZER3_X + 8 * 11, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_BLUE);

				if (REVERSE_16)
				{
					if (i == 0 || i == 4)
						TextMiniXY("CH1: REVERSE-16 OUTPUT", ANALYZER3_X + 8 * 54, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_BLUE);
					else if (i == 2 || i == 6)
						TextMiniXY("CH3: REVERSE-16 OUTPUT", ANALYZER3_X + 8 * 54, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_BLUE);
				}

				NumberMiniXY(audf, ANALYZER3_X + 8 * 8, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);
				NumberMiniXY(audc, ANALYZER3_X + 8 * 12, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);
				NumberMiniXY(pitch, ANALYZER3_X + 8 * 26, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);

				if ((JOIN_16BIT || JOIN_64KHZ || JOIN_15KHZ) && !vol2)	//16-bit without Reverse-16 output
					NumberMiniXY(audf2, ANALYZER3_X + 8 * 28, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);

				NumberMiniXY(vol, ANALYZER3_X + 8 * 61, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);
				NumberMiniXY(dist, ANALYZER3_X + 8 * 73, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);
				if (dist == 0xf0) TextMiniXY("e", ANALYZER3_X + 8 * 73, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);	//empty tile
				NumberMiniXY(audctl, ANALYZER3_X + 8 * 8, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_WHITE);
				NumberMiniXY(skctl, ANALYZER3_X + 8 * 8, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_WHITE);

				TextMiniXY(p, ANALYZER3_X + 8 * 32, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);	//pitch calculation
				TextMiniXY("$", ANALYZER3_X + 8 * 61, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);	//character $ to overwrite the left volume nybble
				TextMiniXY(",", ANALYZER3_X + 8 * 74, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);	//character , to overwrite the right distortion nybble

				sprintf(s, "%d", audnum);
				TextMiniXY(s, ANALYZER3_X + 8 * 4, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);		//register number

				if (IS_RIGHT_POKEY)
				{
					TextMiniXY("POKEY REGISTERS (LEFT)", ANALYZER3_X, ANALYZER3_Y, TEXT_MINI_COLOR_GRAY);
					TextMiniXY("POKEY REGISTERS (RIGHT)", ANALYZER3_X, ANALYZER3_Y + 96, TEXT_MINI_COLOR_GRAY);

					TextMiniXY("1", ANALYZER3_X + 8 * 3, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);
					TextMiniXY("1", ANALYZER3_X + 8 * 3, ANALYZER3_Y + gap2 + 48, TEXT_MINI_COLOR_GRAY);
					TextMiniXY("1", ANALYZER3_X + 8 * 3, ANALYZER3_Y + gap2 + 48 + 8, TEXT_MINI_COLOR_GRAY);
				}
				else TextMiniXY("POKEY REGISTERS", ANALYZER3_X, ANALYZER3_Y, TEXT_MINI_COLOR_GRAY);

				double tuning = g_basetuning;	//defined in Tuning.cpp through initialisation using input parameter
				int basenote = g_basenote;
				int reverse_basenote = (24 - basenote) % 12;	//since things are wack I had to do this
				int FREQ_17 = (g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL;	//useful for debugging I guess
				int cycles = (g_ntsc) ? MAXSCREENCYCLES_NTSC : MAXSCREENCYCLES_PAL;
				int tracks = (g_tracks4_8 == 8) ? 8 : 4;
				char t[12] = { 0 };

				TextMiniXY("A- TUNING:       HZ,", ANALYZER3_X, ANALYZER3_Y + 8 * 9, TEXT_MINI_COLOR_GRAY);
				snprintf(t, 10, "%3.2f", tuning);
				TextMiniXY(t, ANALYZER3_X + 8 * 11, ANALYZER3_Y + 8 * 9, TEXT_MINI_COLOR_WHITE);

				n[0] = notes[reverse_basenote][0];
				n[1] = notes[reverse_basenote][1];
				n[2] = 0;

				TextMiniXY(n, ANALYZER3_X, ANALYZER3_Y + 8 * 9, TEXT_MINI_COLOR_GRAY);	//overwrite A- to the given basenote

				if (g_ntsc) TextMiniXY("NTSC", ANALYZER3_X + 8 * 21, ANALYZER3_Y + 8 * 9, TEXT_MINI_COLOR_BLUE);
				else TextMiniXY("PAL", ANALYZER3_X + 8 * 21, ANALYZER3_Y + 8 * 9, TEXT_MINI_COLOR_BLUE);

				TextMiniXY("FREQ17:        HZ, MAXSCREENCYCLES:      , G_TRACKS4_8:", ANALYZER3_X, ANALYZER3_Y + 8 * 10, TEXT_MINI_COLOR_GRAY);
				snprintf(t, 8, "%d", FREQ_17);
				TextMiniXY(t, ANALYZER3_X + 8 * 8, ANALYZER3_Y + 8 * 10, TEXT_MINI_COLOR_WHITE);
				snprintf(t, 8, "%d", cycles);
				TextMiniXY(t, ANALYZER3_X + 8 * 36, ANALYZER3_Y + 8 * 10, TEXT_MINI_COLOR_WHITE);
				snprintf(t, 2, "%d", tracks);
				TextMiniXY(t, ANALYZER3_X + 8 * 56, ANALYZER3_Y + 8 * 10, TEXT_MINI_COLOR_WHITE);

				if (PITCH)	//if null is read, there is nothing to show. Volume Only mode or invalid parameters may return this
				{
					if (JOIN_WRONG)	//16-bit, but wrong channels, and the volume is 0
					{
						TextMiniXY("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee", ANALYZER3_X + 8 * 17, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);	//masking parts of the line,cursed patch but that works so who cares
					}
					else
					{
						char szBuffer[16];

						//most of the lines below could get some improvements...
						double centnum = 1200 * log2(PITCH / tuning);
						int notenum = (int)round(centnum * 0.01) + 60;
						int note = ((notenum + 96) - basenote) % 12;
						int octave = (notenum - basenote) / 12;
						int cents = (int)round(centnum - (notenum - 60) * 100);

						snprintf(szBuffer, 4, "%03d", cents);
						TextMiniXY(szBuffer, ANALYZER3_X + 8 * 49, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);

						if (cents >= 0)
							TextMiniXY("+", ANALYZER3_X + 8 * 49, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);
						else
							TextMiniXY("-", ANALYZER3_X + 8 * 49, ANALYZER3_Y + a, TEXT_MINI_COLOR_GRAY);

						n[0] = notes[note][0];
						n[1] = notes[note][1];
						n[2] = 0;

						sprintf(szBuffer, "%1d", octave);
						TextMiniXY(n, ANALYZER3_X + 8 * 44, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);
						TextMiniXY(szBuffer, ANALYZER3_X + 8 * 46, ANALYZER3_Y + a, TEXT_MINI_COLOR_WHITE);

					}
				}
			}
		}
		if (pDC) pDC->BitBlt((ANALYZER3_X * sp) / 100, (ANALYZER3_Y * sp) / 100, (680 * sp) / 100, (192 * sp) / 100, g_mem_dc, (ANALYZER3_X * sp) / 100, (ANALYZER3_Y * sp) / 100, SRCCOPY);
	}
	if (DEBUG_MEMORY)	//Atari memory display, do not use unless there is a useful purpose for it
	{
		g_mem_dc->FillSolidRect((ANALYZER3_X* sp) / 100, ((ANALYZER3_Y + 192)* sp) / 100, ((680 + (8 * 42))* sp) / 100, (432 * sp) / 100, RGBBACKGROUND);

		int gap = 0; int gap2 = 32; int page = 0;

		for (int d = 0; d < 40; d++)	//1 memory page => 0x100, 32 bytes per line
		{
			//larger font...
			//GetAtariMemHexStr(0xB200 + (0x10 * d), 16);	//Distortion C page
			//TextXY(g_debugmem, ANALYZER3_X, ANALYZER3_Y + 240 + 16 * d + 8 + gap, TEXT_COLOR_WHITE);
			gap += (d % 8 == 0) ? 8 : 0;
			page += (d % 8 == 0 && d != 0) ? 1 : 0;
			gap2 = 16 * page;
			GetAtariMemHexStr(0xB000 + 0x20 * d, 32); 
			TextMiniXY(g_debugmem, ANALYZER3_X, ANALYZER3_Y + 192 + 8 * d + 8 + gap + gap2, TEXT_MINI_COLOR_WHITE);

			if (d % 8 == 0)
			{
				TextMiniXY("G_ATARIMEM (      ):", ANALYZER3_X, ANALYZER3_Y + 192 + 8 * d + gap + gap2 - 8, TEXT_MINI_COLOR_GRAY);
				NumberMiniXY(page, ANALYZER3_X + 8 * 14, ANALYZER3_Y + 192 + 8 * d + gap + gap2 - 8, TEXT_MINI_COLOR_WHITE);
				TextMiniXY("0XB 00", ANALYZER3_X + 8 * 12, ANALYZER3_Y + 192 + 8 * d + gap + gap2 - 8, TEXT_MINI_COLOR_WHITE);
			}
		}
		if (pDC) pDC->BitBlt((ANALYZER3_X * sp) / 100, ((ANALYZER3_Y + 192) * sp) / 100, ((680 + (8 * 42)) * sp) / 100, (432 * sp) / 100, g_mem_dc, (ANALYZER3_X * sp) / 100, ((ANALYZER3_Y + 192) * sp) / 100, SRCCOPY);
	}
	return 1;
}

BOOL CSong::InfoKey(int vk,int shift,int control)
{
	BOOL CAPSLOCK = GetKeyState(20);	//VK_CAPS_LOCK

	if (m_infoact==0)
	{
		is_editing_infos = 1;
		if (vk == VK_DIVIDE || vk == VK_MULTIPLY || vk == VK_ADD || vk == VK_SUBTRACT) goto edit_ok;	//a workaround so the Octave and Volume can be set anywhere
		if (((!CAPSLOCK && shift) || (CAPSLOCK && !shift)) && (vk == VK_LEFT || vk == VK_RIGHT)) goto edit_ok;	//a workaround so the active instrument can be set anywhere
		if (IsnotMovementVKey(vk))
		{	//saves undo only if it is not cursor movement
			m_undo.ChangeInfo(0,UETYPE_INFODATA);
		}
		if (EditText(vk,shift,control,m_songname,m_songnamecur,SONGNAMEMAXLEN)) m_infoact = 1;
		return 1;
	}

	int i,num;		
	int volatile * infptab[]={&m_speed,&m_mainspeed,&m_instrspeed};
	int infandtab[]={0xff,0xff,0x08};	//maximum current speed, main speed and instrument speed
	int volatile& infp = *infptab[m_infoact-1];
	int infand = infandtab[m_infoact-1];
	
	if ( (num=NumbKey(vk))>=0 && num<=infand)
	{
		i= infp & 0x0f; //lower digit
		if (infand<0x0f)
		{	if (num<=infand) i = num; }
		else
			i = ((i<<4) | num) & infand;
		if (i<=0) i=1;	//all speeds must be at least 1
		m_undo.ChangeInfo(0,UETYPE_INFODATA);
		infp = i;
		return 1;
	}
edit_ok:
	switch (vk)
	{
	case VK_TAB:
		if (control) break;	//do nothing
		if (shift)
		{
			m_infoact = 0;	//Shift+TAB => Name
			is_editing_infos = 1;
		}
		else
		{
			if (m_infoact < 3) m_infoact++; else m_infoact = 1; //TAB => Speed variables 1, 2 or 3
			is_editing_infos = 0;
		}
		return 1;

	case VK_UP:
		if (control && shift) break;	//do nothing
		if (control) goto IncrementInfoPar;
		break;

	case VK_DOWN:
		if (control && shift) break;	//do nothing
		if (control) goto DecrementInfoPar;
		break;

	case VK_LEFT:
	{
		if (control && shift) break;	//do nothing
		if (control)
		{
DecrementInfoPar:
			i = infp;
			i--;
			if (i <= 0) i = infand; //speed must be at least 1
			m_undo.ChangeInfo(0, UETYPE_INFODATA);
			infp = i;
		}
		else
		if (!CAPSLOCK && shift || (CAPSLOCK && !shift && is_editing_infos) || (CAPSLOCK && shift && !is_editing_infos))
		{
			ActiveInstrPrev();
		}
		else
		{
			if (m_infoact > 1) m_infoact--; else m_infoact = 3;
		}
	}
	return 1;
	
	case VK_RIGHT:
	{
		if (control && shift) break;	//do nothing
		if (control)
		{
IncrementInfoPar:
			i = infp;
			i++;
			if (i > infand) i = 1;	//speed must be at least 1
			m_undo.ChangeInfo(0, UETYPE_INFODATA);
			infp = i;
		}
		else
		if (!CAPSLOCK && shift || (CAPSLOCK && !shift && is_editing_infos) || (CAPSLOCK && shift && !is_editing_infos))
		{
			ActiveInstrNext();
		}
		else
		{
			if (m_infoact < 3) m_infoact++; else m_infoact = 1;
		}
	}
	return 1;

	case 13:		//VK_ENTER
		g_activepart = g_active_ti;
		return 1;

	case VK_MULTIPLY:
	{
		OctaveUp();
		return 1;
	}
	break;

	case VK_DIVIDE:
	{
		OctaveDown();
		return 1;
	}
	break;

	case VK_ADD:
	{
		VolumeUp();
		return 1;
	}

	case VK_SUBTRACT:
	{
		VolumeDown();
		return 1;
	}

	}
	return 0;
}

BOOL CSong::InstrKey(int vk,int shift,int control)
{
	//note: if returning 1, then screenupdate is done in RmtView
	TInstrument& ai= m_instrs.m_instr[m_activeinstr];
	int& ap = ai.par[ai.activepar];
	int& ae = ai.env[ai.activeenvx][ai.activeenvy];
	int& at = ai.tab[ai.activetab];
	int i;

	BOOL CAPSLOCK = GetKeyState(20);	//VK_CAPS_LOCK

	if (!control && !shift && NumbKey(vk)>=0)
	{
		if (ai.act==1) //parameters
		{
			int pmax = shpar[ai.activepar].pmax;
			int pfrom = shpar[ai.activepar].pfrom;
			if (NumbKey(vk)>pmax+pfrom) return 0;
			i = ap + pfrom;
			i &= 0x0f; //lower digit
			if (pmax+pfrom>0x0f)
			{
				i = (i<<4) | NumbKey(vk);
				if (i > pmax+pfrom)
					i &= 0x0f;		//leaves only the lower digit
			}
			else
			{
				if (NumbKey(vk)>=pfrom) i = NumbKey(vk);
			}
			i -= pfrom;
			if ( i<0) i=0;
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ap = i;
			goto ChangeInstrumentPar;
		}
		else
		if (ai.act==2) //envelope
		{
			int eand = shenv[ai.activeenvy].pand;
			int num = NumbKey(vk);
			i = num & eand;
			if (i != num) return 0; //something else came out after and number pressed out of range
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ae = i;
			//shift to the right
			i=ai.activeenvx;
			if (i<ai.par[PAR_ENVLEN]) i++;	// else i=0; //length of env
			ai.activeenvx=i;
			goto ChangeInstrumentEnv;
		}
		else
		if (ai.act==3) //table
		{
			int num = NumbKey(vk);
			i = ((at << 4) | num) & 0xff;
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			at = i;
			goto ChangeInstrumentTab;
		}
	}

	//for name, parameters, envelope and table
	switch(vk)
	{
	case VK_TAB:
		if (control) break;	//do nothing
		if (ai.act==0) break;	//is editing text
		if (shift)
		{
			ai.act=0;	//Shift+TAB => Name
			is_editing_instr = 1;
		}
		else
		{
			if (ai.act<3) ai.act++;	else ai.act=1; //TAB 1,2,3
			is_editing_instr = 0;
		}
		m_undo.Separator();
		return 1;

	case VK_LEFT:
		if (!control && (!CAPSLOCK && shift || (CAPSLOCK && !shift && is_editing_instr) || (CAPSLOCK && shift && !is_editing_instr)))
		{
			ActiveInstrPrev();
			return 1;
		}
		break;

	case VK_RIGHT:
		if (!control && (!CAPSLOCK && shift || (CAPSLOCK && !shift && is_editing_instr) || (CAPSLOCK && shift && !is_editing_instr)))
		{
			ActiveInstrNext();
			return 1;
		}
		break;

	case VK_UP:
	case VK_DOWN:
		if (CAPSLOCK && shift) break;

		if (shift && !control) return 0;	//the combination Shift + Control + UP / DOWN is enabled for edit ENVELOPE and TABLE
		if (shift && control && ai.act==1) return 0;	//except for edit PARAM, is not allowed there
		break;

	case VK_MULTIPLY:
		{
			OctaveUp();
			return 1;
		}
		break;

	case VK_DIVIDE: 
		{
			OctaveDown();
			return 1;
		}
		break;

	case VK_SUBTRACT:	//Numlock minus
		if (shift && control)	//S+C+numlock_minus  ...reading the whole curve with a minimum of 0
		{
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			BOOL br=0,bl=0;
			if (ai.act==2 && ai.activeenvy==ENV_VOLUMER) br=1;
			else
			if (ai.act==2 && ai.activeenvy==ENV_VOLUMEL) bl=1;
			else
				br=bl=1;
			for(int i=0; i<=ai.par[PAR_ENVLEN]; i++)
			{
				if (br) { if (ai.env[i][ENV_VOLUMER]>0) ai.env[i][ENV_VOLUMER]--; }
				if (bl) { if (ai.env[i][ENV_VOLUMEL]>0) ai.env[i][ENV_VOLUMEL]--; }
			}
			goto ChangeInstrumentEnv;
		}
		else
			VolumeDown();
		return 1;
		break;
		
	case VK_ADD:		//Numlock plus
		if (shift && control)	//S+C+numlock_plus ...applying the whole curve with maximum f
		{
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			BOOL br=0,bl=0;
			if (ai.act==2 && ai.activeenvy==ENV_VOLUMER) br=1;
			else
			if (ai.act==2 && ai.activeenvy==ENV_VOLUMEL) bl=1;
			else
				br=bl=1;
			for(int i=0; i<=ai.par[PAR_ENVLEN]; i++)
			{
				if (br) { if (ai.env[i][ENV_VOLUMER]<0x0f) ai.env[i][ENV_VOLUMER]++; }
				if (bl) { if (ai.env[i][ENV_VOLUMEL]<0x0f) ai.env[i][ENV_VOLUMEL]++; }
			}
			goto ChangeInstrumentEnv;
		}
		else
			VolumeUp();
		return 1;
		break;
	}

	//and now only for special parts
	if (ai.act==0)
	{
		is_editing_instr = 1;
		//NAME
		if (IsnotMovementVKey(vk))
		{	//saves undo only if it is not cursor movement
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
		}
		if ( EditText(vk,shift,control,ai.name,ai.activenam,INSTRNAMEMAXLEN) ) ai.act=1;

		return 1;
	}
	else
	if (ai.act==1)
	{
		is_editing_instr = 0;
		//PARAMETERS
		switch(vk)
		{
		case VK_UP:
			if (control) goto ParameterInc;
			ai.activepar = shpar[ai.activepar].gup;
			return 1;
			
		case VK_DOWN:
			if (control) goto ParameterDec;			
			ai.activepar = shpar[ai.activepar].gdw;
			return 1;
			
		case VK_LEFT:
			if (control)
			{
ParameterDec:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int v = ap-1;
				if (v<0) v=shpar[ai.activepar].pmax;
				ap=v;
				goto ChangeInstrumentPar;
			}
			else
				ai.activepar = shpar[ai.activepar].gle;
			return 1;
			
		case VK_RIGHT:
			if (control)
			{
ParameterInc:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int v = ap+1;
				if (v>shpar[ai.activepar].pmax) v=0;
				ap=v;
				goto ChangeInstrumentPar;
			}
			else
				ai.activepar = shpar[ai.activepar].gri;
			return 1;
			
		case VK_HOME:
			ai.activepar = PAR_ENVLEN;
			return 1;
			
		case VK_SPACE:
			if (control) break;	//prevents inputing a SPACE while exiting PROVE mode
		case VK_BACK:	//BACKSPACE
		case VK_DELETE:
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ap=0;
			goto ChangeInstrumentPar;
			return 1;

ChangeInstrumentPar:
			//because there has been some change in the instrument parameter => stop this instrument in all channels
			Atari_InstrumentTurnOff(m_activeinstr);
			m_instrs.CheckInstrumentParameters(m_activeinstr);
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;
		}
	}
	else
	if (ai.act==2)
	{
		is_editing_instr = 0;
		//ENVELOPE
		switch(vk)
		{
		case VK_UP:
			if (control)
			{	//
				if (shift)	//SHIFT+CONTROL+UP
				{
					m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
					for(int i=0; i<=ai.par[PAR_ENVLEN]; i++)
					{
						int& ae = ai.env[i][ai.activeenvy];
						ae = (ae+shenv[ai.activeenvy].padd) & shenv[ai.activeenvy].pand;
					}
					goto ChangeInstrumentEnv;
				}
				goto EnvelopeInc;
			}
			i=ai.activeenvy;
			if (i>0)
			{
				i--;
				if (i==0 && g_tracks4_8<=4) i=7;	//mono mode
			}
			else 
				i=7;
			ai.activeenvy=i;
			return 1;
			
		case VK_DOWN: 
			if (control)
			{	//
				if (shift)	//SHIFT+CONTROL+DOWN
				{
					m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
					for(int i=0; i<=ai.par[PAR_ENVLEN]; i++)
					{
						int& ae = ai.env[i][ai.activeenvy];
						ae = (ae+shenv[ai.activeenvy].psub) & shenv[ai.activeenvy].pand;
					}
					goto ChangeInstrumentEnv;
				}
				goto EnvelopeDec;
			}
			i=ai.activeenvy;
			if (i<7) i++; else i=(g_tracks4_8>4)? 0 : 1;
			ai.activeenvy=i;
			return 1;
			
		case VK_LEFT:
			if (control)
			{
EnvelopeDec:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ae = (ae+shenv[ai.activeenvy].psub) & shenv[ai.activeenvy].pand;
				goto ChangeInstrumentEnv;
			}
			else
			{
				i=ai.activeenvx;
				if (i>0) i--; else i=ai.par[PAR_ENVLEN]; //length of env
				ai.activeenvx=i;
			}
			return 1;
			
		case VK_RIGHT:
			if (control)
			{
EnvelopeInc:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ae = (ae+shenv[ai.activeenvy].padd) & shenv[ai.activeenvy].pand;
				goto ChangeInstrumentEnv;
			}
			else
			{
				i=ai.activeenvx;
				if (i<ai.par[PAR_ENVLEN]) i++; else i=0; //length of env
				ai.activeenvx=i;
			}
			return 1;
			
		case VK_HOME:		
			if (control)
			{
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ai.par[PAR_ENVGO] = ai.activeenvx; //sets ENVGO to this column
				goto ChangeInstrumentPar;	//yes, that's fine, it really changed the PARAMETER, even if it's in the envelope
			}
			else
			{
				//goes left to column 0 or to the beginning of the GO loop
				if (ai.activeenvx!=0)
					ai.activeenvx = 0;
				else
					ai.activeenvx=ai.par[PAR_ENVGO];
			}
			return 1;

		case VK_END:
			if (control)
			{
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				if (ai.activeenvx==ai.par[PAR_ENVLEN])	//sets ENVLEN to this column or to the end
					ai.par[PAR_ENVLEN]=ENVCOLS-1;
				else
					ai.par[PAR_ENVLEN]=ai.activeenvx;
				goto ChangeInstrumentPar;	//yes, changed PAR from envelope
			}
			else
			{
				ai.activeenvx=ai.par[PAR_ENVLEN];	//moves the cursor to the right to the end
			}
			return 1;
			
		case 8:			//VK_BACKSPACE:
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ae=0;
			goto ChangeInstrumentEnv;
			return 1;

		case VK_SPACE:	//VK_SPACE
			if (control) break;	//prevents inputing a SPACE while exiting PROVE mode
			{
			 m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			 for (int j=0; j<ENVROWS; j++) ai.env[ai.activeenvx][j]=0;
			 if (ai.activeenvx<ai.par[PAR_ENVLEN]) ai.activeenvx++; //shift to the right
			}
			goto ChangeInstrumentEnv;
			return 1;

		case VK_INSERT:
			if (!control)
			{	//moves the envelope from the current position to the right
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int i,j;
				int ele=ai.par[PAR_ENVLEN];
				int ego=ai.par[PAR_ENVGO];
				if (ele<ENVCOLS-1) ele++;
				if (ai.activeenvx<ego && ego<ENVCOLS-1) ego++;
				for(i=ENVCOLS-2; i>=ai.activeenvx; i--)
				{
					for (j=0; j<ENVROWS; j++) ai.env[i+1][j]=ai.env[i][j];
				}
				//improvement: with shift it will leave it there (it will not erase the column)
				if (!shift) for (j=0; j<ENVROWS; j++) ai.env[ai.activeenvx][j]=0;	
				ai.par[PAR_ENVLEN]=ele;
				ai.par[PAR_ENVGO]=ego;
				goto ChangeInstrumentPar;	//changed length and / or go parameters
			}
			return 0; //without screen update

		case VK_DELETE:
			if (!control)	//!shift &&
			{	//moves the envelope from the current position to the left
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int i,j;
				int ele=ai.par[PAR_ENVLEN];
				int ego=ai.par[PAR_ENVGO];
				if (ele>0)
				{
					ele--;
					for(i=ai.activeenvx; i<ENVCOLS-1; i++)
					{
						for (j=0; j<ENVROWS; j++) ai.env[i][j]=ai.env[i+1][j];
					}
					for (j=0; j<ENVROWS; j++) ai.env[ENVCOLS-1][j]=0;
				}
				else
				{
					for (j=0; j<ENVROWS; j++) ai.env[0][j]=0;
				}
				if (ai.activeenvx<ego) ego--;
				if (ego>ele) ego=ele;
				ai.par[PAR_ENVGO]=ego;
				ai.par[PAR_ENVLEN]=ele;
				if (ai.activeenvx>ele) ai.activeenvx=ele;
				goto ChangeInstrumentPar;	//changed length and / or go parameters
			}
			return 0; //without screen update

ChangeInstrumentEnv:
			//something changed => Save instrument to Atari memory
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;
		}
	}
	if (ai.act==3)
	{
		is_editing_instr = 0;
		//TABLE
		switch(vk)
		{
		case VK_HOME:
			if (control)
			{	//set a TABLE go loop here
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ai.par[PAR_TABGO] = ai.activetab;
				if (ai.activetab>ai.par[PAR_TABLEN]) ai.par[PAR_TABLEN] = ai.activetab;
				goto ChangeInstrumentPar;
			}
			else
			{
				//go to the beginning of the TABLE and to the beginning of the TABLE loop
				if (ai.activetab!=0)
					ai.activetab=0;
				else
					ai.activetab = ai.par[PAR_TABGO];
			}
			return 1;

		case VK_END:
			if (control)
			{	//set TABLE only by location
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				if (ai.activetab==ai.par[PAR_TABLEN])
					ai.par[PAR_TABLEN]=TABLEN-1;
				else
					ai.par[PAR_TABLEN] = ai.activetab;
				goto ChangeInstrumentPar;
			}
			else	//goes to the last parameter in the TABLE
				ai.activetab = ai.par[PAR_TABLEN];
			return 1;

		case VK_UP:
			if (control)
			{
				if (shift)	//Shift+Control+UP
				{
					m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
					for(int i=0; i<=ai.par[PAR_TABLEN]; i++) ai.tab[i]=(ai.tab[i]+1) & 0xff;
					goto ChangeInstrumentTab;
				}
TableInc:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				at = (at+1) & 0xff;
				goto ChangeInstrumentTab;
			}
			return 1;

		case VK_DOWN:
			if (control)
			{
				if (shift)	//Shift+Control+DOWN
				{
					m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
					for(int i=0; i<=ai.par[PAR_TABLEN]; i++) ai.tab[i]=(ai.tab[i]-1) & 0xff;
					goto ChangeInstrumentTab;
				}
TableDec:
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				at = (at-1) & 0xff;
				goto ChangeInstrumentTab;
			}
			return 1;

		case VK_LEFT:
			if (control) goto TableDec;
			i = ai.activetab -1;
			if (i<0) i=ai.par[PAR_TABLEN];
			ai.activetab = i;
			goto ChangeInstrumentTab;

		case VK_RIGHT:
			if (control) goto TableInc;
			i = ai.activetab +1;
			if (i>ai.par[PAR_TABLEN]) i=0;
			ai.activetab = i;
			goto ChangeInstrumentTab;

		case VK_SPACE:	//VK_SPACE: parameter reset and shift by 1 to the right
			if (control) break;	//prevents inputing a SPACE while exiting PROVE mode
			if (ai.activetab<ai.par[PAR_TABLEN]) ai.activetab++;
			//and proceeds the same as VK_BACKSPACE
		case 8:			//VK_BACKSPACE: parameter reset
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			at=0;
			goto ChangeInstrumentTab;

		case VK_INSERT:
			if (!control)
			{	//moves the table from the current position to the right
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int i;
				int tle=ai.par[PAR_TABLEN];
				int tgo=ai.par[PAR_TABGO];
				if (tle<TABLEN-1) tle++;
				if (ai.activetab<tgo && tgo<TABLEN-1) tgo++;
				for(i=TABLEN-2; i>=ai.activetab; i--) ai.tab[i+1]=ai.tab[i];
				if (!shift) ai.tab[ai.activetab]=0; //with the shift it will leave there
				ai.par[PAR_TABLEN]=tle;
				ai.par[PAR_TABGO]=tgo;
				//goto ChangeInstrumentTab; <-- It is not enough!
				goto ChangeInstrumentPar; //changed TABLE LEN or GO, must stop the instrument
			}
			return 0; //without screen update

		case VK_DELETE:
			if (!control) //!shift &&
			{	//moves the table from the current position to the left
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int i;
				int tle=ai.par[PAR_TABLEN];
				int tgo=ai.par[PAR_TABGO];
				if (tle>0)
				{
					tle--;
					for(i=ai.activetab; i<TABLEN-1; i++) ai.tab[i]=ai.tab[i+1];
					ai.tab[TABLEN-1]=0;
				}
				else
					ai.tab[0]=0;
				if (ai.activetab<tgo) tgo--;
				if (tgo>tle) tgo=tle;
				ai.par[PAR_TABLEN]=tle;
				ai.par[PAR_TABGO]=tgo;
				if (ai.activetab>tle) ai.activetab=tle;
				//goto ChangeInstrumentTab; <-- It is not enough!
				goto ChangeInstrumentPar; //changed TABLE LEN or GO, must stop the instrument
			}
			return 0; //without screen update

ChangeInstrumentTab:
			//something changed => Save instrument to Atari memory
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;

		}
	}
	return 0;	//=> SCREENUPDATE will not be performed
}

BOOL CSong::InfoCursorGotoSongname(int x)
{
	x = x/8;
	if (x>=0 && x<SONGNAMEMAXLEN)
	{
		m_songnamecur = x;
		g_activepart=PARTINFO;
		m_infoact = 0;
		is_editing_infos = 1;	//Song Name is being edited
		return 1;
	}
	return 0;
}

BOOL CSong::InfoCursorGotoSpeed(int x)
{
	x = (x-4)/8;
	if (x<2) m_infoact=1;
	else
	if (x<5) m_infoact=2;
	else
		m_infoact=3;
	g_activepart=PARTINFO;
	is_editing_infos = 0;	//Song Speed is being edited
	return 1;
}

BOOL CSong::InfoCursorGotoOctaveSelect(int x, int y)
{
	COctaveSelectDlg dlg;
	CRect rec;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-9,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_octave=m_octave;
	g_mousebutt=0;				//because the dialog sessions of the OnLbuttonUP event
	if (dlg.DoModal()==IDOK)
	{
		m_octave=dlg.m_octave;
		return 1;
	}
	return 0;
}

BOOL CSong::InfoCursorGotoVolumeSelect(int x, int y)
{
	CVolumeSelectDlg dlg;
	CRect rec;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-9,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_volume = m_volume;
	dlg.m_respectvolume = g_respectvolume;

	g_mousebutt=0;				//because the dialog sessions of the OnLbuttonUP event
	if (dlg.DoModal()==IDOK)
	{
		m_volume = dlg.m_volume;
		g_respectvolume = dlg.m_respectvolume;
		return 1;
	}
	return 0;
}

BOOL CSong::InfoCursorGotoInstrumentSelect(int x, int y)
{
	is_editing_instr = 0;
	CInstrumentSelectDlg dlg;
	CRect rec;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-82,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_selected = m_activeinstr;
	dlg.m_instrs = &m_instrs;	//pointer to the instrument object

	g_mousebutt=0;				//because the dialog sessions of the OnLbuttonUP event
	if (dlg.DoModal()==IDOK)
	{
		ActiveInstrSet(dlg.m_selected);
		return 1;
	}
	return 0;
}

BOOL CSong::CursorToSpeedColumn()
{
	if (g_activepart!=PARTTRACKS || SongGetActiveTrack()<0) return 0;
	BLOCKDESELECT;
	m_trackactivecur=3;
	return 1;
}

BOOL CSong::ProveKey(int vk,int shift,int control)
{
	int note,i;
	note = NoteKey(vk);
	if (note>=0)
	{
		i=note+m_octave*12;
		if (i>=0 && i<NOTESNUM)		//only within limits
		{
			SetPlayPressedTonesTNIV(m_trackactivecol,i,m_activeinstr,m_volume);
			if ((control || g_prove==2) && g_tracks4_8>4)
			{	//with control or in prove2 => stereo test
				SetPlayPressedTonesTNIV((m_trackactivecol+4)&0x07,i,m_activeinstr,m_volume);
			}
		}
		return 0; //they don't have to redraw
	}

	if (SongGetGo() >= 0) //is active song go to line => they must not edit anything
	{
		if (!control && (vk == VK_UP || vk == VK_PAGE_UP))  //GO - key up
		{
			m_trackactiveline = 0;
			TrackUp(1);
			return 1;
		}
		if (!control && (vk == VK_DOWN || vk == VK_PAGE_DOWN)) //GO - key down
		{
			m_trackactiveline = m_tracks.m_maxtracklen - 1;
			TrackDown(1, 0);
			return 1;
		}
	}

	switch(vk)
	{
	case VK_LEFT:
		if (control) break;	//do nothing
		if (shift)
			ActiveInstrPrev();
		else if (g_activepart != 1)	//anywhere but tracks
			TrackLeft(1);
		else
			TrackLeft();
		break;

	case VK_RIGHT:
		if (control) break;	//do nothing
		if (shift)
			ActiveInstrNext();
		else if (g_activepart != 1)	//anywhere but tracks
			TrackRight(1);
		else 
			TrackRight();
		break;

	case VK_UP:
		if (shift) break;	//do nothing
		if (control || g_activepart != 1)	//anywhere but tracks
			SongUp();
		else
		{
			if (!g_linesafter) TrackUp(1);
			else TrackUp(g_linesafter);
		}
		break;

	case VK_DOWN:
		if (shift) break;	//do nothing
		if (control || g_activepart != 1)	//anywhere but tracks
			SongDown();
		else
		{
			if (!g_linesafter) TrackDown(1, 0);
			else TrackDown(g_linesafter, 0);	//stoponlastline = 0 => will not stop on the last line of the track
		}
		break;

	case VK_TAB:
		if (shift)
			TrackLeft(1); //SHIFT+TAB
		else if (control)
			CursorToSpeedColumn(); //CTRL+TAB
		else
			TrackRight(1);
		break;

	case VK_SPACE:
		if (control) break;	//prevents inputing a SPACE while exiting PROVE mode
		break;

	case VK_SUBTRACT:
		VolumeDown();
		break;

	case VK_ADD:
		VolumeUp();
		break;

	case VK_DIVIDE:
		OctaveDown();
		break;
	
	case VK_MULTIPLY:
		OctaveUp();
		break;

	case VK_PAGE_UP:
		if (g_activepart != 1)
		{
			if (shift)
				SongSubsongPrev();
			else
			{
				SongUp();
			}
			break;
		}
		else
		if (g_activepart == 1)
		{
			if (!shift && control)
			{
				SongUp();
			}
			else
			if (!control && shift)
			{
				//move to the previous goto
				SongSubsongPrev();
			}
			if (m_play && m_followplay) break;	//prevents moving at all during play+follow
			else
			{
				if (m_trackactiveline > 0)
				{
					m_trackactiveline = ((m_trackactiveline - 1) / g_tracklinehighlight) * g_tracklinehighlight;
				}
			}
		}
		break;

	case VK_PAGE_DOWN:
		if (g_activepart != 1)
		{
			if (shift)
				SongSubsongNext();
			else
			{
				SongDown();
			}
			break;
		}
		else
		if (g_activepart == 1)
		{
			if (!shift && control)
			{
				SongDown();
			}
			else
			if (!control && shift)
			{
				//move to the next goto
				SongSubsongNext();
			}
			if (m_play && m_followplay) break;	//prevents moving at all during play+follow
			else
			{
				i = ((m_trackactiveline + g_tracklinehighlight) / g_tracklinehighlight) * g_tracklinehighlight;
				if (i >= m_tracks.m_maxtracklen) i = m_tracks.m_maxtracklen - 1;
				m_trackactiveline = i;
			}
		}
		break;

	case VK_HOME:
		if (control || shift) break; //do nothing
		if (g_activepart == 1)	//tracks
			m_trackactiveline = 0;		//line 0
		else if (g_activepart == 3)	//song lines
			m_songactiveline = 0;
		break;

	case VK_END:
		if (control || shift) break; //do nothing
		if (g_activepart == 1)	//tracks
		{
			if (TrackGetGoLine() >= 0)
				m_trackactiveline = m_tracks.m_maxtracklen - 1; //last line
			else
				m_trackactiveline = TrackGetLastLine();	//end line
			if (m_trackactiveline < 0) m_trackactiveline = m_tracks.m_maxtracklen - 1; //failsafe in case the active line is out of bounds
		}
		else if (g_activepart == 3)	//song lines
		{
			int i, j, la = 0;
			for (j = 0; j < SONGLEN; j++)
			{
				for (i = 0; i < g_tracks4_8; i++) if (m_song[j][i] >= 0) { la = j; break; }
				if (m_songgo[j] >= 0) la = j;
			}
			m_songactiveline = la;
		}
		break;

	case 13:		//VK_ENTER:
		if (g_activepart == 1)
		{
			int instr, vol;
			if ((BOOL)control != (BOOL)g_keyboard_swapenter)	//control+Enter => plays a whole line (all tracks)
			{
				//for all track columns except the active track column
				for (i = 0; i < g_tracks4_8; i++)
				{
					if (i != m_trackactivecol)
					{
						TrackGetLoopingNoteInstrVol(m_song[m_songactiveline][i], note, instr, vol);
						if (note >= 0)		//is there a note?
							SetPlayPressedTonesTNIV(i, note, instr, vol);	//it will lose it as it is there
						else
							if (vol >= 0) //there is no note, but is there a separate volume?
								SetPlayPressedTonesV(i, vol);				//adjust the volume as it is
					}
				}
			}
			//and now for that active track column
			TrackGetLoopingNoteInstrVol(SongGetActiveTrack(), note, instr, vol);
			if (note >= 0)		//is there a note?
			{
				SetPlayPressedTonesTNIV(m_trackactivecol, note, instr, vol);	//it will lose it as it is there
			}
			else
			if (vol >= 0) //there is no note, but is there a separate volume?
			{
				SetPlayPressedTonesV(m_trackactivecol, vol); //adjust the volume
			}
		TrackDown(1, 0);	//move down 1 step always
		}
		else
		if (g_activepart != 1)
		{
			g_activepart = g_active_ti;
			return 1;
		}
	break;

	default:
		return 0;
		break;
	}
	return 1;
}


BOOL CSong::TrackKey(int vk, int shift, int control)
{
//
#define VKX_SONGINSERTLINE	73		//VK_I
#define VKX_SONGDELETELINE	85		//VK_U
#define VKX_SONGDUPLICATELINE 79	//VK_O
#define VKX_SONGPREPARELINE 80		//VK_P
#define VKX_SONGPUTNEWTRACK 78		//VK_N
#define VKX_SONGMAKETRACKSDUPLICATE 68	//VK_D
//
	int note, i, j;

	if (g_trackcl.IsBlockSelected() && SongGetActiveTrack() != g_trackcl.m_seltrack) BLOCKDESELECT;

	if (SongGetGo() >= 0) //is active song go to line => they must not edit anything
	{
		if (!control && (vk == VK_UP || vk == VK_PAGE_UP))  //GO - key up
		{
			m_trackactiveline = 0;
			TrackUp(1);
			//TrackUp(g_linesafter);	//assumes the pattern it went from was on line 0
			return 1;
		}
		if (!control && (vk == VK_DOWN || vk == VK_PAGE_DOWN)) //GO - key down
		{
			m_trackactiveline = m_tracks.m_maxtracklen - 1;
			TrackDown(1, 0);
			//TrackDown(g_linesafter, 0);	//doesn't work too well, better reset to line 0
			return 1;
		}
		if (!control && !shift) return 0;
		if (control && (vk == 8 || vk == 71)) //control+backspace or control+G
		{
			SongTrackGoOnOff();
			return 1;
		}
		if (control && !shift && (vk == VKX_SONGINSERTLINE || vk == VKX_SONGDELETELINE || vk == VKX_SONGPREPARELINE || vk == VKX_SONGDUPLICATELINE || vk == VK_PAGE_UP || vk == VK_PAGE_DOWN)) goto TrackKeyOk;
		if (vk != VK_LEFT && vk != VK_RIGHT && vk != VK_UP && vk != VK_DOWN) return 0;
	}
TrackKeyOk:

	switch (m_trackactivecur)
	{
	case 0: //note column
		if (control) break;		//with control, notes are not entered (break continues)
		note = NoteKey(vk);
		if (note >= 0)
		{
insertnotes:
			i = note + m_octave * 12;
			if (i >= 0 && i < NOTESNUM)		//only within limits
			{
				BLOCKDESELECT;
				//Quantization
				if (m_play && m_followplay && (m_speeda < (m_speed / 2)))
				{
					m_quantization_note = i;
					m_quantization_instr = m_activeinstr;
					m_quantization_vol = m_volume;
					return 1;
				}
				//end Quantization
				if (TrackSetNoteActualInstrVol(i))
				{
					SetPlayPressedTonesTNIV(m_trackactivecol, i, m_activeinstr, TrackGetVol());
					if (!(m_play && m_followplay)) TrackDown(g_linesafter);
				}
			}
			return 1;
		}
		else //the numbers 1-6 on the numeral are overwritten by an octave
			if ((j = Numblock09Key(vk)) >= 1 && j <= 6 && m_trackactiveline <= TrackGetLastLine())
			{
				note = TrackGetNote();
				if (note >= 0)		//is there a note?
				{
					BLOCKDESELECT;
					note = (note % 12) + ((j - 1) * 12);		//changes its octave according to the number pressed on the numblock
					if (note >= 0 && note < NOTESNUM)
					{
						int instr = TrackGetInstr(), vol = TrackGetVol();
						if (TrackSetNoteInstrVol(note, instr, vol))
							SetPlayPressedTonesTNIV(m_trackactivecol, note, instr, vol);
					}
				}
				if (!(m_play && m_followplay)) TrackDown(g_linesafter);
				return 1;
			}
		break;

	case 1: //instrument column
		i = NumbKey(vk);
		note = NoteKey(vk);	//workaround: the note key is known early in case it is needed
		if (i >= 0 && !shift && !control)
		{
			BLOCKDESELECT;
			if (TrackGetNote() >= 0) //the instrument number can only be changed if there is a note
			{
				j = ((TrackGetInstr() & 0x0f) << 4) | i;
				if (j >= INSTRSNUM) j &= 0x0f;	//leaves only the lower digit
				TrackSetInstr(j);
			}
			else goto testnotevalue;	//attempt to catch a fail by testing the other possible condition anyway
			return 1;
		}
		else if (note >= 0 && !shift && !control)
		{
testnotevalue:
			BLOCKDESELECT;
			if (TrackGetNote() >= 0) break; //do not input a note if there is already a note!
			else goto insertnotes;	//force a note insertion otherwise
		}
		break;

	case 2: //volume column
		i = NumbKey(vk);
		if (i >= 0 && !shift && !control)
		{
			BLOCKDESELECT;
			if (TrackSetVol(i) && !(m_play && m_followplay)) TrackDown(g_linesafter);
			return 1;
		}
		break;

	case 3: //speed column
		i = NumbKey(vk);
		if (i >= 0 && !shift && !control)
		{
			BLOCKDESELECT;
			j = TrackGetSpeed();
			if (j < 0) j = 0;
			j = ((j & 0x0f) << 4) | i;
			if (j >= TRACKMAXSPEED) j &= 0x0f;	//leaves only the lower digit
			if (j <= 0) j = -1;	//zero does not exist
			TrackSetSpeed(j);
			return 1;
		}
		break;

	}

	switch (vk)
	{
	case VK_UP:
		if (control && shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//volume change incrementing
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockVolumeChange(m_activeinstr, 1);
		}
		else 
		if (shift && !control)
		{
			//block selection
			BLOCKSETBEGIN;
			if (!g_linesafter) TrackUp(1);
			else TrackUp(g_linesafter);
			BLOCKSETEND;
		}
		else 
		if (control && !shift)
		{
			if (ISBLOCKSELECTED)
			{
				BLOCKDESELECT;
				break;
			}
			else SongUp();
		}
		else
		{
			BLOCKDESELECT;
			if (!g_linesafter) TrackUp(1);
			else TrackUp(g_linesafter);
		}
		break;

	case VK_DOWN:
		if (control && shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//volume change decrementing
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockVolumeChange(m_activeinstr, -1);
		}
		else 
		if (shift && !control)
		{
			//block selection
			BLOCKSETBEGIN;
			if (!g_linesafter) TrackDown(1,0);
			else TrackDown(g_linesafter,0);	//will not stop on the last line
			BLOCKSETEND;
		}
		else 
		if (control && !shift)
		{
			if (ISBLOCKSELECTED)
			{
				BLOCKDESELECT;
				break;
			}
			else SongDown();
		}
		else
		{
			BLOCKDESELECT;
			if (!g_linesafter) TrackDown(1, 0);
			else TrackDown(g_linesafter, 0);	//will not stop on the last line
		}
		break;

	case VK_LEFT:
		if (control && shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//instrument changes decrementing
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockInstrumentChange(m_activeinstr, -1);
		}
		else
		if (shift && !control)
			ActiveInstrPrev();
		else 
		if (control && !shift)
		{
			if (ISBLOCKSELECTED)
			{
				BLOCKDESELECT;
				break;
			}
			else SongTrackDec();
		}
		else
		{
			BLOCKDESELECT;
			TrackLeft();
		}
		break;

	case VK_RIGHT:
		if (control && shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//instrument changes incrementing
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockInstrumentChange(m_activeinstr, 1);
		}
		else 
		if (shift && !control)
			ActiveInstrNext();
		else
		if (control && !shift)
		{
			if (ISBLOCKSELECTED)
			{
				BLOCKDESELECT;
				break;
			}
			else SongTrackInc();
		}
		else
		{
			BLOCKDESELECT;
			TrackRight();
		}
		break;

	case VK_PAGE_UP:
		if (!shift && control)
		{
			BLOCKDESELECT;
			SongUp();
		}
		else
		if (!control && shift)
		{
			//move to the previous goto
			BLOCKDESELECT;
			SongSubsongPrev();
		}
		else
		if (m_play && m_followplay) break;	//prevents moving at all during play+follow
		else
		{
			BLOCKDESELECT;
			if (m_trackactiveline>0)
			{
				m_trackactiveline = ((m_trackactiveline-1) / g_tracklinehighlight) * g_tracklinehighlight;
			}
		}
		break;

	case VK_PAGE_DOWN:
		if (!shift && control)
		{
			BLOCKDESELECT;
			SongDown();
		}
		else
		if (!control && shift)
		{
			//move to the next goto
			BLOCKDESELECT;
			SongSubsongNext();
		}
		else
		if (m_play && m_followplay) break;	//prevents moving at all during play+follow
		else
		{
			BLOCKDESELECT;
			i = ((m_trackactiveline+g_tracklinehighlight) / g_tracklinehighlight) * g_tracklinehighlight;
			if (i>=m_tracks.m_maxtracklen) i = m_tracks.m_maxtracklen-1;
			m_trackactiveline = i;
		}
		break;

	case VK_SUBTRACT:
		VolumeDown();
		break;

	case VK_ADD:
		VolumeUp();
		break;

	case VK_DIVIDE:
		OctaveDown();
		break;

	case VK_MULTIPLY:
		OctaveUp();
		break;

	case VK_TAB:
		BLOCKDESELECT;
		if (shift)
			TrackLeft(1); //SHIFT+TAB
		else if (control)
			CursorToSpeedColumn(); //CTRL+TAB
		else
			TrackRight(1);
		break;

	case VK_ESCAPE:
		BLOCKDESELECT;
		break;
	
	case 65:	//VK_A
		if (g_trackcl.IsBlockSelected() && shift && control)
		{	//Shift+control+A
			//switch ALL / no ALL
			g_trackcl.BlockAllOnOff();
		}
		else
		if (control && !shift)
		{
			//control+A
			//selection of the whole track (from 0 to the length of that track)
			g_trackcl.BlockDeselect();
			g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),0);
			if (TrackGetGoLine() >= 0)
				g_trackcl.BlockSetEnd(m_tracks.m_maxtracklen - 1);
			else
				g_trackcl.BlockSetEnd(m_tracks.GetLastLine(SongGetActiveTrack()));
		}
		break;

	case 66:	//VK_B			//restore block from backup
		if (g_trackcl.IsBlockSelected() && control && !shift)
		{
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			g_trackcl.BlockRestoreFromBackup();
		}
		break;

	case 67:	//VK_C
		if (control && !shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			g_trackcl.BlockCopyToClipboard();
		}
		break;

	case 69:	//VK_E
		if (control && !shift)		//exchange block and clipboard
		{
			if (g_trackcl.IsBlockSelected()) 
			{
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
				if (!g_trackcl.BlockExchangeClipboard()) m_undo.DropLast();
			}
		}
		break;

	case 0x4D:	//VK_M
		if (control && !shift)
		{
			BLOCKDESELECT;
			BlockPaste(1);	//paste merge
		}
		break;

	case 86:	//VK_V
		if (control && !shift)
		{
			BLOCKDESELECT;
			BlockPaste();	//classic paste
		}
		break;

	case 88:	//VK_X
		if (control && !shift)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			g_trackcl.BlockCopyToClipboard();
			g_trackcl.BlockClear();
		}
		break;

	case 70:	//VK_F
		if (control && !shift && g_trackcl.IsBlockSelected())
		{
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			if (!g_trackcl.BlockEffect()) m_undo.DropLast();
		}
		break;

	case 71:	//VK_G		//song goto on/off
		BLOCKDESELECT;
		if (control && !shift) SongTrackGoOnOff();	//control+G => goto on/off line in the song
		break;

	case VKX_SONGPUTNEWTRACK:	//VK_N
		BLOCKDESELECT;
		if (control && !shift)
			SongPutnewemptyunusedtrack();
		break;

	case VKX_SONGMAKETRACKSDUPLICATE:	//VK_D
		BLOCKDESELECT;
		if (control && !shift)
			SongMaketracksduplicate();
		break;

	case VK_HOME:
		if (control)
			TrackSetGo();
		else
		{
			if (shift)
			{
				BLOCKSETBEGIN;
				m_trackactiveline = 0;		//line 0
				BLOCKSETEND;
			}
			else
			{
				if (g_trackcl.IsBlockSelected())
				{
					//sets to the first line in the block
					int bfro,bto;
					g_trackcl.GetFromTo(bfro,bto);
					m_trackactiveline=bfro;
				}
				else
				{
					if (m_trackactiveline!=0)
						m_trackactiveline = 0;		//line 0
					else
					{
						i=TrackGetGoLine();
						if (i>=0) m_trackactiveline = i;	//at the beginning of the GO loop
					}
					BLOCKDESELECT;
				}
			}
		}
		break;

	case VK_END:
		if (control)
			TrackSetEnd();
		else
		{
			if (shift)
			{
				BLOCKSETBEGIN;
				if (TrackGetGoLine() >= 0)
					m_trackactiveline = m_tracks.m_maxtracklen - 1; //last line
				else
					m_trackactiveline = TrackGetLastLine();	//end line
				BLOCKSETEND;
				if (m_trackactiveline < 0)
				{
					m_trackactiveline = m_tracks.m_maxtracklen - 1; //failsafe in case the active line is out of bounds
					BLOCKDESELECT;	//prevents selecting invalid data
				}
			}
			else
			{
				if (g_trackcl.IsBlockSelected())
				{
					//sets to the first line in the block
					int bfro,bto;
					g_trackcl.GetFromTo(bfro,bto);
					m_trackactiveline=bto;
				}
				else
				{
					i = TrackGetLastLine();
					if (i != m_trackactiveline)
					{
						m_trackactiveline = i;	//at the end of the GO loop or end line
						if (m_trackactiveline < 0) m_trackactiveline = m_tracks.m_maxtracklen - 1; //failsafe in case the active line is out of bounds
					}
					else m_trackactiveline = m_tracks.m_maxtracklen - 1; //last line
					BLOCKDESELECT;
				}
			}
		}
		break;

	case 13:		//VK_ENTER:
		int instr, vol, oldline;
		{
			if (shift && control)
			{
				BLOCKDESELECT;
				TrackSetEnd();
				break;
			}
			if (!shift && (BOOL)control != (BOOL)g_keyboard_swapenter)	//control+Enter => plays a whole line (all tracks)
			{
				//for all track columns except the active track column
				for(i=0; i<g_tracks4_8; i++)
				{
					if (i!=m_trackactivecol)
					{
						TrackGetLoopingNoteInstrVol(m_song[m_songactiveline][i],note,instr,vol);
						if (note>=0)		//is there a note?
							SetPlayPressedTonesTNIV(i,note,instr,vol);	//it will lose it as it is there
						else
						if (vol>=0) //there is no note, but is there a separate volume?
							SetPlayPressedTonesV(i,vol);				//adjust the volume as it is
					}
				}
			}
			//and now for that active track column
			TrackGetLoopingNoteInstrVol(SongGetActiveTrack(),note,instr,vol);
			if (note>=0)		//is there a note?
			{
				SetPlayPressedTonesTNIV(m_trackactivecol,note,instr,vol);	//it will lose it as it is there
				if (shift && !control)	//with the shift, this instrument and the volume will "pick up" as current (only if it is not 0)
				{
					ActiveInstrSet(instr);
					if (vol>0) m_volume = vol;
				}
			}
			else
			if (vol>=0) //there is no note, but is there a separate volume?
			{
				SetPlayPressedTonesV(m_trackactivecol,vol); //adjust the volume
				if (shift && !control && vol>0) m_volume = vol; //"picks up" the volume as current (only if it is not 0)
			}
		}
		oldline = m_trackactiveline;
		TrackDown(1, 0);	//move down 1 step always
		if (oldline == m_trackactiveline) m_trackactiveline++;	//hack, force a line move even if TrackDown prevents it after Enter called it, otherwise the last line would get stuck
		if (g_trackcl.IsBlockSelected())	//if a block is selected, it moves (and plays) only in it
		{
			int bfro,bto;
			g_trackcl.GetFromTo(bfro,bto);
			if (m_trackactiveline<bfro || m_trackactiveline>bto) m_trackactiveline=bfro;
		}
		break;

	case VKX_SONGINSERTLINE:	//VK_I:
		if (control && !shift)
			goto insertline;
			break;

	case VKX_SONGDELETELINE:	//VK_U:
		if (control && !shift)
			goto deleteline;
			break;

	case VK_INSERT:
		{
insertline:
			BLOCKDESELECT;
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,0);
			m_tracks.InsertLine(SongGetActiveTrack(),m_trackactiveline);
		}
		break;

	case VK_DELETE:
		if (g_trackcl.IsBlockSelected())
		{
			//the block is selected, so it deletes it
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			g_trackcl.BlockClear();
		}
		else
		if (!shift)
		{
deleteline:
			BLOCKDESELECT;
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,0);
			m_tracks.DeleteLine(SongGetActiveTrack(),m_trackactiveline);
		}
		break;

	case VK_SPACE:
		if (control) break; //fixes the "return to EDIT MODE space input" bug, by ignoring SPACE if CTRL is also detected
		BLOCKDESELECT;
		if (TrackDelNoteInstrVolSpeed(1+2+4+8)) //all
		{
			if (!(m_play && m_followplay)) TrackDown(g_linesafter);
		}
		break;

	case 8:			//VK_BACKSPACE:
	{
		BLOCKDESELECT;
		int r = 0;
		switch (m_trackactivecur)
		{
		case 0:	//note
		case 1: //instrument
			r = TrackDelNoteInstrVolSpeed(1 + 2); //delete note + instrument
			break;
		case 2: //volume
			r = TrackDelNoteInstrVolSpeed(1 + 2 + 4); //delete note + instrument + volume
			break;
		case 3: //speed
			r = TrackSetSpeed(-1);	//delete speed
			break;
		}
		if (r)
		{
			if (!(m_play && m_followplay)) TrackDown(g_linesafter);
		}
	}
		break;

	case VK_F1:
		if (control)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//transpose down by 1 semitone
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockNoteTransposition(m_activeinstr, -1);
		}
		break;

	case VK_F2:
		if (control)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//transpose up by 1 semitone
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockNoteTransposition(m_activeinstr, 1);
		}
		break;

	case VK_F3:
		if (control)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//transpose down by 1 octave
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockNoteTransposition(m_activeinstr, -12);
		}
		break;

	case VK_F4:
		if (control)
		{
			if (!g_trackcl.IsBlockSelected())
			{	//if no block is selected, make a block at the current location
				g_trackcl.BlockSetBegin(m_trackactivecol, SongGetActiveTrack(), m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			//transpose up by 1 octave
			m_undo.ChangeTrack(SongGetActiveTrack(), m_trackactiveline, UETYPE_TRACKDATA);
			g_trackcl.BlockNoteTransposition(m_activeinstr, 12);
		}
		break;

	default:
		return 0;
		break;
	}
	return 1;
}

BOOL CSong::TrackCursorGoto(CPoint point)
{
	int xch,x,y;
	xch = (point.x / (16 * 8));
	x = (point.x - (xch * 16 * 8)) / 8;
	y=(point.y+0)/16-8+g_cursoractview;	//m_trackactiveline;

	if (y>=0 && y<m_tracks.m_maxtracklen)
	{
		if (xch>=0 && xch<g_tracks4_8) m_trackactivecol=xch;
		if (m_play && m_followplay)	//prevents moving at all during play+follow
			goto notracklinechange;
		else
			m_trackactiveline=y;
	}
	else
		return 0;
notracklinechange:
	switch(x)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		m_trackactivecur = 0;
		break;
	case 4:
	case 5:
	case 6:
		m_trackactivecur = 1;
		break;
	case 7:
	case 8:
	case 9:
		m_trackactivecur = 2;
		break;
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:	//filling more area avoids jumping all over the place, between the speed column and the next channel's note column
	case 16:
		m_trackactivecur = 3;
		break;
	}
	g_activepart=PARTTRACKS;
	return 1;
}

BOOL CSong::TrackUp(int lines)
{
	if (m_play && m_followplay)	//prevents moving at all during play+follow
		return 0;
	m_undo.Separator();
	m_trackactiveline -= lines;
	if (m_trackactiveline < 0)
	{
		if (ISBLOCKSELECTED)
		{
			m_trackactiveline = 0;	//prevent moving anywhere else
			return 1;
		}
		if (g_keyboard_updowncontinue)
		{
			BLOCKDESELECT;
			SongUp();
		}
		m_trackactiveline = m_trackactiveline + TrackGetLastLine() + 1;
		if (m_trackactiveline < 0 || TrackGetGoLine() >= 0) m_trackactiveline = m_trackactiveline + m_tracks.m_maxtracklen;
		if (m_trackactiveline > m_tracks.m_maxtracklen) m_trackactiveline = m_trackactiveline - TrackGetLastLine() - 1;
	}
	return 1;
}

BOOL CSong::TrackDown(int lines, BOOL stoponlastline)
{
	if (m_play && m_followplay)	//prevents moving at all during play+follow
		return 0;
	if (!g_keyboard_updowncontinue && stoponlastline && m_trackactiveline + lines > TrackGetLastLine()) return 0;
	m_undo.Separator();
	m_trackactiveline += lines;	
	if (SongGetActiveTrack() >= 0 && TrackGetGoLine() < 0)
	{
		int trlen = TrackGetLastLine() + 1;
		if (m_trackactiveline >= trlen) 
		{
			if (ISBLOCKSELECTED)
			{
				m_trackactiveline = trlen - 1;	//prevent moving anywhere else
				return 1;
			}
			m_trackactiveline = m_trackactiveline % trlen; 
			if (g_keyboard_updowncontinue)
			{ 
				BLOCKDESELECT;
				SongDown(); 
			}
		}	
	}
	else
	{
		if (m_trackactiveline >= m_tracks.m_maxtracklen) 
		{ 
			if (ISBLOCKSELECTED)
			{
				m_trackactiveline = m_tracks.m_maxtracklen - 1;	//prevent moving anywhere else
				return 1;
			}
			m_trackactiveline = m_trackactiveline % m_tracks.m_maxtracklen; 
			if (g_keyboard_updowncontinue) 
			{
				BLOCKDESELECT;
				SongDown();
			}
		}	
	}
	return 1;
}

BOOL CSong::TrackLeft(BOOL column)
{
	m_undo.Separator();
	if (column || m_trackactiveline>TrackGetLastLine()) goto track_leftcolumn;
	m_trackactivecur--;
	if (m_trackactivecur<0)
	{
		m_trackactivecur = 3;	//previous speed column
track_leftcolumn:
		m_trackactivecol--;
		if (m_trackactivecol<0) m_trackactivecol=g_tracks4_8-1;
	}
	return 1;
}

BOOL CSong::TrackRight(BOOL column)
{
	m_undo.Separator();
	if (column || m_trackactiveline>TrackGetLastLine()) goto track_rightcolumn;
	m_trackactivecur++;
	if (m_trackactivecur > 3)	//speed column
	{
		m_trackactivecur=0;
track_rightcolumn:
		m_trackactivecol++;
		if (m_trackactivecol>=g_tracks4_8) m_trackactivecol=0;
	}
	return 1;
}

void CSong::TrackGetLoopingNoteInstrVol(int track,int& note,int& instr,int& vol)
{
	//set the current visible note to a possible goto loop
	int line,len,go;
	len = m_tracks.GetLastLine(track)+1;
	go = m_tracks.GetGoLine(track);
	if (m_trackactiveline < len)
		line = m_trackactiveline;
	else
	{
		if (go>=0)
			line = (m_trackactiveline - len) % (go - len) + go;
		else
		{
			note=instr=vol=-1;
			return;
		}
	}
	note = m_tracks.GetNote(track,line);
	instr = m_tracks.GetInstr(track,line);
	vol = m_tracks.GetVol(track,line);
}

int* CSong::GetUECursor(int part)
{
	int *cursor;
	switch(part)
	{
	case PARTTRACKS:
		cursor=new int[4];
		cursor[0]=m_songactiveline;
		cursor[1]=m_trackactiveline;
		cursor[2]=m_trackactivecol;
		cursor[3]=m_trackactivecur;
		break;

	case PARTSONG:
		cursor=new int[2];
		cursor[0]=m_songactiveline;
		cursor[1]=m_trackactivecol;
		break;

	case PARTINSTRS:
		{
		cursor=new int[6];
		cursor[0]=m_activeinstr;
		TInstrument *in=&(GetInstruments()->m_instr[m_activeinstr]);
		cursor[1]=in->act;
		cursor[2]=in->activeenvx;
		cursor[3]=in->activeenvy;
		cursor[4]=in->activepar;
		cursor[5]=in->activetab;
		//=in->activenam; It omits that any change in the cursor position in the name is not a reason for undo separation
		}
		break;

	case PARTINFO:
		{
		cursor=new int[1];
		cursor[0]=m_infoact;
		}
		break;

	default:
		cursor=NULL;
	}
	return cursor;
}

void CSong::SetUECursor(int part,int* cursor)
{
	switch(part)
	{
	case PARTTRACKS:
		m_songactiveline=cursor[0];
		m_trackactiveline=cursor[1];
		m_trackactivecol=cursor[2];
		m_trackactivecur=cursor[3];
		g_activepart=g_active_ti=PARTTRACKS;
		break;

	case PARTSONG:
		m_songactiveline=cursor[0];
		m_trackactivecol=cursor[1];
		g_activepart=PARTSONG;
		break;

	case PARTINSTRS:
		m_activeinstr=cursor[0];
		//the other parameters 1-5 are within the instrument (TInstrument structure), so it is not necessary to set
		g_activepart=g_active_ti=PARTINSTRS;
		break;

	case PARTINFO:
		m_infoact = cursor[0];
		g_activepart=PARTINFO;
		break;

	default:
		return;	//don't change g_activepart !!!

	}
}

BOOL CSong::UECursorIsEqual(int* cursor1, int* cursor2, int part)
{
	int len;
	switch(part)
	{
	case PARTTRACKS:	len=4;
						break;
	case PARTSONG:		len=2;
						break;
	case PARTINSTRS:	len=6;
						break;
	case PARTINFO:		len=1;
						break;
	default:
		return 0;
	}
	for(int i=0; i<len; i++) if (cursor1[i]!=cursor2[i]) return 0;
	return 1;
}


//----------


BOOL CSong::SongKey(int vk,int shift,int control)
{
	int isgo = (m_songgo[m_songactiveline]>=0)? 1:0;

	if (!control && NumbKey(vk)>=0)
	{
		return SongTrackSetByNum(NumbKey(vk));
	}

	switch(vk)
	{
	case VK_UP:
		BLOCKDESELECT;
		SongUp();
		break;

	case VK_DOWN: 
		BLOCKDESELECT;
		SongDown();
		break;

	case VK_LEFT:
		if (shift)
			ActiveInstrPrev();
		else
		if (control)
		{
			if (isgo)
				SongTrackGoDec();
			else
				SongTrackDec();
		}
		else
			TrackLeft(1);
		break;

	case VK_RIGHT:
		if (shift)
			ActiveInstrNext();
		else
		if (control)
		{
			if (isgo)
				SongTrackGoInc();
			else
				SongTrackInc();
		}
		else
			TrackRight(1);
		break;

	case VK_TAB:
		if (shift)
			TrackLeft(1); //SHIFT+TAB
		else
			TrackRight(1);
		break;

	case VKX_SONGDELETELINE:	//Control+VK_U:
		if (!control) break;
	case VK_DELETE:
		SongDeleteLine(m_songactiveline);
		break;

	case VKX_SONGINSERTLINE:	//Control+VK_I:
		if (!control) break;
	case VK_INSERT:
		SongInsertLine(m_songactiveline);
		break;

	case VKX_SONGDUPLICATELINE:	//Control+VK_O
		if (control)
			SongInsertCopyOrCloneOfSongLines(m_songactiveline);
		break;

	case VKX_SONGPREPARELINE:	//Control+VK_P
		if (control)
			SongPrepareNewLine(m_songactiveline);
		break;

	case VKX_SONGPUTNEWTRACK:	//Control+VK_N
		if (control)
			SongPutnewemptyunusedtrack();
		break;

	case VKX_SONGMAKETRACKSDUPLICATE:	//Control+VK_D
		BLOCKDESELECT;
		if (control)
			SongMaketracksduplicate();
		break;

	case 8:			//VK_BACKSPACE:
		if (isgo) 
			SongTrackGoOnOff();	//Go off
		else
			SongTrackEmpty();
		break;

	case 71:		//VK_G
		if (control)
			SongTrackGoOnOff();	//Go on/off
		break;

	case 13:		//VK_ENTER
		g_activepart = g_active_ti;
		break;

	case VK_HOME:
		m_songactiveline=0;
		break;

	case VK_END:
		{
			int i,j,la=0;
			for(j=0; j<SONGLEN; j++)
			{
				for(i=0; i<g_tracks4_8; i++) if (m_song[j][i]>=0) { la=j; break; }
				if (m_songgo[j]>=0) la=j;
			}
			m_songactiveline=la;
		}
		break;

	case VK_PAGE_UP:
		if (shift)
			SongSubsongPrev();
		else
			SongUp();
		break;

	case VK_PAGE_DOWN:
		if (shift)
			SongSubsongNext();
		else
			SongDown();
		break;

	case VK_MULTIPLY:
	{
		OctaveUp();
		return 1;
	}
	break;

	case VK_DIVIDE:
	{
		OctaveDown();
		return 1;
	}
	break;

	case VK_ADD:
	{
		VolumeUp();
		return 1;
	}

	case VK_SUBTRACT:
	{
		VolumeDown();
		return 1;
	}

	default:
		return 0;
		break;
	}
	return 1;
}


BOOL CSong::SongCursorGoto(CPoint point)
{
	int xch,y;
	xch=((point.x+4)/(3*8));
	y=(point.y+0)/16-2+m_songactiveline;
	if (y>=0 && y<SONGLEN)
	{
		if (xch>=0 && xch<g_tracks4_8) m_trackactivecol=xch;
		if (y!=m_songactiveline)
		{
			g_activepart=PARTSONG;
			if (m_play && m_followplay)
			{
				int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
				Stop();
				m_songplayline = m_songactiveline = y;
				m_trackplayline = m_trackactiveline = 0;
				Play(mode, m_followplay); // continue playing using the correct parameters
			}
			else
				m_songactiveline = y;
		}
		
	}
	else
		return 0;
	m_trackactivecol=xch;
	g_activepart=PARTSONG;
	return 1;
}

BOOL CSong::SongUp()
{
	BLOCKDESELECT;
	m_undo.Separator();
	m_songactiveline--;
	if (m_songactiveline<0) m_songactiveline=SONGLEN-1;

	if (m_play && m_followplay)
	{
		int isgo = (m_songgo[m_songactiveline] >= 0) ? 1 : 0;
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline -= isgo;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongDown()
{
	BLOCKDESELECT;
	m_undo.Separator();
	m_songactiveline++;
	if (m_songactiveline>=SONGLEN) m_songactiveline=0;

	if (m_play && m_followplay)
	{
		int isgo = (m_songgo[m_songactiveline] >= 0) ? 1 : 0;
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline += isgo;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongSubsongPrev()
{
	m_undo.Separator();
	int i;
	i=m_songactiveline-1;
	if (m_trackactiveline==0) i--;	//is on 0. line => search for 1 songline driv
	for(; i>=0; i--)
	{
		if (m_songgo[i]>=0)
		{
			m_songactiveline=i+1;
			break;
		}
	}
	if (i<0) m_songactiveline=0;
	m_trackactiveline=0;
	if (m_play && m_followplay)
	{
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongSubsongNext()
{
	m_undo.Separator();
	int i;
	for(i=m_songactiveline; i<SONGLEN; i++)
	{
		if (m_songgo[i]>=0)
		{
			if (i<(SONGLEN-1))
				m_songactiveline=i+1;
			else
				m_songactiveline=SONGLEN-1; //Goto on the last songline (=> it is not possible to set a line below it!)
			m_trackactiveline=0;
			break;
		}
	}
	if (m_play && m_followplay)
	{
		int mode = (m_play == MPLAY_TRACK) ? MPLAY_TRACK : MPLAY_FROM;	//play track in loop, else, play from cursor position
		Stop();
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		Play(mode, m_followplay); // continue playing using the correct parameters
	}
	return 1;
}

BOOL CSong::SongTrackSet(int t)
{
	if ( t>=-1 && t<TRACKSNUM)
	{
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	return 1;
}

BOOL CSong::SongTrackSetByNum(int num)
{
	int i;
	if (m_songgo[m_songactiveline]<0) // GO ?
	{	//changes track
		i = SongGetActiveTrack();
		if (i<0) i=0;
		i &= 0x0f;	//just the lower digit
		i = (i<<4) | num;
		if ( i >= TRACKSNUM ) i &= 0x0f;
		return SongTrackSet(i);
	}
	else
	{	//changes GO parameter
		i = m_songgo[m_songactiveline];
		if (i<0) i=0;
		i &= 0x0f;	//just the lower digit
		i = (i<<4) | num;
		if ( i >= SONGLEN ) i &= 0x0f;
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGGO);
		m_songgo[m_songactiveline] = i;
		return 1;
	}
}

BOOL CSong::SongTrackDec()
{
	if (m_songgo[m_songactiveline]<0)
	{
		int t = m_song[m_songactiveline][m_trackactivecol] -1;
		if (t<-1) t=TRACKSNUM-1;
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_songactiveline] -1;
		if (g<0) g=SONGLEN-1;
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGGO);
		m_songgo[m_songactiveline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackInc()
{
	if (m_songgo[m_songactiveline]<0)
	{
		int t = m_song[m_songactiveline][m_trackactivecol] +1;
		if (t>=TRACKSNUM) t=-1;
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGTRACK);
		m_song[m_songactiveline][m_trackactivecol] = t;
	}
	else
	{	//GO is there
		int g = m_songgo[m_songactiveline] +1;
		if (g>=SONGLEN) g=0;
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGGO);
		m_songgo[m_songactiveline] = g;
	}
	return 1;
}

BOOL CSong::SongTrackEmpty()
{
	m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGTRACK);
	m_song[m_songactiveline][m_trackactivecol] = -1;
	return 1;
}

BOOL CSong::SongTrackGoOnOff()
{
	//GO on/off
	m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGGO);
	m_songgo[m_songactiveline] = (m_songgo[m_songactiveline]<0) ? 0:-1;
	return 1;
}

BOOL CSong::SongInsertLine(int line)
{
	m_undo.ChangeSong(line,m_trackactivecol,UETYPE_SONGDATA,0);
	int j,go;
	for(int i=SONGLEN-2; i>=line; i--)
	{
		for(j=0; j<g_tracks4_8; j++) m_song[i+1][j] = m_song[i][j];
		go = m_songgo[i];
		if (go>0 && go>=line) go++;
		m_songgo[i+1] = go;
	}
	for(j=0; j<g_tracks4_8; j++) m_song[line][j] = -1;
	m_songgo[line] = -1;
	for (int i = 0; i < line; i++)
	{
		if (m_songgo[i]>=line) m_songgo[i]++;
	}
	if (IsBookmark() && m_bookmark.songline>=line)
	{
		m_bookmark.songline++;
		if (m_bookmark.songline>=SONGLEN) ClearBookmark(); //just pushed the bookmark out of the song => cancel the bookmark
	}
	return 1;
}

BOOL CSong::SongDeleteLine(int line)
{
	m_undo.ChangeSong(line,m_trackactivecol,UETYPE_SONGDATA,0);
	int j,go;
	for(int i=line; i<SONGLEN-1; i++)
	{
		for(j=0; j<g_tracks4_8; j++) m_song[i][j] = m_song[i+1][j];
		go = m_songgo[i+1];
		if (go>0 && go>line) go--;
		m_songgo[i] = go;
	}
	for (int i = 0; i < line; i++)
	{
		if (m_songgo[i]>line) m_songgo[i]--;
	}
	for(j=0; j<g_tracks4_8; j++) m_song[SONGLEN-1][j] = -1;
	m_songgo[SONGLEN-1] = -1;
	if (IsBookmark() && m_bookmark.songline>=line)
	{
		m_bookmark.songline--;
		if (m_bookmark.songline<line) ClearBookmark(); //just deleted the songline with the bookmark
	}
	return 1;
}

BOOL CSong::SongInsertCopyOrCloneOfSongLines(int& line)
{
	int i,j,k,d,n,sou,des;
	n = (line>0) ? line-1 : 0;
	CInsertCopyOrCloneOfSongLinesDlg dlg;

	dlg.m_linefrom=n;
	dlg.m_lineto=n;
	dlg.m_lineinto=line;
	dlg.m_clone=0;
	dlg.m_tuning=0;
	dlg.m_volumep=100;	//100%

	if (dlg.DoModal()!=IDOK) return 1;
	BLOCKDESELECT;					//the block is deselected only if it is OK

	BYTE tracks[TRACKSNUM];
	memset(tracks,0,TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);
	int clonedto[TRACKSNUM];
	for(i=0; i<TRACKSNUM; i++) clonedto[i]=-1; //init

	for(i=dlg.m_linefrom; i<=dlg.m_lineto; i++)
	{
		n = i - dlg.m_linefrom;
		sou = i;
		des = line + n;
		BOOL diss=(des<=sou);
		if (diss) sou+=n;
		BOOL sngo=0;
		if (sou<SONGLEN) sngo=(m_songgo[sou]>=0);

		if (diss) sou++;
		if (sou<0 || sou>=SONGLEN || des<0 || des>=SONGLEN)
		{
			CString s;
			s.Format("Copy/clone operation had to be aborted\nbecause overrun of song range occured.\nThere was %i song lines inserted only.",n);
			MessageBox(g_hwnd,s,"Warning",MB_ICONSTOP);
			return 0;
		}

		SongInsertLine(des);	//inserted blank line

		if (dlg.m_clone && !sngo)
		{
			//clones
			//SongPrepareNewLine(des,sou,0);	//omits empty columns

			m_undo.Separator(-1); //associates the previous insert lines to the next change
			m_undo.ChangeTrack(0,0,UETYPE_TRACKSALL,1); //with separator

			for(j=0; j<g_tracks4_8; j++)
			{
				k = m_song[sou][j]; //original track
				d = -1;				//resulting track (initial initialization)
				if (k<0) continue;  //is there --
				if (clonedto[k]>=0) 
				{
					d=clonedto[k];	//this one has already been cloned, so it will also use it
				}
				else
				{
					d = FindNearTrackBySongLineAndColumn(sou,j,tracks);
					if (d>=0)
					{
						tracks[d]=TF_USED;
						clonedto[k]=d;
						TrackCopyFromTo(k,d);
						//edit cloned track according to dlg.m_tuning and dlg.m_volumep
						ModifyTrack(m_tracks.GetTrack(d),0,TRACKLEN-1,-1,dlg.m_tuning,0,dlg.m_volumep);
					}
					else
					{
						CString s;
						s.Format("Clone operation had to be aborted\nbecause out of unused empty tracks.\nThere was %i song line(s) inserted only.",n+1);
						MessageBox(g_hwnd,s,"Warning",MB_ICONSTOP);
						return 0;
					}
				}
				m_song[des][j]=d;
			}
		}
		else
		{
			//copies
			m_songgo[des]=m_songgo[sou];
			for(j=0; j<g_tracks4_8; j++) m_song[des][j]=m_song[sou][j];
		}
	}

	return 1;
}

BOOL CSong::SongPrepareNewLine(int& line,int sourceline,BOOL alsoemptycolumns) //Inserts a songline with unused empty tracks
{
	int i,k;

	if (sourceline<0) sourceline=line+sourceline; //for -1 it is line-1

	SongInsertLine(line);	//inserts a blank line

	//prepares an online set of unused empty tracks

	BYTE tracks[TRACKSNUM];
	memset(tracks,0,TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	int count=0;
	for(i=0; i<g_tracks4_8; i++)
	{
		if (!alsoemptycolumns && sourceline>=0 && m_song[sourceline][i]<0) continue;

		k = FindNearTrackBySongLineAndColumn(sourceline,i,tracks);
		if (k>=0)
		{
			m_song[line][i]=k;
			tracks[k]=TF_USED;
			count++;
		}
	}

	if (count<g_tracks4_8)
	{
		if (count==0)
			MessageBox(g_hwnd,"There isn't any empty unused track in song.","Error",MB_ICONERROR);
		else
			MessageBox(g_hwnd,"Not enough empty unused tracks in song.","Error",MB_ICONERROR);
		return 0;
	}

	return 1;
}

int CSong::FindNearTrackBySongLineAndColumn(int songline,int column, BYTE *arrayTRACKSNUM)
{
	int j,k,t;
	for(j=songline; j>=0; j--)
	{
		if (m_songgo[j]>=0) continue;
		if ((t=m_song[j][column])>=0)
		{
			//found the default track t
			for(k=t+1; k<TRACKSNUM; k++)
			{
				if (arrayTRACKSNUM[k]==0) return k;
			}
			//because it did not find any behind it, it will try to look in front of it instead
			for(k=t-1; k>=0; k--)
			{
				if (arrayTRACKSNUM[k]==0) return k;
			}
		}
	}
	//will search for the first one usable from the beginning
	for(k=0; k<TRACKSNUM; k++)
	{
		if (arrayTRACKSNUM[k]==0) return k;
	}
	return -1;
}

BOOL CSong::SongPutnewemptyunusedtrack()
{
	int line = SongGetActiveLine();
	if (m_songgo[line]>=0) return 0;		//it can't be done on the "GO TO LINE" line

	m_undo.ChangeSong(line,m_trackactivecol,UETYPE_SONGTRACK,0);

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	int k=-1;
	m_song[line][cl]=-1;	//at current position in song --

	BYTE tracks[TRACKSNUM];
	memset(tracks,0,TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	if (act>=0 && !tracks[act])
		k = act;
	else
		k = FindNearTrackBySongLineAndColumn(line,cl,tracks);

	if (k<0)
	{
		m_song[line][cl]=act;
		MessageBox(g_hwnd,"There isn't any empty unused track in song.","Error",MB_ICONERROR);
		//UpdateShiftControlKeys();
		return 0;
	}
	
	m_song[line][cl]=k;
	return 1;	
}

BOOL CSong::SongMaketracksduplicate()
{
	int line = SongGetActiveLine();
	if (m_songgo[line]>=0) return 0;		//it can't be done on the "GO TO LINE" line

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	if (act<0) return 0;			//cannot be duplicated, no track selected

	m_undo.ChangeSong(line,cl,UETYPE_SONGTRACK,-1); //just cast

	int k=-1;
	m_song[line][cl]=-1;	//at current position in song --

	BYTE tracks[TRACKSNUM];
	memset(tracks,0,TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	if (!(tracks[act]&TF_USED) )
	{
		//not used anywhere else
		m_song[line][cl]=act;
		int r=MessageBox(g_hwnd,"This track is used only once in song.\nAre you sure to make duplicate?","Make track's duplicate...",MB_OKCANCEL | MB_ICONQUESTION);
		if (r==IDOK)
			k = FindNearTrackBySongLineAndColumn(line,cl,tracks);
		else
		{
			m_undo.DropLast();
			return 0;
		}
	}
	else
		k = FindNearTrackBySongLineAndColumn(line,cl,tracks);

	if (k<0)
	{
		m_song[line][cl]=act;
		MessageBox(g_hwnd,"There isn't any empty unused track in song.","Error",MB_ICONERROR);
		//UpdateShiftControlKeys();
		m_undo.DropLast();
		return 0;
	}

	m_undo.ChangeTrack(k,m_trackactiveline,UETYPE_TRACKDATA,1);

	//copies source track act to k
	TrackCopyFromTo(act,k);

	m_song[line][cl]=k;

	return 1;	
}


//--clipboard functions

void CSong::TrackCopy()
{
	int i = SongGetActiveTrack();
	if (i<0 || i>=TRACKSNUM) return;

	TTrack& at = *m_tracks.GetTrack(i);
	TTrack& tot = g_trackcl.m_trackcopy;

	memcpy((void*)(&tot),(void*)(&at),sizeof(TTrack));
}

void CSong::TrackPaste()
{
	if (g_trackcl.m_trackcopy.len<=0) return;
	int i = SongGetActiveTrack();
	if (i<0 || i>=TRACKSNUM) return;

	TTrack& fro = g_trackcl.m_trackcopy;
	TTrack& at = *m_tracks.GetTrack(i);

	memcpy((void*)(&at),(void*)(&fro),sizeof(TTrack));
}

void CSong::TrackDelete()
{
	int i = SongGetActiveTrack();
	if (i<0 || i>=TRACKSNUM) return;

	m_tracks.ClearTrack(i); 
}

void CSong::TrackCut()
{
	TrackCopy();
	TrackDelete();
}

void CSong::TrackCopyFromTo(int fromtrack,int totrack)
{
	if (fromtrack<0 || fromtrack>=TRACKSNUM) return;
	if (totrack<0 || totrack>=TRACKSNUM) return;

	TTrack& at = *m_tracks.GetTrack(fromtrack);
	TTrack& tot = *m_tracks.GetTrack(totrack);

	memcpy((void*)(&tot),(void*)(&at),sizeof(TTrack));
}

void CSong::BlockPaste(int special)
{
	m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
	int lines = g_trackcl.BlockPasteToTrack(SongGetActiveTrack(),m_trackactiveline,special);
	if (lines>0)
	{
		int lastl = m_trackactiveline+lines-1;
		//resets the beginning of the block to this location
		g_trackcl.BlockDeselect();
		g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline);
		g_trackcl.BlockSetEnd(lastl);
		//moves the current line to the last bottom row of the pasted block
		m_trackactiveline=lastl;
	}
}

void CSong::InstrCopy()
{
	int i = GetActiveInstr();
	TInstrument& ai = m_instrs.m_instr[i];
	memcpy((void*)(&m_instrclipboard),(void*)(&ai),sizeof(TInstrument));
}

void CSong::InstrPaste(int special)
{
	if (m_instrclipboard.act<0) return;	//he has never been filled with anything

	int i = GetActiveInstr();

	m_undo.ChangeInstrument(i,0,UETYPE_INSTRDATA,1);

	TInstrument& ai = m_instrs.m_instr[i];

	Atari_InstrumentTurnOff(i); //turns off this instrument on all channels

	int x,y;
	BOOL bl=0,br=0,ep=0;
	BOOL bltor=0,brtol=0;

	switch (special)
	{
	case 0: //normal paste
		memcpy((void*)(&ai),(void*)(&m_instrclipboard),sizeof(TInstrument));
		ai.act=ai.activenam=0; //so that the cursor is at the beginning of the instrument name
		break;
	
	case 1: //volume L/R
		bl=br=1;
		goto InstrPaste_Envelopes;
	case 2: //volume R
		br=1;
		goto InstrPaste_Envelopes;
	case 3: //volume L
		bl=1;
		goto InstrPaste_Envelopes;
	case 4: //envelope parameters
		ep=1;
InstrPaste_Envelopes:
		for(x=0; x<=m_instrclipboard.par[PAR_ENVLEN]; x++)
		{
			if (br) ai.env[x][ENV_VOLUMER]=m_instrclipboard.env[x][ENV_VOLUMER];
			if (bl) ai.env[x][ENV_VOLUMEL]=m_instrclipboard.env[x][ENV_VOLUMEL];
			if (bltor) ai.env[x][ENV_VOLUMER]=m_instrclipboard.env[x][ENV_VOLUMEL];
			if (brtol) ai.env[x][ENV_VOLUMEL]=m_instrclipboard.env[x][ENV_VOLUMER];
			if (ep)
			{
				for(y=ENV_DISTORTION; y<ENVROWS; y++) ai.env[x][y]=m_instrclipboard.env[x][y];
			}
		}
		ai.par[PAR_ENVLEN]=m_instrclipboard.par[PAR_ENVLEN];
		ai.par[PAR_ENVGO]=m_instrclipboard.par[PAR_ENVGO];
		ai.activeenvx=0;
		break;

	case 5: //TABLE
		for(x=0; x<=m_instrclipboard.par[PAR_TABLEN]; x++) ai.tab[x]=m_instrclipboard.tab[x];
		ai.par[PAR_TABLEN]=m_instrclipboard.par[PAR_TABLEN];
		ai.par[PAR_TABGO]=m_instrclipboard.par[PAR_TABGO];
		ai.activetab=0;
		break;

	case 6: //vol+env
		br=bl=ep=1;
		goto InstrPaste_Envelopes;
	case 8: //volume L to R
		bltor=1;
		goto InstrPaste_Envelopes;
	case 9: //volume R to L
		brtol=1;
		goto InstrPaste_Envelopes;

	case 7: //vol+env insert to cursor
		int sx=m_instrclipboard.par[PAR_ENVLEN]+1;
		if (ai.activeenvx+sx>ENVCOLS) sx=ENVCOLS-ai.activeenvx;
		for(x=ENVCOLS-2; x>=ai.activeenvx; x--) //offset
		{
			int i=x+sx;
			if (i>=ENVCOLS) continue;
			for(y=0; y<ENVROWS; y++) ai.env[i][y]=ai.env[x][y];
		}
		for(x=0; x<sx; x++) //insertion
		{
			int i=ai.activeenvx+x;
			for(y=0; y<ENVROWS; y++) ai.env[i][y]=m_instrclipboard.env[x][y];
		}
		int i=ai.par[PAR_ENVLEN]+sx;
		if (i>=ENVCOLS) i=ENVCOLS-1;
		ai.par[PAR_ENVLEN]=i;
		if (ai.par[PAR_ENVGO]>ai.activeenvx)
		{
			i=ai.par[PAR_ENVGO]+sx;
			if (i>=ENVCOLS) i=ENVCOLS-1;
			ai.par[PAR_ENVGO]=i;
		}
		i=ai.activeenvx+sx;
		if (i>=ENVCOLS) i=ENVCOLS-1;
		ai.activeenvx=i;
		break;

	}
	m_instrs.ModificationInstrument(i); //write to Atari RAM
}

void CSong::InstrCut()
{
	InstrCopy();
	InstrDelete();
}

void CSong::InstrInfo(int instr,TInstrInfo* iinfo,int instrto)
{
	if (instr<0 || instr>=INSTRSNUM) return;
	if (instrto<instr || instr>=INSTRSNUM) instrto=instr;

	int i,j,ain;
	int inttrack;
	int intrack[TRACKSNUM];
	int noftrack=0;
	int globallytimes=0;
	int withnote[NOTESNUM];
	for(i=0; i<NOTESNUM; i++) withnote[i]=0;
	int minnote=NOTESNUM,maxnote=-1;
	int minvol=16,maxvol=-1;
	int infrom=INSTRSNUM,into=-1;
	for(i=0; i<TRACKSNUM; i++)
	{
		inttrack=0;
		TTrack& at=*m_tracks.GetTrack(i);
		ain=-1;
		for(j=0; j<at.len; j++)
		{
			if (at.instr[j]>=0) ain=at.instr[j];
			if (ain>=instr && ain<=instrto)
			{
				inttrack=1;
				if (ain>into) into=ain;
				if (ain<infrom) infrom=ain;
				int note=at.note[j];
				if (note>=0 && note<NOTESNUM)
				{
					globallytimes++; //some note with this instrument => started
					withnote[note]++;
					if (note>maxnote) maxnote=note;
					if (note<minnote) minnote=note;
				}
				int vol=at.volume[j];
				if (vol>=0 && vol<=15)
				{
					if (vol>maxvol) maxvol=vol;
					if (vol<minvol) minvol=vol;
				}
			}
		}
		intrack[i]=inttrack;
		if (inttrack) noftrack++;
	}

	if (iinfo)
	{	//iinfo != NULL => set values
		iinfo->count=globallytimes;
		iinfo->usedintracks=noftrack;
		iinfo->instrfrom=infrom;
		iinfo->instrto=into;
		iinfo->minnote=minnote;
		iinfo->maxnote=maxnote;
		iinfo->minvol=minvol;
		iinfo->maxvol=maxvol;
	}
	else
	{	//iinfo == NULL => shows dialog
		CString s,s2;
		s.Format("Instrument: %02X\nName: %s\nUsed in %i tracks, globally %i times.\nFrom note: %s\nTo note: %s\nMin volume: %X\nMax volume: %X",
			instr, m_instrs.GetName(instr), noftrack, globallytimes,
			minnote<NOTESNUM? notes[minnote] : "-",
			maxnote>=0? notes[maxnote] : "-",
			minvol<=15? minvol : 0,
			maxvol>=0? maxvol : 0 );
		
		if (globallytimes>0)
		{
			s+="\n\nNote listing:\n";
			int lc=0;
			for(i=0;i<NOTESNUM; i++)
			{
				if (withnote[i])
				{ 
					s+=notes[i];
					lc++;
					if (lc<12)
						s+=" ";
					else
					{	s+="\n"; lc=0; }
				}
			}
			s+="\n\nTrack listing:\n";
			lc=0;
			for(i=0; i<TRACKSNUM; i++)
			{
				if (intrack[i])
				{
					s2.Format("%02X",i);
					s+=s2;
					lc++;
					if (lc<16)
						s+=" ";
					else
					{	s+="\n"; lc=0; }
				}
			}
		}
		MessageBox(g_hwnd,(LPCTSTR)s,"Instrument info",MB_ICONINFORMATION);
	}
}

int CSong::InstrChange(int instr)
{
	//Change all the instrument occurences.
	if (instr<0 || instr>=INSTRSNUM) return 0;

	CInstrumentChangeDlg dlg;
	dlg.m_song=this;
	dlg.m_combo9=dlg.m_combo11=instr;
	dlg.m_combo10=dlg.m_combo12=instr;
	dlg.m_onlytrack=SongGetActiveTrack();
	dlg.m_onlysonglinefrom=dlg.m_onlysonglineto=SongGetActiveLine();

	if (dlg.DoModal()==IDOK)
	{
		//hide all tracks and the whole song
		m_undo.ChangeTrack(0,0,UETYPE_TRACKSALL,-1);
		m_undo.ChangeSong(0,0,UETYPE_SONGDATA,1);

		int snotefrom=dlg.m_combo1;
		int snoteto=dlg.m_combo2;
		int svolmin=dlg.m_combo3;
		int svolmax=dlg.m_combo4;
		int sinstrfrom=dlg.m_combo11;
		int sinstrto=dlg.m_combo12;

		int dnotefrom=dlg.m_combo5;
		int dnoteto=dlg.m_combo6;
		int dvolmin=dlg.m_combo7;
		int dvolmax=dlg.m_combo8;
		int dinstrfrom=dlg.m_combo9;
		int dinstrto=dlg.m_combo10;

		int i,j,t,r;

		int onlytrack=dlg.m_onlytrack;

		//!!!!!!!!!!!!!!!!!!!!!
		int onlychannels=dlg.m_onlychannels;
		int onlysonglinefrom=dlg.m_onlysonglinefrom;
		int onlysonglineto=dlg.m_onlysonglineto;
		
		unsigned char track_yn[TRACKSNUM];
		memset(track_yn,0,TRACKSNUM);
		int track_column[TRACKSNUM];	//the first occurrence in the selected area of the song
		int track_line[TRACKSNUM];		//the first occurrence in the selected area of the song
		for(i=0; i<TRACKSNUM; i++) track_column[i]=track_line[i]=-1; //init
		
		int onlysomething=0;
		int trackcreated=0; //number of newly created songs
		int songchanges=0;	//number of changes in the song

		if (onlychannels>=0 || (onlysonglinefrom>=0 && onlysonglineto>=0))
		{
			if (onlychannels<=0) onlychannels=0xff; //all
			if (onlysonglinefrom<0) onlysonglinefrom=0; //from the beginning
			if (onlysonglineto<0) onlysonglineto=SONGLEN-1; //to the end
			onlysomething=1;
			unsigned char r;
			for(j=0; j<SONGLEN; j++)
			{
				if (m_songgo[j]>=0) continue; //there is a goto
				for(i=0; i<g_tracks4_8; i++)
				{
					t=m_song[j][i];
					if (t<0 || t>=TRACKSNUM) continue;
					r=(onlychannels&(1<<i)) && j>=onlysonglinefrom && j<=onlysonglineto;
					track_yn[t]|= (r)? 1 : 2;	//1=yes, 2=no, 3=yesno (copy)
					if (r && track_column[t]<0)
					{
						//the first occurrence in the selected area of the song
						track_column[t]=i;
						track_line[t]=j;
					}
				}
			}
		}
		else
		if (onlytrack>=0)
		{
			track_yn[onlytrack]=1;	//1=yes
			onlysomething=1;
		}

		if (dnoteto>=NOTESNUM) dnoteto=dnotefrom+(snoteto-snotefrom);
		if (dvolmax>=16) dvolmax=dvolmin+(svolmax-svolmin);
		if (dinstrto>=INSTRSNUM) dinstrto=dinstrfrom+(sinstrto-sinstrfrom);

		double notecoef;
		if ( snoteto-snotefrom>0 ) 
			notecoef=(double)(dnoteto-dnotefrom)/(snoteto-snotefrom);
		else
			notecoef=0;

		double volcoef;
		if ( svolmax-svolmin>0 )
			volcoef=(double)(dvolmax-dvolmin)/(svolmax-svolmin);
		else
			volcoef=0;

		double instrcoef;
		if ( sinstrto-sinstrfrom>0 ) 
			instrcoef=(double)(dinstrto-dinstrfrom)/(sinstrto-sinstrfrom);
		else
			instrcoef=0;

		int lasti,lastn,changes;
		int track_changeto[TRACKSNUM];

		for(i=0; i<TRACKSNUM; i++)
		{
			track_changeto[i]=-1; //initialise
			if (onlysomething && ((track_yn[i]&1)!=1) ) continue; //it wants to change only some and this one is not

			TTrack& st=*m_tracks.GetTrack(i);
			TTrack at; //destination track
			//make a copy
			memcpy((void*)(&at),(void*)(&st),sizeof(TTrack));
			changes=0;
			lasti=lastn=-1;
			for(j=0; j<at.len; j++)
			{
				if (at.instr[j]>=0) lasti=at.instr[j];
				if (at.note[j]>=0) lastn=at.note[j];
				if (   lasti>=sinstrfrom
					&& lasti<=sinstrto
					&& lastn>=snotefrom
					&& lastn<=snoteto
					&& at.volume[j]>=svolmin
					&& at.volume[j]<=svolmax )
				{
					if (at.note[j]>=0)
					{
						int note = dnotefrom + (int)((double)(at.note[j]-snotefrom)*notecoef+0.5); 
						while (note>=NOTESNUM) note-=12;
						if (note!=at.note[j])
						{
							at.note[j]=note;
							changes=1;
						}
					}
					if (at.instr[j]>=0)
					{
						int ins = dinstrfrom + (int)((double)(at.instr[j]-sinstrfrom)*instrcoef+0.5); 
						while (ins>=INSTRSNUM) ins=INSTRSNUM-1;
						if (ins!=at.instr[j])
						{
							at.instr[j]=ins;
							changes=1;
						}
					}
					int vol = dvolmin + (int)((double)(at.volume[j]-svolmin)*volcoef+0.5);
					if (vol>15) vol=15;
					if (vol!=at.volume[j])
					{
						at.volume[j]=vol;
						changes=1;
					}
				}
			}
			if (changes) //made some changes
			{
				if (track_yn[i]&2)
				{
					//the track occurs both inside and outside the area
					//create a new track
					int k;
					BYTE tracks[TRACKSNUM];
					memset(tracks,0,TRACKSNUM); //init
					MarkTF_USED(tracks);
					MarkTF_NOEMPTY(tracks);
					k = FindNearTrackBySongLineAndColumn(track_line[i],track_column[i],tracks);
					if (k<0)
					{
						MessageBox(g_hwnd,"There isn't any empty unused track in song.","Error",MB_ICONERROR);
						//UpdateShiftControlKeys();
						return 0;
					}
					//copies the changed copy (at) to the new track (nt)
					TTrack *nt=m_tracks.GetTrack(k);
					memcpy((void*)nt,(void*)(&at),sizeof(TTrack));
					trackcreated++;
					//at least once put it in the song (due to the search in the song used tracks)
					m_song[track_line[i]][track_column[i]]=k;
					songchanges++;
					//will change all occurrences
					track_changeto[i]=k;
				}
				else
				{
					//occurs only within the area to copy, the reduced copy (data) to the original track (st)
					memcpy((void*)(&st),(void*)(&at),sizeof(TTrack));
				}
			}

		}
		//subsequent changes in the song
		if (onlysomething)
		{
			for(j=0; j<SONGLEN; j++)
			{
				if (m_songgo[j]>=0) continue; //there is a goto
				for(i=0; i<g_tracks4_8; i++)
				{
					t=m_song[j][i];
					if (t<0 || t>=TRACKSNUM) continue;
					r=(onlychannels&(1<<i)) && j>=onlysonglinefrom && j<=onlysonglineto;
					if (r && track_changeto[t]>=0)
					{
						m_song[j][i]=track_changeto[t];
						songchanges++;
					}
				}
			}
		}
		if (trackcreated>0 || songchanges>0)
		{
			CString s;
			s.Format("Instrument changes implications:\nNew tracks created: %u\nChanges in song: %u",trackcreated,songchanges);
			MessageBox(g_hwnd,(LPCTSTR)s,"Instrument changes",MB_ICONINFORMATION);
		}
		return 1;
	}
	return 0;
}

void CSong::TrackInfo(int track)
{
	if (track<0 || track>=TRACKSNUM) return;

	static char* cnames[]={"L1","L2","L3","L4","R1","R2","R3","R4"};
	
	int i,ch;
	int trackusedincolumn[SONGTRACKS];
	int lines=0,total=0;

	for(ch=0; ch<SONGTRACKS; ch++) trackusedincolumn[ch]=0;

	for(int sline=0; sline<SONGLEN; sline++)
	{
		if (m_songgo[sline]>=0) continue;	//goto line is ignored

		BOOL thisline=0;
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n==track) { trackusedincolumn[ch]++; total++; thisline=1; }
		}

		if (thisline) lines++;
	}

	CString s,s2;
	s.Format("Track: %02X\nUsing in song:\n",track);
	for(ch=0; ch<g_tracks4_8; ch++)
	{
		i=trackusedincolumn[ch];
		s2.Format("%s: %i   ",cnames[ch],i);
		s+=s2;
	}

	s2.Format("\nUsed in %i songlines, globally %i times.",lines,total);
	s+=s2;
	
	MessageBox(g_hwnd,(LPCTSTR)s,"Track Info",MB_ICONINFORMATION);
}

void CSong::SongCopyLine()
{
	for(int i=0; i<g_tracks4_8; i++) m_songlineclipboard[i]=m_song[m_songactiveline][i];
	m_songgoclipboard = m_songgo[m_songactiveline];
}

void CSong::SongPasteLine()
{
	if (m_songgoclipboard<-1) return;
	m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGDATA);
	for(int i=0; i<g_tracks4_8; i++) m_song[m_songactiveline][i] = m_songlineclipboard[i];
	m_songgo[m_songactiveline] = m_songgoclipboard;
}

void CSong::SongClearLine()
{
	m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGDATA);
	for(int i=0; i<g_tracks4_8; i++) m_song[m_songactiveline][i] = -1;
	m_songgo[m_songactiveline] = -1;
}

void CSong::TracksOrderChange()
{
	Stop();	//stop the sound first
	CSongTracksOrderDlg dlg;
	dlg.m_songlinefrom.Format("%02X",m_TracksOrderChange_songlinefrom);
	dlg.m_songlineto.Format("%02X",m_TracksOrderChange_songlineto);
	if (dlg.DoModal()==IDOK)
	{
		m_undo.ChangeSong(m_songactiveline,m_trackactivecol,UETYPE_SONGDATA,1);

		int m_buff[8];
		int i,j;

		int f=Hexstr((char*)(LPCTSTR)dlg.m_songlinefrom,2);
		int t=Hexstr((char*)(LPCTSTR)dlg.m_songlineto,2);

		if (f<0 || f>=SONGLEN || t<0 || t>=SONGLEN || t<f)
		{
			MessageBox(g_hwnd,"Bad songline (from-to) range.","Error",MB_ICONERROR);
			return;
		}

		m_TracksOrderChange_songlinefrom=f;
		m_TracksOrderChange_songlineto=t;

		int c=0;
		for(i=0; i<g_tracks4_8; i++) if (dlg.m_tracksorder[i]<0) c++;
		if (c>0)
		{
			CString s;
			s.Format("Warning: %u song column(s) will be cleared completely.\nAre you sure to do it?",c);
			if (MessageBox(g_hwnd,s,"Warning",MB_YESNOCANCEL | MB_ICONWARNING)!=IDYES) return;
		}

		for(i=m_TracksOrderChange_songlinefrom; i<=m_TracksOrderChange_songlineto; i++)
		{
			for(j=0; j<g_tracks4_8; j++) 
			{
				m_buff[j]=m_song[i][j];
				m_song[i][j]=-1;
			}
			for(j=0; j<g_tracks4_8; j++)
			{
				int z=dlg.m_tracksorder[j];
				if (z>=0) 
					m_song[i][j]=m_buff[z];
				else
					m_song[i][j]=-1;
			}
		}
	}
}

void CSong::Songswitch4_8(int tracks4_8)
{
	Stop();	//stop the sound first

	CString wrn="Warning: Undo operation won't be possible!!!\n";
	int i,j;
	if (tracks4_8==4) 
	{
		int p=0;
		for(i=0; i<SONGLEN; i++)
		{
			for(j=4; j<8; j++) if (m_song[i][j]>=0) p++;
		}

		if (p>0) wrn+="\nWarning: Song switch to mono 4 tracks will erase all the R1,R2,R3,R4 entries in song list.\n";
	}

	wrn+="\nAre you sure to do it?";
	int res=MessageBox(g_hwnd,wrn,"Song switch mono/stereo",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
	if (res!=IDYES) return;

	m_undo.Clear();

	if (tracks4_8==4)
	{
		if (m_trackactivecol>=4) { m_trackactivecol=3; m_trackactivecur=0; }
		g_tracks4_8=4;
		for(i=0; i<SONGLEN; i++)
		{
			for(j=4; j<8; j++) m_song[i][j]=-1;
		}
	}
	else
	if (tracks4_8==8) g_tracks4_8=8;
}

int CSong::GetEffectiveMaxtracklen()
{
	//calculate the largest track length used
	int so,i,max=1;
	for(so=0; so<SONGLEN; so++)
	{
		if (m_songgo[so]>=0) continue; //go to line is ignored
		int min=m_tracks.m_maxtracklen;
		int p=0;
		for(i=0; i<g_tracks4_8; i++)
		{
			int t=m_song[so][i];
			int m=m_tracks.GetLength(t);
			if (m<0) continue;
			p++;
			if (m<min) min=m;
		}
		//min = the shortest track length on this songline
		if (p>0 && min>max) max=min;
	}
	return max;
}

void CSong::ChangeMaxtracklen(int maxtracklen)
{
	if (maxtracklen<=0 || maxtracklen>TRACKLEN) return;
	//
	int i,j;
	for(i=0; i<TRACKSNUM; i++)
	{
		TTrack &tt=*m_tracks.GetTrack(i);
		//clear
		for(j=tt.len; j<TRACKLEN; j++)
		{
			tt.note[j]=tt.instr[j]=tt.volume[j]=tt.speed[j]=-1;
		}
		//
		if (tt.len>=maxtracklen)
		{
			tt.go=-1; //cancel GO
			tt.len=maxtracklen; //adjust length
		}
	}
	if (m_trackactiveline>=maxtracklen) m_trackactiveline = maxtracklen-1;
	if (m_trackplayline >= maxtracklen) m_trackplayline = maxtracklen-1;
	m_tracks.m_maxtracklen=maxtracklen;
}

void CSong::TracksAllBuildLoops(int& tracksmodified,int& beatsreduced)
{
	Stop(); //stop the sound first

	int i;
	int p=0,u=0;
	for(i=0; i<TRACKSNUM; i++)
	{
		int r=GetTracks()->TrackBuildLoop(i);
		if (r>0) { p++; u+=r; }
	}
	tracksmodified=p;
	beatsreduced=u;
}

void CSong::TracksAllExpandLoops(int& tracksmodified,int& loopsexpanded)
{
	Stop(); //stop the sound first

	int i;
	int p=0,u=0;
	for(i=0; i<TRACKSNUM; i++)
	{
		int r=GetTracks()->TrackExpandLoop(i);
		if (r>0) { p++; u+=r; }
	}
	tracksmodified=p;
	loopsexpanded=u;
}

void CSong::SongClearUnusedTracksAndParts(int& clearedtracks, int& truncatedtracks,int& truncatedbeats)
{
	int i,j,ch;
	int tracklen[TRACKSNUM];
	BOOL trackused[TRACKSNUM];
	for(i=0; i<TRACKSNUM; i++)
	{
		//initialise
		tracklen[i]=-1;
		trackused[i]=0;
	}

	for(int sline=0; sline<SONGLEN; sline++)
	{
		if (m_songgo[sline]>=0) continue;	//goto line is ignored

		int nejkratsi=m_tracks.m_maxtracklen;
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			trackused[n]=1;
			TTrack& tr=*m_tracks.GetTrack(n);
			if (tr.go>=0) continue;	//there is a loop => it has a maximum length
			if (tr.len<nejkratsi) nejkratsi=tr.len;
		}
		
		//"nejkratsi" is the shortest track in this song line
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			if (tracklen[n]<nejkratsi) tracklen[n]=nejkratsi; //if it needs a longer size, it will expand to the length it needs
		}
	}

	int ttracks=0,tbeats=0;
	//and now it cuts those tracks
	for(i=0; i<TRACKSNUM; i++)
	{
		int nlen=tracklen[i];
		if (nlen<1) continue;	//if they don't have the length of at least 1 they are skipped
		TTrack& tr=*m_tracks.GetTrack(i);
		if (tr.go<0)
		{
			//there is no loop
			if (nlen<tr.len)
			{
				//for what must cut, is there anything at all?
				for(j=nlen; j<tr.len; j++)
				{
					if (tr.note[j]>=0 || tr.instr[j]>=0 || tr.volume[j]>=0 || tr.speed[j]>=0)
					{
						//Yeah, there's something, so cut it
						ttracks++;
						tbeats+=tr.len-nlen;
						tr.len=nlen; //cut what is not needed
						break;
					}
				}
				//there is no break;
			}
		}
		else
		{
			//there is a loop
			if (tr.len>=nlen)	//the beginning of the loop is further than the required track length
			{
				ttracks++;
				tbeats+=tr.len-nlen;
				tr.len=nlen;	//short track
				tr.go=-1;		//cancel loop
			}
		}
	}

	//delete empty tracks not used in the song
	int ctracks=0;
	for(i=0; i<TRACKSNUM; i++)
	{
		if ( !trackused[i] && !m_tracks.IsEmptyTrack(i) ) 
		{ 
			m_tracks.ClearTrack(i); 
			ctracks++; 
		}
	}

	clearedtracks=ctracks;
	truncatedtracks=ttracks;
	truncatedbeats=tbeats;
}

int CSong::SongClearDuplicatedTracks()
{
	int i,j,ch;
	int trackto[TRACKSNUM];

	for(i=0; i<TRACKSNUM; i++) trackto[i]=-1;

	int clearedtracks=0;
	for(i=0; i<TRACKSNUM-1; i++)
	{
		if (m_tracks.IsEmptyTrack(i)) continue;	//does not compare empty
		for(j=i+1;j<TRACKSNUM; j++)
		{
			if (m_tracks.IsEmptyTrack(j)) continue;
			if (m_tracks.CompareTracks(i,j))
			{
				m_tracks.ClearTrack(j);	//j is the same as i, so j is deleted.
				trackto[j]=i;			//these tracks have to be replaced by tracks i
				clearedtracks++;
			}
		}
	}

	//analyse the song and make changes to the deleted tracks
	for(int sline=0; sline<SONGLEN; sline++)
	{
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			if (trackto[n]>=0) m_song[sline][ch]=trackto[n];
		}
	}

	return clearedtracks;
}

int CSong::SongClearUnusedTracks()
{
	int i,ch;
	BOOL trackused[TRACKSNUM];

	for(i=0; i<TRACKSNUM; i++) trackused[i]=0;

	for(int sline=0; sline<SONGLEN; sline++)
	{
		if (m_songgo[sline]>=0) continue;	//goto line is ignored

		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			trackused[n]=1;
		}
	}

	//delete all tracks unused in the song
	int clearedtracks=0;
	for(i=0; i<TRACKSNUM; i++)
	{
		if (!trackused[i])
		{ 
			if (!m_tracks.IsEmptyTrack(i)) clearedtracks++;
			m_tracks.ClearTrack(i); 
		}
	}

	return clearedtracks;
}

void CSong::RenumberAllTracks(int type) //1..after columns, 2..after lines
{
	int i,j,sline;
	int movetrackfrom[TRACKSNUM],movetrackto[TRACKSNUM];

	for(i=0; i<TRACKSNUM; i++) movetrackfrom[i]=movetrackto[i]=-1;

	int order=0;

	//test the song
	if (type==2)
	{
		//horizontally along the lines
		for(sline=0; sline<SONGLEN; sline++)
		{
			if (m_songgo[sline]>=0) continue;	//goto line is ignored
			for(i=0; i<g_tracks4_8; i++)
			{
				int n=m_song[sline][i];
				if (n<0 || n>=TRACKSNUM) continue;	//--
				if (movetrackfrom[n]<0)
				{
					movetrackfrom[n]=order;
					movetrackto[order]=n;
					order++;
				}
			}
		}
	}
	else
	if (type==1)
	{
		//vertically in columns
		for(i=0; i<g_tracks4_8; i++)
		{
			for(sline=0; sline<SONGLEN; sline++)
			{
				if (m_songgo[sline]>=0) continue;	//goto line is ignored
				int n=m_song[sline][i];
				if (n<0 || n>=TRACKSNUM) continue;	//--
				if (movetrackfrom[n]<0)
				{
					movetrackfrom[n]=order;
					movetrackto[order]=n;
					order++;
				}
			}
		}
	}
	else
		return;	//unknown type

	//then add empty tracks not used in the song
	for(i=0; i<TRACKSNUM; i++)
	{
		if (movetrackfrom[i]<0 && !m_tracks.IsEmptyTrack(i))
		{
			movetrackfrom[i]=order;
			movetrackto[order]=i;
			order++;
		}
	}

	//precisely numbered in the song
	for(sline=0; sline<SONGLEN; sline++)
	{
		//if (m_songgo[sline]>=0) continue;	//goto line is not omitted here (the numbers mentioned below it will also change)
		for(i=0; i<g_tracks4_8; i++)
		{
			int n=m_song[sline][i];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			m_song[sline][i]=movetrackfrom[n];
		}
	}

	//physical data transfer in tracks
	TTrack buft;
	for(i=0; i<order; i++)
	{
		int n=movetrackto[i];	// swap i <--> n
		if (n==i) continue;		//they are the same, so they don't have to shuffle anything
		memcpy((void*)(&buft),(void*)(m_tracks.GetTrack(i)),sizeof(TTrack));	// i -> buffer
		memcpy((void*)(m_tracks.GetTrack(i)),(void*)(m_tracks.GetTrack(n)),sizeof(TTrack)); // n -> i
		memcpy((void*)(m_tracks.GetTrack(n)),(void*)(&buft),sizeof(TTrack));	// buffer -> n
		//
		for(j=i; j<order; j++)
		{
			if (movetrackto[j]==i) movetrackto[j]=n;
		}
	}
}

int CSong::ClearAllInstrumentsUnusedInAnyTrack()
{
	//go through all existing tracks and find unused instruments
	int i,j,t;
	BOOL instrused[INSTRSNUM];

	for(i=0; i<INSTRSNUM; i++) instrused[i]=0;
	for(i=0; i<TRACKSNUM; i++)
	{
		TTrack& tr=*m_tracks.GetTrack(i);
		int nlen=tr.len;
		for(j=0; j<nlen; j++)
		{
			t=tr.instr[j];
			if (t>=0 && t<INSTRSNUM) instrused[t]=1;	//instrument "t" is used
		}
	}

	//delete unused instruments here
	int clearedinstruments=0;
	for(i=0; i<INSTRSNUM; i++)
	{
		if (!instrused[i])
		{
			//unused
			if (m_instrs.CalculateNoEmpty(i)) clearedinstruments++;	//is it empty? yes => it will be deleted
			m_instrs.ClearInstrument(i);
		}
	}

	return clearedinstruments;
}

void CSong::RenumberAllInstruments(int type)
{
	//type=1...remove gaps, 2=order by using in tracks, type=3...order by instrument names

	int i,j,k,ins;
	int moveinstrfrom[INSTRSNUM],moveinstrto[INSTRSNUM];

	for(i=0; i<INSTRSNUM; i++) moveinstrfrom[i]=moveinstrto[i]=-1;

	int order=0;

	//analyse all tracks
	for(i=0; i<TRACKSNUM; i++)
	{
		TTrack& tr=*m_tracks.GetTrack(i);
		int tlen=tr.len;
		for(j=0; j<tlen; j++)
		{
			ins= tr.instr[j];
			if (ins<0 || ins>=INSTRSNUM) continue;
			if (moveinstrfrom[ins]<0)
			{
				moveinstrfrom[ins]=order;
				moveinstrto[order]=ins;
				order++;
			}
		}
	}

	//and now it adds even those that are not used in any track
	for(i=0; i<INSTRSNUM; i++)
	{
		if (moveinstrfrom[i]<0 && m_instrs.CalculateNoEmpty(i))
		{
			moveinstrfrom[i]=order;
			moveinstrto[order]=i;
			order++;
		}
	}

	TInstrument bufi;
	if (type==1)
	{
		//remove gaps
		int di=0;
		for(i=0; i<INSTRSNUM; i++)
		{
			if (moveinstrfrom[i]>=0) //this instrument is used somewhere or is empty
			{
				//move to
				if (i!=di)
				{
					memcpy((void*)(&m_instrs.m_instr[di]),(void*)(&m_instrs.m_instr[i]),sizeof(TInstrument));
					//and delete instrument i
					m_instrs.ClearInstrument(i);
				}
				//change table accordingly
				moveinstrfrom[i]=di;
				moveinstrto[di]=i;
				di++;
			}
		}
	}
	else
	if (type==2)
	{
		//order by using in tracks
		//moveinstrfrom [instr] and moveinstrto [order] have it ready, so it can physically switch straight away
		for(i=0; i<order; i++)
		{
			int n=moveinstrto[i];	//swap i <--> n
			if (n==i) continue;
			memcpy((void*)(&bufi),(void*)(&m_instrs.m_instr[i]),sizeof(TInstrument)); // i -> buffer
			memcpy((void*)(&m_instrs.m_instr[i]),(void*)(&m_instrs.m_instr[n]),sizeof(TInstrument)); // n -> i
			memcpy((void*)(&m_instrs.m_instr[n]),(void*)(&bufi),sizeof(TInstrument)); // buffer -> n
			//
			for(j=i; j<order; j++)
			{
				if (moveinstrto[j]==i) moveinstrto[j]=n;
			}
		}
		//and now delete the others (due to the corresponding names of unused empty instruments)
		for(i=order; i<INSTRSNUM; i++) m_instrs.ClearInstrument(i);
	}
	else
	if (type==3)
	{
		//order by instrument name
		BOOL iused[INSTRSNUM];
		for(i=0; i<INSTRSNUM; i++)
		{
			iused[i]=(moveinstrfrom[i]>=0);
			moveinstrfrom[i]=i;	//the default is to keep the same order
		}
		//and now bubblesort arrange those that are iused [i]
		for(i=INSTRSNUM-1; i>0; i--)
		{
			for(j=0; j<i; j++)
			{
				k=j+1;
				//compare instrument j and k and either let or swap
				BOOL swap=0;

				if (iused[j] != iused[k])
				{
					//one is used and one is unused
					if (iused[k]) swap=1; //the second is used (=> the first is the one used), so swap
				}
				else
				{
					//both are used or both are not used
					char *name1=m_instrs.GetName(j);
					char *name2=m_instrs.GetName(k);
					if (_strcmpi(name1, name2) > 0) swap = 1; //they are the other way around, so they are swapped
				}

				if (swap)
				{
					//swap j and k
					memcpy((void*)(&bufi),(void*)(&m_instrs.m_instr[j]),sizeof(TInstrument)); // j -> buffer
					memcpy((void*)(&m_instrs.m_instr[j]),(void*)(&m_instrs.m_instr[k]),sizeof(TInstrument)); // k -> j
					memcpy((void*)(&m_instrs.m_instr[k]),(void*)(&bufi),sizeof(TInstrument)); // buffer -> k
					//adjust table accordingly
					int p;
					for(p=0; p<INSTRSNUM; p++)
					{
						if (moveinstrfrom[p]==k) moveinstrfrom[p]=j;
						else
						if (moveinstrfrom[p]==j) moveinstrfrom[p]=k;
					}

					BOOL b=iused[j];
					iused[j]=iused[k];
					iused[k]=b;
				}
			}
		}
		//still used unused empty instruments (due to their shift, so the name of their number 20: Instrument 21 did not match)
		for(i=0; i<INSTRSNUM; i++)
		{
			if (!iused[i]) m_instrs.ClearInstrument(i);
		}
	}
	else
		return;


	//and now it has to be renumbered in all tracks according to the moveinstrfrom [instr] table
	for(i=0; i<TRACKSNUM; i++)
	{
		TTrack& tr=*m_tracks.GetTrack(i);
		int tlen=tr.len;
		for(j=0; j<tlen; j++)
		{
			ins= tr.instr[j];
			if (ins<0 || ins>=INSTRSNUM) continue;
			tr.instr[j]= moveinstrfrom[ins];
		}
	}

	//and finally write all the instruments in Atari memory
	for(i=0; i<INSTRSNUM; i++) m_instrs.ModificationInstrument(i); //writes to Atari

	//Hooray, done
}



//
//--------------------------------------------------------------------------------------
//

BOOL CSong::SetBookmark()
{
	if (m_songactiveline>=0 && m_songactiveline<SONGLEN
		&& m_trackactiveline>=0 && m_trackactiveline<m_tracks.m_maxtracklen
		&& m_speed>=0)
	{
		m_bookmark.songline=m_songactiveline;
		m_bookmark.trackline=m_trackactiveline;
		m_bookmark.speed = m_speed;
		return 1;
	}
	return 0;
}

BOOL CSong::Play(int mode, BOOL follow, int special)
{
	m_undo.Separator();

	if (mode==MPLAY_BOOKMARK && !IsBookmark()) return 0; //if there is no bookmark, then nothing.

	if (m_play)
	{
		if (mode!=MPLAY_FROM) Stop(); //already playing and wants something other than play from edited pos.
		else
		if (!m_followplay) Stop(); //is playing and wants to play from edited pos. but not followplay
	}

	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;

	switch (mode)
	{
	case MPLAY_SONG: //whole song from the beginning including initialization (due to portamentum etc.)
		Atari_InitRMTRoutine();
		m_songplayline = 0;					
		m_trackplayline = 0;
		m_speed = m_mainspeed;
		break;
	case MPLAY_FROM: //song from the current position
		if (m_play && m_followplay) //is playing with follow play
		{
			m_play = MPLAY_FROM;
			m_followplay = follow;
			//g_screenupdate=1;
			return 1;
		}
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline;
		break;
	case MPLAY_TRACK: //just the current tracks around
Play3:
		m_songplayline = m_songactiveline;
		m_trackplayline = (special==0)? 0 : m_trackactiveline;
		break;
	case MPLAY_BLOCK: //only in the block
		if (!g_trackcl.IsBlockSelected())
		{	//no block is selected, so the track plays
			mode=MPLAY_TRACK; 
			goto Play3; 
		}
		else
		{
			int bfro,bto;
			g_trackcl.GetFromTo(bfro,bto);
			m_songplayline = g_trackcl.m_selsongline;
			m_trackplayline = m_trackplayblockstart = bfro;
			m_trackplayblockend = bto;
		}
		break;
	case MPLAY_BOOKMARK: //from the bookmark
		m_songplayline = m_bookmark.songline;
		m_trackplayline = m_bookmark.trackline;
		//m_speed = m_bookmark.speed; //comment out so bookmark keep the same speed in memory, won't force it to reset it each time
		break;

	case MPLAY_SEEK_NEXT: //from seeking next
		m_songactiveline++;
		if (m_songactiveline>255) m_songactiveline = 255;		
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		if (mode == MPLAY_SEEK_NEXT) mode = MPLAY_FROM;
		break;

	case MPLAY_SEEK_PREV: //from seeking prev
		m_songactiveline--;
		if (m_songactiveline<0) m_songactiveline = 0;		
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline = 0;
		if (mode == MPLAY_SEEK_PREV) mode = MPLAY_FROM;
		break;

	}

	if (m_songgo[m_songplayline]>=0)	//there is a goto
	{
		m_songplayline=m_songgo[m_songplayline];	//goto where
		m_trackplayline=0;							//from the beginning of that track
		if (m_songgo[m_songplayline]>=0)
		{
			//goto into another goto
			MessageBox(g_hwnd,"There is recursive \"Go to line\" to other \"Go to line\" in song.","Recursive \"Go to line\"...",MB_ICONSTOP);
			return 0;
		}
	}

	WaitForTimerRoutineProcessed();
	m_followplay = follow;
	g_screenupdate=1;
	PlayBeat();						//sets m_speeda
	m_speeda++;						//(Original comment by Raster, April 27, 2003) adds 1 to m_speed, for what the real thing will take place in Init
	if (m_followplay)	//cursor following the player
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;
	}
	g_playtime=0;
	m_play = mode;

	if (SAPRDUMP == 3)	//the SAP-R dumper initialisation flag was set 
	{
		for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset lines play counter first
		if (m_play == MPLAY_BLOCK)
			m_playcount[m_trackplayline] += 1;	//increment the track line play count early, so it will be detected as the selection block loop
		else
			m_playcount[m_songplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point
		SAPRDUMP = 1;	//set the SAPR dumper with the "is currently recording data" flag 
	}
	return 1;
}

BOOL CSong::Stop()
{
	m_undo.Separator();
	m_play = MPLAY_STOP;
	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;
	SetPlayPressedTonesSilence();
	WaitForTimerRoutineProcessed();	//The Timer Routine will run at least once
	return 1;
}

BOOL CSong::SongPlayNextLine()
{
	m_trackplayline=0;	//first track pattern line 

	//normal play, play from current position, or play from bookmark => shift to the next line  
	if (m_play==MPLAY_SONG || m_play==MPLAY_FROM || m_play==MPLAY_BOOKMARK)
	{ 
		m_songplayline++;	//increment the song line by 1
		if (m_songplayline > 255) m_songplayline = 0;	//above 255, roll over to 0
	}

	//when a goto line is encountered, the player will jump right to the defined line and continue playback from that position
	if (m_songgo[m_songplayline]>=0)	//if a goto line is set here...
		m_songplayline=m_songgo[m_songplayline];	//goto line ?? 

	if (SAPRDUMP == 1)	//the SAPR dumper is running with the "is currently recording data" flag 
	{
		m_playcount[m_songplayline] += 1;	//increment the position counter by 1
		int count = m_playcount[m_songplayline];	//fetch that line play count for the next step

		if (count > 1)	//a value above 1 means a full playback loop has been completed, the line play count incremented twice
		{
			loops++;	//increment the dumper iteration count by 1
			SAPRDUMP = 2;	//set the "write SAP-R data to file" flag
			if (loops == 1)
			{
				for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset the lines play count before the next step 
				m_playcount[m_songplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 
			}
			if (loops == 2)
			{
				ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
				m_play = MPLAY_STOP;	//stop the player
			}
		}
	} 
	return 1;
}

BOOL CSong::PlayBeat()
{
	int t,tt,xline,len,go,speed;
	int note[SONGTRACKS],instr[SONGTRACKS],vol[SONGTRACKS];
	TTrack *tr;

	for(t=0; t<g_tracks4_8; t++)		//done here to make it behave the same as in the routine
	{
		note[t] = -1;
		instr[t] = -1;
		vol[t] = -1;
	}

TrackLine:
	speed = m_speed;

	for(t=0; t<g_tracks4_8; t++)
	{
		tt = m_song[m_songplayline][t];
		if (tt<0) continue; //--
		tr = m_tracks.GetTrack(tt);
		len = tr->len;
		go = tr->go;
		if (m_trackplayline>=len)
		{
			if (go>=0) 
				xline = ((m_trackplayline-len) % (len-go)) + go;
			else
			{
				//if it is the end of the track, but it is a block play or the first PlayBeat call (when m_play = 0)
				if (m_play==MPLAY_BLOCK || m_play==MPLAY_STOP) { note[t]=-1; instr[t]=-1; vol[t]=-1; continue; }
				//otherwise a normal predecision to the next line in the song
				SongPlayNextLine();
				goto TrackLine;
			}
		}
		else
			xline = m_trackplayline;

		if (tr->note[xline]>=0)	note[t] = tr->note[xline];
		instr[t] = tr->instr[xline];	//due to the same behavior as in the routine
		if (tr->volume[xline]>=0) vol[t] = tr->volume[xline];
		if (tr->speed[xline]>0) speed=tr->speed[xline];
	}

	//only now is the changed speed set
	m_speeda = m_speed = speed;

	//active note, instrument and volume settings
	for(t=0; t<g_tracks4_8; t++)
	{
		int n=note[t];
		int i=instr[t];
		int v=vol[t];
		if (v>=0 && v<16)
		{
			if (n>=0 && n<NOTESNUM /*&& i>=0 && i<INSTRSNUM*/)		//adjustment for routine compatibility
			{
				if (i<0 || i>=INSTRSNUM) i=255;						//adjustment for routine compatibility
				Atari_SetTrack_NoteInstrVolume(t,n,i,v);
			}
			else
			{
				Atari_SetTrack_Volume(t,v);
			}
		}
	}

	if (m_play == MPLAY_BLOCK)
	{	//most of this code was copied directly from the songline function, it's literally the same principle but on a trackline basis instead
		if (SAPRDUMP == 1)	//the SAPR dumper is running with the "is currently recording data" flag 
		{
			m_playcount[m_trackplayline] += 1;	//increment the position counter by 1
			int count = m_playcount[m_trackplayline];	//fetch that line play count for the next step
			if (count > 1)	//a value above 1 means a full playback loop has been completed, the line play count incremented twice
			{
				loops++;	//increment the dumper iteration count by 1
				SAPRDUMP = 2;	//set the "write SAP-R data to file" flag
				if (loops == 1)
				{
					for (int i = 0; i < 256; i++) { m_playcount[i] = 0; }	//reset the lines play count before the next step 
					m_playcount[m_trackplayline] += 1;	//increment the line play count early, to ensure that same line will be detected again as the loop point for the next dumper iteration 
				}
				if (loops == 2)
				{
					ChangeTimer((g_ntsc) ? 17 : 20);	//reset the timer in case it was set to a different value
					m_play = MPLAY_STOP;	//stop the player
				}
			}
		}
	}

	return 1;
}

BOOL CSong::PlayVBI()
{
	if (!m_play) return 0;	//not playing

	m_speeda--;
	if (m_speeda>0) return 0;	//too soon to update

	m_trackplayline++;

	//m_play mode 4 => only plays range in block
	if (m_play==MPLAY_BLOCK && m_trackplayline>m_trackplayblockend) m_trackplayline=m_trackplayblockstart;

	//if none of the tracks end with "end", then it will end when reaching m_maxtracklen
	if (m_trackplayline>=m_tracks.m_maxtracklen)
		SongPlayNextLine();

	PlayBeat();	//1 pattern track line play

	if (m_speeda==m_speed && m_followplay)	//playing and following the player
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;

		//Quantization
		if (m_quantization_note>=0 && m_quantization_note<NOTESNUM
			&& m_quantization_instr>=0 && m_quantization_instr<INSTRSNUM
			)
		{
			int vol = m_quantization_vol;
			if (g_respectvolume)
			{
				int v=TrackGetVol();
				if (v>=0 && v<=MAXVOLUME) vol=v;
			}

			if ( TrackSetNoteInstrVol(m_quantization_note,m_quantization_instr,vol) )
			{
				SetPlayPressedTonesTNIV(m_trackactivecol,m_quantization_note,m_quantization_instr,vol);
			}
		}
		else
		if (m_quantization_note==-2) //Special case (midi NoteOFF)
		{
			TrackSetNoteActualInstrVol(-1);
			TrackSetVol(0);
		}
		m_quantization_note=-1; //cancel the quantized note
		//end of Q
	}

	g_screenupdate=1;
	
	return 1;
}

void CSong::TimerRoutine()
{
	//things that are solved 1x for vbi
	PlayVBI();

	//play tones if there are key presses
	PlayPressedTones();

	//--- Rendered Sound ---//
	m_pokey.RenderSound1_50(m_instrspeed);		//rendering of a piece of sample (1 / 50s = 20ms), instrspeed

	if (m_play) g_playtime++;					//if the song is currently playing, increment the timer

	//--- NTSC timing hack during playback ---//
	if (!SAPRDUMP && m_play && g_ntsc)
	{
		//the NTSC timing cannot be divided to an integer
		//the optimal timing would be 16.666666667ms, which is typically rounded to 17
		//unfortunately, things run too slow with 17, or too fast 16
		//a good enough compromise for now is to make use of a '17-17-16' miliseconds "groove"
		//this isn't proper, but at least, this makes the timing much closer to the actual thing
		//the only issue with this is that the sound will have very slight jitters during playback 
		if (g_playtime % 3 == 0) ChangeTimer(17);
		else if (g_playtime % 3 == 2) ChangeTimer(16);
	}

	//--- PICTURE DRAWING ---//
	if (g_screena>0)
		g_screena--;
	else
	{
		if (g_screenupdate) //Does it want to redraw?
		{
			g_invalidatebytimer=1;
			AfxGetApp()->GetMainWnd()->Invalidate();
		}
	}
	g_timerroutineprocessed=1;	//TimerRoutine took place
}

//--------------------------------------

CUndo::CUndo()
{
	for(int i=0; i<MAXUNDO; i++) m_uar[i]=NULL;
}

CUndo::~CUndo()
{
	for(int i=0; i<MAXUNDO; i++) DeleteEvent(i);
}

void CUndo::Init(CSong* song)
{
	m_song=song;
	Clear();
}

void CUndo::Clear()
{
	m_head=0;
	m_tail=0;
	m_headmax=0;
	m_undosteps=m_redosteps=0;
	for(int i=0; i<MAXUNDO; i++) DeleteEvent(i);
}

char CUndo::DeleteEvent(int i)
{
	TUndoEvent* ue=m_uar[i];
	if (!ue) return 1;
	char sep=ue->separator; //storage for return
	if (ue->cursor) delete[] ue->cursor;
	if (ue->pos) delete[] ue->pos;
	if (ue->data) delete[] ue->data;
	delete ue;
	m_uar[i]=NULL;
	return sep;
}

BOOL CUndo::Undo()
{
	if (m_head==m_tail)	return 0; //nothing to keep

	m_song->Stop();

	int prev;
	char sep;
	do
	{
		m_head=(m_head+MAXUNDO-1)%MAXUNDO;
		PerformEvent(m_head);
		if (m_head==m_tail) break;
		prev=(m_head+MAXUNDO-1)%MAXUNDO;
		sep=m_uar[prev]->separator;
	} while(sep==-1);

	m_undosteps--;
	m_redosteps++;

	return 1;
}

BOOL CUndo::Redo()
{
	if (m_head==m_headmax) return 0; //nothing to return

	m_song->Stop();

	char sep;
	do
	{
		sep=PerformEvent(m_head);
		m_head=(m_head+1)%MAXUNDO;
	} while(sep==-1);

	m_redosteps--;
	m_undosteps++;

	return 1;
}

void CUndo::InsertEvent(TUndoEvent *ue)
{
	if (!g_changes)
	{
		g_changes=1;	//there has been some change
		m_song->SetRMTTitle();
	}
	//add cursor
	ue->part=g_activepart;
	ue->cursor=m_song->GetUECursor(g_activepart);
	if (m_uar[m_head]) DeleteEvent(m_head);
	m_uar[m_head]=ue;
	//is there an event already?
	if (m_head!=m_tail)
	{
		TUndoEvent* le=m_uar[(m_head+MAXUNDO-1)%MAXUNDO];
		if (   ue->part==le->part
			&& ue->type==le->type
			&& !le->separator
			&& !ue->separator
			&& m_song->UECursorIsEqual(ue->cursor,le->cursor,ue->part)
			&& PosIsEqual(ue->pos,le->pos,ue->type)
			)
		{
			//the last event is at the same cursor position and with the same data
			DeleteEvent(m_head); //erases it from memory
			//and will not count it among undo events, just end the maximum undo
			m_headmax=m_head;
			m_redosteps=0;
			return;
		}
	}
	//
	if (ue->separator!=-1) m_undosteps++; //only complete events are included
	m_head=(m_head+1)%MAXUNDO;
	if (   ( m_undosteps>UNDOSTEPS )
		|| ((m_head+1)%MAXUNDO==m_tail) )
	{
		char sep;
		do
		{
			sep=DeleteEvent(m_tail);
			m_tail=(m_tail+1)%MAXUNDO;
		} while(sep==-1);
		m_undosteps--;
	}
	m_headmax=m_head;
	m_redosteps=0;
}

void CUndo::DropLast()
{
	if (m_head==m_tail) return;
	m_head = (m_head+MAXUNDO-1)%MAXUNDO;
	DeleteEvent(m_head);
	m_undosteps--;		//will count this step
}

void CUndo::Separator(int sep)
{
	TUndoEvent* le;
	le=m_uar[(m_head+MAXUNDO-1)%MAXUNDO];
	if (!le) return;
	if (sep<0 && le->separator>=0) m_undosteps--; //the number of undo counted in InsertEvent
	le->separator=sep;
}

void CUndo::ChangeTrack(int tracknum,int trackline,int type, char separator)
{
	if (tracknum<0) return;
	TTrack *tr=m_song->GetTracks()->GetTrack(tracknum);

	//An event with the original status at a different place
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[2];
	ue->pos[0]=tracknum;
	ue->pos[1]=trackline;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_NOTEINSTRVOL:
		data=new int[3];
		data[0]=tr->note[trackline];
		data[1]=tr->instr[trackline];
		data[2]=tr->volume[trackline];
		break;

	case UETYPE_NOTEINSTRVOLSPEED:
		data=new int[4];
		data[0]=tr->note[trackline];
		data[1]=tr->instr[trackline];
		data[2]=tr->volume[trackline];
		data[3]=tr->speed[trackline];
		break;

	case UETYPE_SPEED:
		data=new int[1];
		data[0]=tr->speed[trackline];
		break;

	case UETYPE_LENGO:
		data=new int[2];
		data[0]=tr->len;
		data[1]=tr->go;
		break;

	case UETYPE_TRACKDATA: //whole track
		data=(int*)new TTrack;
		memcpy((void*)data,tr,sizeof(TTrack));
		break;

	case UETYPE_TRACKSALL: //all tracks
		{
		data=(int*)new TTracksAll;
		m_song->GetTracks()->GetTracksAll((TTracksAll*)data); //fill with data
		}
		break;

	default:
		MessageBox(g_hwnd,"CUndo::ChangeTrack BAD!","Internal error",MB_ICONERROR);
		data=NULL;
	}

	ue->data=(void*)data;
	//
	InsertEvent(ue);
}

void CUndo::ChangeSong(int songline,int trackcol,int type, char separator)
{
	if (songline<0 || trackcol<0) return;

	//An event with the original status at a different place
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[2];
	ue->pos[0]=songline;
	ue->pos[1]=trackcol;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_SONGTRACK:
		data=new int[1];
		data[0]=m_song->SongGetTrack(songline,trackcol);
		break;

	case UETYPE_SONGGO:
		data=new int[1];
		data[0]=m_song->SongGetGo(songline);
		break;

	case UETYPE_SONGDATA: //whole song
		{
		TSong* song=new TSong;
		memcpy(song->song,m_song->GetSong(),sizeof(song->song));
		memcpy(song->songgo,m_song->GetSongGo(),sizeof(song->songgo));
		data=(int*)song;
		}
		break;

	default:
		MessageBox(g_hwnd,"CUndo::ChangeSong BAD!","Internal error",MB_ICONERROR);
		data=NULL;
	}

	ue->data=(void*)data;
	//
	InsertEvent(ue);
}

void CUndo::ChangeInstrument(int instrnum,int paridx,int type, char separator)
{
	if (instrnum<0) return;
	TInstrument *instr=&(m_song->GetInstruments()->m_instr[instrnum]);

	//An event with the original status at a different place
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[2];
	ue->pos[0]=instrnum;
	ue->pos[1]=paridx;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_INSTRDATA:	//whole instrument
		{
		TInstrument* ins=new TInstrument;
		memcpy(ins,instr,sizeof(TInstrument));
		data=(int*)ins;
		}
		break;

	case UETYPE_INSTRSALL: //all instruments
		{
		TInstrumentsAll* insall=new TInstrumentsAll;
		memcpy(insall,m_song->GetInstruments()->GetInstrumentsAll(),sizeof(TInstrumentsAll));
		data=(int*)insall;
		}
		break;

	default:
		MessageBox(g_hwnd,"CUndo::ChangeInstrument BAD!","Internal error",MB_ICONERROR);
		data=NULL;
	}

	ue->data=(void*)data;
	//
	InsertEvent(ue);
}

void CUndo::ChangeInfo(int paridx,int type, char separator)
{
	//An event with the original status at a different place
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[1];
	ue->pos[0]=paridx;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_INFODATA:	//whole info
		{
		TInfo* inf=new TInfo;
		m_song->GetSongInfoPars(inf); //fill with "inf" values taken from m_song
		data=(int*)inf;
		}
		break;

	default:
		MessageBox(g_hwnd,"CUndo::ChangeInfo BAD!","Internal error",MB_ICONERROR);
		data=NULL;
	}

	ue->data=(void*)data;
	//
	InsertEvent(ue);
}

BOOL CUndo::PosIsEqual(int* pos1, int* pos2, int type)
{
	int len;
	switch(type>>6)	//  /64
	{
	case 0:		len=POSGROUPTYPE0_63SIZE;
				break;
	case 1:		len=POSGROUPTYPE64_127SIZE;
				break;
	case 2:		len=POSGROUPTYPE128_191SIZE;
				break;
	default:
		return 0;
	}
	for(int i=0; i<len; i++) if (pos1[i]!=pos2[i]) return 0;
	return 1;
}

void ExchangeInt(int& a, int& b)
{
	int c=a;
	a=b;
	b=c;
}

char CUndo::PerformEvent(int i)
{
	TUndoEvent* ue=m_uar[i];
	if (!ue) return 1;
	//keeps the separator as a return value
	char sep=ue->separator;
	//set cursor there (change and g_activepart)
	m_song->SetUECursor(ue->part,ue->cursor);
	//ue->separator=0; //default separator = 0
	BLOCKDESELECT;
	//
	switch(ue->type)
	{
	case UETYPE_NOTEINSTRVOL:
		{
		int tracknum=ue->pos[0];
		int trackline=ue->pos[1];
		TTrack& tr=*(m_song->GetTracks()->GetTrack(tracknum));
		int *data=(int*)ue->data;
		ExchangeInt(tr.note[trackline],data[0]);
		ExchangeInt(tr.instr[trackline],data[1]);
		ExchangeInt(tr.volume[trackline],data[2]);
		}
		break;

	case UETYPE_NOTEINSTRVOLSPEED:
		{
		int tracknum=ue->pos[0];
		int trackline=ue->pos[1];
		TTrack& tr=*(m_song->GetTracks()->GetTrack(tracknum));
		int *data=(int*)ue->data;
		ExchangeInt(tr.note[trackline],data[0]);
		ExchangeInt(tr.instr[trackline],data[1]);
		ExchangeInt(tr.volume[trackline],data[2]);
		ExchangeInt(tr.speed[trackline],data[3]);
		}
		break;

	case UETYPE_SPEED:
		{
		int tracknum=ue->pos[0];
		int trackline=ue->pos[1];
		TTrack& tr=*(m_song->GetTracks()->GetTrack(tracknum));
		int *data=(int*)ue->data;
		ExchangeInt(tr.speed[trackline],data[0]);
		}
		break;

	case UETYPE_LENGO:
		{
		int tracknum=ue->pos[0];
		int trackline=ue->pos[1];
		TTrack& tr=*(m_song->GetTracks()->GetTrack(tracknum));
		int *data=(int*)ue->data;
		ExchangeInt(tr.len,data[0]);
		ExchangeInt(tr.go,data[1]);
		}
		break;

	case UETYPE_TRACKDATA: //whole track
		{
		int tracknum=ue->pos[0];
		TTrack* tr=(m_song->GetTracks()->GetTrack(tracknum));
		TTrack temp;
		memcpy((void*)&temp,tr,sizeof(TTrack));
		memcpy(tr,ue->data,sizeof(TTrack));
		memcpy(ue->data,(void*)&temp,sizeof(TTrack));
		}
		break;

	case UETYPE_TRACKSALL: //all tracks
		{
		TTracksAll* temp = new TTracksAll;
		m_song->GetTracks()->GetTracksAll(temp); //from temp
		m_song->GetTracks()->SetTracksAll((TTracksAll*)ue->data); //data to tracksall
		memcpy(ue->data,(void*)temp,sizeof(TTracksAll));
		delete temp;
		int maxtl= m_song->GetTracks()->m_maxtracklen;
		if (m_song->GetActiveLine()>=maxtl) m_song->SetActiveLine(maxtl-1);
		}
		break;

	//
	case UETYPE_SONGTRACK:
		{
		int songline=ue->pos[0];
		int trackcol=ue->pos[1];
		int *data=(int*)ue->data;
		ExchangeInt((*m_song->GetSong())[songline][trackcol],data[0]);
		}
		break;

	case UETYPE_SONGGO:
		{
		int songline=ue->pos[0];
		int trackcol=ue->pos[1];
		int *data=(int*)ue->data;
		ExchangeInt((*m_song->GetSongGo())[songline],data[0]);
		}
		break;

	case UETYPE_SONGDATA: //whole song
		{
		TSong temp;
		TSong* data=(TSong*)ue->data;
		//temp <= song
		memcpy((void*)&temp.song,m_song->GetSong(),sizeof(temp.song));
		memcpy((void*)&temp.songgo,m_song->GetSongGo(),sizeof(temp.songgo));
		memcpy((void*)&temp.bookmark,m_song->GetBookmark(),sizeof(temp.bookmark));
		//song <= data from undo
		memcpy(m_song->GetSong(),data->song,sizeof(temp.song));
		memcpy(m_song->GetSongGo(),data->songgo,sizeof(temp.songgo));
		memcpy(m_song->GetBookmark(),&data->bookmark,sizeof(temp.bookmark));
		//data for redo <= temp
		memcpy(data->song,(void*)&temp.song,sizeof(temp.song));
		memcpy(data->songgo,(void*)&temp.songgo,sizeof(temp.songgo));
		memcpy(&data->bookmark,(void*)&temp.bookmark,sizeof(temp.bookmark));
		}
		break;

	case UETYPE_INSTRDATA: //whole instrument
		{
		int instrnum=ue->pos[0];
		TInstrument* in=&(m_song->GetInstruments()->m_instr[instrnum]);
		TInstrument temp;
		memcpy((void*)&temp,in,sizeof(TInstrument));
		memcpy((void*)in,ue->data,sizeof(TInstrument));
		memcpy(ue->data,(void*)&temp,sizeof(TInstrument));
		//must save to Atari
		m_song->GetInstruments()->ModificationInstrument(instrnum);
		}
		break;

	case UETYPE_INSTRSALL: //all instruments
		{
		TInstrumentsAll* temp=new TInstrumentsAll;
		TInstrumentsAll* insall=m_song->GetInstruments()->GetInstrumentsAll();
		memcpy(temp,insall,sizeof(TInstrumentsAll));
		memcpy(insall,ue->data,sizeof(TInstrumentsAll));
		memcpy(ue->data,temp,sizeof(TInstrumentsAll));
		delete temp;
		//must save to Atari
		for(i=0; i<INSTRSNUM; i++) m_song->GetInstruments()->ModificationInstrument(i);
		}
		break;

	case UETYPE_INFODATA:
		{
		TInfo in;
		m_song->GetSongInfoPars(&in); //fill "in" with values taken from m_song
		TInfo* data=(TInfo*)ue->data;
		m_song->SetSongInfoPars(data); //set values in m_song with values from "data"
		memcpy(ue->data,(void*)&in,sizeof(TInfo));
		}
		break;

	default:
		MessageBox(g_hwnd,"PerformEvent BAD!","Internal error",MB_ICONERROR);
	}
	return sep; //returns separator
}
