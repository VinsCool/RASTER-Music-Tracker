//
// R_MUSIC.CPP
//

// 25.12.2002 21:45 .... neuveritelne, ono to saveuje a loaduje RMTcka v Atari object file formatu!!!
//
// 25.12.2002 22:22 .... neuveritelne, ono to exportuje FUNKCNI SAP modul!!!
//
// 26.12.2002 10:59 .... stvoril jsem monstrum s montrooznimi moznostmi. Umi to hrat i jako SID ze C64. ;-)
//
// 27.4.2003 .... oprava vlekle chyby s vynechanim 1/50 instrumentu u prvnich tonu (diky za upozorneni od Memblers from Indiana)
//
// 11.2.2004 ... pridano generovani FEAT_VOLUMEMIN, FEAT_TABLEGO

#include "stdafx.h"
#include <fstream>
using namespace std;
//#include <mmsystem.h>
#include <malloc.h>

#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"


#include "MainFrm.h"	//!

#include "r_music.h"
#include "Atari6502.h"

#include "global.h"

//-------

#define BLOCKSETBEGIN	g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline)
#define BLOCKSETEND		g_trackcl.BlockSetEnd(m_trackactiveline)
#define BLOCKDESELECT	g_trackcl.BlockDeselect()
#define ISBLOCKSELECTED	g_trackcl.IsBlockSelected()


CString GetFilePath(CString pathandfilename)
{
	//vstup:  c:/neco/nekde/kdovikde\nebo\taky\tohle.ext
	//vystup: c:/neco/nekde/kdovikde\nebo\taky
	//(proste od zacatku po posledni lomitko (/ nebo \)
	CString res;
	int pos=pathandfilename.ReverseFind('/');
	if (pos<0) pos=pathandfilename.ReverseFind('\\');
	if (pos>=0)
		res=pathandfilename.Mid(0,pos); //od 0 do pos
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
	//instrnumonly<0   => vsechny instrumenty
	//            >=0  => pouze ten jeden instrument
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

//-------

CTrackClipboard::CTrackClipboard()
{
	m_song = NULL;
	m_trackcopy.len = -1;	//pro kopirovani kompletniho tracku i se smyckama atd.
	m_trackbackup.len = -1; //zalozni
	m_all = 1;				//1=vsechny udalosti / 0=jen u udalosti s instrumentem stejnym jako je aktualne nastaveny
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
		//nove oznaceny zacatek bloku
		m_selcol = col;
		m_seltrack = track;
		m_selsongline = m_song->SongGetActiveLine();
		m_selfrom = m_selto = line;
		//uschova si ten track tak jak byl ted
		memcpy((void*)(&m_trackbackup),(void*)(m_song->GetTracks()->GetTrack(track)),sizeof(TTrack));
		//a inicializuje base track
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

	//promitne momentalni stav tracku do base tracku
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

	//musi to delat pozpatku kvuli GO smycce
	//(kdyz by totiz nejdriv shora prepsal noty, ktere se pak opakuji v GO smycce, tak by
	//se do clipboardu misto tech puvodnich smyckovych objevili uz ty cerstve prepsane)
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
			{	//blok je delsi nez delka dat v clipboardu, takze doplni prazdnymi radky
				ts.note[line]=-1;
				ts.instr[line]=-1;
				ts.volume[line]=-1;
				ts.speed[line]=-1;
			}
		}
		else
		{	//cast bloku presahujici za end
			if (go>=0)
			{			//je tam smycka
				td.note[j]=ts.note[xline];
				td.instr[j]=ts.instr[xline];
				td.volume[j]=ts.volume[xline];
				td.speed[j]=ts.speed[xline];
			}
			else
			{			//neni tam smycka
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
		//Je vybrany blok, takze PASTE bude do nej misto na line pozici
		track = m_seltrack;
		GetFromTo(bfro,bto);
	}
	
	TTrack& ts = m_track;
	TTrack& td = *m_song->GetTracks()->GetTrack(track);
	int i,j;
	int linemax;
	
	if (bfro>=0) //je vybrany blok (pokracovani)
	{
		line = bfro;
		linemax = bto+1;
	}
	else
		linemax = line + ts.len;

	if (linemax > m_song->GetTracks()->m_maxtracklen) linemax = m_song->GetTracks()->m_maxtracklen;
	if (line>td.len)
	{
		//pokud provadi paste pod --end-- line, vyprazdni mezeru mezi --end-- a koncem mista kam pastuje
		//puvodne     i<line  , ale protoze u nekterych merguje k puvodnimu, tak promaze az po konec linemax
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
			if (ts.volume[j]>=0) td.volume[i]=ts.volume[j]; //je-li zdrojova volume nezaporna, zapise ji
			else //zdrojova volume je zaporna, tj.vymazat ji muze jen tam kde neni note+instr
			if (td.note[i]<0 && td.instr[i]<0) td.volume[i]=-1; //vymaze jen u samostatnych volume
		}
	}
	else
	if (special==3) //speeds only
	{
		for(i=line,j=0; i<linemax; i++,j++) td.speed[i]=ts.speed[j];
	}

	//pokud je to za koncem tracku, prodlouzi mu delku
	if (linemax>td.len) td.len=linemax;
	return (bfro>=0)? 0 : linemax-line;	//kdyz bylo paste do bloku, vraci 0
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
		//promitne momentalni stav tracku do base tracku
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

	//Info do status baru
	CString s;
	s.Format("Note transposition: %+i",m_changenote);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockInstrumentChange(int instr,int addinstr)
{
	if (!IsBlockSelected() || m_seltrack<0) return;

	if (instr!=m_instrbase)
	{
		//promitne momentalni stav tracku do base tracku
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

	//Info do status baru
	CString s;
	s.Format("Instrument change: %+i",m_changeinstr);
	SetStatusBarText((LPCTSTR)s);
}

void CTrackClipboard::BlockVolumeChange(int instr,int addvol)
{
	if (!IsBlockSelected() || m_seltrack<0) return;

	if (instr!=m_instrbase)
	{
		//promitne momentalni stav tracku do base tracku
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
		if (td.instr[i]>=0) lasti=td.instr[i]; //aby kdyz je samotne volume, aby poznal ze to patri k tomu nastroji nad tim
		if (td.volume[i]>=0 && (lasti==instr || m_all))
		{
			j=m_trackbase.volume[i]+m_changevolume;
			if (j>MAXVOLUME) j=MAXVOLUME;
			if (j<0) j=0;
			td.volume[i]=j;
		}
	}

	//Info do status baru
	CString s;
	s.Format("Volume change: %+i",m_changevolume);
	SetStatusBarText((LPCTSTR)s);
}


BOOL CTrackClipboard::BlockEffect()
{
	if (!IsBlockSelected() || m_seltrack<0) return 0;

	CEffectsDlg dlg;

	if (m_all) dlg.m_info = "Changes will provided for all events in the block";
	else
		dlg.m_info.Format("Changes will provided for instrument %02X events only",m_song->GetActiveInstr());

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
	//UpdateShiftControlKeys();

	return (r==IDOK);
}


//-------


const char *notes[]=
{ "C-1","C#1","D-1","D#1","E-1","F-1","F#1","G-1","G#1","A-1","A#1","H-1",
  "C-2","C#2","D-2","D#2","E-2","F-2","F#2","G-2","G#2","A-2","A#2","H-2",
  "C-3","C#3","D-3","D#3","E-3","F-3","F#3","G-3","G#3","A-3","A#3","H-3",
  "C-4","C#4","D-4","D#4","E-4","F-4","F#4","G-4","G#4","A-4","A#4","H-4",
  "C-5","C#5","D-5","D#5","E-5","F-5","F#5","G-5","G#5","A-5","A#5","H-5",
  "C-6","???","???","???"
};

//pro castecne selektovani v tracku
/*
static char *colac[]=
//  z   C   #    1      I   I       V   S   S
{ "\x06\x03\x03\x03\x06\x06\x06\x06\x06\x06\x06",
  "\x06\x06\x06\x06\x06\x03\x03\x06\x06\x06\x06",
  "\x06\x06\x06\x06\x06\x06\x06\x06\x03\x06\x06",
  "\x06\x06\x06\x06\x06\x06\x06\x06\x06\x03\x03"
};
*/

static char csel1[11]={6,COLOR_SELECTED,COLOR_SELECTED,COLOR_SELECTED,6,6,6,6,6,6,6};
static char csel2[11]={6,6,6,6,6,COLOR_SELECTED,COLOR_SELECTED,6,6,6,6};
static char csel3[11]={6,6,6,6,6,6,6,6,COLOR_SELECTED,6,6};
static char csel4[11]={6,6,6,6,6,6,6,6,6,COLOR_SELECTED,COLOR_SELECTED};

static char *colac[]=
//  z   C   #    1      I   I       V   S   S
{
	csel1,csel2,csel3,csel4
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
	{ 0,INSTRS_PX+17*8,INSTRS_PY+ 9*16,"TLEN:", 0x1f, 0x1f, 1, 8, 1,15,15 },
	{ 1,INSTRS_PX+18*8,INSTRS_PY+10*16,"TGO:" , 0x1f, 0x1f, 0, 0, 2,16,16 },
	{ 2,INSTRS_PX+17*8,INSTRS_PY+11*16,"TSPD:", 0x3f, 0x3f, 1, 1, 3,17,17 },
	{ 3,INSTRS_PX+17*8,INSTRS_PY+12*16,"TYPE:", 0x01, 0x01, 0, 2, 4,18,18 },
	{ 4,INSTRS_PX+17*8,INSTRS_PY+13*16,"MODE:", 0x01, 0x01, 0, 3, 5,19,19 },
	//ENVELOPE: LEN GO VSLIDE VMIN
	{ 5,INSTRS_PX+17*8,INSTRS_PY+2*16,"ELEN:"  ,0x3f, 0x2f, 1, 4, 6, 9, 9 },
	{ 6,INSTRS_PX+18*8,INSTRS_PY+3*16,"EGO:"   ,0x3f, 0x2f, 0, 5, 7,10,10 },
	{ 7,INSTRS_PX+15*8,INSTRS_PY+4*16,"VSLIDE:",0xff, 0xff, 0, 6, 8,11,11 },
	{ 8,INSTRS_PX+17*8,INSTRS_PY+5*16,"VMIN:"  ,0x0f, 0x0f, 0, 7, 0,11,11 },
	//EFFECT: DELAY VIBRATO FSHIFT
	{ 9,INSTRS_PX+ 2*8,INSTRS_PY+ 2*16,"DELAY:",  0xff,0xff, 0,19,10, 5, 5 },
	{10,INSTRS_PX+ 0*8,INSTRS_PY+ 3*16,"VIBRATO:",0x03,0x03, 0, 9,11, 6, 6 },
	{11,INSTRS_PX+ 1*8,INSTRS_PY+ 4*16,"FSHIFT:", 0xff,0xff, 0,10,12, 7, 7 },
	//AUDCTL: 00-07
	{12,INSTRS_PX+ 2*8,INSTRS_PY+ 6*16,  "15KHZ:",0x01,0x01,0,11,13, 0, 0 },
	{13,INSTRS_PX+ 2*8,INSTRS_PY+ 7*16,  "FI2+4:",0x01,0x01,0,12,14, 0, 0 },
	{14,INSTRS_PX+ 2*8,INSTRS_PY+ 8*16,  "FI1+3:",0x01,0x01,0,13,15, 0, 0 },
	{15,INSTRS_PX+ 2*8,INSTRS_PY+ 9*16,  "CH4+3:",0x01,0x01,0,14,16, 0, 0 },
	{16,INSTRS_PX+ 2*8,INSTRS_PY+10*16,  "CH2+1:",0x01,0x01,0,15,17, 1, 1 },
	{17,INSTRS_PX+ 0*8,INSTRS_PY+11*16,"1.79CH3:",0x01,0x01,0,16,18, 2, 2 },
	{18,INSTRS_PX+ 0*8,INSTRS_PY+12*16,"1.79CH1:",0x01,0x01,0,17,19, 3, 3 },
	{19,INSTRS_PX+ 2*8,INSTRS_PY+13*16,  "POLY9:",0x01,0x01,0,18, 9, 4, 4 }
};

/*
#define V0PAR_TAB0		0
#define V0PAR_TAB1		1
#define V0PAR_TAB2		2
#define V0PAR_TAB3		3
#define V0PAR_TAB4		4
#define V0PAR_TAB5		5
#define V0PAR_TAB6		6
#define V0PAR_TAB7		7
#define V0PAR_TABLEN	8
#define V0PAR_TABGO		9
#define V0PAR_TABSPD	10
#define V0PAR_TABTYPE	11
#define V0PAR_TABMODE	12

#define V0PAR_ENVLEN	13
#define V0PAR_ENVGO		14
#define V0PAR_VSLIDE	15
#define V0PAR_VMIN		16
#define V0PAR_DELAY		17
#define V0PAR_VIBRATO	18
#define V0PAR_FSHIFT	19
#define V0PAR_POLY9		20
#define V0PAR_15KHZ		21
*/

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
	{   0,0x0f,1,-1,"VOLUME R:"  ,INSTRS_EX+2*8,INSTRS_EY+2*16 },	//volume right
	{   0,0x0f,1,-1,"VOLUME L:"	 ,INSTRS_EX+2*8,INSTRS_EY+8*16 },	//volume left
	{   0,0x0e,2,-2,"DISTORTION:",INSTRS_EX+0*8,INSTRS_EY+9*16 },	//distortion 0,2,4,6,...
	{   0,0x07,1,-1,"COMMAND:"	 ,INSTRS_EX+3*8,INSTRS_EY+10*16 },	//command 0-7
	{   0,0x0f,1,-1,"X/:"		 ,INSTRS_EX+8*8,INSTRS_EY+11*16 },	//X
	{   0,0x0f,1,-1,"Y\\:"		 ,INSTRS_EX+8*8,INSTRS_EY+12*16 },	//Y
	{   9,0x01,1,-1,"FILTER:"	 ,INSTRS_EX+4*8,INSTRS_EY+13*16 },	//filter *
	{   9,0x01,1,-1,"PORTAMENTO:",INSTRS_EX+0*8,INSTRS_EY+14*16 }		//portamento *
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
	Atari_InstrumentTurnOff(it); //vypne tento instrument na vsech generatorech

	int i,j;
	char *s = m_instr[it].name; 
	memset(s,' ',INSTRNAMEMAXLEN);
	sprintf(s,"Instrument %02X",it);
	s[strlen(s)] = ' '; //preplacne 0x00 za koncem textu
	s[INSTRNAMEMAXLEN]=0;

	m_instr[it].act=0;				//active name
	m_instr[it].activenam=0;		//0.ty znak v name
	m_instr[it].activepar=PAR_ENVLEN;	//default je ENVELOPE LEN
	m_instr[it].activeenvx=0;
	m_instr[it].activeenvy=1;		//volume levy
	m_instr[it].activetab=0;		//0.prvek v tabulce

	m_instr[it].octave=0;
	m_instr[it].volume=MAXVOLUME;
	
	for(i=0; i<PARCOUNT; i++) m_instr[it].par[i]=0;
	for(i=0; i<ENVCOLS; i++)
	{
		for(j=0; j<ENVROWS; j++) m_instr[it].env[i][j]=0; //rand()&0x0f;			//0;
	}
	for(i=0; i<TABLEN; i++) m_instr[it].tab[i]=0;

	m_iflag[it] = 0;	//init instrument flagu

	ModificationInstrument(it);			//promitne do Atari mem

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
		if (iotype==IOINSTR_TXT && !CalculateNoEmpty(i)) continue; //do TXT jen neprazdne instrumenty
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

int CInstruments::SaveInstrument(int instr,ofstream& ou, int iotype)
{
	TInstrument& ai=m_instr[instr];

	int j,k;

	switch(iotype)
	{
	case IOINSTR_RTI:
		{
		//RTI file
		static char head[4]="RTI";
		head[3]=1;			//typ 1
		ou.write(head,4);	//4 byty hlavicka RTI1 (binarni 1)
		ou.write((const char*) ai.name,sizeof(ai.name)); //jmeno 32bytu + 33. je binarni nula ukoncujici string
		unsigned char ibf[MAXATAINSTRLEN];
		BYTE len = InstrToAta(instr,ibf,MAXATAINSTRLEN);
		ou.write((char*)&len,sizeof(len));				//delka instrumentu v Atari bytech
		if (len>0) ou.write((const char*)&ibf,len);					//data instrumentu
		}
		break;

	case IOINSTR_RMW:
		//name instrumentu
		ou.write((const char*) ai.name,sizeof(ai.name));

		char bfpar[PARCOUNT],bfenv[ENVCOLS][ENVROWS],bftab[TABLEN];
		//
		//
		for (j=0; j<PARCOUNT; j++) bfpar[j]=ai.par[j];
		ou.write(bfpar, sizeof(bfpar));
		//
		for(k=0; k<ENVROWS; k++)
		{
			for (j=0; j<ENVCOLS; j++)
				bfenv[j][k] = ai.env[j][k];
		}
		ou.write((char*)bfenv, sizeof(bfenv));
		//
		for (j=0; j<TABLEN; j++) bftab[j]=ai.tab[j];
		ou.write(bftab, sizeof(bftab));
		//
		//+editacni doplnky:
		ou.write((char*)&ai.act,sizeof(ai.act));
		ou.write((char*)&ai.activenam,sizeof(ai.activenam));
		ou.write((char*)&ai.activepar,sizeof(ai.activepar));
		ou.write((char*)&ai.activeenvx,sizeof(ai.activeenvx));
		ou.write((char*)&ai.activeenvy,sizeof(ai.activeenvy));
		ou.write((char*)&ai.activetab,sizeof(ai.activetab));
		//octaves and volumes
		ou.write((char*)&ai.octave,sizeof(ai.octave));
		ou.write((char*)&ai.volume,sizeof(ai.volume));
		break;

	case IOINSTR_TXT:
		//TXT file
		CString s,nambf;
		nambf=ai.name;
		nambf.TrimRight();
		s.Format("[INSTRUMENT]\n%02X: %s\n",instr,(LPCTSTR)nambf);
		ou << (LPCTSTR)s;
		//parametry instrumentu
		for(j=0; j<NUMBEROFPARS; j++)
		{
			s.Format("%s %X\n",shpar[j].name,ai.par[j]+shpar[j].pfrom);
			ou << (LPCTSTR)s;
		}
		//table
		ou << "TABLE: ";
		for(j=0; j<=ai.par[PAR_TABLEN]; j++)
		{
			s.Format("%02X ",ai.tab[j]);
			ou << (LPCTSTR)s;
		}
		ou << endl;
		//envelope
		for(k=0; k<ENVROWS; k++)
		{
			char bf[ENVCOLS+1];
			for (j=0; j<=ai.par[PAR_ENVLEN]; j++)
			{
				bf[j]= CharL4(ai.env[j][k]);
			}
			bf[ai.par[PAR_ENVLEN]+1]=0; //ukonceni bufferu
			s.Format("%s %s\n",shenv[k].name,bf);
			ou << (LPCTSTR)s;
		}
		ou << "\n"; //mezera
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
		if (instr<0 || instr>=INSTRSNUM) return 0;
		ClearInstrument(instr);	//nejdriv ho smaze nez bude nacitat
		TInstrument& ai=m_instr[instr];
		char head[4];
		in.read(head,4);	//4 byty hlavicka
		if (strncmp(head,"RTI",3)!=0) return 0;		//neni tam RTI hlavicka
		int version=head[3];
		if (version>=2) return 0;					//je to verze 2 a vic (podporovana je jen 0 a 1)

		in.read((char*) ai.name,sizeof(ai.name)); //jmeno 32 bytu + 33.binarni ukoncujici nula

		BYTE len;
		in.read((char*)&len,sizeof(len));				//delka instrumentu v Atari bytech
		if (len>0)
		{
			unsigned char ibf[MAXATAINSTRLEN];
			in.read((char *)&ibf,len);
			BOOL r;
			if (version==0) 
				r=AtaV0ToInstr(ibf,instr);
			else
				r=AtaToInstr(ibf,instr);
			ModificationInstrument(instr);	//zapise do Atari ram
			if (!r) return 0; //nejaky problem s instrumentem
		}
		}
		break;

	case IOINSTR_RMW:
		{
		//RMW
		if (instr<0 || instr>=INSTRSNUM) return 0;
		ClearInstrument(instr);	//nejdriv ho smaze nez bude nacitat
		TInstrument& ai=m_instr[instr];
		//jmeno instrumentu
		in.read((char *) ai.name,sizeof(ai.name));


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
		ModificationInstrument(instr);	//zapise do Atari mem
		//
		//+editacni doplnky:
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
			in.getline(line,1024); //prvni radek instrumentu
			int iins=Hexstr(line,2);

			if (instr==-1) instr=iins; //prebere cislo instrumentu

			if (instr<0 || instr>=INSTRSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearInstrument(instr);	//nejdriv ho smaze nez bude nacitat
			TInstrument& ai=m_instr[instr];

			char* value = line+4;
			Trimstr(value);
			memset(ai.name,' ',INSTRNAMEMAXLEN);
			int lname=INSTRNAMEMAXLEN;
			if (strlen(value)<=INSTRNAMEMAXLEN) lname=strlen(value);
			strncpy(ai.name,value,lname);

			int v,j,k,vlen;
			while(!in.eof())
			{
				in.read((char*)&b,1);
				if (b=='[') goto InstrEnd;	//konec instrumentu (zacatek neceho dalsiho)
				line[0]=b;
				in.getline(line+1,1024);
				
				value=strstr(line,": ");
				if (value)
				{
					value[1]=0;	//preplacne mezeru ukoncenim
					value+=2;	//prvni znak za mezerou
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
		ModificationInstrument(instr);	//zapise do Atari mem
		break;

	}

	return 1;
}


BOOL CInstruments::ModificationInstrument(int instr)
{
	unsigned char* ata = g_atarimem + instr*256 +0x4000;
	g_rmtroutine = 0;			//vypnuti RMT rutiny
	BYTE r = InstrToAta(instr,ata,MAXATAINSTRLEN);
	g_rmtroutine = 1;			//zapnuti RMT rutiny
	RecalculateFlag(instr);
	return r;
}

void CInstruments::CheckInstrumentParameters(int instr)
{
	TInstrument& ai=m_instr[instr];
	//kontrola ENVELOPE len-go smycky
	if (ai.par[PAR_ENVGO]>ai.par[PAR_ENVLEN]) ai.par[PAR_ENVGO]=ai.par[PAR_ENVLEN];
	//kontrola TABLE len-go smycky
	if (ai.par[PAR_TABGO]>ai.par[PAR_TABLEN]) ai.par[PAR_TABGO]=ai.par[PAR_TABLEN];
	//kontrola kurzoru v envelope
	if (ai.activeenvx>ai.par[PAR_ENVLEN]) ai.activeenvx=ai.par[PAR_ENVLEN];
	//kontrola kurzoru v table
	if (ai.activetab>ai.par[PAR_TABLEN]) ai.activetab=ai.par[PAR_TABLEN];
	//neco se zmenilo => Ulozeni instrumentu "do Atarka"
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
		//filter ma prednost pred bass16, tj. kdyz je soucasne filter i bass16, bass16 neplati
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
	 11 0 (nevyuzito)
	*/

	const int INSTRPAR=12;			//od 12.byte zacina table

	int tablelast = par[PAR_TABLEN]+INSTRPAR;		//od 12.byte zacina table
	ata[0] = tablelast;
	ata[1] = par[PAR_TABGO]+INSTRPAR;				//od 12.byte zacina table
	ata[2] = par[PAR_ENVLEN]*3 +tablelast+1;	//za tablkou je envelope
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
	ata[11]= 0; //nevyuzito

	//0-delka table: prvky table
	for(i=0; i<=par[PAR_TABLEN]; i++) ata[INSTRPAR+i]=ai.tab[i];

	//envelope je za table
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
	return tablelast + 1 + (len+1)*3;	//vraci datovou delku instrumentu
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
	 11 0 (nevyuzito)			vynechano
	*/

	const int INSTRPAR=11;			//RMF (standardne je to 12)

	int tablelast = par[PAR_TABLEN]+INSTRPAR;	 //+12	//od 12.byte zacina table
	ata[0] = tablelast;
	ata[1] = par[PAR_TABGO]+INSTRPAR;				//od 12.byte zacina table
	ata[2] = par[PAR_ENVLEN]*3 +tablelast+1+1;	//za tablkou je envelope  //RMF +1
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
	ata[11]= 0; //nevyuzito

	//0-delka table: prvky table
	for(i=0; i<=par[PAR_TABLEN]; i++) ata[INSTRPAR+i]=ai.tab[i];

	//envelope je za table
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
	return tablelast + 1 + (len+1)*3;	//vraci datovou delku instrumentu
}



BOOL CInstruments::AtaV0ToInstr(unsigned char *ata, int instr)
{
	//STARA VERZE INSTRUMENTU

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
		env[ENV_VOLUMER] = (stereo)? (ata[j]>>4) : (ata[j] & 0x0f); //je-li mono, pak VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER]  = ata[j+1]>>7;
		env[ENV_COMMAND] = (ata[j+1]>>4) & 0x07;
		env[ENV_DISTORTION] = ata[j+1] & 0x0e;	//suda cisla 0,2,4,..,14
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

	//provereni rozsahu tab a envelope
	if (tablen>=TABLEN || tabgo>tablen
		|| envlen>=ENVCOLS || envgo>envlen)
	{
		//tablen prekracuje hranici nebo tabgo prekracuje tablen
		//nebo envlen ...
		return 0; 
	}

	//teprve jsou-li rozsahy ok, meni obsah instrumentu
	
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
		env[ENV_VOLUMER] = (stereo)? (ata[j]>>4) : (ata[j] & 0x0f); //je-li mono, pak VOLUME R = VOLUME L
		env[ENV_VOLUMEL] = ata[j] & 0x0f;
		env[ENV_FILTER]  = ata[j+1]>>7;
		env[ENV_COMMAND] = (ata[j+1]>>4) & 0x07;
		env[ENV_DISTORTION] = ata[j+1] & 0x0e;	//suda cisla 0,2,4,..,14
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
	return 0; //je prazdny
}


BOOL CInstruments::DrawInstrument(int it)
{
	int i;
	char s[128];

	TInstrument& t = m_instr[it];

	sprintf(s,"INSTRUMENT %02X",it);
	TextXY(s,INSTRS_X,INSTRS_Y);
	int size=(t.par[PAR_ENVLEN]+1)*3+(t.par[PAR_TABLEN]+1)+12;
	sprintf(s,"(SIZE %u BYTES)",size);
	TextMiniXY(s,INSTRS_X+14*8,INSTRS_Y+5);
	DrawName(it);

	TextMiniXY("EFFECT",INSTRS_PX,INSTRS_PY+1*16+8);
	TextMiniXY("AUDCTL",INSTRS_PX+0*8,INSTRS_PY+5*16+8);
	TextMiniXY("ENVELOPE",INSTRS_PX+15*8,INSTRS_PY+16+8);
	TextMiniXY("TABLE",INSTRS_PX+15*8,INSTRS_PY+8*16+8);
	//TextXY("AUDCTL",INSTRS_X+13*8,INSTRS_PY+12*16);
	//
	TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX+11*8-1,INSTRS_EY+3*16,1);
	
	if (t.act==2)	//pouze kdyz je kurzor na editaci envelope
	{
		sprintf(s,"POS %02X",t.activeenvx);
		TextXY(			   s,INSTRS_EX+2*8,INSTRS_EY+5*16,1); //sedou
	}
	//
	for(i=1; i<ENVROWS; i++) //vynechava vypis "VOLUME R:"
	{
		TextXY(shenv[i].name, shenv[i].xpos, shenv[i].ypos);
	}

	if (g_tracks4_8>4)
	{
		TextXY(shenv[0].name, shenv[0].xpos, shenv[0].ypos); //"VOLUME R:"
		TextDownXY("\x0e\x0e\x0e\x0e", INSTRS_EX+11*8-1,INSTRS_EY-2*16,1);
		g_mem_dc->MoveTo(INSTRS_EX+12*8-1,INSTRS_EY+2*16-1);
		g_mem_dc->LineTo(INSTRS_EX+12*8+ENVCOLS*8,INSTRS_EY+2*16-1);
	}

	for(i=0; i<NUMBEROFPARS; i++) DrawPar(i,it);
	
	//ikona u TABLE TYPE
	i = (t.par[PAR_TABTYPE]==0) ? 1 : 2;
	IconMiniXY(i,shpar[PAR_TABTYPE].x+8*8+2,shpar[PAR_TABTYPE].y+7);
	//napis dole u TABLE
	TextMiniXY((i==1)? "TABLE OF NOTES":"TABLE OF FREQS",INSTRS_TX,INSTRS_TY-8);

	//ikona u TABLE MODE
	i = (t.par[PAR_TABMODE]==0) ? 3 : 4;
	IconMiniXY(i,shpar[PAR_TABMODE].x+8*8+2,shpar[PAR_TABMODE].y+7);

	s[1]=0;

	//ENVELOPE
	int len = t.par[PAR_ENVLEN];	//par 13 je delka evelope
	for(i=0; i<=len; i++) DrawEnv(i,it);

	//ENVELOPE LOOP SIPKY
	int go = t.par[PAR_ENVGO];		//par 14 je envelope GO smycka
	if (go<len)
	{
		s[0]='\x07';	//Go odtud
		TextXY(s,INSTRS_EX+12*8+len*8,INSTRS_EY+7*16);
		s[0]='\x06';	//Go sem

		int lengo = len-go;
		if (lengo>3) NumberMiniXY(lengo+1,INSTRS_EX+11*8+4+go*8+lengo*4,INSTRS_EY+7*16+4); //len-go cisilko
	}
	else
		s[0]='\x16';	//GO odtud-sem
	TextXY(s,INSTRS_EX+12*8+go*8,INSTRS_EY+7*16);
	if (go>2) NumberMiniXY(go,INSTRS_EX+11*8+go*4,INSTRS_EY+7*16+4); //GO cisilko

	//TABLE
	len = t.par[PAR_TABLEN];	//delka table
	for(i=0; i<=len; i++) DrawTab(i,it);

	//TABLE LOOP SIPKY
	go = t.par[PAR_TABGO];		//table GO smycka
	if (len==0)
	{
		TextXY("\x18",INSTRS_TX+4,INSTRS_TY+8+16);
	}
	else
	{
		TextXY("\x19",INSTRS_TX+go*8*3,INSTRS_TY+8+16);
		TextXY("\x1a",INSTRS_TX+8+len*8*3,INSTRS_TY+8+16);
	}

	//vymezeni prostoru pro Envelope VOLUME
	g_mem_dc->MoveTo(INSTRS_EX+12*8-1,INSTRS_EY+7*16-1);
	g_mem_dc->LineTo(INSTRS_EX+12*8+ENVCOLS*8,INSTRS_EY+7*16-1);

	//ohraniceni vsech casti instrumentu
	/*
	CBrush br(RGB(112,112,112));
	g_mem_dc->FrameRect(CRect(INSTRS_X-2,INSTRS_Y-2,INSTRS_X+38*8+4,INSTRS_Y+2*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_PX-2,INSTRS_PY+16-2,INSTRS_PX+29*8,INSTRS_PY+15*16+4),&br);
	//g_mem_dc->FrameRect(CRect(INSTRS_EX,INSTRS_EY-2*16-2,INSTRS_EX+48*8,INSTRS_EY+15*16+2),&br);
	g_mem_dc->FrameRect(CRect(INSTRS_TX-2,INSTRS_TY-2,INSTRS_TX+54*8+4,INSTRS_TY+2*16+8),&br);
	*/

	if (!g_viewinstractivehelp) return 1; //nechce help => konec
	//chce help, pokracujeme


//oddelujici cara
#define HORIZONTALLINE {	g_mem_dc->MoveTo(INSTRS_HX,INSTRS_HY-1); g_mem_dc->LineTo(INSTRS_HX+93*8,INSTRS_HY-1); }

	if (t.act==2)	//je kurzor na envelope?
	{
		switch(t.activeenvy)
		{
		case ENV_DISTORTION:
			{
			int d = t.env[t.activeenvx][ENV_DISTORTION];
			static char* distor_help[8] = {
				"Distortion 0. (AUDC $0v)",
				"Distortion 2. (AUDC $2v)",
				"Distortion 4. (AUDC $4v)",
				"Distortion 12, 16bit bass tones by join of two generators. (AUDC $Cv)",
				"Distortion 8. (AUDC $8v)",
				"Distortion 10, pure tones. (AUDC $Av)",
				"Distortion 12, bass tones - bass table 1. (AUDC $Cv)",
				"Distortion 12, bass tones - bass table 2. (AUDC $Cv)" };
			char* hs = distor_help[(d>>1)&0x07];
			TextXY(hs,INSTRS_HX,INSTRS_HY,1);
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
				"Set FILTER_SHFRQ += frequency $XY. Play BASE_NOTE.",
				"Set BASE_NOTE = $XY, play BASE_NOTE. / If $XY == $80, then VOLUME ONLY mode." };
			char* hs = comm_help[c & 0x07];
			TextXY(hs,INSTRS_HX,INSTRS_HY,1);
			//HORIZONTALLINE;
			}
			break;

		case ENV_X:
		case ENV_Y:
			{
			char i = (t.env[t.activeenvx][ENV_X]<<4) | t.env[t.activeenvx][ENV_Y];
			sprintf(s,"XY: $%02X = %i = %+i",(unsigned char)i,(unsigned char)i,i);
			TextXY(s,INSTRS_HX,INSTRS_HY,1);
			//HORIZONTALLINE;
			}
			break;
		}
	}
	else
	if (t.act==1)	//kurzor je na main parametrech
	{
		switch(t.activepar)
		{
		case PAR_DELAY:
			{
			unsigned char i = (t.par[t.activepar]);
			if (i>0)
				sprintf(s,"$%02X = %i",i,i);
			else
				sprintf(s,"$00 = no effects.");
			TextXY(s,INSTRS_HX,INSTRS_HY,1);
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
			TextXY(s,INSTRS_HX,INSTRS_HY,1);
			//HORIZONTALLINE;
			}
			break;

		/*
		case PAR_AUDCTL:
			{
				unsigned char i = (t.par[t.activepar]);
				if (i)
				{
					CString h;
					h.Format("$%02X = ",i);
					if (i&0x80) h+="POLY9 ";
					if (i&0x40) h+="CH1:1.79 ";
					if (i&0x20) h+="CH3:1.79 ";
					if (i&0x10) h+="JOIN(2+1) ";
					if (i&0x08) h+="JOIN(4+3) ";
					if (i&0x04) h+="FILTER(1+3) ";
					if (i&0x02) h+="FILTER(2+4) ";
					if (i&0x01) h+="64KHZ";
					TextXY((char*)(LPCTSTR)h,INSTRS_HX,INSTRS_HY,1);
				}
			}
			break;
		*/
		}
	}
	if (t.act==3)	//kurzor je na table
	{
		char i = (t.tab[t.activetab]);
		sprintf(s,"$%02X = %+i",(unsigned char)i,i);
		TextXY(s,INSTRS_HX,INSTRS_HY,1);
		//HORIZONTALLINE;
	}


	return 1;
}

BOOL CInstruments::DrawName(int it)
{
	char* s=GetName(it);
	int n=-1,c=0;
	if (!g_prove && g_activepart==PARTINSTRS && m_instr[it].act==0) //je aktivni zmena jmena instrumentu
	{
		n=m_instr[it].activenam; //ktere pismenko
		c=6; //cervene
	}
	TextXY("NAME:",INSTRS_X,INSTRS_Y+16,0);
	TextXYSelN(s,n,INSTRS_X+6*8,INSTRS_Y+16,c);
	return 1;
}

BOOL CInstruments::DrawPar(int p,int it)
{
	char s[2];
	s[1]=0;
	//TextXY(shpar[p].name,shpar[p].x,shpar[p].y);
	char *txt=shpar[p].name;
	int x=shpar[p].x;
	int y=shpar[p].y;

	char a;
	int c=0;				//c<<4;
	for(int i=0; a=(txt[i]); i++,x+=8)
		g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,(a & 0x7f)<<3,c,SRCCOPY);
	
	//selektnuty parametr?
	if (!g_prove && g_activepart==PARTINSTRS && m_instr[it].act==1 && m_instr[it].activepar==p) c=COLOR_SELECTED;	//color 3

	x+=8;
	int showpar = m_instr[it].par[p] + shpar[p].pfrom;	//nektere parametry jsou 0..x ale vypisuje se 1..x+1
	if (shpar[p].pmax+shpar[p].pfrom>0x0f)
	{
		s[0]=CharH4(showpar);
		TextXY(s,x,y,c);
	}

	x+=8;
	s[0]=CharL4(showpar);
	TextXY(s,x,y,c);	

	return 1;
}

BOOL CInstruments::CursorGoto(int instr,CPoint point,int pzone)
{
	if (instr<0 || instr>=INSTRSNUM) return 0;
	TInstrument& tt=m_instr[instr];
	int x,y;
	switch(pzone)
	{
	case 0:
		//envelope velka tabulka
		g_activepart=PARTINSTRS;
		tt.act=2;	//aktivni je envelope
		x=point.x/8;
		if (x>=0 && x<=tt.par[PAR_ENVLEN]) tt.activeenvx=x;
		y=point.y/16+1;
		if (y>=1 && y<ENVROWS) tt.activeenvy=y;
		return 1;
	case 1:
		//envelope radek cisel hlasitosti praveho kanalu
		g_activepart=PARTINSTRS;
		tt.act=2;	//aktivni je envelope
		x=point.x/8;
		if (x>=0 && x<=tt.par[PAR_ENVLEN]) tt.activeenvx=x;
		tt.activeenvy=0;
		return 1;
	case 2:
		//TABLE
		g_activepart=PARTINSTRS;
		tt.act=3;	//aktivni je table
		x=(point.x+4)/(3*8);
		if (x>=0 && x<=tt.par[PAR_TABLEN]) tt.activetab=x;
		return 1;
	case 3:
		//INSTRUMENT NAME
		g_activepart=PARTINSTRS;
		tt.act=0;	//aktivni je name
		x=point.x/8-6;
		if (x>=0 && x<=INSTRNAMEMAXLEN) tt.activenam=x;
		if (x<0) tt.activenam=0;
		return 1;
	case 4:
		//INSTRUMENT PARAMETERS
	{
		x=point.x/8;
		y=point.y/16;
		if (x>11 && x<15) return 0; //stredni prazdna cast
		if (y<0 || y>12) return 0; //pro jistotu
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
			tt.act=1;	//aktivni je parameters
			return 1;
		}
	}
		return 0;

	case 5:
		//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
		//left mouse butt
		//meni GO a pripadne posouva LEN
		x=point.x/8;
		if (x<0) x=0; else if (x>=ENVCOLS) x=ENVCOLS-1;
		tt.par[PAR_ENVGO]=x;
		if (tt.par[PAR_ENVLEN]<x) tt.par[PAR_ENVLEN]=x;
CG_InstrumentParametersChanged:
		//protoze doslo k nejake zmene parametruuu instrumentu => stopne tento instrument na vsech generatorech
		Atari_InstrumentTurnOff(instr);
		CheckInstrumentParameters(instr);
		//neco se zmenilo => Ulozeni instrumentu "do Atarka"
		ModificationInstrument(instr);
		return 1;
	case 6:
		//INSTRUMENT SET ENVELOPE LEN/GO PARAMETER by MOUSE
		//right mouse butt
		//meni LEN a pripadne posouva GO
		x=point.x/8;
		if (x<0) x=0; else if (x>=ENVCOLS) x=ENVCOLS-1;
		tt.par[PAR_ENVLEN]=x;
		if (tt.par[PAR_ENVGO]>x) tt.par[PAR_ENVGO]=x;
		goto CG_InstrumentParametersChanged;
	case 7:
		//TABLE SET LEN/GO PARAMETER by MOUSE
		//left mouse butt
		//meni GO a pripadne posouva LEN
		x=(point.x+4)/(3*8);
		if (x<0) x=0; else if (x>=TABLEN) x=TABLEN-1;
		tt.par[PAR_TABGO]=x;
		if (tt.par[PAR_TABLEN]<x) tt.par[PAR_TABLEN]=x;
		goto CG_InstrumentParametersChanged;
	case 8:
		//TABLE SET LEN/GO PARAMETER by MOUSE
		//right mouse butt
		//meni LEN a pripadne posouva GO
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
	if (tt.par[PAR_TABTYPE]==0 )  //jen pro TABTYPE NOTES
	{
		int nsh=tt.tab[0];	//posun not podle table 0
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
	if (tt.par[PAR_TABTYPE]==0 )  //jen pro TABTYPE NOTES
	{
		int nsh=tt.tab[0];	//posun not podle table 0
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
		//hlasitostni krivka levy kanal (dolni)
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+4,INSTRS_EX+12*8+len*8,INSTRS_EY+3*16+4+4*16);
		return 1;
	case 1:
		//hlasitostni krivka pravy kanal (horni)
		if (g_tracks4_8<=4) return 0;
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY-2*16+4,INSTRS_EX+12*8+len*8,INSTRS_EY-2*16+4+4*16);
		return 1;
	case 2:
		//envelope oblast velka tabulka
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+0+5*16,INSTRS_EX+12*8+len*8,INSTRS_EY+3*16+0+5*16+7*16);
		return 1;
	case 3:
		//envelope oblast cisel hlasitosti pro pravy kanal
		if (g_tracks4_8<=4) return 0;
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY-2*16+0+4*16,INSTRS_EX+12*8+len*8,INSTRS_EY-2*16+0+4*16+16);
		return 1;
	case 4:
		//instrument table radek
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
		//envelope oblast pod levou (dolni) krivkou hlasitosti
		rect.SetRect(INSTRS_EX+12*8,INSTRS_EY+3*16+0+4*16,INSTRS_EX+12*8+ENVCOLS*8,INSTRS_EY+3*16+0+4*16+16);
		return 1;
	case 9:
		//instrument table + 1 radek pod table parametry
		rect.SetRect(INSTRS_TX,INSTRS_TY+8+1*16,INSTRS_TX+TABLEN*24-8,INSTRS_TY+8+2*16);
		return 1;
	}
	return 0;
}

BOOL CInstruments::DrawEnv(int e, int it)
{
	TInstrument& in = m_instr[it];
	int volR= in.env[e][ENV_VOLUMER] & 0x0f; //volume right
	int volL= in.env[e][ENV_VOLUMEL] & 0x0f; //volume left/mono
	int c;
	int x=INSTRS_EX+12*8+e*8;
	char s[2],a;
	s[1]=0;
	int ay= (in.act==2 && in.activeenvx==e)? in.activeenvy : -1;

	COLORREF color = (in.env[e][ENV_COMMAND]==0x07 && in.env[e][ENV_X]==0x08 && in.env[e][ENV_Y]==0x00) ?
		RGB(128,255,255) : RGB(255,255,255);


	//volume sloupec
	if (volL) g_mem_dc->FillSolidRect(x,INSTRS_EY+3*16+4+4*(15-volL),8,volL*4,color);
	if (g_tracks4_8>4 && volR) g_mem_dc->FillSolidRect(x,INSTRS_EY-2*16+4+4*(15-volR),8,volR*4,color);
	for(int j=0; j<8; j++)
	{
		if ( (a=shenv[j].ch)!=0 )
		{
			if (m_instr[it].env[e][j])
				s[0]=a;
			else
				s[0]=8;	//znak . v envelope
		}
		else
			s[0]=CharL4(m_instr[it].env[e][j]);
		c = (j==ay && !g_prove && g_activepart==PARTINSTRS)? COLOR_SELECTED : 0;	//selectovany znak?
		if (j==0)
		{
			if (g_tracks4_8>4) TextXY(s,x,INSTRS_EY+2*16,c);		 //volume R je mimo ostatni 
		}
		else
			TextXY(s,x,INSTRS_EY+7*16+j*16,c);
	}
	return 1;
}

BOOL CInstruments::DrawTab(int p,int it)
{
	TInstrument& in = m_instr[it];
	char s[4];
	//male cisilko
	s[0]=CharH4(p);
	s[1]=CharL4(p);
	s[2]=0;
	//TextMiniXY(s,INSTRS_TX+7*8+p*20,INSTRS_TY);
	TextMiniXY(s,INSTRS_TX+p*24,INSTRS_TY);
	//table parametr
	sprintf(s,"%02X",in.tab[p]);
	int c = (in.act==3 && in.activetab==p && g_activepart==PARTINSTRS)? COLOR_SELECTED : 0;	//selectovany znak?
	//TextXY(s,INSTRS_TX+7*8+p*20,INSTRS_TY+8,c);
	TextXY(s,INSTRS_TX+p*24,INSTRS_TY+8,c);
	return 1;
}

//----------------------------------------------

CTracks::CTracks()
{
	m_maxtracklen = 64;			//default hodnota
	g_cursoractview = 0;
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
			ou.write((char*) &at.len,sizeof(at.len));
			ou.write((char*) &at.go,sizeof(at.go));
			//
			//vsechno
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
			ou << "\n"; //mezera
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
			ClearTrack(track);	//promaze
			TTrack& at=m_track[track];
			in.read((char*) &at.len,sizeof(at.len));
			in.read((char*) &at.go,sizeof(at.go));
			//
			//vsechno
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
			in.getline(line,1024); //prvni radek tracku

			int ttr=Hexstr(line,2);
			int tlen=Hexstr(line+4,2);
			int tgo=Hexstr(line+6,2);

			if (track==-1) track = ttr;

			if (track<0 || track>=TRACKSNUM)
			{
				NextSegment(in);
				return 1;
			}

			ClearTrack(track);	//promaze
			TTrack& at=m_track[track];

			if (tlen<=0 || tlen>m_maxtracklen) tlen=m_maxtracklen;
			at.len=tlen;
			if (tgo>=tlen) tgo=-1;
			at.go=tgo;

			int idx=0;
			while(!in.eof())
			{
				in.read((char*)&b,1);
				if (b=='[') return 1;	//konec tracku (zacatek neceho dalsiho)
				if (b==10 || b==13)		//hned na zacatku radku je EOL
				{
					NextSegment(in);
					return 1;
				}

				memset(line,0,16);		//promazani
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

				if (at.note[idx]>=0 && at.instr[idx]<0) at.instr[idx]=0;	//pokud je nota bez instrumentu, pak tam prihodi instrument 0
				if (at.instr[idx]>=0 && at.note[idx]<0) at.instr[idx]=-1;	//pokud je instrument bez noty, pak ho zrusi
				if (at.note[idx]>=0 && at.volume[idx]<0) at.volume[idx]=MAXVOLUME;	//pokud je nota bez hlasitosti, prida maximalni hlasitost


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
			ou.write((char*) &m_maxtracklen,sizeof(m_maxtracklen));
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
				if (!CalculateNoEmpty(i)) continue;	//uklada jen neprazdne tracky
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

	in.read((char*) &m_maxtracklen,sizeof(m_maxtracklen));
	g_cursoractview = 0;

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
		{ WRITEATIDX(62 | (pause<<6)); }						\
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

		if (volume>=0 || speed>=0 || t.go==i) //neco tam bude, zapise prazdne takty predtim
		{
			if (pause>0)
			{
				WRITEPAUSE(pause);
				pause=0;
			}
			
			if (t.go==i) goidx=idx;	//sem se bude skakat go smyckou

			//speed je pred notami
			if (speed>=0)
			{
				//je speed
				WRITEATIDX(63);		//63 = zmena speed
				WRITEATIDX(speed & 0xff);

				/* CHYBA!!! - pause ma byt vzdycky = 0 !
				if (note>=0 || volume>=0)
					pause=0;
				else
					pause=1;	//v tomto radku je jen speed bez jine udalosti
				*/
				pause=0;
			}
		}
		
		//co to bude
		if (note>=0 && instr>=0 && volume>=0)
		{
			//nota,instr,vol
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
			//jen volume
			WRITEATIDX( ((volume & 0x03)<<6)
					|	61		//61 = empty note (nastavuje se pouze volume)
					 );
			WRITEATIDX( (volume & 0x0c)>>2 );	//bez instrumentu
			pause=0;
		}
		else
			pause++;
	}
	//konec smycky

	if (t.len<m_maxtracklen)	//track je kratsi nez maximalni delka
	{
		if (pause>0)	//zbyva pred koncem jeste nejaka pauza?
		{
			//tak tu pauzu zapise
			WRITEPAUSE(pause);
			pause=0;
		}
		
		if (t.go>=0 && goidx>=0)	//je tam go smycka?
		{
			//zapise go smycku
			WRITEATIDX( 0x80 | 63 );	//go povel
			WRITEATIDX( goidx );
		}
		else
		{
			//zapise konec
			WRITEATIDX( 255 );		//konec
		}
	}
	else
	{		//track je dlouhy jako maximalni delka
		if (pause>0)
		{
			WRITEPAUSE(pause);	//zapise je
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
		{ WRITEATIDX(62 | (pause<<6)); }						\
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

		if (volume>=0 || speed>=0 || t.go==i) //neco tam bude, zapise prazdne takty predtim
		{
			if (pause>0)
			{
				WRITEPAUSE(pause);
				pause=0;
			}
			
			if (t.go==i) goidx=idx;	//sem se bude skakat go smyckou

			//speed je pred notami
			if (speed>=0)
			{
				//je speed
				WRITEATIDX(63);		//63 = zmena speed
				WRITEATIDX(speed & 0xff);

				/* CHYBA!!! - pause ma byt vzdycky = 0 !
				if (note>=0 || volume>=0)
					pause=0;
				else
					pause=1;	//v tomto radku je jen speed bez jine udalosti
				*/
				pause=0;
			}
		}
		
		//co to bude
		if (note>=0 && instr>=0 && volume>=0)
		{
			//nota,instr,vol
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
			//jen volume
			WRITEATIDX( ((volume & 0x03)<<6)
					|	61		//61 = empty note (nastavuje se pouze volume)
					 );
			WRITEATIDX( (volume & 0x0c)>>2 );	//bez instrumentu
			pause=0;
		}
		else
			pause++;
	}
	//konec smycky

	if (t.len<m_maxtracklen)	//track je kratsi nez maximalni delka
	{
		if (t.go>=0 && goidx>=0)	//je tam go smycka?
		{
			if (pause>0)	//zbyva pred koncem jeste nejaka pauza?
			{
				//tak tu pauzu zapise
				WRITEPAUSE(pause);
				pause=0;
			}
			//zapise go smycku
			WRITEATIDX( 0x80 | 63 );	//go povel
			WRITEATIDX( goidx );
		}
		else
		{
			//zapise nekonecnou pauzu
			WRITEATIDX( 255 );		//RMF nekonecna pauza
		}
	}
	else
	{	//track je dlouhy jako maximalni delka
		if (pause>0)
		{
			WRITEATIDX( 255 );		//RMF nekonecna pauza
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
		//na konci tracku je go smycka
		if (sour[len-2]==128+63) goidx=sour[len-1];	//ulozi si jeji index
	}

	int line=0;
	int i=0;
	while(i<len)
	{
		if (i==goidx) t.go = line;		//sem skace goidx => nastavi go na tuto line

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
			c = sour[i] & 0xc0;		//nejvyssi 2 bity
			if (c==0)
			{	//jsou nulove
				if (sour[i+1]==0) break;			//nekonecna pauza =>konec
				line += sour[i+1];	//posun line
				i+=2;
			}
			else
			{	//jsou nenulove
				line += (c>>6);		//horni 2 bity primo urcuji pauzu 1-3
				i++;
			}
			continue;
		}
		else
		if (b==63)	//speed nebo gosmycka nebo end
		{
			c = sour[i] & 0xc0;		//11000000
			if (c==0)				//nejvyssi 2 bity jsou 0 ?  (00xxxxxx)
			{
				//speed
				t.speed[line]=sour[i+1];
				i+=2;
				//bez posunu line
				continue;
			}
			else
			if (c==0x80)			//nejvyssi bit=1?   (10xxxxxx)
			{
				//go smycka
				t.len = line; //tim padem je tady konec
				break;
			}
			else
			if (c==0xc0)			//nejvyssi dva bity=1?  (11xxxxxx)
			{
				//end
				t.len = line;
				break;
			}
		}
	}

	return 1;
}

BOOL CTracks::DrawTrack(int col,int x,int y,int tr,int line_cnt, int aline,int cactview, int pline,BOOL isactive,int acu)
{
	//cactview = cursor active view line
	char s[16];
	int i,line,xline,n,c,len,last,go;

	len=last=0;
	TTrack *tt=NULL;
	strcpy(s,"--  ----  ");				//strcpy(s,"--  ---- ‚");
	if (tr>=0)
	{
		tt=&m_track[tr];
		s[0]=CharH4(tr);
		s[1]=CharL4(tr);
		len=last=tt->len;	//len a last
		go=tt->go;	//go
		if (IsEmptyTrack(tr))
		{
			//prazdny track
			strncpy(s+3,"EMPTY",5);
		}
		else
		{
			//neprazdny track
			if (len>=0)
			{
				s[4]=CharH4(len);
				s[5]=CharL4(len);
			}
			if (go>=0)
			{
				s[6]=CharH4(go);
				s[7]=CharL4(go);
				last=m_maxtracklen;
			}
		}
	}
	c = (GetChannelOnOff(col))? 0 : 1;
	TextXY(s,x+8,y,c);
	y+=32;
	//--

	for(i=0; i<line_cnt; i++,y+=TRACK_LINE_H)
	{
		line = cactview + i;		//  -8 lines shora
		if (line<0 || line>=m_maxtracklen) continue;
		if (line>=last)
		{
			if (line == aline && isactive) c=6;	//cervena
			else
			if (line == pline) c=2;	//zluta
			else
				c=1;		//seda
			
			if (line==last && len>0)
			{
				if (c==1) c=0;	//end neni sedy, ale bily
				TextXY("\x12\x12\x13\x14\x15\x12\x12\x12",x+1*8,y,c);
			}
			else
				TextXY(".",x+5*8,y,c);
			continue;
		}
		strcpy(s," --- -- -  ");
		if (tt)
		{
			if (line<len || go<0) xline=line;
			else
				xline = ((line-len) % (len-go)) + go;

			if ( (n=tt->note[xline])>=0 )
			{ 
				//nota
				s[1]=notes[n][0];		// C
				s[2]=notes[n][1];		// #
				s[3]=notes[n][2];		// 1
			}
			if ( (n=tt->instr[xline])>=0 )
			{
				//instrument
				s[5]=CharH4(n);
				s[6]=CharL4(n);
			}
			if ( (n=tt->volume[xline])>=0 )
			{
				//volume
				s[8]=CharL4(n);
			}
			if ( (n=tt->speed[xline])>=0 )
			{
				//speed
				s[9]=CharH4(n);
				s[10]=CharL4(n);
			}
		}

		if (line==go)
		{
			if (line==len-1)
				s[0]='\x11';			//sipka doleva-nahoru-doprava
			else
				s[0]='\x0f';			//sipka nahoru-doprava
		}
		else
		if (line==len-1 && go>=0) s[0]='\x10'; //sipka doleva-nahoru

		//barvy
		if (line == aline && isactive) c=6;	//cervena
		else
		if (line == pline) c=2;	//zluta
		else
		if (line>=len) c=1;		//seda 
		else
		if ((line % g_tracklinehighlight) == 0) c=5;	//modra
		else
			c=0;				//bila

		if (!g_prove && g_activepart==PARTTRACKS && line<len && c==6)
			TextXYCol(s,x,y,colac[acu]);
		else
			TextXY(s,x,y,c);
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
	//if (note>NOTESNUM || track<0 || track>=TRACKSNUM || line>=m_track[track].len) return 0;
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
		//volume prepisuje jen je-li prazdne nebo chce-li ho zrusit (-1)
		if (m_track[track].volume[line]<0 || vol<0) 
				m_track[track].volume[line]=vol;
	}
	else
		m_track[track].volume[line]=vol;	//vzdycky
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
	return (track>=0 && track<TRACKSNUM)? m_track[track].len-1 : -1;	//m_maxtracklen-1;	//puvodne tu bylo jen ... : -1
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
	//proveri zda je track neprazdny
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& t = m_track[track];
	if (t.len != m_maxtracklen)	//ma nejakou jinou delku nez maximalni
		return 1;				//takze je neprazdny
	else
	{
		for(int i=0; i<t.len; i++)
		{
			if (	t.note[i]>=0			//nejaka nota, hlasitost nebo speed?
				||	t.volume[i]>=0
				||	t.speed[i]>=0 )
			{
				return 1;	//takze je neprazdny
			}
		}
	}
	return 0;	//je prazdny
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
			) return 0;		//nasel neco jineho
	}
	return 1;	//nenasel rozdil => jsou stejne
}

int CTracks::TrackOptimizeVol0(int track)
{
	//odstrani nadbytecne udaje volume 0
	if (track<0 || track>=TRACKSNUM) return 0;
	TTrack& tr = m_track[track];
	int lastzline=-1;
	//int lastznote=-1;
	int kline=-1;	//kandidat na smazani vcetne noty
	for(int i=0; i<tr.len; i++)
	{
		if (tr.volume[i]==0)
		{
			if (lastzline>=0)
			{
				if (kline>=0)	//nejaky kandidat na smazani? (nota+vol0 uprostred mezi nulovymi hlasitostmi)
				{
					tr.note[kline]=tr.instr[kline]=tr.volume[kline]=-1;
				}
				if (tr.note[i]<0 && tr.instr[i]<0) 
					tr.volume[i]=-1;	//zrusi tuto volume
				else
					kline=i;
			}
			else
			{
				//toto je momentalne posledni radek s nulovou hlasitosti
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
	if (IsEmptyTrack(track)) return 0;	//prazdny track
	if (tr.go>=0) return 0;		//uz tam loop je
	if (tr.len!=m_maxtracklen) return 0;	//neni to plna delka => nemuze tam vyrabet loop

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
				//podarilo se mu najit smycku dlouhou aspon 2 takty trvajici az do konce
				//proveri, jestli v te smycce neni jen prazdno
				int p=0;
				for(m=0; i+m<tr.len; m++)
				{
					if (   tr.note[j+m]>=0
						|| tr.instr[j+m]>=0
						|| tr.volume[j+m]>=0
						|| tr.speed[j+m]>=0 )
					{
						p++;
						if (p>1) //ano, nasel aspon dva nenulove radky uvnitr loopu
						{
							tr.len=i;
							tr.go=j;
							return k;	//vraci delku nalezeneho loopu
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
	if (IsEmptyTrack(track)) return 0;	//prazdny track
	int i = TrackExpandLoop(tr);
	return i;	//delka rozbaleneho loopu
}

int CTracks::TrackExpandLoop(TTrack* ttrack)
{
	if (!ttrack) return 0;
	TTrack& tr = *ttrack;
	if (tr.go<0) return 0;		//zadny loop tam neni

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
	tr.len=m_maxtracklen;	//plna delka
	tr.go=-1;				//zadny loop

	return i;	//delka rozbaleneho loopu
}

void CTracks::GetTracksAll(TTracksAll *dest_ta)
{
	dest_ta->maxtracklength=m_maxtracklen;
	memcpy(dest_ta->tracks,&m_track,sizeof(m_track));
	//{ return (TTracksAll*)&m_track; };
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

	ClearSong(8);	//defaultne je 8 tracku stereo

	//spusteni Timeru
	m_timer=0;
	g_timerroutineprocessed=0;
	g_song = this;

	m_quantization_note=-1; //init
	m_quantization_instr=-1;
	m_quantization_vol=-1;

	//Timer 1/50s = 20ms, 1/60s = 17ms
	//Pal nebo NTSC
	ChangeTimer((g_ntsc)? 17 : 20);
	//m_timer = timeSetEvent(20, 0, G_TimerRoutine,(ULONG) (NULL), TIME_PERIODIC);
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
	g_tracks4_8 = numoftracks;	//skladba pro 4/8 generatory
	g_rmtroutine = 1;			//zapnuto provadeni RMT rutiny
	g_prove=0;
	g_respectvolume=0;
	g_rmtstripped_adr_module = 0x4000;	//defaultni standardni adresa pro stripped RMT moduly
	g_rmtstripped_sfx = 0;		//standardne neni sfx odruda stripped RMT
	g_rmtstripped_gvf = 0;		//standardne neni GlobalVolumeFade Feat
	g_rmtmsxtext = "";			//vymaze text pro export MSX
	g_expasmlabelprefix = "MUSIC";	//defaultni label prefix pro export ASM simple notation

	PlayPressedTonesInit();

	//	
	m_play=MPLAY_STOP;
	g_playtime=0;
	m_followplay=1;
	m_mainspeed=m_speed=m_speeda=16;
	m_instrspeed=1;
	//
	g_activepart=g_active_ti=PARTTRACKS;	//tracks
	//
	m_songplayline = m_songactiveline = m_songtopline = 0;
	m_trackactiveline = m_trackplayline = 0;
	m_trackactivecol = m_trackactivecur = 0;
	m_activeinstr = 0;
	m_octave = 0;
	m_volume = MAXVOLUME;
	//
	ClearBookmark();
	//
	m_infoact=0;

	memset(m_songname,' ',SONGNAMEMAXLEN);
	strncpy(m_songname,"Noname song",11);
	m_songname[SONGNAMEMAXLEN]=0;

	m_songnamecur=0;
	//
	m_filename = "";
	//m_fileunsaved = 0; //neni unsaved
	m_filetype = 0;	//zadny
	m_exporttype = 0; //zadny
	//
	m_TracksOrderChange_songlinefrom = 0x00;
	m_TracksOrderChange_songlineto   = SONGLEN-1;

	//pocet radku po vlozeni noty/space
	g_linesafter=1; //pocatecni hodnota
	CMainFrame *mf = ((CMainFrame*)AfxGetMainWnd());
	if (mf) mf->m_c_linesafter.SetCurSel(g_linesafter);

	for(int i=0; i<SONGLEN; i++)
	{
		for(int j=0; j<SONGTRACKS; j++)
		{
			m_song[i][j]=-1;	//TRACK --
		}
		m_songgo[i]=-1;		//neni GO
	}

	//prazdne clipboardy
	g_trackcl.Init(this);
	g_trackcl.Empty();
	//
	m_instrclipboard.act = -1;			//podle -1 pozna ze je prazdny
	m_songgoclipboard = -2;				//podle -2 pozna ze je prazdny

	//a smaze vsechny tracky i instrumenty
	m_tracks.InitTracks();
	m_instrs.InitInstruments();

	//Inicializace Undo
	m_undo.Init(this);

	//Zmeny v modulu
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
	if (cmd==0x80) { cmd=0x90; pr2=0; }	//key off
	else
	if (cmd==0xf0)
	{
		if (mv[0]==0xff)
		{
			//System Reset
MIDISystemReset:
			Atari_InitRMTRoutine(); //reinit RMT rutiny
			for(int i=1; i<16; i++)	//az od 1, protoze se jedna o MULTITIMBRAL 2-16
			{
				g_midi_notech[i]=-1;	//posledne stlacene klavesy na jednotlivych kanalech
				g_midi_voluch[i]=0;		//hlasitosti
				g_midi_instch[i]=0;		//cisla instrumentu
			}
		}
		
		return; //KONEC
	}

	if (chn>0 && chn<10)
	{
		//2-10 kanal (chn=1-9) se uziva pro multitimbral L1-L4,R1-R4
		int atc=(chn-1)%g_tracks4_8;	//atari track 0-7 (resp. 0-3 v mono)
		int note,vol;
		if (cmd==0x90)
		{
			if (chn==9)
			{
				//chanel 10 (chn=9) ...drums channel
				if (pr2>0) //"note on" libovolnou nenulovou hlasitosti
				{
					//v planu
					//provede record vsech g_midi_....ch[0 az g_tracks4_8] do tracku
					//a posun record line o jedno niz

				}
			}
			else
			{
				//chanel 2-9 (chn=1-8)
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

		return; //KONEC
	}
	
	//nasledujici se provadi pouze na kanalu 0 (chn=0)

	if (!g_focus) return;	//kdyz nema focus, nezapisuje se

	if (cmd==0x90)
	{
		//key on/off
		int note = pr1-36;		//od 3.oktavy
		int vol;
		if (pr2==0)
		{
			if (!g_midi_noteoff) return;	//note off se nerozpoznava
			vol = 0;			//keyoff
		}
		else
		if (g_midi_tr)
		{
			vol = g_midi_volumeoffset + pr2/8;	//dynamika
			if (vol==0) vol++;		//vol=1
			else
			if (vol>15) vol=15;

			if (m_volume!=vol) g_screenupdate=1;
			m_volume = vol;
		}
		else
		{
			vol = m_volume;
		}

		if (note>=0 && note<NOTESNUM)		//jenom v mezich
		{
			/*
			CString s;
			s.Format("SHIFT= %i, CONTROL=%i",shift,control);
			SetStatusBarText(s);
			*/
			if (g_activepart!=PARTTRACKS || g_prove || g_shiftkey || g_controlkey) goto Prove_midi;

			if (vol>0)
			{
				//hlasitost>0 => zapsat notu
				//Quantization
				if ( m_play && m_followplay && (m_speeda<(m_speed/2)) )
				{
					m_quantization_note=note;
					m_quantization_instr=m_activeinstr;
					m_quantization_vol=vol;
					g_midi_notech[chn]=note; //viz nize
				} //konec Q
				else
				if (TrackSetNoteInstrVol(note,m_activeinstr,vol))
				{
					BLOCKDESELECT;
					g_midi_notech[chn]=note; //posledne stlacena klavesa na tomto midi kanalu
					if (g_respectvolume)
					{
						int v = TrackGetVol();
						if (v>=0 && v<=MAXVOLUME) vol=v;
					}
					goto NextLine_midi;
				}
			}
			else
			{
				//hlasitost=0 => noteOff => smazat notu a zapsat jen volume 0
				if (g_midi_notech[chn]==note) //je to opravdu ta posledne stlacena?
				{
					if (m_play && m_followplay && (m_speed<(m_speed/2)) )
					{
						m_quantization_note=-2;
					}
					else
					if (TrackSetNoteActualInstrVol(-1) && TrackSetVol(0))
						goto NextLine_midi;
				}
			}

			if (0) //dovnitr skace jen pres goto
			{
NextLine_midi:
				if (!(m_play && m_followplay)) TrackDown(g_linesafter);	//posouva jen kdyz neni followplay
				g_screenupdate=1;
				//SetPlayPressedTonesTNIV(m_trackactivecol,i,m_activeinstr,TrackGetVol());
Prove_midi:
				SetPlayPressedTonesTNIV(m_trackactivecol,note,m_activeinstr,vol);
				if ((g_prove==2 || g_controlkey) && g_tracks4_8>4)
				{	//s controlem nebo v prove2 => stereo test
					SetPlayPressedTonesTNIV((m_trackactivecol+4)&0x07,note,m_activeinstr,vol);
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
		if (apos+g_tracks4_8>max) return len;		//kdyby mel prelezt buffer

		if ( (go=m_songgo[sline])>=0)
		{
			//je tam go radek
			dest[apos]= 254;		//go povel
			dest[apos+1] = go;		//cislo kam skace
			WORD goadr = adr + (go*g_tracks4_8);
			dest[apos+2] = goadr & 0xff;	//dolni byte
			dest[apos+3] = (goadr>>8);		//horni byte
			if (g_tracks4_8>4)
			{
				for(int j=4; j<g_tracks4_8; j++) dest[apos+j]=255; //pro poradek
			}
			len = sline*g_tracks4_8 +4; //toto je prozatim konec (goto ma 4 byty i pro 8 tracku)
		}
		else
		{
			//jsou tam cisla tracku
			for(int i=0; i<g_tracks4_8; i++)
			{
				j = m_song[sline][i];
				if (j>=0 && j<TRACKSNUM) 
				{ 
					dest[apos+i]=j;
					len = (sline+1) * g_tracks4_8;		//toto je prozatim konec
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
		if (b==254 && col==0)		//go povel pouze v 0.tracku
		{
			//m_songgo[line]=sour[i+1];  //drive to bral podle cisla ve sloupci 1
			//ale dulezitejsi je go vektor, takze se to dela radej podle nej
			int ptr=sour[i+2]|(sour[i+3]<<8); //goto vektor
			int go=(ptr-adr)/g_tracks4_8;
			if (go>=0 && go<(len/g_tracks4_8) && go<SONGLEN)
				m_songgo[line]=go;
			else
				m_songgo[line]=0;	//misto neplatneho skoku da skok na radek 0
			i+= g_tracks4_8;
			if (i>=len)	return 1;		//konci to goto radkem
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
			if (line>=SONGLEN) return 1;	//aby nahodou nepretekl
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
		{	//nadpis RMT cisla verze a autora
			s1.LoadString(IDS_RMTVERSION);
			s2.LoadString(IDS_RMTAUTHOR);
			s.Format("%s, %s",s1,s2);
		}
	}
	else
	{
		s=m_filename;
		//if (m_fileunsaved) s+=" [Unsaved!]";
		if (g_changes) s+=" *";
	}
	AfxGetApp()->GetMainWnd()->SetWindowText(s);
}

int CSong::WarnUnsavedChanges()
{
	//vraci 1 kdyz se nema pokracovat v procesu
	if (!g_changes) return 0;
	int r=MessageBox(g_hwnd,"Save current changes?","Current song has been changed",MB_YESNOCANCEL|MB_ICONQUESTION);
	if (r==IDCANCEL) return 1;
	if (r==IDYES)
	{
		FileSave();
		SetRMTTitle();
		if (g_changes) return 1; //nepodarilo se ulozit nebo to stornoval
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
		FileOpen((LPCTSTR)filename,0); //bez Warningu na changes
	}
}

void CSong::FileOpen(const char *filename, BOOL warnunsavedchanges)
{
	//zastavi hudbu
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
		//jestli neda ok, tak konec
		if ( fid.DoModal() == IDOK )
		{
			fn = fid.GetPathName();
			type = fid.m_ofn.nFilterIndex;
		}
	}

	if ( (fn!="") && type) //pouze kdyz byl vybran soubor ve FileDialogu nebo byl zadan pri spusteni
	{
		//pouzije fn co vybral ve FileDialogu nebo co bylo zadano pri spusteni //fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn); //jen cesta

		if (type<1 || type>3) return;

		ifstream in(fn,ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		//vymaze nynejsi song
		ClearSong(g_tracks4_8);

		int result;
		switch (type)
		{
		case 1: //prvni volba v Dialogu (RMT)
			result=LoadRMT(in);
			m_filetype = IOTYPE_RMT;
			//g_tracks4_8 = 4;		//hack!!!!!!!! konverze na mono ;-)
			break;
		case 2: //druha volba v Dialogu (TXT)
			result=Load(in,IOTYPE_TXT);
			m_filetype = IOTYPE_TXT;
			break;
		case 3: //treti volba v Dialogu (RMW)
			result=Load(in,IOTYPE_RMW);
			m_filetype = IOTYPE_RMW;
			break;
		}
		if (!result)
		{
			//neco v Load funkci selhalo
			ClearSong(g_tracks4_8);		//vymaze vsechno
			SetRMTTitle();
			g_screenupdate = 1;	//musi to zobrazit
			return;
		}

		in.close();
		m_filename = fn;
		
		//init speedu
		m_speed = m_mainspeed;

		//nazev do okna
		//AfxGetApp()->GetMainWnd()->SetWindowText(m_filename);
		SetRMTTitle();

		//vsechny generatory ON (unmute all)
		SetChannelOnOff(-1,1);		//-1=vsechny,1=zapnout

		if (m_instrspeed>0x04)
		{
			int r=MessageBox(g_hwnd,"Hi music cracker! ;-)\nInstrument speed greater than 4 is unsupported officially.\nIt can cause various problems with RMT players, insufficient CPU 6502 power, etc.\nAre you sure, that you want keep this nonstandard instrument speed?","WARNING",MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
			if (r!=IDYES) m_instrspeed=0x04;
		}

		g_screenupdate = 1;
	}
}

void CSong::FileSave()
{
	//zastavi hudbu
	Stop();

	if (m_filename=="" || m_filetype==0) 
	{
		FileSaveAs();
		return;
	}

	if (m_filetype==IOTYPE_RMT && !TestBeforeFileSave())
	{
		MessageBox(g_hwnd,"Warning!\nNo data has been saved!","Warning",MB_ICONEXCLAMATION);
		//m_fileunsaved=1;
		//AfxGetApp()->GetMainWnd()->SetWindowText(m_filename + " [Unsaved!]");
		SetRMTTitle();
		return;
	}
	/*
	else
		m_fileunsaved=0;
	*/
	
	ofstream out(m_filename, (m_filetype == IOTYPE_TXT)?ios::out:ios::binary);
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
//		out.setmode(filebuf::text);
		r = Save(out,IOTYPE_TXT);
		break;
	case IOTYPE_RMW: //RMW
		//zapamatuje si aktualni octave a volume pro aktivni instrument (kvuli ukladani do RMW)
		//(protoze to se uklada jen pri zmene instrumentu a mohl zmenit oktavu nebo volume
		// pred ulozenim bez nasledne zmeny aktualniho instrumentu)
		m_instrs.MemorizeOctaveAndVolume(m_activeinstr,m_octave,m_volume);
		// a ted ulozi:
		r = Save(out,IOTYPE_RMW);
		break;
	}

	if (!r) //nepovedlo se ulozeni
	{
		out.close();
		DeleteFile(m_filename);
		MessageBox(g_hwnd,"RMT save aborted.\nWarning!\nDestination file was deleted!","Save aborted",MB_ICONEXCLAMATION);
	}
	else
	{
		//povedlo se
		g_changes=0;	//zmeny jsou ulozeny
	}

	//AfxGetApp()->GetMainWnd()->SetWindowText(m_filename);
	SetRMTTitle();

	//zavirani jen kdyz je "out" otevren (protoze v IOTYPE_RMT si ho muze zavrit jiz driv)
	if (out.is_open()) out.close();
}

void CSong::FileSaveAs()
{
	//zastavi hudbu
	Stop();
	
	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"RMT song file (*.rmt)|*.rmt|TXT song files (*.txt)|*.txt|RMW song work file (*.rmw)|*.rmw||");
	fod.m_ofn.lpstrTitle = "Save song as...";
	//if (m_filename!="") fod.m_ofn.

	if (g_lastloadpath_songs!="")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fod.m_ofn.lpstrInitialDir = g_path_songs;

	//predchysta nazev souboru podle posledne ulozeneho
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
			fod.m_ofn.nMaxFile = 1020;	//o 4 byty mene, zichr je zichr ;-)
		}
	}

	//predchysta typ podle posledne ulozeneho
	if (m_filetype==IOTYPE_RMT) fod.m_ofn.nFilterIndex = 1;
	if (m_filetype==IOTYPE_TXT) fod.m_ofn.nFilterIndex = 2;
	if (m_filetype==IOTYPE_RMW) fod.m_ofn.nFilterIndex = 3;
	
	//jestli neda ok, tak konec
	if ( fod.DoModal() == IDOK )
	{
		int type = fod.m_ofn.nFilterIndex;
		if (type<1 || type>3) return;

		m_filename = fod.GetPathName();
		const char* exttype[]={".rmt",".txt",".rmw"};
		CString ext=m_filename.Right(4);
		ext.MakeLower();
		if (ext!=exttype[type-1]) m_filename += exttype[type-1];

		g_lastloadpath_songs = GetFilePath(m_filename); //jen cesta

		switch (type)
		{
		case 1: //prvni volba
			m_filetype = IOTYPE_RMT;
			break;
		case 2: //druha volba
			m_filetype = IOTYPE_TXT;
			break;
		case 3: //treti volba
			m_filetype = IOTYPE_RMW;
			break;
		default:
			return;
		}

		FileSave();
	}
}

void CSong::FileNew()
{
	//zastavi hudbu
	Stop();

	if (WarnUnsavedChanges()) return;

	//
	CFileNewDlg dlg;
	if (dlg.DoModal() == IDOK )
	{
		m_tracks.m_maxtracklen = dlg.m_maxtracklen;
		g_cursoractview = 0;

		int i = dlg.m_combotype;
		g_tracks4_8 = (i==0)? 4 : 8;
		ClearSong(g_tracks4_8);
		SetRMTTitle();

		//automaticky predchystany
		//nulty radek songu
		for(i=0; i<g_tracks4_8; i++) m_song[0][i] = i;
		m_songgo[1] = 0;	//a goto v prvnim radku songu

		//vsechny generatory ON (unmute all)
		SetChannelOnOff(-1,1);		//-1=vsechny,1=zapnout

		//uklidi undo
		m_undo.Clear();

		g_screenupdate = 1;
	}
}


int l_lastimporttypeidx=-1;		//aby pri dalsim importu mel predvybrany ten typ co importoval posledne

void CSong::FileImport()
{
	//zastavi hudbu
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

	//jestli neda ok, tak konec
	if ( fid.DoModal() == IDOK )
	{
		Stop();

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn);	//jen cesta

		int type = fid.m_ofn.nFilterIndex;
		if (type<1 || type>2) return;

		l_lastimporttypeidx = type;

		ifstream in(fn,ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		int r=0;

		switch (type)
		{
		case 1: //prvni volba v Dialogu (MOD)

			r = ImportMOD(in);

			break;
		case 2: //druha volba v Dialogu (TMC)

			r = ImportTMC(in);

			break;
		case 3: //treti volba v Dialogu
			break;
		}

		in.close();
		m_filename = "";
		
		if (!r)	//import se nepovedl?
			ClearSong(g_tracks4_8);			//smaze vsechno
		else
		{
			//init speedu
			m_speed = m_mainspeed;

			//nazev do okna
			AfxGetApp()->GetMainWnd()->SetWindowText("Imported "+fn);
			//SetRMTTitle();
		}
		//vsechny generatory ON (unmute all)
		SetChannelOnOff(-1,1);		//-1=vsechny,1=zapnout

		g_screenupdate = 1;
	}
}


void CSong::FileExportAs()
{
	//zastavi hudbu
	Stop();

	if (!TestBeforeFileSave())
	{
		MessageBox(g_hwnd,"Warning!\nNo data has been saved!","Warning",MB_ICONEXCLAMATION);
		return;
	}

	CFileDialog fod(FALSE, 
					NULL,
					NULL,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					"RMT stripped song file (*.rmt)|*.rmt|SAP file (*.sap)|*.sap|XEX Atari executable msx (*.xex)|*.xex|ASM simple notation source (*.asm)|*.asm||"); //RMF special faster songdata format (*.rmf)|*.rmf||");
	fod.m_ofn.lpstrTitle = "Export song as...";

	if (g_lastloadpath_songs!="")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
	if (g_path_songs!="") fod.m_ofn.lpstrInitialDir = g_path_songs;

	if (m_exporttype==IOTYPE_RMTSTRIPPED) fod.m_ofn.nFilterIndex = 1;
	if (m_exporttype==IOTYPE_SAP) fod.m_ofn.nFilterIndex = 2;
	if (m_exporttype==IOTYPE_XEX) fod.m_ofn.nFilterIndex = 3;
	if (m_exporttype==IOTYPE_ASM) fod.m_ofn.nFilterIndex = 4;
	if (m_exporttype==IOTYPE_RMF) fod.m_ofn.nFilterIndex = 5;
	
	//jestli neda ok, tak konec
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		int type = fod.m_ofn.nFilterIndex;
		if (type<1 || type>5) return;
		const char* exttype[]={".rmt",".sap",".xex",".asm",".rmf"};
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext!=exttype[type-1]) fn += exttype[type-1];

		g_lastloadpath_songs = GetFilePath(fn); //jen cesta

		ofstream out(fn,ios::binary);
		if (!out)
		{
			MessageBox(g_hwnd,"Can't create this file: " +fn,"Write error",MB_ICONERROR);
			return;
		}

		int r;
		switch (type)
		{
		case 1: //RMTOPT
			r = Export(out,IOTYPE_RMTSTRIPPED,(char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_RMTSTRIPPED;
			break;
		case 2: //SAP
			if (m_instrspeed>0x04) //nemuze udelat SAP rychlejsi nez 4 tps
			{
				MessageBox(g_hwnd,"Sorry, but instrument speed greater than 4 is unsupported officially.\nIt isn't possible to make SAP file.","Error",MB_ICONERROR);
				r=0;
			}
			else
				r = Export(out,IOTYPE_SAP);
			m_exporttype = IOTYPE_SAP;
			break;
		case 3: //XEX
			//MessageBox(g_hwnd,"Not implemeted yet.","Sorry",MB_ICONINFORMATION);
			if (m_instrspeed>0x04) //XEX nepodporuje rychlost vetsi nez 4 tps
			{
				MessageBox(g_hwnd,"Sorry, but standard player routine unsupport instrument speed greater than 4.\nIt isn't possible to make XEX file.","Error",MB_ICONERROR);
				r=0;
			}
			else
				r = Export(out,IOTYPE_XEX);
			m_exporttype = IOTYPE_XEX;
			break;
		case 4: //ASM
			r = Export(out,IOTYPE_ASM,(char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_ASM;
			break;
		case 5: //RMF
			r = Export(out,IOTYPE_RMF,(char*)(LPCTSTR)fn);
			m_exporttype = IOTYPE_RMF;
			break;
		}

		out.close();

		if (!r)
		{
			DeleteFile(fn);
			MessageBox(g_hwnd,"Export aborted.\nWarning: Export file was deleted!","Export aborted",MB_ICONEXCLAMATION);
		}

	}
}

void CSong::FileInstrumentSave()
{
	//zastavi hudbu
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

	//jestli neda ok, tak konec
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext!=".rti") fn += ".rti";

		g_lastloadpath_instruments=GetFilePath(fn);	//jen cesta

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
	//zastavi hudbu
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
	
	//jestli neda ok, tak konec
	if ( fid.DoModal() == IDOK )
	{
		m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA,1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_instruments=GetFilePath(fn);	//jen cesta

		ifstream in(fn,ios::binary);
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

	//zastavi hudbu
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

	//jestli neda ok, tak konec
	if ( fod.DoModal() == IDOK )
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext=fn.Right(4);
		ext.MakeLower();
		if (ext!=".txt") fn += ".txt";

		g_lastloadpath_tracks=GetFilePath(fn);	//jen cesta

		ofstream ou(fn);	//default je text mode
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

	//zastavi hudbu
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
	
	//jestli neda ok, tak konec
	if ( fid.DoModal() == IDOK )
	{
		m_undo.ChangeTrack(0,0,UETYPE_TRACKSALL,1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_tracks=GetFilePath(fn);	//jen cesta
		ifstream in(fn);	//default je text mode
		if (!in)
		{
			MessageBox(g_hwnd,"Can't open this file: " +fn,"Open error",MB_ICONERROR);
			return;
		}

		char line[1025];
		int nt=0;		//pocet tracku
		int type=0;		//typ pri nacitani vice tracku
		while(NextSegment(in)) //bude tedy hledat zacatek dalsiho segmentu "["
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

		//in.close();
		//in.open(fn);
		in.seekg(0);	//znovu na zacatek
		in.clear();		//vynuluje priznak ze byl na konci
		int nr=0;
		NextSegment(in);	//posune za prvni "["
		while(!in.eof())
		{
			in.getline(line,1024);
			Trimstr(line);
			if (strcmp(line,"TRACK]")==0)
			{
				int tt = (type==0)? track : -1;
				if ( m_tracks.LoadTrack(tt,in,IOTYPE_TXT) )
				{
					nr++;	//pocet nactenych tracku
					if (type==0)
					{
						track++;	//posun o 1 pro nacteni dalsiho
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
				NextSegment(in);	//posune za dalsi "["
		}
		in.close();

		if (nr==0 || nr>1) //pokud nenacetl zadny nebo vice nez 1
		{
			CString s;
			s.Format("%i track(s) loaded.",nr);
			MessageBox(g_hwnd,s,"Track(s) loading finished.",MB_ICONINFORMATION);
		}
	}
}

#define RMWMAINPARAMSCOUNT		30		//celkove se v RMP saveuje 30 parametru
#define DEFINE_MAINPARAMS int* mainparams[RMWMAINPARAMSCOUNT]= {		\
	&g_tracks4_8,												\
	(int*)&m_speed,(int*)&m_mainspeed,(int*)&m_instrspeed,		\
	(int*)&m_songactiveline,(int*)&m_songplayline,				\
	(int*)&m_trackactiveline,(int*)&m_trackplayline,			\
	(int*)&g_activepart,(int*)&g_active_ti,						\
	(int*)&g_prove,(int*)&g_respectvolume,						\
	&g_tracklinehighlight,										\
	&g_tracklinealtnumbering,									\
	&g_trackcursorverticalrange,										\
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
	&m_infoact,&m_songnamecur,									\
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
		ou.write((char*)m_songname,sizeof(m_songname));
		//
		DEFINE_MAINPARAMS;
		
		int p = RMWMAINPARAMSCOUNT; //pocet ukladanych parametru
		ou.write((char*) &p,sizeof(p));		//zapise pocet main parametru
		for(int i=0; i<p; i++)
			ou.write((char*) mainparams[i],sizeof(mainparams[0]));
		//
		
		//zapise komplet song a songgo
		ou.write((char*)m_song,sizeof(m_song));
		ou.write((char*)m_songgo,sizeof(m_songgo));
		}
		break;

	case IOTYPE_TXT:
		{
		CString s,nambf;
		char bf[16];
		nambf=m_songname;
		nambf.TrimRight();
		s.Format("[MODULE]\nRMT: %X\nNAME: %s\nMAXTRACKLEN: %02X\nMAINSPEED: %02X\nINSTRSPEED: %X\nVERSION: %02X\n",g_tracks4_8,(LPCTSTR)nambf,m_tracks.m_maxtracklen,m_mainspeed,m_instrspeed,RMTFORMATVERSION);
		ou << s << "\n"; //mezera
		ou << "[SONG]\n";
		int i,j;
		//hledani delky songu
		int lens=-1;
		for(i=0; i<SONGLEN; i++)
		{
			if (m_songgo[i]>=0) { lens=i; continue; }
			for(j=0; j<g_tracks4_8; j++)
			{
				if (m_song[i][j]>=0 && m_song[i][j]<TRACKSNUM) { lens=i; break; }
			}
		}

		//zapis songu
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
					ou << "\n";			//za poslednim konec radku
				else
					ou << " ";			//mezi nima mezera
			}
		}

		ou << "\n"; //mezera
		}
		break;
	}

	m_instrs.SaveAll(ou,iotype);
	m_tracks.SaveAll(ou,iotype);

	return 1;
}


int CSong::Load(ifstream& in,int iotype)
{
	ClearSong(8);	//je predchystany na 8

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
	in.read((char*)m_songname,sizeof(m_songname));
	//
	DEFINE_MAINPARAMS;
	int p=0;
	in.read((char*)&p,sizeof(p));			//precte si pocet main parametru
	for(int i=0; i<p; i++)
		in.read((char*) mainparams[i],sizeof(mainparams[0]));
	//

	//precte komplet song a songgo
	in.read((char*)m_song,sizeof(m_song));
	in.read((char*)m_songgo,sizeof(m_songgo));

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
		//cteni po prvni "[" (vcetne)
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
						value[1]=0;	//preplacne mezeru
						value+=2;	//posune se na prvni znak za mezeru
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
						g_cursoractview = 0;
						m_tracks.InitTracks();				//reinicializace
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
						//cislo verze zatim neni u TXT na nic potreba,
						//protoze to si vybira jen parametry ktere zna
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
				m_instrs.LoadInstrument(-1,in,iotype); //-1 => prebere si cislo instrumentu z TXT zdroje
			}
			else
			if (strcmp(line,"TRACK]")==0)
			{
				m_tracks.LoadTrack(-1,in,iotype);	//-1 => prebere si cislo tracku z TXT zdroje
			}
			else
				NextSegment(in); //bude tedy hledat zacatek dalsiho segmentu
		}
	}

	return 1;
}

int CSong::TestBeforeFileSave()
{
	//provadi se u Exportu (vsechno krom RMW) jeste pred tim, nez se provede prepsani ciloveho souboru
	//takze pokud vrati 0, export se ukonci a k prepsani souboru nedojde.

	//Zkusi vytvorit modul
	unsigned char mem[65536];
	int adr_module=0x4000;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];
	int maxadr;
	
	//zkusi udelat jen tak naprazdno RMT modul
	maxadr = MakeModule(mem,adr_module,IOTYPE_RMT,instrsaved,tracksaved);
	if (maxadr<0) return 0;	//pokud se nepodarilo vytvorit modul

	//A ted se bude hlidat, zda je song ukoncen GOTO radkem
	//a zda tam neni GOTO na GOTO radek.
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
					last=i; //tracky
					tr++;
					break;
				}
			}
		}
	}

	//if (tr<1) wrnmsg += "Warning: No tracks are placed into song.\n";

	if (last<0)
	{
		errmsg += "Error: Song is empty.\n";
	}

	for(i=0; i<=last; i++)
	{
		if (m_songgo[i]>=0)
		{
			//je tam GO radek
			go = m_songgo[i];	//kam skace
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
			//jsou tam tracky nebo volno
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
		s.Format("Error: Song line [%02X]: Unexpected end of song. You have to use \"go to line\" at the foot of song.\n",last+1);
		errmsg += s;
	}

	//vysledky

	if (errmsg!="" || wrnmsg!="")
	{
		if (errmsg=="")
		{
			//jsou tam jen warningy
			wrnmsg += "\nIgnore this warning(s) and save anyway?";
			r = MessageBox(g_hwnd,wrnmsg,"Warnings",MB_YESNO | MB_ICONQUESTION);
			if (r==IDYES) return 1;
			return 0;
		}
		//jsou tam i errory
		MessageBox(g_hwnd,errmsg + wrnmsg,"Errors",MB_ICONERROR);
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
	ok=0;	//jestli v danem subsongu nalezl nejake nenulove tracky (ruzne od --)

	for(i=0; i<=lastgo; i++)
	{
		if (songp[i]<0)
		{
			apos=i;
			while(songp[apos]<0)
			{
				n = m_songgo[apos];
				songp[apos] = asub;
				if (n>=0) //skace na jiny radek
					apos=n;
				else
				{
					if (!ok)
					{	//nenalezl zatim zadne tracky v tomto subsongu
						for(j=0; j<g_tracks4_8; j++)
						{
							if (m_song[apos][j]>=0)
							{	//uz nasel - tim padem toto bude zacatek subsongu
								s.Format("%02X ",apos);
								resultstr+=s;
								ok=1;		//zacatek tohoto subsongu je jiz zapsan
								break;
							}
						}
					}
					apos++;
					if (apos>=SONGLEN) break;
				}
			}
			if (ok) asub++;	//posune se na dalsi jen jestlize subsong vubec neco obsahuje
			ok=0; //inicializace pro dalsi hledani
		}
	}
	return asub;
}

#define EOL "\x0d\x0a"

#include "res/data_rmt_msxsap4sap8_sys.cpp"

int CSong::Export(ofstream& ou,int iotype, char* filename)
{
	unsigned char mem[65536];

	int adr_init= (g_tracks4_8==4)? 0x394e : 0x3a5a;	//u sap4 je kratsi rutina nez u sap8
	int adr_subsongs = adr_init+11; //init rutina ma 11 bytu a za ni je tabulka pro subsongy
	int adr_player=0x3403;
	int adr_module=0x4000;		//standardni RMT jsou ukladany na $4000
	int adr_msxrunadr=0x3e00;
	int adr_msxvideo =0x3f00;
	int adr_msxcolor =adr_msxvideo+200;
	int adr_msxvcaddition = adr_msxvideo+201;
	int maxadr=adr_module;
	int i;

	WORD adrfrom,adrto;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];

	BOOL head_ffff=1;		//hlavicka FF FF na zacatku souboru

	//vytvori modul
	memset(mem,0,65536);
	maxadr = MakeModule(mem,adr_module,iotype,instrsaved,tracksaved);
	if (maxadr<0) return 0;	//pokud se nepodarilo vytvorit modul

	CString s;
	switch(iotype)
	{
	case IOTYPE_RMT:
		break;

	case IOTYPE_RMTSTRIPPED:
	case IOTYPE_RMF:			//temer stejny princip jako pro RMT STRIPPED
		{
			//vytvori si variantu pro SFX (tj. vcetne nepouzitych instrumentu a tracku)
			BYTE instrsaved2[INSTRSNUM];
			BYTE tracksaved2[TRACKSNUM];
			int maxadr2 = MakeModule(mem,adr_module,IOTYPE_RMT,instrsaved2,tracksaved2);
			if (maxadr2<0) return 0;	//pokud se nepodarilo vytvorit modul
			
			//Dialog pro urceni adresy RMT modulu v pameti
			CExpRMTDlg dlg;
			dlg.m_len = maxadr-adr_module;
			dlg.m_len2 = maxadr2-adr_module;

			//if (g_rmtstripped_adr_module+dlg.m_len>0x10000) g_rmtstripped_adr_module = 0x10000-dlg.m_len;
			dlg.m_adr = g_rmtstripped_adr_module;	//globalni, aby pri opakovanem exportu zustavalo zachovano v nabidce
			//RMT FEATures definitions
			//ComposeRMTFEATstring(s,filename,instrsaved,tracksaved,0);
			//dlg.m_rmtfeat = s;
			//CString s2;
			//ComposeRMTFEATstring(s2,filename,instrsaved2,tracksaved2,1);
			//dlg.m_rmtfeat2 = s2;
			dlg.m_sfx = g_rmtstripped_sfx;
			dlg.m_gvf = g_rmtstripped_gvf;
			dlg.m_nos = g_rmtstripped_nos;

			dlg.m_song=this;
			dlg.m_filename=filename;
			dlg.m_instrsaved=instrsaved;
			dlg.m_instrsaved2=instrsaved2;
			dlg.m_tracksaved=tracksaved;
			dlg.m_tracksaved2=tracksaved2;
			//
			if (dlg.DoModal() != IDOK)
			{
				return 0;	
			}

			g_rmtstripped_adr_module = adr_module = dlg.m_adr;
			g_rmtstripped_sfx = dlg.m_sfx;
			g_rmtstripped_gvf = dlg.m_gvf;
			g_rmtstripped_nos = dlg.m_nos;
			//znovu vygeneruje modul podle zadane adresy "adr"
			memset(mem,0,65536);
			if (!g_rmtstripped_sfx) //bud bez nepouzitych instrumentu a tracku
				maxadr = MakeModule(mem,adr_module,iotype,instrsaved,tracksaved);
			else //nebo s nimi
				maxadr = MakeModule(mem,adr_module,IOTYPE_RMT,instrsaved,tracksaved);
			if (maxadr<0) return 0; //nepodarilo-li se vytvorit modul
		}
		break;

	case IOTYPE_SAP:
		{
			CExpSAPDlg dlg;
			//
			s = m_songname;
			s.TrimRight();		//oreze mezery za nazvem
			s.Replace('"','\''); //nahradi uvozovky apostrofem
			dlg.m_name = s;
			//
			dlg.m_author = "???";
			GetSubsongParts(dlg.m_subsongs);
			//
			CTime time = CTime::GetCurrentTime();
			dlg.m_date = time.Format("%d/%m/%Y");
			//
			if (dlg.DoModal() != IDOK)
			{
				return 0;	
			}
			//
			//s.Format("%srmt_sap%i.sys",g_prgpath,g_tracks4_8);	//../rmt_sap4.sys nebo /rmt_sap8.sys
			//int r = LoadBinaryFile((char*)(LPCSTR)s,mem,adrfrom,adrto);
			int r;
			if (g_tracks4_8==4)
				r=LoadDataAsBinaryFile(data_rmt_sap4_sys,sizeof(data_rmt_sap4_sys),mem,adrfrom,adrto);
			else
				r=LoadDataAsBinaryFile(data_rmt_sap8_sys,sizeof(data_rmt_sap8_sys),mem,adrfrom,adrto);
			if (!r)
			{
				//MessageBox(g_hwnd,"Can't open RMT SAP system routines.","Export aborted",MB_ICONERROR);
				MessageBox(g_hwnd,"Fatal error with RMT SAP system routines.","Export aborted",MB_ICONERROR);
				return 0;	
			}
			ou << "SAP" << EOL;
			//
			s = dlg.m_author;
			s.TrimRight();		//oreze mezery za nazvem
			s.Replace('"','\''); //nahradi uvozovky apostrofem
			ou << "AUTHOR \"" << s << "\"" << EOL;
			//
			s = dlg.m_name;
			s.TrimRight();		//oreze mezery za nazvem
			s.Replace('"','\''); //nahradi uvozovky apostrofem
			ou << "NAME \"" << s << "\"" << EOL;
			//
			s = dlg.m_date;
			s.TrimRight();		//oreze mezery za nazvem
			s.Replace('"','\''); //nahradi uvozovky apostrofem
			ou << "DATE \"" << s << "\"" << EOL;
			//
			s = dlg.m_subsongs + " ";	//mezera za poslednim znakem kvuli parsovani
			s.MakeUpper();
			int subsongs=0;
			BYTE subpos[MAXSUBSONGS];
			subpos[0]=0; //defaultne zacina na 0. songline
			BYTE a,n,isn=0;
			for(i=0; i<s.GetLength(); i++) //parsuje radek "Subsongs" z ExportSAP dialogu
			{
				a=s.GetAt(i);
				if (a>='0' && a<='9') {	n=(n<<4)+(a-'0'); isn=1; }
				else
				if (a>='A' && a<='F') {	n=(n<<4)+(a-'A'+10); isn=1;	}
				else
				{
					if (isn)
					{
						subpos[subsongs]=n;
						subsongs++;
						if (subsongs>=MAXSUBSONGS) break;
						isn=0;
					}
				}
			}
			if (subsongs>1)
			{
				ou << "SONGS " << subsongs << EOL;
				//a zapise do pameti za SAP init rutinu tabulku zacatku subsonguu
				for(i=0; i<subsongs; i++) mem[adr_subsongs+i]=subpos[i];
				adrto+=subsongs;	//posune konec bloku co se uklada
			}
			else
			{
				//prepise zacatek SAP init rutiny
				//ze    tax   lda subsongs,x	  (4 bytes)
				//na	nop nop lda #defaultpos   (4 bytes)
				mem[adr_init]=0xEA;		//NOP
				mem[adr_init+1]=0xEA;	//NOP
				mem[adr_init+2]=0xA9;	//LDA #
				mem[adr_init+3]=subpos[0];	//prvni cislo je defaultni
				adr_init+=2;	//posune init za ty dva NOPy
				//tim padem neni nutno zapisovat nic na adr_subsongs a prodluzovat modul
			}
			//
			ou << "TYPE B" << EOL;
			s.Format("INIT %04X", adr_init);
			ou << s << EOL;
			s.Format("PLAYER %04X", adr_player);
			ou << s << EOL;
			if (m_instrspeed>1)
			{	//zrychlene instrumenty
				if (m_instrspeed==2) ou << "FASTPLAY 156" << EOL;
				else
				if (m_instrspeed==3) ou << "FASTPLAY 104" << EOL;
				else
				if (m_instrspeed==4) ou << "FASTPLAY 78" << EOL;
			}
			if (g_tracks4_8>4)
			{	//stereo modul
				ou << "STEREO" << EOL;
			}
			SaveBinaryBlock(ou,mem,adrfrom,adrto,1);
			head_ffff=0;
		}
		break;

	case IOTYPE_XEX:
		{
			CExpMSXDlg dlg;
			s = m_songname;
			s.TrimRight();
			CTime time = CTime::GetCurrentTime();

			if (g_rmtmsxtext!="")
			{
				dlg.m_txt = g_rmtmsxtext;	//necha ten z minula
			}
			else
			{
				dlg.m_txt = s + "\x0d\x0a";
				if (g_tracks4_8>4) dlg.m_txt += "STEREO";
				dlg.m_txt += "\x0d\x0a" + time.Format("%d/%m/%Y");
				dlg.m_txt += "\x0d\x0a";
				dlg.m_txt += "Author: (press SHIFT key)\x0d\x0a";
				dlg.m_txt += "Author: ???";
			}
			//dlg.m_meter = 1;

			s="Playing speed will be adjusted to ";
			s+= g_ntsc ? "60" : "50";
			s+="Hz on PAL and also on NTSC systems.";
			dlg.m_speedinfo=s;

			//
			if (dlg.DoModal() != IDOK)
			{
				return 0;	
			}
		
			g_rmtmsxtext = dlg.m_txt;
			g_rmtmsxtext.Replace("\x0d\x0d","\x0d");	//13, 13 => 13

			//
			//s.Format("%srmt_sap%i.sys",g_prgpath,g_tracks4_8);	//../rmt_sap4.sys nebo /rmt_sap8.sys
			//int r = LoadBinaryFile((char*)(LPCSTR)s,mem,adrfrom,adrto);
			int r;
			if (g_tracks4_8==4)
				r=LoadDataAsBinaryFile(data_rmt_sap4_sys,sizeof(data_rmt_sap4_sys),mem,adrfrom,adrto);
			else
				r=LoadDataAsBinaryFile(data_rmt_sap8_sys,sizeof(data_rmt_sap8_sys),mem,adrfrom,adrto);
			if (!r)
			{
				//MessageBox(g_hwnd,"Can't open RMT SAP system routines.","Export aborted",MB_ICONERROR);
				MessageBox(g_hwnd,"Fatal error with RMT SAP system routines.","Export aborted",MB_ICONERROR);
				return 0;	
			}
			SaveBinaryBlock(ou,mem,adrfrom,adrto,1);
			head_ffff=0;

			//r = LoadBinaryFile((char*)((LPCSTR)(g_prgpath+"rmt_msx.sys")),mem,adrfrom,adrto); //rmt_msx.sys
			r=LoadDataAsBinaryFile(data_rmt_msx_sys,sizeof(data_rmt_msx_sys),mem,adrfrom,adrto);
			if (!r)
			{
				//MessageBox(g_hwnd,"Can't open RMT MSX system routines.","Export aborted",MB_ICONERROR);
				MessageBox(g_hwnd,"Fatal error with RMT MSX system routines.","Export aborted",MB_ICONERROR);
				return 0;	
			}
			SaveBinaryBlock(ou,mem,adrfrom,adrto,0);

			memset(mem+adr_msxvideo,32,40*5);
			int p=0,q=0;
			char a;
			for(i=0; i<dlg.m_txt.GetLength(); i++)
			{
				a = dlg.m_txt.GetAt(i);
				if (a=='\n') { p+=40; q=0; }
				else
				{
					mem[adr_msxvideo+p+q]=a;
					q++;
				}
				if (p+q>=5*40) break;
			}
			StrToAtariVideo((char*)mem+adr_msxvideo,200);
			mem[adr_msxcolor]= (dlg.m_meter)? dlg.m_metercolor : 0;
			const BYTE vcval[8]={ 156, 78, 52, 39, 132, 66, 44, 33 };
			//                                     131 - by melo byt, ale naschval je o 1 vic kvuli delitelnosti 2,3,4
			//                                     RMT MSX player pocita se 132kou!
			int i = m_instrspeed-1;	//0-3
			if (g_ntsc) i+=4;		//0-3 nebo 4-7
			mem[adr_msxvcaddition]= vcval[i];
			SaveBinaryBlock(ou,mem,adr_msxvideo,adr_msxvideo+40*5+2-1,0);	//video+color+vcaddition (-1 toadrr vcetne)

			//run addr se prida az za modul jako posledni blok
		}
		break;

	case IOTYPE_ASM:
		{
			CExpASMDlg dlg;
			CString s,snot;
			int maxova=16;		//maximalni pocet dat na radku
			dlg.m_labelsprefix=g_expasmlabelprefix;

			if (dlg.DoModal()!=IDOK) return 0;

			g_expasmlabelprefix = dlg.m_labelsprefix;
			CString lprefix=g_expasmlabelprefix;
			BYTE tracks[TRACKSNUM];
			memset(tracks,0,TRACKSNUM); //init
			MarkTF_USED(tracks);
			//MarkTF_NOEMPTY(tracks);

			ou << ";ASM notation source";
			ou << EOL << "XXX\tequ $FF\t;empty note value";
			if (!lprefix.IsEmpty())
			{
				s.Format("%s_data",lprefix);
				ou << EOL << s;
			}

			if (dlg.m_type==1)
			{
				//tracks
				int t,i,j,not,dur,ins;
				for(t=0; t<TRACKSNUM; t++)
				{
					if (!(tracks[t] & TF_USED)) continue;
					s.Format(";Track $%02X",t);
					ou << EOL << s;
					if (!lprefix.IsEmpty())
					{
						s.Format("%s_track%02X",lprefix,t);
						ou << EOL << s;
					}
					//TTrack& tt=*m_tracks.GetTrack(t);
					TTrack& origtt=*m_tracks.GetTrack(t);
					TTrack tt; //docasny track
					memcpy((void*)&tt,(void*)&origtt,sizeof(TTrack)); //udela si kopii origtt do tt
					m_tracks.TrackExpandLoop(&tt); //expanduje tt kvuli GO smyckam
					int ova=maxova;
					for(i=0; i<tt.len; i++)
					{
						if (ova>=maxova)
						{
							ou << EOL << "\tdta ";
							ova=0;
						}
						not=tt.note[i];
						if (not>=0)
						{
							ins=tt.instr[i];
							if (dlg.m_notes==1) //noty
								not=m_instrs.GetNote(ins,not);
							else //frekvence
								not=m_instrs.GetFrequency(ins,not);
						}
						if (not>=0) snot.Format("$%02X",not); else snot="XXX";
						for(dur=1; i+dur<tt.len && tt.note[i+dur]<0;dur++);
						if (dlg.m_durations==1)
						{
							if (ova>0) ou << ",";
							ou << snot; ova++;
							for(j=1; j<dur; j++,ova++)
							{
								if (ova>=maxova)
								{
									ova=0;
									ou << EOL << "\tdta XXX";
								}
								else
									ou << ",XXX";
							}
						}
						else
						if (dlg.m_durations==2)
						{
							if (ova>0) ou << ",";
							ou << snot;
							ou << "," << dur;
							ova+=2;
						}
						else
						if (dlg.m_durations==3)
						{
							if (ova>0) ou << ",";
							ou << dur << ",";
							ou << snot;
							ova+=2;
						}

						i+=dur-1;
					}
				}
			}
			else
			if (dlg.m_type==2)
			{
				//song columns
				int clm;
				for(clm=0; clm<g_tracks4_8; clm++)
				{
					BYTE finished[SONGLEN];
					memset(finished,0,SONGLEN);
					int sline=0;
					static char* cnames[]={"L1","L2","L3","L4","R1","R2","R3","R4"};
					s.Format(";Song column %s",cnames[clm]);
					ou << EOL << s;
					if (!lprefix.IsEmpty())
					{
						s.Format("%s_column%s",lprefix,cnames[clm]);
						ou << EOL << s;
					}
					while(sline>=0 && sline<SONGLEN && !finished[sline])
					{
						finished[sline]=1;
						s.Format(";Song line $%02X",sline);
						ou << EOL << s;
						if (m_songgo[sline]>=0)
						{
							sline=m_songgo[sline]; //GOTO line
							s.Format(" Go to line $%02X",sline);
							ou << s;
							continue;
						}
						int trackslen=m_tracks.m_maxtracklen;
						for(i=0; i<g_tracks4_8; i++)
						{
							int at=m_song[sline][i];
							if (at<0 || at>=TRACKSNUM) continue;
							if (m_tracks.GetGoLine(at)>=0) continue;
							int al=m_tracks.GetLastLine(at)+1;
							if (al<trackslen) trackslen=al;
						}

						int t,i,j,not,dur,ins;
						int ova=maxova;
						t=m_song[sline][clm];
						if (t<0) 
						{
							ou << " Track --";
							if (!lprefix.IsEmpty())
							{
								s.Format("%s_column%s_line%02X",lprefix,cnames[clm],sline);
								ou << EOL << s;
							}
							if (dlg.m_durations==1)
							{
								for(i=0; i<trackslen; i++,ova++)
								{
									if (ova>=maxova)
									{
										ova=0;
										ou << EOL << "\tdta XXX";
									}
									else
										ou << ",XXX";
								}
							}
							else
							if (dlg.m_durations==2)
							{
								ou << EOL << "\tdta XXX,";
								ou << trackslen;
								ova+=2;
							}
							else
							if (dlg.m_durations==3)
							{
								ou << EOL << "\tdta ";
								ou << trackslen << ",XXX";
								ova+=2;
							}
							sline++;
							continue;
						}

						s.Format(" Track $%02X",t);
						ou << s;
						if (!lprefix.IsEmpty())
						{
							s.Format("%s_column%s_line%02X",lprefix,cnames[clm],sline);
							ou << EOL << s;
						}

						TTrack& origtt=*m_tracks.GetTrack(t);
						TTrack tt; //docasny track
						memcpy((void*)&tt,(void*)&origtt,sizeof(TTrack)); //udela si kopii origtt do tt
						m_tracks.TrackExpandLoop(&tt); //expanduje tt kvuli GO smyckam
						for(i=0; i<trackslen; i++)
						{
							if (ova>=maxova)
							{
								ova=0;
								ou << EOL << "\tdta ";
							}

							not=tt.note[i];
							if (not>=0)
							{
								ins=tt.instr[i];
								if (dlg.m_notes==1) //noty
									not=m_instrs.GetNote(ins,not);
								else //frekvence
									not=m_instrs.GetFrequency(ins,not);
							}
							if (not>=0) snot.Format("$%02X",not); else snot="XXX";
							for(dur=1; i+dur<trackslen && tt.note[i+dur]<0;dur++);
							if (dlg.m_durations==1)
							{
								if (ova>0) ou << ",";
								ou << snot; ova++;
								for(j=1; j<dur; j++,ova++)
								{
									if (ova>=maxova)
									{
										ova=0;
										ou << EOL << "\tdta XXX";
									}
									else
										ou << ",XXX";
								}
							}
							else
							if (dlg.m_durations==2)
							{
								if (ova>0) ou << ",";
								ou << snot;
								ou << "," << dur;
								ova+=2;
							}
							else
							if (dlg.m_durations==3)
							{
								if (ova>0) ou << ",";
								ou << dur << ",";
								ou << snot;
								ova+=2;
							}
							i+=dur-1;
						}
						sline++;
					} //while (sline ... )
				} //for clm=0 to g_tracks4_8
			}
			ou << EOL;
		}
		break;

	default:
		return 0;
	}

	// ULOZI VLASTNI RMT MODUL
	if (iotype != IOTYPE_ASM) SaveBinaryBlock(ou,mem,adr_module,maxadr-1,head_ffff);

	//DODATECNA DATA u nekterych typu
	switch(iotype)
	{
	case IOTYPE_RMT:
		{
			//pro RMT prida dalsi blok s nazvem songu a instrumentuuu
			//jednotlive nazvy jsou zprava orezane o mezery a ukoncene nulou
			int adrsongname = maxadr;
			s = m_songname;
			s.TrimRight();
			int lens = s.GetLength()+1;	//vcetne 0 za stringem
			strncpy((char*)(mem+adrsongname),(LPCSTR)s,lens);
			int adrinstrnames = adrsongname+lens;
			for(i=0; i<INSTRSNUM; i++)
			{
				if (instrsaved[i])
				{
					s = m_instrs.GetName(i);
					s.TrimRight();
					lens = s.GetLength()+1;	//vcetne 0 za stringem
					strncpy((char*)(mem+adrinstrnames),s,lens);
					adrinstrnames+=lens;
				}
			}
			//a ted ten 2.blok ulozi
			SaveBinaryBlock(ou,mem,adrsongname,adrinstrnames-1,0);
		}
		break;

	case IOTYPE_XEX:
		{
			//prida run addr na konec
			mem[0x2e0]=adr_msxrunadr & 0xff;
			mem[0x2e1]=adr_msxrunadr >> 8;	//run adr.
			SaveBinaryBlock(ou,mem,0x2e0,0x2e1,0);	//zapise blok s run adresou
		}
		break;
	}
	//
	return 1;
}


//******************************************
//IMPORT je v samostatnem souboru import.cpp
#include "import.cpp"
//Obsahuje metody:
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
	
	//RMT prvni hlavni blok RMT songu

	len = LoadBinaryBlock(in,mem,bfrom,bto);

	if (len>0)
	{
		int r = DecodeModule(mem,bfrom,bto+1,instrloaded,trackloaded);
		if (!r)
		{
			MessageBox(g_hwnd,"Bad RMT data format or old tracker version.","Open error",MB_ICONERROR);
			return 0;
		}
		//hlavni blok modulu je v poradku => prebere jeho zavadeci adresu
		g_rmtstripped_adr_module = bfrom;
		bto_mainblock = bto;
	}
	else
	{
		MessageBox(g_hwnd,"Corrupted file or unsupported format version.","Open error",MB_ICONERROR);
		return 0;	//v prvnim bloku nenacetl zadna data
	}

	//RMT - ted bude chtit nacitat druhy blok se jmeny)
	
	len = LoadBinaryBlock(in,mem,bfrom,bto);
	if (len<1)
	{
		CString s;
		s.Format("It is probably some stripped RMT song file.\nName of song and names of instruments are missing.\n\nModule memory location $%04X - $%04X.",g_rmtstripped_adr_module,bto_mainblock);
		MessageBox(g_hwnd,(LPCTSTR)s,"Info",MB_ICONINFORMATION);
		return 1;
	}

	char a;
	for(j=0; j<SONGNAMEMAXLEN && (a=mem[bfrom+j]); j++)
		m_songname[j]=a;
	
	for(k=j;k<SONGNAMEMAXLEN; k++) m_songname[k]=' '; //doplni mezery

	int adrinames=bfrom+j+1; //+1 to je ta nula za nazvem
	for(i=0; i<INSTRSNUM; i++)
	{
		if (instrloaded[i])
		{
			for(j=0; j<INSTRNAMEMAXLEN && (a=mem[adrinames+j]); j++)
				m_instrs.m_instr[i].name[j] = a;

			for(k=j; k<INSTRNAMEMAXLEN; k++) m_instrs.m_instr[i].name[k]=' '; //doplni mezery
				adrinames += j+1; //+1 to je nula za nazvem
		}
	}
	return 1;
}


BOOL CSong::ComposeRMTFEATstring(CString& dest, char* filename, BYTE *instrsaved,BYTE* tracksaved, BOOL sfx, BOOL gvf, BOOL nos)
{

#define DEST(var,str)	s.Format("%s\t\tequ %i\t\t;(%i times)\n",str,(var>0),var); dest+=s;

	dest.Format(";* --------BEGIN--------\n;* %s\n",filename);
	//
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


	//rozbor, ktere instrumenty se pouzivaji na kterych generatorech
	// a jestli je tam nekde zmena rychlosti
	int instrongx[INSTRSNUM][SONGTRACKS];
	memset(&instrongx,0,INSTRSNUM*SONGTRACKS*sizeof(instrongx[0][0]));
	//probere song
	for(int sl=0; sl<SONGLEN; sl++)
	{
		if (m_songgo[sl]>=0) continue;	//go radek
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

	//probere ted jednotlive instrumenty a co pouzivaji
	for(i=0; i<INSTRSNUM; i++)
	{
		if (instrsaved[i])
		{
			TInstrument& ai=m_instrs.m_instr[i];
			//commandy
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
			if (ai.par[PAR_TABGO]) usedtablego++;	//nenulove table go
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
			if (ai.par[PAR_DELAY]) //jen kdyz je effect delay nenulove
			{
				if (ai.par[PAR_VIBRATO]) usedeffectvibrato++;
				if (ai.par[PAR_FSHIFT]) usedeffectfshift++;
			}
		}
	}

	//generuje string

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
	//vsechny tracky pouzite v songu
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
	//vraci maxadr (ukazuje na prvni volnou adresu za modulem)
	//a nastavuje pole instrsaved a tracksaved
	if (iotype==IOTYPE_RMF) return MakeRMFModule(mem,adr,instrsaved,tracksaved);

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;
	memset(instrsave,0,INSTRSNUM);	//init
	memset(tracksave,0,TRACKSNUM); //init

	strncpy((char*)(mem+adr),"RMT",3);
	mem[adr+3]=g_tracks4_8+'0';	//4 nebo 8
	mem[adr+4]=m_tracks.m_maxtracklen & 0xff;
	mem[adr+5]=m_mainspeed & 0xff;
	mem[adr+6]=m_instrspeed;		//instr speed 1-4
	mem[adr+7]=RMTFORMATVERSION;	//cislo verze RMT formatu

	//v RMT se budou ukladat vsechny neprazdne tracky a neprazdne instrumenty
	//v ostatnich pouze neprazdne pouzite tracky a v nich pouzite instrumenty
	int i,j;

	//vsechny tracky pouzite v songu
	MarkTF_USED(tracksave);

	if (iotype==IOTYPE_RMT)
	{
		//do RMT se krom pouzitych pridavaji i vsechny neprazdne tracky
		//vsechny neprazdne tracky
		MarkTF_NOEMPTY(tracksave);
	}

	//vsechny instrumenty v trackach, ktere se budou ukladat
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
		//do RMT se krom instrumentu pouzitych v trackach co jsou v songu ukladaji i vsechny neprazdne instrumenty
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

	//a ted ma ukladat:
	//instrumenty numinstr
	//tracky numtracks
	//song songlines

	int adrpinstruments = adr + 16;
	int adrptrackslbs = adrpinstruments + numinstrs*2;
	int adrptrackshbs = adrptrackslbs + numtracks;

	int adrinstrdata = adrptrackshbs + numtracks; //za tabulkou hbytuuu trackuuu
	//uklada data instrumentu a zapisuje jejich zacatky do table
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

	int adrtrackdata = adrinstrdata;	//za daty instrumentuuu
	//uklada data tracku a zapisuje jejich zacatky do table
	for(i=0; i<numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = m_tracks.TrackToAta(i,mem+adrtrackdata,MAXATATRACKLEN);
			if (lentrack<1)
			{	//nelze ulozit do RMT
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

	int adrsong = adrtrackdata;		//za daty trackuuu

	//ulozi od adrsong data songu
	//int lensong = SongToAta(mem+adrsong,g_tracks4_8*SONGLEN,adrsong);  //<---HRUBKA S MAX.VELIKOSTI BUFFERU!
	int lensong = SongToAta(mem+adrsong,0x10000-adrsong,adrsong);

	int endofmodule = adrsong+lensong;

	//zapise vypocitane pointery do hlavicky
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
	//vraci maxadr (ukazuje na prvni volnou adresu za modulem)
	//a nastavuje pole instrsaved a tracksaved

	BYTE* instrsave = instrsaved;
	BYTE* tracksave = tracksaved;

	memset(instrsave,0,INSTRSNUM);	//init
	memset(tracksave,0,TRACKSNUM); //init

	mem[adr+0]=m_instrspeed;		//instr speed 1-4
	mem[adr+1]=m_mainspeed & 0xff;

	//v RMT se budou ukladat vsechny neprazdne tracky a neprazdne instrumenty
	//v ostatnich pouze neprazdne pouzite tracky a v nich pouzite instrumenty
	int i,j;

	//vsechny tracky pouzite v songu
	MarkTF_USED(tracksave);

	//vsechny instrumenty v trackach, ktere se budou ukladat
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

	//a ted ma ukladat:
	//song songlines
	//instrumenty numinstr
	//tracky numtracks

	//Zjisti delku songu a nejkratsi delky jednotlivych tracku v songlinech
	int songlines=0,a;
	int emptytrackusedinline=-1;
	BOOL emptytrackused=0;
	int songline_trackslen[SONGLEN];
	for(i=0; i<SONGLEN; i++)
	{
		int minlen = m_tracks.m_maxtracklen;	//init
		if (m_songgo[i]>=0)  //Go radek
		{
			songlines=i+1;	//prozatimni konec
			if (emptytrackusedinline>=0) emptytrackused=1;
		}
		else
		{
			for(j=0; j<g_tracks4_8; j++)
			{
				a = m_song[i][j];
				if (a>=0 && a<TRACKSNUM)
				{
					songlines=i+1;	//prozatimni konec
					int tl = m_tracks.GetLength(a);
					if (tl<minlen) minlen = tl; //je mensi nez prozatimni nejkratsi
				}
				else
					emptytrackusedinline=i;
			}
		}
		songline_trackslen[i]= minlen;
	}
	int lensong = (songlines-1) * (g_tracks4_8*2+3);	//songlines-1, protoze posledni GOTO se tam davat nemusi

	//uklada data instrumentu a zapisuje jejich zacatky do table
	//do prozatimni pameti meminstruments vzhledem k zacatku 0
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

	//uklada data tracku a zapisuje jejich zacatky do table
	//do prozatimni pameti memtracks vzhledem k zacatku 0
	unsigned char memtracks[65536];
	WORD trackpointers[TRACKSNUM];
	memset(memtracks,0,65536);
	memset(trackpointers,0,TRACKSNUM*2);
	int adrtrackdata = 0;	//od zacatku
	//empty track
	int adremptytrack=0;
	if (emptytrackused)
	{
		//memtracks[0]=62;	//pauza
		//memtracks[1]=0;		//nekonecna
		memtracks[0]=255;	//nekonecna pauza (FASTER modifikace)
		adrtrackdata += 1; //za empty track
	}
	for(i=0; i<numtracks; i++)
	{
		if (tracksave[i])
		{
			int lentrack = m_tracks.TrackToAtaRMF(i,memtracks+adrtrackdata,MAXATATRACKLEN);
			if (lentrack<1)
			{	//nelze ulozit do RMT
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


	//Zkonstruuje Song uz nacisto
	int apos,go;
	for(int sline=0; sline<songlines; sline++)
	{
		apos=adrsong+sline*(g_tracks4_8*2+3);
		if ( (go=m_songgo[sline])>=0)
		{
			//je tam go radek
			//=> do songline o 1 nadtimto zmeni GO nextline na GO nekam jinam
			//
			if (apos>=0) //GOTO hned na 0.songline
			{
				WORD goadr = adrsong + (go*(g_tracks4_8*2+3));
				mem[apos-2] = goadr & 0xff;		//dolni byte
				mem[apos-1] = (goadr>>8);		//horni byte
			}
			for(int j=0; j<g_tracks4_8*2+3; j++) mem[apos+j]=255; //pro poradek
			mem[apos+g_tracks4_8*2+2]=go;
		}
		else
		{
			//jsou tam cisla tracku
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

				mem[apos+g_tracks4_8*2] = songline_trackslen[sline]; //maxtracklen pro tuto songline
				WORD nextsongline = apos+ g_tracks4_8*2+3;
				mem[apos+g_tracks4_8*2+1] = nextsongline & 0xff;	//db
				mem[apos+g_tracks4_8*2+2] = (nextsongline >> 8);	//hb
			}
		}
	}

	//INSTRUMENTS cast (instrument pointers table a instruments data)
	//pripocita posun v tabulce pointru
	for(i=0; i<numinstrs; i++)
	{
		WORD ai = meminstruments[i*2] + (meminstruments[i*2+1]<<8) + adrinstruments;
		meminstruments[i*2] = ai & 0xff;
		meminstruments[i*2+1] = (ai >> 8);
	}
	//ulozi instrument table pointry a instrdata
	memcpy(mem+adrinstruments,meminstruments,leninstruments);

	//TRACKS cast
	memcpy(mem+adrtracks,memtracks,lentracks);

	//zapise vypocitane pointery do hlavicky
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

	if (strncmp((char*)(mem+adr),"RMT",3)!=0) return 0; //neni tam RMT
	b = mem[adr+3];
	if (b!='4' && b!='8') return 0;	//neni to RMT4 nebo RMT8
	g_tracks4_8 = b & 0x0f;
	b = mem[adr+4];
	m_tracks.m_maxtracklen = (b>0)? b:256;	//0 => 256
	g_cursoractview = 0;
	b = mem[adr+5];
	m_mainspeed = b;
	if (b<1) return 0;		//nemuze byt nulova rychlost
	b = mem[adr+6];
	if (b<1 || b>8) return 0;		//instrument speed je mensi nez 1 nebo vetsi nez 8 (pozn.: melo by jit max 4, ale dovoli az 8 a vypise jen warning)
	m_instrspeed = b;
	int version = mem[adr+7];
	if (version > RMTFORMATVERSION)	return 0;	//version byte je vetsi nez co to umi


	//Ted uz je nastaveno m_tracks.m_maxtracklen na hodnotu podle hlavicky z RMT modulu, takze
	//musi znovu reinicializovat Tracky aby se jim vsem tato hodnota nastavila jako delka
	m_tracks.InitTracks();

	int adrpinstruments = mem[adr+ 8] + (mem[adr+ 9]<<8);
	int adrptrackslbs   = mem[adr+10] + (mem[adr+11]<<8);
	int adrptrackshbs   = mem[adr+12] + (mem[adr+13]<<8);
	int adrsong		    = mem[adr+14] + (mem[adr+15]<<8);

	int numinstrs = (adrptrackslbs-adrpinstruments)/2;
	int numtracks = (adrptrackshbs-adrptrackslbs);
	int lensong = adrend - adrsong;

	//dekodovani jednotlivych instrumentuuu
	for(i=0; i<numinstrs; i++)
	{
		int instrdata = mem[adrpinstruments+i*2] + (mem[adrpinstruments+i*2+1]<<8);
		if (instrdata==0) continue; //vynechane instrumenty maji ukazatel db,hb = 0
		//tenhle ma ukazatel nenulovy
		BOOL r;
		if (version==0)
			r=m_instrs.AtaV0ToInstr(mem+instrdata,i);
		else
			r=m_instrs.AtaToInstr(mem+instrdata,i);
		m_instrs.ModificationInstrument(i);	//zapise do Atari ram
		if (!r) return 0; //nejaky problem s instrumentem => KONEC
		instrloaded[i]=1;
	}

	//dekodovani jednotlivych trackuuu
	for(i=0; i<numtracks; i++)
	{
		int track=i;
		int trackdata = mem[adrptrackslbs+i] + (mem[adrptrackshbs+i]<<8);
		if (trackdata==0) continue; //vynechane tracky maji ukazatel db,hb=0

		//konec tracku pozna podle adresy dalsiho tracku a u posledniho podle adresy songu,
		//ktery nasleduje za daty posledniho tracku
		int trackend=0;
		for(j=i; j<numtracks; j++)
		{
			trackend = (j+1==numtracks)? adrsong : mem[adrptrackslbs+j+1] + (mem[adrptrackshbs+j+1]<<8);
			if (trackend!=0) break;
			i++;	//aby pak pokracoval az od dalsiho a preskocil ten vynechany
		}
		int tracklen = trackend - trackdata;
		//
		BOOL r;
		r=m_tracks.AtaToTrack(mem+trackdata,tracklen,track);
		if (!r) return 0; //nejaky problem s trackem => KONEC
		trackloaded[track]=1;
	}

	//dekodovani songu
	BOOL r;
	r=AtaToSong(mem+adrsong,lensong,adrsong);
	if (!r) return 0; //nejaky problem se songem => KONEC

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
		if ( (v=m_playptvolume[t])>=0) //volume se nastavuje jako posledni
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

int CSong::TopLine()
{
	if (m_songactiveline < m_songtopline) {
		m_songtopline = m_songactiveline;
	} else if (m_songactiveline > g_songlines-3) {
		m_songtopline = m_songactiveline - (g_songlines - 3);
	} else if (m_songactiveline < m_songtopline + 3) {
		m_songtopline = m_songactiveline - 3;
		if (m_songtopline < 0) m_songtopline = 0;
	}
	return m_songtopline;
}

BOOL CSong::DrawSong()
{
	int line,i,j,k,y,t;
	char s[32],c;
	TextXY("SONG",g_song_x+8,SONG_Y);

	//tisk L1 .. L4 R1 .. R4 s cervenym aktualnim trackem
	k=g_song_x+6*8;
	s[0]='L';
	s[2]=0;
	for(i=0; i<4; i++,k+=24)
	{
		s[1]=i+49;	//znak 1-4
		if (GetChannelOnOff(i))
		{
			if (m_trackactivecol==i) c=6;	//aktivni kanal cervene
			else c=0; //normalni zapnuty kanal
		}
		else c=1; //vypnute kanaly jsou sedou barvou
		TextXY(s,k,SONG_Y,c);
	}
	s[0]='R';
	for(i=4; i<g_tracks4_8; i++,k+=24)
	{
		s[1]=i+49-4;	//znak 1-4
		if (GetChannelOnOff(i))
		{
			if (m_trackactivecol==i) c=6;	//aktivni kanal cervene
			else c=0; //normalni zapnuty kanal
		}
		else c=1; //vypnute kanaly jsou sedou barvou
		TextXY(s,k,SONG_Y,c);
	}

	int top_line = TopLine();

	y=SONG_Y+SONG_HEADER_H;
	for(i=0; i<g_songlines; i++,y+=SONG_LINE_H)	//vypisuje 5 radku songu
	{
		line = top_line + i; 		//2 radky nad
		if (line<0 || line>255)
		{
			//ClearXY(SONG_X+16,y,27);		//smaze 27 pismen
			continue;
		}
		strcpy(s,"XX:");
		s[0]= CharH4(line);
		s[1]= CharL4(line);
		c = (line==m_songplayline)? 2:0;
		TextXY(s,g_song_x+16,y,c);

		if ((j=m_songgo[line])>=0)
		{
			//je tam GO radek
			s[0]=CharH4(j);
			s[1]=CharL4(j);
			s[2]=0;
			c = (line==m_songactiveline)? 6:0;
			TextXY("Go to line",g_song_x+16+32,y,c);
			if (line==m_songactiveline && !g_prove && g_activepart==PARTSONG) c=COLOR_SELECTED;
			TextXY(s,g_song_x+16+32+11*8,y,c);
		}
		else
		{
			//jsou tam cisla tracku
			s[2]=0;
			for (j=0,k=32; j<g_tracks4_8; j++,k+=24)
			{
				if ( (t=m_song[line][j]) >=0 )
				{
					s[0]= CharH4(t);
					s[1]= CharL4(t);
				}
				else
				{
					s[0]=s[1]='-';	//--
				}
				if (line==m_songactiveline && j==m_trackactivecol)
					c= (!g_prove && g_activepart==PARTSONG)? COLOR_SELECTED:6;
				else
					c = (line==m_songplayline)? 2:0;
				TextXY(s,g_song_x+16+k,y,c);
			}
		}
	}
	TextXY("\x04\x05",g_song_x,SONG_Y+SONG_HEADER_H+(m_songactiveline-top_line)*SONG_LINE_H,6);	//sipka
	return 1;
}

BOOL CSong::DrawTracks()
{
	static char* tnames="L1L2L3L4R1R2R3R4";
	char s[16],stmp[16];
	int i,x,y,tr,line,c;
	int t;

	if (SongGetGo()>=0)		//je to GO radek, nebude tracky vykreslovat
	{
		sprintf(s,"Go to line %02X",SongGetGo());
		TextXY(s,TRACKS_X+40*8,TRACKS_Y+8*16);
		return 1;
	}

	int g_track_scroll_margin = 3;

	// If on the top, move the first visible line up
	if (m_trackactiveline < g_cursoractview+g_track_scroll_margin) {
		g_cursoractview = m_trackactiveline-g_track_scroll_margin;
		if (g_cursoractview < 0) g_cursoractview = 0;
	}

	// If on the bottom, move the first visible line to bottom
	if (m_trackactiveline >= (g_cursoractview+g_tracklines-g_track_scroll_margin)) {
		g_cursoractview = m_trackactiveline - g_tracklines+g_track_scroll_margin+1;
	}
/*
	if (m_trackactiveline>g_cursoractview+g_trackcursorverticalrange)
		g_cursoractview=m_trackactiveline-g_trackcursorverticalrange;
	else
	if (m_trackactiveline<g_cursoractview-g_trackcursorverticalrange)
		g_cursoractview=m_trackactiveline+g_trackcursorverticalrange;
*/

	y = TRACKS_Y+TRACKS_HEADER_H;
	//====== Track line numbers
	strcpy(s,"--‚");

	for(i=0; i<g_tracklines; i++,y+=16)
	{
		line = g_cursoractview + i;		// - 8;		//8 lines shora
		if (line<0 || line>=m_tracks.m_maxtracklen)
		{
			//ClearXY(TRACKS_X,y,3);
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
		//if (line == m_trackactiveline) c=6;		//cervena
		//else
		if (line == m_trackplayline) c=2;		//zluta
		else
		if ((line % g_tracklinehighlight) == 0) c=5;	//modra
		else
			c=0;
		TextXY(s,TRACKS_X,y,c);
	}

	//====== Track captions

	strcpy(s,"TRACK_XX  ");
	x = TRACKS_X+5*8;
	for(i=0; i<g_tracks4_8; i++, x+=11*8)
	{
		s[6]=tnames[i*2];
		s[7]=tnames[i*2 +1];
		c = (GetChannelOnOff(i)) ? 0 : 1;	//vypnute kanaly jsou sedou barvou
		TextXY(s,x+8,TRACKS_Y,c);
		//track v aktualnim radku songu
		tr = m_song[m_songactiveline][i];
		//prehrava se?
		if ( m_song[m_songplayline][i] == tr ) t = m_trackplayline; else t = -1;
		m_tracks.DrawTrack(i,x,TRACKS_Y+LINE_H,tr, g_tracklines , m_trackactiveline,g_cursoractview,t,(m_trackactivecol==i),m_trackactivecur);
	}


	//selektovany blok
	if (g_trackcl.IsBlockSelected())
	{
		x = TRACKS_X+6*8+ g_trackcl.m_selcol *11*8 -2;
		int xt = x + 10*8+4;
		y = TRACKS_Y+TRACKS_HEADER_H;
		int bfro,bto;
		g_trackcl.GetFromTo(bfro,bto);

		int yf = bfro-g_cursoractview;
		int yt = bto-g_cursoractview+1;
		int p1=1,p2=1;
		if (yf<0)  { yf=0; p1=0; }
		if (yt>g_tracklines) { yt=g_tracklines; p2=0; }

		if (yf<g_tracklines && yt>0 
			&& g_trackcl.m_seltrack==SongGetActiveTrackInColumn(g_trackcl.m_selcol)
			&& g_trackcl.m_selsongline==SongGetActiveLine())
		{
			//obdelnik vymezujici selektnuty blok
			CPen redpen(PS_SOLID,1,RGB(255,255,255));
			CPen* origpen = g_mem_dc->SelectObject(&redpen);
			g_mem_dc->MoveTo(x,y+yf*TRACK_LINE_H);
			g_mem_dc->LineTo(x,y+yt*TRACK_LINE_H);
			g_mem_dc->MoveTo(xt,y+yf*TRACK_LINE_H);
			g_mem_dc->LineTo(xt,y+yt*TRACK_LINE_H);
			if (p1) { g_mem_dc->MoveTo(x,y+yf*TRACK_LINE_H); g_mem_dc->LineTo(xt,y+yf*TRACK_LINE_H); }
			if (p2) { g_mem_dc->MoveTo(x,y+yt*TRACK_LINE_H); g_mem_dc->LineTo(xt+1,y+yt*TRACK_LINE_H); }
			g_mem_dc->SelectObject(origpen);
		}
		char tx[96];
		char s1[4],s2[4];
		GetTracklineText(s1,bfro);
		GetTracklineText(s2,bto);
		sprintf(tx,"%i line(s) [%s-%s] selected in the track %02X",bto-bfro+1,s1,s2,g_trackcl.m_seltrack);
		TextXY(tx,STATUS_X,STATUS_Y);
		x = TRACKS_X+4*8 + strlen(tx)*8 +8;
		if (g_trackcl.m_all)
			strcpy(tx,"[change ALL events]");
		else
			sprintf(tx,"[change events for instr %02X only]",m_activeinstr);
		TextXY(tx,x,STATUS_Y,6);
	}

	//cary vymezujici aktualni radek
	x = (g_tracks4_8==4)? TRACKS_X+(93-4*11)*8 : TRACKS_X+93*8;
	y = TRACKS_Y+TRACKS_HEADER_H-2+(m_trackactiveline-g_cursoractview)*TRACK_LINE_H;
	g_mem_dc->MoveTo(TRACKS_X,y);
	g_mem_dc->LineTo(x,y);
	g_mem_dc->MoveTo(TRACKS_X,y+TRACK_LINE_H+2+1);
	g_mem_dc->LineTo(x,y+TRACK_LINE_H+2+1);

	//cara vymezujici hranici mezi left/right
	if (g_tracks4_8>4)
	{
		int fl,tl;
//		fl=8-g_cursoractview; if (fl<0) fl=0;
//		tl=8-g_cursoractview+m_tracks.m_maxtracklen; if (tl>g_tracklines) tl=g_tracklines;
		fl = 0;
		tl = g_tracklines;
		g_mem_dc->MoveTo(TRACKS_X+50*8-7,TRACKS_Y+TRACKS_HEADER_H-2+fl*TRACK_LINE_H);
		g_mem_dc->LineTo(TRACKS_X+50*8-7,TRACKS_Y+TRACKS_HEADER_H+2+tl*TRACK_LINE_H);
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
	int i,c;

	if (g_prove>0)
		TextXY((g_prove==1)?"PROVE MODE 1":"PROVE MODE 2 - STEREO",INFO_X,INFO_Y+2*16,6);
	
	if (!g_prove && g_activepart==PARTINFO && m_infoact==0) //info? && edit name?
	{
		i=m_songnamecur;
		c=6; //cervene
	}
	else
	{
		i=-1;
		c=0; //bila
	}
	TextXYSelN(m_songname,i,INFO_X,INFO_Y,c);


	sprintf(s,"MUSIC SPEED: %02X/%02X/%X  MAXTRACKLENGTH: %02X  %s",
		m_speed,m_mainspeed,m_instrspeed,
		m_tracks.m_maxtracklen,
		(g_tracks4_8==4)? "MONO-4-TRACKS" : "STEREO-8-TRACKS"
		);
	TextXY(s,INFO_X,INFO_Y+1*16);
	if (!g_prove && g_activepart==PARTINFO)
	{
		switch (m_infoact)
		{
		case 1:	//speed
			s[15]=0; //za cislem speed
			TextXY(s+13,INFO_X+13*8,INFO_Y+1*16,COLOR_SELECTED);	//selected
			break;
		case 2: //mainspeed
			s[18]=0; //za cislem main speed
			TextXY(s+16,INFO_X+16*8,INFO_Y+1*16,COLOR_SELECTED);	//selected
			break;
		case 3: //instrspeed
			s[20]=0; //za cislem instrspeed
			TextXY(s+19,INFO_X+19*8,INFO_Y+1*16,COLOR_SELECTED);	//selected
			break;
		}
	}
		
	sprintf(s,"%02X: %s",m_activeinstr,m_instrs.GetName(m_activeinstr));
	TextXY(s,INFO_X,INFO_Y+3*16);
	sprintf(s,"OCTAVE %i-%i",m_octave+1,m_octave+2);
	TextXY(s,INFO_X+47*8,INFO_Y+3*16);
	sprintf(s,"VOLUME %X",m_volume);
	TextXY(s,INFO_X+49*8,INFO_Y+4*16);
	if (g_respectvolume) TextXY("\x17",INFO_X+57*8,INFO_Y+4*16);	//respect volume mode

	//nad instrumentem radek s flagy
	BYTE flag = m_instrs.GetFlag(m_activeinstr);

	int x=INFO_X;	//+4*8;
	const int y=INFO_Y+4*16;
	int g=(m_trackactivecol%4) +1;		//generator 1 az 4
	if (flag&IF_FILTER)
	{
		if (g>2) 
		{
			TextMiniXY("NO_FILTER",x,y,1);	//gray
			x += 10*8;
		}
		else
		{
			if (g==1)
				TextMiniXY("FILTER(1+3)",x,y);
			else
				TextMiniXY("FILTER(2+4)",x,y);
			x += 12*8;
		}
	}
	if (flag&IF_BASS16)
	{
		if (g==2)
		{
			TextMiniXY("BASS16(2+1)",x,y);
			x += 12*8;
		}
		else
		if (g==4)
		{
			TextMiniXY("BASS16(4+3)",x,y);
			x += 12*8;
		}
		else
		{
			TextMiniXY("NO_BASS16",x,y,1);	//gray
			x += 10*8;
		}
	}
	if (flag&IF_PORTAMENTO)
	{
		TextMiniXY("PORTAMENTO",x,y);
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
		TextMiniXY(s,x,y);
		x += 6*8;
	}

	return 1;
}

BOOL CSong::DrawPlaytimecounter(CDC *pDC = NULL)
{
	if (!g_viewplaytimecounter) return 0;

//#define PLAYTC_X	SONG_X+7
#define PLAYTC_Y	SONG_Y-8
#define PLAYTC_S	4*8
#define PLAYTC_H	8

	int fps = (g_ntsc)? 60 : 50;

	int time10 = (g_playtime%fps)*10/fps;
	int ts = g_playtime/fps;	//celkovy cas v sekundach
	int timesec = ts%60;	//sekundy 0-59
	int timemin = ts/60;	//minuty 0-...

	char timstr[5];
	if (!timemin)
	{
		//cas od 00.0 do 59.9
		timstr[0]=(timesec/10) | '0';
		timstr[1]=(timesec%10) | '0';
		timstr[2]='.';
		timstr[3]=(time10) | '0';
	}
	else
	{
		//cas od 1:00 do 9:99
		timstr[0]=(timemin%10) | '0';
		timstr[1]=':';
		timstr[2]=(timesec/10) | '0';
		timstr[3]=(timesec%10) | '0';
	}
	timstr[4]=0; //ukonceni
	TextMiniXY(timstr,g_song_x+7,PLAYTC_Y,(m_play)? 2 : 0);
	if (pDC) pDC->BitBlt(g_song_x+7,PLAYTC_Y,PLAYTC_S,PLAYTC_H,g_mem_dc,g_song_x+7,PLAYTC_Y,SRCCOPY);
	return 1;
}

BOOL CSong::DrawAnalyzer(CDC *pDC = NULL)
{
	if (!g_viewanalyzer) return 0;

#define ANALYZER_X	TRACKS_X+6*8+2
#define ANALYZER_Y	TRACKS_Y-8
#define ANALYZER_S	4
#define ANALYZER_H	4
#define ANALYZER_HP	8

#define ANALYZER2_X	SONG_X+6*8+1
#define ANALYZER2_Y	TRACKS_Y-8
#define ANALYZER2_S	1
#define ANALYZER2_H	4
#define ANALYZER2_HP 8

#define Hook1(g1,g2)			\
	{							\
		g_mem_dc->MoveTo(ANALYZER_X+ANALYZER_S*15/2+11*8*(g1),ANALYZER_Y-1);		\
		g_mem_dc->LineTo(ANALYZER_X+ANALYZER_S*15/2+11*8*(g1),ANALYZER_Y-yu);		\
		g_mem_dc->LineTo(ANALYZER_X+ANALYZER_S*15/2+11*8*(g2),ANALYZER_Y-yu);		\
		g_mem_dc->LineTo(ANALYZER_X+ANALYZER_S*15/2+11*8*(g2),ANALYZER_Y);			\
	}
#define Hook2(g1,g2)			\
	{							\
		g_mem_dc->MoveTo(ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g1),ANALYZER_Y-1);		\
		g_mem_dc->LineTo(ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g1),ANALYZER_Y-yu);		\
		g_mem_dc->LineTo(ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g2),ANALYZER_Y-yu);		\
		g_mem_dc->LineTo(ANALYZER2_X+ANALYZER2_S*15/2+3*8*(g2),ANALYZER_Y);			\
	}

#define COL_BLOCK	56
#define RGBMUTE		RGB(96,96,96)
#define RGBNORMAL	RGB(255,255,255)
#define RGBVOLUMEONLY	RGB(128,255,255)

	int audf,audc,vol;
	static int idx[8]={0xd200,0xd202,0xd204,0xd206,0xd210,0xd212,0xd214,0xd216};
	int col[8];
	int yu=7;
	for(int i=0; i<g_tracks4_8; i++) col[i]=72;
	int a;
	COLORREF acol;

	if (g_active_ti==PARTTRACKS) //vetsi vzhled pro track edit mode
	{
		g_mem_dc->FillSolidRect(ANALYZER_X,ANALYZER_Y-ANALYZER_HP,g_tracks4_8*11*8-8,ANALYZER_H+ANALYZER_HP,RGB(72,72,72));
		a = g_atarimem[0xd208]; //audctl1
		if (a&0x04) { col[2]=COL_BLOCK; Hook1(0,2); yu-=2; }
		if (a&0x02) { col[3]=COL_BLOCK;	Hook1(1,3); yu-=2; }
		if (a&0x10) { col[0]=COL_BLOCK;	Hook1(0,1); yu-=2; }
		if (a&0x08) { col[2]=COL_BLOCK;	Hook1(2,3); yu-=2; }
		yu=7;
		a = g_atarimem[0xd218]; //audctl2
		if (a&0x04) { col[2+4]=COL_BLOCK; Hook1(0+4,2+4); yu-=2; }
		if (a&0x02) { col[3+4]=COL_BLOCK; Hook1(1+4,3+4); yu-=2; }
		if (a&0x10) { col[0+4]=COL_BLOCK; Hook1(0+4,1+4); yu-=2; }
		if (a&0x08) { col[2+4]=COL_BLOCK; Hook1(2+4,3+4); yu-=2; }
		
		for(int i=0; i<g_tracks4_8; i++)
		{
			audf = g_atarimem[idx[i]];
			audc = g_atarimem[idx[i]+1];
			vol =  audc & 0x0f;
			a=i*11*8;
			g_mem_dc->FillSolidRect(ANALYZER_X+a,ANALYZER_Y,15*ANALYZER_S ,ANALYZER_H,RGB(col[i],col[i],col[i]));
			acol = GetChannelOnOff(i)? ((audc & 0x10)? RGBVOLUMEONLY : RGBNORMAL) : RGBMUTE;
			if (vol) g_mem_dc->FillSolidRect(ANALYZER_X+a+(15-vol)*ANALYZER_S/2,ANALYZER_Y,vol*ANALYZER_S,ANALYZER_H,acol);
			if (g_viewpokeyregs)
			{
				NumberMiniXY(audf,ANALYZER_X+10+a,ANALYZER_Y-8);
				NumberMiniXY(audc,ANALYZER_X+36+a,ANALYZER_Y-8);
			}
		}

		if (g_viewpokeyregs)
		{
			NumberMiniXY(g_atarimem[0xd208],ANALYZER_X+23+1*8*11+44,ANALYZER_Y-0);
			if (g_tracks4_8>4) NumberMiniXY(g_atarimem[0xd218],ANALYZER_X+23+5*8*11+44,ANALYZER_Y-0);
		}

		if (pDC) pDC->BitBlt(ANALYZER_X,ANALYZER_Y-ANALYZER_HP,g_tracks4_8*11*8-8,ANALYZER_H+ANALYZER_HP+4,g_mem_dc,ANALYZER_X,ANALYZER_Y-ANALYZER_HP,SRCCOPY);
	}
	else //mensi vzhled pro instrument edit mode
	{
		g_mem_dc->FillSolidRect(ANALYZER2_X,ANALYZER2_Y-ANALYZER2_HP,g_tracks4_8*3*8-8,ANALYZER2_H+ANALYZER2_HP,RGB(72,72,72));
		a = g_atarimem[0xd208]; //audctl1
		if (a&0x04) { col[2]=COL_BLOCK; Hook2(0,2); yu-=2; }
		if (a&0x02) { col[3]=COL_BLOCK;	Hook2(1,3); yu-=2; }
		if (a&0x10) { col[0]=COL_BLOCK;	Hook2(0,1); yu-=2; }
		if (a&0x08) { col[2]=COL_BLOCK;	Hook2(2,3); yu-=2; }
		yu=7;
		a = g_atarimem[0xd218]; //audctl2
		if (a&0x04) { col[2+4]=COL_BLOCK; Hook2(0+4,2+4); yu-=2; }
		if (a&0x02) { col[3+4]=COL_BLOCK; Hook2(1+4,3+4); yu-=2; }
		if (a&0x10) { col[0+4]=COL_BLOCK; Hook2(0+4,1+4); yu-=2; }
		if (a&0x08) { col[2+4]=COL_BLOCK; Hook2(2+4,3+4); yu-=2; }
		
		for(int i=0; i<g_tracks4_8; i++)
		{
			audc = g_atarimem[idx[i]+1];
			vol =  audc & 0x0f;
			g_mem_dc->FillSolidRect(ANALYZER2_X+i*3*8,ANALYZER2_Y,15*ANALYZER2_S ,ANALYZER2_H,RGB(col[i],col[i],col[i]));
			acol = GetChannelOnOff(i)? ((audc & 0x10)? RGBVOLUMEONLY : RGBNORMAL) : RGBMUTE;
			if (vol) g_mem_dc->FillSolidRect(ANALYZER2_X+i*3*8+(15-vol)*ANALYZER2_S/2,ANALYZER2_Y,vol*ANALYZER2_S,ANALYZER2_H,acol);
		}

		if (pDC) pDC->BitBlt(ANALYZER2_X,ANALYZER2_Y-ANALYZER2_HP,g_tracks4_8*3*8-8,ANALYZER2_H+ANALYZER2_HP,g_mem_dc,ANALYZER2_X,ANALYZER2_Y-ANALYZER2_HP,SRCCOPY);
	}

	return 1;
}

BOOL CSong::InfoKey(int vk,int shift,int control)
{
	if (m_infoact==0)
	{
		if (IsnotMovementVKey(vk))
		{	//uklada undo jen kdyz to neni pohyb kurzoru
			m_undo.ChangeInfo(0,UETYPE_INFODATA);
		}
		if ( EditText(vk,shift,control,m_songname,m_songnamecur,SONGNAMEMAXLEN) ) m_infoact=1;
		return 1;
	}

	int i,num;		
	int volatile * infptab[]={&m_speed,&m_mainspeed,&m_instrspeed};
	int infandtab[]={0xff,0xff,0x04};
	int volatile& infp = *infptab[m_infoact-1];
	int infand = infandtab[m_infoact-1];
	
	//ctyri pro oboje
	//if (m_infoact==3) infand = (g_tracks4_8>4)? 3 : 4;	//max. instrspeed je 3 pro stereo, 4 pro mono
		
		
	if ( (num=NumbKey(vk))>=0 && num<=infand)
	{
		i= infp & 0x0f; //nizsi cifra
		if (infand<0x0f)
		{	if (num<=infand) i = num; }
		else
			i = ((i<<4) | num) & infand;
		if (i<=0) i=1;	//vsechny speedy mohou byt minimalne 1
		m_undo.ChangeInfo(0,UETYPE_INFODATA);
		infp = i;
		return 1;
	}

	switch(vk)
	{
	case VK_TAB:
		if (shift)
		{
			m_infoact=0;	//Shift+TAB => Name
		}
		else
		{
			if (m_infoact<3) m_infoact++; else m_infoact=1; //TAB 1 a 2 a 3
		}
		return 1;

	case VK_UP:
		if (control) goto IncrementInfoPar;
		break;

	case VK_DOWN:
		if (control) goto DecrementInfoPar;
		break;

	case VK_LEFT:
		if (control)
		{
DecrementInfoPar:
			i = infp;
			i--;
			if (i<=0) i=infand; //speedy mohou byt minimalne 1
			m_undo.ChangeInfo(0,UETYPE_INFODATA);
			infp = i;
		}
		else
		{
			if (m_infoact>1) m_infoact--; else m_infoact=3;
		}
		return 1;
	
	case VK_RIGHT:
		if (control)
		{
IncrementInfoPar:
			i = infp;
			i++;
			if (i>infand) i=1;	//speedy mohou byt minimalne 1
			m_undo.ChangeInfo(0,UETYPE_INFODATA);
			infp = i;
		}
		else
		{
			if (m_infoact<3) m_infoact++; else m_infoact=1;
		}
		return 1;

	case 13:		//VK_ENTER
		g_activepart = g_active_ti;
		return 1;

	}
	return 0;
}

BOOL CSong::InstrKey(int vk,int shift,int control)
{
	//poznamka: vraci-li 1, pak se v RmtView dela screenupdate
	TInstrument& ai= m_instrs.m_instr[m_activeinstr];
	int& ap = ai.par[ai.activepar];
	int& ae = ai.env[ai.activeenvx][ai.activeenvy];
	int& at = ai.tab[ai.activetab];
	int i;

	if (!control && !shift && NumbKey(vk)>=0)
	{
		//if (control)
		//	return SongTrackSetByNum(NumbKey(vk));		//<--v Instrument modu nebude pres CTRL+cislo menit song

		if (ai.act==1) //parameters
		{
			//int pand = shpar[ai.activepar].pand;
			int pmax = shpar[ai.activepar].pmax;
			int pfrom = shpar[ai.activepar].pfrom;
			if (NumbKey(vk)>pmax+pfrom) return 0;
			i = ap + pfrom;
			i &= 0x0f; //dolni cifra
			if (pmax+pfrom>0x0f)
			{
				i = (i<<4) | NumbKey(vk);
				if (i > pmax+pfrom)
					i &= 0x0f;		//necha pouze dolni cifru
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
			if (i != num) return 0; //po andu vyslo neco jineho => stlaceno cislo mimo rozsah
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ae = i;
			//posun doprava
			i=ai.activeenvx;
			if (i<ai.par[PAR_ENVLEN]) i++;	// else i=0; //delka env
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

	//pro name,parametrs, envelope i table
	switch(vk)
	{
	case VK_TAB:
		if (ai.act==0) break;	//edituje text
		if (shift)
		{
			//if (ai.act>0) ai.act--;	else ai.act=2;
			ai.act=0;	//Shift+TAB => Name
		}
		else
		{
			if (ai.act<3) ai.act++;	else ai.act=1; //TAB 1,2,3
		}
		m_undo.Separator();
		return 1;

	case VK_LEFT:
		if (shift)
		{
			ActiveInstrPrev();
			return 1;
		}
		break;

	case VK_RIGHT:
		if (shift)
		{
			ActiveInstrNext();
			return 1;
		}
		break;

	case VK_UP:
	case VK_DOWN:
		if (shift && !control) return 0;	//kombinace Shift+Control+UP/DOWN je pro edit ENVELOPE a TABLE povolena
		if (shift && control && ai.act==1) return 0;	//krom edit PARAM, tam neni povolena
		break;

	case VK_PAGE_UP:
		if (shift) 
		{
			OctaveUp();
			return 1;
		}
		break;

	case VK_PAGE_DOWN: 
		if (shift)
		{
			OctaveDown();
			return 1;
		}
		break;

	case VK_SUBTRACT:	//Numlock minus
		if (shift && control)	//S+C+numlock_minus  ...odecitani cele krivky s minimem 0
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
		if (shift && control)	//S+C+numlock_plus ...pricitani cele krivky s maximem f
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

	//a ted jen pro specialni casti

	if (ai.act==0)
	{
		//NAME
		if (IsnotMovementVKey(vk))
		{	//uklada undo jen kdyz to neni pohyb kurzoru
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
		}
		if ( EditText(vk,shift,control,ai.name,ai.activenam,INSTRNAMEMAXLEN) ) ai.act=1;

		return 1;
	}
	else
	if (ai.act==1)
	{
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
				//ap = (ap-1) & shpar[ai.activepar].pand;
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
				// ap = (ap+1) & shpar[ai.activepar].pand;
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
			
		case 8:		//VK_BACKSPACE:
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ap=0;
			goto ChangeInstrumentPar;
			return 1;

ChangeInstrumentPar:
			//protoze doslo k nejake zmene parametruuu instrumentu => stopne tento instrument na vsech generatorech
			Atari_InstrumentTurnOff(m_activeinstr);

			m_instrs.CheckInstrumentParameters(m_activeinstr);
			/*
			//kontrola ENVELOPE len-go smycky
			if (ai.par[PAR_ENVGO]>ai.par[PAR_ENVLEN]) ai.par[PAR_ENVGO]=ai.par[PAR_ENVLEN];
			//kontrola TABLE len-go smycky
			if (ai.par[PAR_TABGO]>ai.par[PAR_TABLEN]) ai.par[PAR_TABGO]=ai.par[PAR_TABLEN];
			//kontrola kurzoru v envelope
			if (ai.activeenvx>ai.par[PAR_ENVLEN]) ai.activeenvx=ai.par[PAR_ENVLEN];
			//kontrola kurzoru v table
			if (ai.activetab>ai.par[PAR_TABLEN]) ai.activetab=ai.par[PAR_TABLEN];
			*/
			//neco se zmenilo => Ulozeni instrumentu "do Atarka"
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;
		}
	}
	else
	if (ai.act==2)
	{
		//ENVELOPE
		switch(vk)
		{
		case VK_UP:
			if (control)
			{
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
				if (i==0 && g_tracks4_8<=4) i=7;	//mono rezim
			}
			else 
				i=7;
			ai.activeenvy=i;
			return 1;
			
		case VK_DOWN: 
			if (control)
			{
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
				if (i>0) i--; else i=ai.par[PAR_ENVLEN]; //delka env
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
				if (i<ai.par[PAR_ENVLEN]) i++; else i=0; //delka env
				ai.activeenvx=i;
			}
			return 1;
			
		case VK_HOME:		
			if (control)
			{
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ai.par[PAR_ENVGO] = ai.activeenvx; //nastavi ENVGO na tento sloupec
				goto ChangeInstrumentPar;	//ano, to je v poradku, opravdu zmenil PARAMETR, i kdyz je v envelope
			}
			else
			{
				//prejde doleva na 0.sloupec nebo na zacatek GO smycky
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
				if (ai.activeenvx==ai.par[PAR_ENVLEN])	//nastavi ENVLEN na tento sloupec nebo na konec
					ai.par[PAR_ENVLEN]=ENVCOLS-1;
				else
					ai.par[PAR_ENVLEN]=ai.activeenvx;
				goto ChangeInstrumentPar;	//ano, zmenil PAR z envelope
			}
			else
			{
				ai.activeenvx=ai.par[PAR_ENVLEN];	//prejde kursorem doprava na konec
			}
			return 1;
			
		case 8:			//VK_BACKSPACE:
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			ae=0;
			goto ChangeInstrumentEnv;
			return 1;

		case VK_SPACE:	//VK_SPACE
			{
			 m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			 for (int j=0; j<ENVROWS; j++) ai.env[ai.activeenvx][j]=0;
			 if (ai.activeenvx<ai.par[PAR_ENVLEN]) ai.activeenvx++; //posun doprava
			}
			goto ChangeInstrumentEnv;
			return 1;

		case VK_INSERT:
			if (!control)
			{	//posune envelope od aktualni pozice doprava
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
				//vylepseni: se shiftem to necha tam (nebude promazavat sloupec)
				if (!shift) for (j=0; j<ENVROWS; j++) ai.env[ai.activeenvx][j]=0;
				ai.par[PAR_ENVLEN]=ele;
				ai.par[PAR_ENVGO]=ego;
				goto ChangeInstrumentPar;	//zmenil parametry delky a/nebo go
			}
			return 0; //bez screen update

		case VK_DELETE:
			if (!control)	//!shift &&
			{	//posune envelope od aktualni pozice doleva
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
				goto ChangeInstrumentPar;	//zmenil parametry delky a/nebo go
			}
			return 0; //bez screen update

ChangeInstrumentEnv:
			//neco se zmenilo => Ulozeni instrumentu "do Atarka"
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;
		}
	}
	if (ai.act==3)
	{
		//TABLE
		switch(vk)
		{
		case VK_HOME:
			if (control)
			{	//nastavi sem TABLE go smycku
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				ai.par[PAR_TABGO] = ai.activetab;
				if (ai.activetab>ai.par[PAR_TABLEN]) ai.par[PAR_TABLEN] = ai.activetab;
				goto ChangeInstrumentPar;
			}
			else
			{
				//prechod na zacatek TABLE a na zacatek TABLE smycky
				if (ai.activetab!=0)
					ai.activetab=0;
				else
					ai.activetab = ai.par[PAR_TABGO];
			}
			return 1;

		case VK_END:
			if (control)
			{	//nastavi TABLE len podle mista
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				if (ai.activetab==ai.par[PAR_TABLEN])
					ai.par[PAR_TABLEN]=TABLEN-1;
				else
					ai.par[PAR_TABLEN] = ai.activetab;
				goto ChangeInstrumentPar;
			}
			else	//prejde na posledni parametr v TABLE
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

		case VK_SPACE:	//VK_SPACE: vynulovani parametru a posun o 1 doprava
			if (ai.activetab<ai.par[PAR_TABLEN]) ai.activetab++;
			//a pokracuje stejnym jako VK_BACKSPACE
		case 8:			//VK_BACKSPACE: vynulovani parametru
			m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
			at=0;
			goto ChangeInstrumentTab;

		case VK_INSERT:
			if (!control)
			{	//posune table od aktualni pozice doprava
				m_undo.ChangeInstrument(m_activeinstr,0,UETYPE_INSTRDATA);
				int i;
				int tle=ai.par[PAR_TABLEN];
				int tgo=ai.par[PAR_TABGO];
				if (tle<TABLEN-1) tle++;
				if (ai.activetab<tgo && tgo<TABLEN-1) tgo++;
				for(i=TABLEN-2; i>=ai.activetab; i--) ai.tab[i+1]=ai.tab[i];
				if (!shift) ai.tab[ai.activetab]=0; //se shiftem to tam necha
				ai.par[PAR_TABLEN]=tle;
				ai.par[PAR_TABGO]=tgo;
				//goto ChangeInstrumentTab; <-- to nestaci!
				goto ChangeInstrumentPar; //zmenil TABLE LEN nebo GO, musi stopnout instrument
			}
			return 0; //bez screen update

		case VK_DELETE:
			if (!control) //!shift &&
			{	//posune table od aktualni pozice doleva
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
				//goto ChangeInstrumentTab; <-- to nestaci!
				goto ChangeInstrumentPar; //zmenil TABLE LEN nebo GO, musi stopnout instrument
			}
			return 0; //bez screen update

ChangeInstrumentTab:
			//neco se zmenilo => Ulozeni instrumentu "do Atarka"
			m_instrs.ModificationInstrument(m_activeinstr);
			return 1;

		}
	}

	return 0;	//=> nebude se delat SCREENUPDATE
}

BOOL CSong::InfoCursorGotoSongname(int x)
{
	x = x/8;
	if (x>=0 && x<SONGNAMEMAXLEN)
	{
		m_songnamecur = x;
		g_activepart=PARTINFO;
		m_infoact = 0;
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
	return 1;
}

BOOL CSong::InfoCursorGotoOctaveSelect(int x, int y)
{
	COctaveSelectDlg dlg;
	CRect rec;
	//AfxGetApp()->GetMainWnd()->GetActiveWindow()->GetWindowRect(&rec);
	//dlg.m_pos.x=rec.left+x-64-5;	//-32;
	//dlg.m_pos.y=rec.top+y+58; //32;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-9,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_octave=m_octave;
	g_mousebutt=0;				//protoze dialog sezere OnLbuttonUP udalost
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
	//AfxGetApp()->GetMainWnd()->GetWindowRect(&rec);
	//int px=rec.left+x-64-5;	//-32;
	//if (px<0) px=0;
	//dlg.m_pos.x=px;
	//dlg.m_pos.y=rec.top+y+58; //32;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-9,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_volume = m_volume;
	dlg.m_respectvolume = g_respectvolume;

	g_mousebutt=0;				//protoze dialog sezere OnLbuttonUP udalost
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
	CInstrumentSelectDlg dlg;
	CRect rec;
	//AfxGetApp()->GetMainWnd()->GetWindowRect(&rec);
	//int px=rec.left+x-64-5-32-10-30;	//-32;
	//if (px<0) px=0;
	//dlg.m_pos.x=px;
	//dlg.m_pos.y=rec.top+y+58; //32;
	::GetWindowRect(g_viewhwnd,&rec);
	dlg.m_pos=rec.TopLeft()+CPoint(x-64-82,y-7);
	if (dlg.m_pos.x<0) dlg.m_pos.x=0;
	dlg.m_selected = m_activeinstr;
	dlg.m_instrs = &m_instrs;	//ukazatel na objekt instrumentuuu

	g_mousebutt=0;				//protoze dialog sezere OnLbuttonUP udalost
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

	if (Numblock09Key(vk)>=0)
	{
		if (shift)
		{
			ActiveInstrSet(Numblock09Key(vk));
			return 1;
		}
	}

	note = NoteKey(vk);
	if (note>=0)
	{
		i=note+m_octave*12;
		if (i>=0 && i<NOTESNUM)		//jenom v mezich
		{
			SetPlayPressedTonesTNIV(m_trackactivecol,i,m_activeinstr,m_volume);
			if ((control || g_prove==2) && g_tracks4_8>4)
			{	//s controlem nebo v prove2 => stereo test
				SetPlayPressedTonesTNIV((m_trackactivecol+4)&0x07,i,m_activeinstr,m_volume);
			}
		}
		return 0; //nemusi prekreslovat
	}

	switch(vk)
	{
	case VK_LEFT:
		if (shift)
			ActiveInstrPrev();
		else
			TrackLeft(1);
		break;

	case VK_RIGHT:
		if (shift)
			ActiveInstrNext();
		else
			TrackRight(1);
		break;

	case VK_TAB:
		if (shift)
			TrackLeft(1);
		else
			TrackRight(1);
		break;


	case VK_SPACE:
		SetPlayPressedTonesTNIV(m_trackactivecol,-1,-1,0);
		if ((control || g_prove==2) && g_tracks4_8>4)
		{	//s controlem nebo prove2 = stereo test
			SetPlayPressedTonesTNIV((m_trackactivecol+4)&0x07,-1,-1,0);
		}
		return 0;	//nemusi prekreslovat
		//break;

	case VK_SUBTRACT:
		VolumeDown();
		//if (control) SetPlayPressedTonesV(m_trackactivecol,m_volume);
		break;

	case VK_ADD:
		VolumeUp();
		//if (control) SetPlayPressedTonesV(m_trackactivecol,m_volume);
		break;

	case VK_PAGE_UP:
		OctaveUp();
		break;

	case VK_PAGE_DOWN:
		OctaveDown();
		break;

	default:
		return 0;
		break;
	}
	return 1;
}


BOOL CSong::TrackKey(int vk,int shift,int control)
{

#define VKX_SONGINSERTLINE	73		//VK_I
#define VKX_SONGDELETELINE	85		//VK_U
#define VKX_SONGDUPLICATELINE 79	//VK_O
#define VKX_SONGPREPARELINE 80		//VK_P
#define VKX_SONGPUTNEWTRACK 78		//VK_N
#define VKX_SONGMAKETRACKSDUPLICATE 77	//VK_M

	int note,i,j;

	if (g_trackcl.IsBlockSelected() && SongGetActiveTrack()!=g_trackcl.m_seltrack) BLOCKDESELECT;

	/* 1.01
	if (NumbKey(vk)>=0)
	{
		if (control)
			return SongTrackSetByNum(NumbKey(vk));
	}
	*/

	if (SongGetGo()>=0) //je aktivni song go radek => nesmi nic editovat
	{
		if (!control && vk==VK_UP)  //GO - key up
		{ 
			m_trackactiveline=0;
			TrackUp();
			return 1;
		}
		if (!control && vk==VK_DOWN) //GO - key down
		{ 
			m_trackactiveline=m_tracks.m_maxtracklen-1;
			TrackDown(1,0); 
			return 1;
		}
		if (!control && !shift) return 0;
		if (control && (vk==8 || vk==71) ) //control+backspace nebo control+G
		{
			SongTrackGoOnOff();
			return 1;
		}
		if (control && !shift && (vk==VKX_SONGINSERTLINE || vk==VKX_SONGDELETELINE || vk==VKX_SONGPREPARELINE || vk==VKX_SONGDUPLICATELINE || vk==VK_PAGE_UP || vk==VK_PAGE_DOWN)) goto TrackKeyOk;
		if (vk!=VK_LEFT && vk!=VK_RIGHT && vk!=VK_UP && vk!=VK_DOWN) return 0;
	}
TrackKeyOk:


	switch (m_trackactivecur)
	{
	case 0: //sloupec not
		if (control) break;		//s controlem se noty nezadavaji (breakem pokracuje dal)
		note = NoteKey(vk);
		if (note>=0)
		{
			i=note+m_octave*12;
			if (i>=0 && i<NOTESNUM)		//jenom v mezich
			{
				BLOCKDESELECT;
				//Quantization
				if ( m_play && m_followplay && (m_speeda<(m_speed/2)) )
				{
					m_quantization_note=i;
					m_quantization_instr=m_activeinstr;
					m_quantization_vol=m_volume;
					return 1;
				}
				// konec Quantization
				if (TrackSetNoteActualInstrVol(i) )
				{
					SetPlayPressedTonesTNIV(m_trackactivecol,i,m_activeinstr,TrackGetVol());
					if (!(m_play && m_followplay)) TrackDown(g_linesafter);
				}
			}
			return 1;
		}
		else //cislama 1-6 na numeraku se prepisuje oktava
		if ( (j=Numblock09Key(vk))>=1 && j<=6 && m_trackactiveline<=TrackGetLastLine())
		{
			note = TrackGetNote();
			if (note>=0)		//je tam nejaka nota?
			{
				BLOCKDESELECT;
				note = (note % 12) + ((j-1)*12);		//zmeni jeji oktavu podle stlaceneho cisla na numblocku
				if (note>=0 && note<NOTESNUM)
				{
					int instr=TrackGetInstr(),vol=TrackGetVol();
					if (TrackSetNoteInstrVol(note,instr,vol))
						SetPlayPressedTonesTNIV(m_trackactivecol,note,instr,vol);
				}
			}
			if (!(m_play && m_followplay)) TrackDown(g_linesafter);
			return 1;
		}

		break;

	case 1: //sloupec instrumentu
		i = NumbKey(vk);
		if (i>=0 && !shift && !control)
		{
			BLOCKDESELECT;
			if (TrackGetNote()>=0) //cislo instrumentu lze menit jen pouze je-li tam nota
			{
				j= ((TrackGetInstr()&0x0f)<<4) | i;
				if (j>=INSTRSNUM) j &= 0x0f;	//ponecha jen dolni cifru
				TrackSetInstr(j);
			}
			return 1;
		}
		break;
	
	case 2: //sloupec volume
		i = NumbKey(vk);
		if (i>=0 && !shift && !control)
		{
			BLOCKDESELECT;
			if ( TrackSetVol(i) && !(m_play && m_followplay) ) TrackDown(g_linesafter);
			return 1;
		}
		break;

	case 3: //sloupec speed
		i = NumbKey(vk);
		if (i>=0 && !shift && !control)
		{
			BLOCKDESELECT;
			j = TrackGetSpeed();
			if (j<0) j=0;
			j= (( j & 0x0f )<<4) | i;
			if (j>=TRACKMAXSPEED) j &= 0x0f;	//ponecha jen dolni cifru
			if (j<=0) j= -1;	//nulova neexistuje
			TrackSetSpeed(j);
			return 1;
		}
		break;

	}


	switch(vk)
	{
	case VK_UP:
		if (shift)
		{
			//selektovani bloku
			BLOCKSETBEGIN;
			TrackUp();
			BLOCKSETEND;
		}
		else
		{
			BLOCKDESELECT;
			if (control)
				SongUp();
			else
				TrackUp();
		}
		break;

	case VK_DOWN: 
		if (shift)
		{
			//selektovani bloku
			BLOCKSETBEGIN;
			TrackDown(1,0);	//nebude zastavovat na poslednim radku
			BLOCKSETEND;
		}
		else
		{
			BLOCKDESELECT;
			if (control)
				SongDown();
			else
				TrackDown(1,0);	//stoponlastline=0 =>nebude zastavovat na poslednim radku tracku
		}
		break;

	case VK_LEFT:
		if (shift)
		{
			if (ISBLOCKSELECTED && control)
			{
				//zmeny instrumentu v bloku
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
				g_trackcl.BlockInstrumentChange(m_activeinstr,-1);
			}
			else
				ActiveInstrPrev();
		}
		else
		{
			BLOCKDESELECT;
			if (control)
				SongTrackDec();
			else
				TrackLeft();
		}
		break;

	case VK_RIGHT:
		if (shift)
		{
			if (ISBLOCKSELECTED && control)
			{
				//zmeny instrumentu v bloku
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
				g_trackcl.BlockInstrumentChange(m_activeinstr,1);
			}
			else
				ActiveInstrNext();
		}
		else
		{
			BLOCKDESELECT;
			if (control)
				SongTrackInc();
			else
				TrackRight();
		}
		break;

	case VK_PAGE_UP:
		if (shift)
		{
			if (ISBLOCKSELECTED && control) 
			{
				//transpozice v bloku nahoru (k vyssim notam)
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
				g_trackcl.BlockNoteTransposition(m_activeinstr,1);
			}
			else
				OctaveUp();
		}
		else
		if (control)
		{
			//posun na zacatek za goto
			SongSubsongPrev();
		}
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
		if (shift)
		{
			if (ISBLOCKSELECTED && control)
			{
				//transpozice v bloku dolu (k nizsim notam)
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
				g_trackcl.BlockNoteTransposition(m_activeinstr,-1);
			}
			else
				OctaveDown();
		}
		else
		if (control)
		{
			//posun za prvni goto
			SongSubsongNext();
		}
		else
		{
			BLOCKDESELECT;
			i = ((m_trackactiveline+g_tracklinehighlight) / g_tracklinehighlight) * g_tracklinehighlight;
			if (i>=m_tracks.m_maxtracklen) i = m_tracks.m_maxtracklen-1;
			m_trackactiveline = i;
		}
		break;

	case VK_SUBTRACT:
		if (shift && ISBLOCKSELECTED && control)
		{
			//zeslabovani v bloku
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
			g_trackcl.BlockVolumeChange(m_activeinstr,-1);
		}
		else
			VolumeDown();
		break;

	case VK_ADD:
		if (shift && ISBLOCKSELECTED && control)
		{
			//zesilovani v bloku
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA);
			g_trackcl.BlockVolumeChange(m_activeinstr,1);
		}
		else
			VolumeUp();
		break;

	case VK_TAB:
		BLOCKDESELECT;
		if (shift)
			TrackLeft(1);
		else
			TrackRight(1);
		break;

	
	case 65:	//VK_A
		if (g_trackcl.IsBlockSelected() && shift && control)
		{	//Shift+control+A
			//prepinani ALL / no ALL
			g_trackcl.BlockAllOnOff();
		}
		else
		if (control && !shift)
		{
			//control+A
			//selektovani celeho tracku (od 0 po delku toho tracku)
			g_trackcl.BlockDeselect();
			g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),0);
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
		if (control)
		{
Block_copy:
			if (!g_trackcl.IsBlockSelected())
			{	//kdyz neni vybran blok, udela blok na aktualnim miste
				g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline);
				g_trackcl.BlockSetEnd(m_trackactiveline);
			}
			g_trackcl.BlockCopyToClipboard();
		}
		break;

	case 69:	//VK_E
		if (control)		//exchange block and clipboard
		{
			if (g_trackcl.IsBlockSelected()) 
			{
				m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
				if (!g_trackcl.BlockExchangeClipboard()) m_undo.DropLast();
			}
		}
		break;

	case 86:	//VK_V
		if (control)
		{
Block_paste:
			BlockPaste();	//klasicke paste
		}
		break;

	case 88:	//VK_X
		if (control && g_trackcl.IsBlockSelected())
		{
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			g_trackcl.BlockCopyToClipboard();
			g_trackcl.BlockClear();
		}
		break;

	case 70:	//VK_F
		if (control && g_trackcl.IsBlockSelected())
		{
			//g_controlkey=1;			//aby se nezapisovali noty pres MIDI
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			if (!g_trackcl.BlockEffect())
				m_undo.DropLast();
			//g_controlkey=0;			//protoze dialog "sezere" pusteni Control key (pri Control+F)
		}
		break;

	case 90:	//VK_Z		//speed sloupec
		if (control) CursorToSpeedColumn();	//control+Z => speed sloupec
		break;

	case 71:	//VK_G		//song goto on/off
		BLOCKDESELECT;
		if (control) SongTrackGoOnOff();	//control+G => goto on/off line v songu
		break;

	case VKX_SONGINSERTLINE:	//VK_I:
		BLOCKDESELECT;
		if (control)
			SongInsertLine(m_songactiveline);
		break;
	
	case VKX_SONGDELETELINE:	//VK_U:
		BLOCKDESELECT;
		if (control)
			SongDeleteLine(m_songactiveline);
		break;

	case VKX_SONGDUPLICATELINE:	//VK_O:
		//BLOCKDESELECT; dela se uvnitr SongInsertCopyOrCloneOfSongLines jen kdyz zvoli OK.
		if (control)
			SongInsertCopyOrCloneOfSongLines(m_songactiveline);
		break;

	case VKX_SONGPREPARELINE:	//VK_P
		BLOCKDESELECT;
		if (control)
			SongPrepareNewLine(m_songactiveline);
		break;

	case VKX_SONGPUTNEWTRACK:	//VK_N
		BLOCKDESELECT;
		if (control)
			SongPutnewemptyunusedtrack();
		break;

	case VKX_SONGMAKETRACKSDUPLICATE:	//VK_M
		BLOCKDESELECT;
		if (control)
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
				m_trackactiveline = 0;		//na 0.radek
				BLOCKSETEND;
			}
			else
			{
				if (g_trackcl.IsBlockSelected())
				{
					//nastavi na prvni radek v bloku
					int bfro,bto;
					g_trackcl.GetFromTo(bfro,bto);
					m_trackactiveline=bfro;
				}
				else
				{
					if (m_trackactiveline!=0)
						m_trackactiveline = 0;		//na 0.radek
					else
					{
						i=TrackGetGoLine();
						if (i>=0) m_trackactiveline = i;	//na zacatek GO smycky
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
				m_trackactiveline = TrackGetLastLine();
				BLOCKSETEND;
			}
			else
			{
				if (g_trackcl.IsBlockSelected())
				{
					//nastavi na prvni radek v bloku
					int bfro,bto;
					g_trackcl.GetFromTo(bfro,bto);
					m_trackactiveline=bto;
				}
				else
				{
					if (SongGetActiveTrack()<0)
						m_trackactiveline = m_tracks.m_maxtracklen-1; //neni tam zadny track, takze skok uplne dolu
					else
						m_trackactiveline = TrackGetLastLine();
					BLOCKDESELECT;
				}
			}
		}
		break;

	case 13:		//VK_ENTER:
		{
			int instr,vol;
			if ( (BOOL)control != (BOOL)g_keyboard_swapenter)	//control+Enter => hraje cely radek (vsechny tracky)
			{
				//pro vsechny sloupce tracku krom aktivniho sloupce tracku
				for(i=0; i<g_tracks4_8; i++)
				{
					if (i!=m_trackactivecol)
					{
						TrackGetLoopingNoteInstrVol(m_song[m_songactiveline][i],note,instr,vol);
						if (note>=0)		//je tam nejaka nota?
							SetPlayPressedTonesTNIV(i,note,instr,vol);	//prehraje ji tak jak tam je
						else
						if (vol>=0) //neni tam nota, ale je tam samostatne volume?
							SetPlayPressedTonesV(i,vol);				//nastavi tu hlasitost tak jak tam je
					}
				}
			}

			//a ted pro ten aktivni sloupec tracku
			TrackGetLoopingNoteInstrVol(SongGetActiveTrack(),note,instr,vol);
			if (note>=0)		//je tam nejaka nota?
			{
				SetPlayPressedTonesTNIV(m_trackactivecol,note,instr,vol);	//prehraje ji tak jak tam je
				if (shift)	//se shiftem si navic tento instrument a volume "nabere" jako aktualni (jen neni-li 0)
				{
					ActiveInstrSet(instr);
					if (vol>0) m_volume = vol;
				}
			}
			else
			if (vol>=0) //neni tam nota, ale je tam samostatne volume?
			{
				SetPlayPressedTonesV(m_trackactivecol,vol); //nastavi tu hlasitost
				if (shift && vol>0) m_volume = vol; //"nabere" si tu volume jako aktualni (jen neni-li 0)
			}
		}
		TrackDown(1,0);	//stoponlastline=0 => jede dal

		//pokud je selektnuty blok, pohybuje se (a playuje) pouze v nem
		if (g_trackcl.IsBlockSelected())
		{
			int bfro,bto;
			g_trackcl.GetFromTo(bfro,bto);
			if (m_trackactiveline<bfro || m_trackactiveline>bto) m_trackactiveline=bfro;
		}
		break;

	case VK_INSERT:
		if (control)
		{
			goto Block_copy;
		}
		else
		if (shift)
		{
			goto Block_paste;
		}
		else
		{
			BLOCKDESELECT;
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,0);
			m_tracks.InsertLine(SongGetActiveTrack(),m_trackactiveline);
		}
		break;

	case VK_DELETE:
		if (g_trackcl.IsBlockSelected())
		{
			//je selektovany blok, takze ho smaze
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,1);
			g_trackcl.BlockClear();
		}
		else
		if (!shift)
		{
			BLOCKDESELECT;
			m_undo.ChangeTrack(SongGetActiveTrack(),m_trackactiveline,UETYPE_TRACKDATA,0);
			m_tracks.DeleteLine(SongGetActiveTrack(),m_trackactiveline);
		}
		break;


	case VK_SPACE:
		BLOCKDESELECT;
		if (TrackDelNoteInstrVolSpeed(1+2+4+8)) //vsechny
		{
			if (!(m_play && m_followplay)) TrackDown(g_linesafter);
		}
		break;

	case 8:			//VK_BACKSPACE:
		BLOCKDESELECT;
		if (control)
		{
			int isgo = (m_songgo[m_songactiveline]>=0)? 1:0;
			if (isgo) 
				SongTrackGoOnOff();	//Go off
			else
				SongTrackEmpty();
		}
		else
		{
			int r=0;
			switch(m_trackactivecur)
			{
			case 0:	// nota
			case 1: // instrument
				r=TrackDelNoteInstrVolSpeed(1+2); //smaz notu+instrument
				break;
			case 2: // volume
				r=TrackDelNoteInstrVolSpeed(1+2+4); //smaz notu+instrument+volume
				break;
			case 3: // speed
				r=TrackSetSpeed(-1);
				break;
			}
			if (r)
			{
				if (!(m_play && m_followplay)) TrackDown(1);
			}
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
	xch=(point.x/(11*8));
	x=(point.x-(xch*11*8))/8;
	y=(point.y+0)/TRACK_LINE_H+g_cursoractview;	//m_trackactiveline;
	if (y>=0 && y<m_tracks.m_maxtracklen)
	{
		if (xch>=0 && xch<g_tracks4_8) m_trackactivecol=xch;
		m_trackactiveline=y;
	}
	else
		return 0;
	switch(x)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			m_trackactivecur=0;
			break;
		case 4:
		case 5:
		case 6:
			m_trackactivecur=1;
			break;
		case 7:
			m_trackactivecur=2;
			break;
		case 8:
		case 9:
		case 10:
			m_trackactivecur=3;
			break;
	}
	g_activepart=PARTTRACKS;
	return 1;
}


BOOL CSong::TrackUp()
{
	m_undo.Separator();
	m_trackactiveline--;
	if (m_trackactiveline<0)
	{
		if (!g_keyboard_updowncontinue)
			m_trackactiveline=m_tracks.m_maxtracklen-1;
		else
		{
			SongUp();
			m_trackactiveline=TrackGetLastLine();
			if (m_trackactiveline<0 || TrackGetGoLine()>=0) m_trackactiveline=m_tracks.m_maxtracklen-1;
		}
	}
	return 1;
}

BOOL CSong::TrackDown(int lines, BOOL stoponlastline)
{
	if (!g_keyboard_updowncontinue && stoponlastline && m_trackactiveline+lines>TrackGetLastLine()) return 0;
	m_undo.Separator();
	m_trackactiveline+=lines;	//m_trackactiveline++;
	if (!g_keyboard_updowncontinue)
	{
		if (m_trackactiveline>=m_tracks.m_maxtracklen) m_trackactiveline= m_trackactiveline % m_tracks.m_maxtracklen; //=0;
	}
	else
	{
		if (SongGetActiveTrack()>=0 && TrackGetGoLine()<0)
		{
			int trlen=TrackGetLastLine()+1;
			if (m_trackactiveline>=trlen) { m_trackactiveline=m_trackactiveline % trlen; SongDown(); }	//=0;
		}
		else
		{
			if (m_trackactiveline>=m_tracks.m_maxtracklen) { m_trackactiveline= m_trackactiveline % m_tracks.m_maxtracklen; SongDown(); }	//=0
		}
	}
	return 1;
}

BOOL CSong::TrackLeft(BOOL column)
{
	m_undo.Separator();
	if (column || m_trackactiveline>TrackGetLastLine()) goto track_leftcolumn;
	m_trackactivecur--;
	if (m_trackactivecur==1 && TrackGetNote()<0) //neni-li nota, preskakuje sloupec instrumentu
		m_trackactivecur=0;
	else
	if (m_trackactivecur<0)
	{
		m_trackactivecur=2;
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
	if (m_trackactivecur==1 && TrackGetNote()<0) //neni-li nota, preskakuje sloupec instrumentu
		m_trackactivecur=2;
	else
	if (m_trackactivecur>2)
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
	//preda aktualni notu s prihlednutim na pripadnou goto smycku
	//int track = SongGetActiveTrack();
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

/*
void CSong::TrackGetPosition(TNoteState *state)
{
	state->tracknum=SongGetActiveTrack();
	state->songline=m_songactiveline;
	state->trackline=m_trackactiveline;
	state->trackcol=m_trackactivecol;
	state->trackcur=m_trackactivecur;
}

void CSong::TrackGetNoteDataByPosition(TNoteState *dest, TNoteState *pos)
{
	int tracknum=SongGetTrack(pos->songline,pos->trackcol);
	if (tracknum>=0)
	{
		TTrack& t = *m_tracks.GetTrack(tracknum);
		int i=pos->trackline;
		dest->data[0]=tracknum;
		dest->data[1]=t.note[i];
		dest->data[2]=t.instr[i];
		dest->data[3]=t.volume[i];
		dest->data[4]=t.speed[i];
	}
	else
	{
		dest->data[0]=dest->data[1]=dest->data[2]=dest->data[3]=dest->data[4]=-1;
	}
}

void CSong::TrackSetByNoteState(TNoteState *state)
{
	int tracknum=state->data[0];
	m_song[state->songline][state->trackcol]=tracknum; //song
	if (tracknum>=0)
	{
		TTrack& t = *m_tracks.GetTrack(tracknum);
		int i=state->trackline;
		t.note[i]=state->data[1];
		t.instr[i]=state->data[2];
		t.volume[i]=state->data[3];
		t.speed[i]=state->data[4];
	}
	//nastavi kurzor na pozici
	m_songactiveline=state->songline;
	m_trackactiveline=state->trackline;
	m_trackactivecol=state->trackcol;
	m_trackactivecur=state->trackcur;
}
*/

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
		//=in->activenam; Vynechava, aby kazda zmena pozice kurzoru v nazvu nebyla duvodem ke undo separaci
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
		//ostatni parametry 1-5 jsou v ramci instrumentu (struktury TInstrument),
		//takze neni nutno nastavovat
		g_activepart=g_active_ti=PARTINSTRS;
		break;

	case PARTINFO:
		m_infoact = cursor[0];
		g_activepart=PARTINFO;
		break;

	default:
		return;	//nemeni g_activepart !!!

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

	if (NumbKey(vk)>=0)
	{
		//if (control)	- v songu je to i s controlem i primo jen cislo bez controlu
		return SongTrackSetByNum(NumbKey(vk));
	}

	switch(vk)
	{
	case VK_UP:
		SongUp();
		break;

	case VK_DOWN: 
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

	case VKX_SONGMAKETRACKSDUPLICATE:	//Control+VK_M
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
		if (control)
			SongSubsongPrev();
		else
		{
			if (m_songactiveline>=g_songlines) 
				m_songactiveline-=g_songlines;
			else
				m_songactiveline=0;
		}
		break;

	case VK_PAGE_DOWN:
		if (control)
			SongSubsongNext();
		else
		{
			if (m_songactiveline<SONGLEN-g_songlines) 
				m_songactiveline+=g_songlines;
			else
				m_songactiveline=SONGLEN-1;
		}
		break;

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
	//x=(point.x-(xch*3*8))/8;
	int top_line = TopLine();

	y = (point.y+0)/SONG_LINE_H+top_line;

	if (y>=0 && y<SONGLEN)
	{
		if (xch>=0 && xch<g_tracks4_8) m_trackactivecol=xch;
		if (y!=m_songactiveline)
		{
			m_songactiveline=y;
			g_activepart=PARTSONG;
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
	m_undo.Separator();
	m_songactiveline--;
	if (m_songactiveline<0) m_songactiveline=SONGLEN-1;
	return 1;
}

BOOL CSong::SongDown()
{
	m_undo.Separator();
	m_songactiveline++;
	if (m_songactiveline>=SONGLEN) m_songactiveline=0;
	return 1;
}

BOOL CSong::SongSubsongPrev()
{
	m_undo.Separator();
	int i;
	i=m_songactiveline-1;
	if (m_trackactiveline==0) i--;	//je na 0.radku => hledat o 1 songline driv
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
				m_songactiveline=SONGLEN-1; //Goto na poslednim songline (=> neni mozno nastavit radek pod nim!)
			m_trackactiveline=0;
			break;
		}
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
	{	//zmeni track
		i = SongGetActiveTrack();
		if (i<0) i=0;
		i &= 0x0f;	//jen dolni cifra
		i = (i<<4) | num;
		if ( i >= TRACKSNUM ) i &= 0x0f;
		return SongTrackSet(i);
	}
	else
	{	//zmeni GO parametr
		i = m_songgo[m_songactiveline];
		if (i<0) i=0;
		i &= 0x0f;	//jen dolni cifra
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
	{	//je tam GO
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
	{	//je tam GO
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
	for(int i=0; i<line; i++)
	{
		if (m_songgo[i]>=line) m_songgo[i]++;
	}
	if (IsBookmark() && m_bookmark.songline>=line)
	{
		m_bookmark.songline++;
		if (m_bookmark.songline>=SONGLEN) ClearBookmark(); //prave vystrcil bookmark az pryc ven ze songu => zrusit bookmark
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
	for(int i=0; i<line; i++)
	{
		if (m_songgo[i]>line) m_songgo[i]--;
	}
	for(j=0; j<g_tracks4_8; j++) m_song[SONGLEN-1][j] = -1;
	m_songgo[SONGLEN-1] = -1;
	if (IsBookmark() && m_bookmark.songline>=line)
	{
		m_bookmark.songline--;
		if (m_bookmark.songline<line) ClearBookmark(); //prave smazal songlajnu s bookmarkem
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
	BLOCKDESELECT;					//blok se deselektuje jen kdyz da OK

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

		SongInsertLine(des);	//vlozeny prazdny radek

		if (dlg.m_clone && !sngo)
		{
			//klonuje
			//SongPrepareNewLine(des,sou,0);	//prazdne sloupce vynechava

			m_undo.Separator(-1); //pridruzi predchozi insert lines k nasledujici zmene
			m_undo.ChangeTrack(0,0,UETYPE_TRACKSALL,1); //se separatorem

			for(j=0; j<g_tracks4_8; j++)
			{
				k = m_song[sou][j]; //puvodni track
				d = -1;				//vysledny track (pocatecni inicializace)
				if (k<0) continue;  //je tam --
				if (clonedto[k]>=0) 
				{
					d=clonedto[k];	//tento jiz byl naklonovan, takze ho tez pouzije
				}
				else
				{
					d = FindNearTrackBySongLineAndColumn(sou,j,tracks);
					if (d>=0)
					{
						tracks[d]=TF_USED;
						clonedto[k]=d;
						TrackCopyFromTo(k,d);
						//upravit naklonovany track dle dlg.m_tuning a dlg.m_volumep
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
			//kopiruje
			m_songgo[des]=m_songgo[sou];
			for(j=0; j<g_tracks4_8; j++) m_song[des][j]=m_song[sou][j];
		}
	}

	return 1;
}

BOOL CSong::SongPrepareNewLine(int& line,int sourceline,BOOL alsoemptycolumns) //Vlozi songline s nepouzitymi prazdnymi tracky
{
	int i,k;

	if (sourceline<0) sourceline=line+sourceline; // pro -1 je to line-1

	SongInsertLine(line);	//vlozi prazdny radek

	//pripravi na line sadu nepouzitych prazdnych tracku

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
			//nasel vychozi track t
			for(k=t+1; k<TRACKSNUM; k++)
			{
				if (arrayTRACKSNUM[k]==0) return k;
			}
			//protoze nenasel zadny za nim, tak zkusi hledat pred nim
			for(k=t-1; k>=0; k--)
			{
				if (arrayTRACKSNUM[k]==0) return k;
			}
		}
	}
	//bude hledat prvni pouzitelny od zacatku
	for(k=0; k<TRACKSNUM; k++)
	{
		if (arrayTRACKSNUM[k]==0) return k;
	}
	return -1;
}

BOOL CSong::SongPutnewemptyunusedtrack()
{
	int line = SongGetActiveLine();
	if (m_songgo[line]>=0) return 0;		//nelze to udelat na "GO TO LINE" radku

	m_undo.ChangeSong(line,m_trackactivecol,UETYPE_SONGTRACK,0);

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	int k=-1;
	m_song[line][cl]=-1;	//na aktualni pozici v songu da --

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
	if (m_songgo[line]>=0) return 0;		//nelze to udelat na "GO TO LINE" radku

	int cl = GetActiveColumn();
	int act = m_song[line][cl];
	if (act<0) return 0;			//nelze duplikovat, neni tam zvolen zadny track

	m_undo.ChangeSong(line,cl,UETYPE_SONGTRACK,-1); //jen cast

	int k=-1;
	m_song[line][cl]=-1;	//na aktualni pozici v songu da --

	BYTE tracks[TRACKSNUM];
	memset(tracks,0,TRACKSNUM); //init
	MarkTF_USED(tracks);
	MarkTF_NOEMPTY(tracks);

	if (!(tracks[act]&TF_USED) )
	{
		//neni nikde jinde pouzity
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

	//zkopiruje zdrojovy track act do k
	TrackCopyFromTo(act,k);

	m_song[line][cl]=k;

	return 1;	
}


//--clipboardove funkce

void CSong::TrackCopy()
{
	int i = SongGetActiveTrack();
	if (i<0 || i>=TRACKSNUM) return;

	TTrack& at = *m_tracks.GetTrack(i);
	TTrack& tot = g_trackcl.m_trackcopy;

	/*
	for(i=0; i<at.len; i++)
	{
		tot.note[i] = at.note[i];
		tot.instr[i] = at.instr[i];
		tot.volume[i] = at.volume[i];
		tot.speed[i] = at.speed[i];
	}
	tot.len = at.len;
	tot.go = at.go;
	*/
	memcpy((void*)(&tot),(void*)(&at),sizeof(TTrack));
}

void CSong::TrackPaste()
{
	if (g_trackcl.m_trackcopy.len<=0) return;
	int i = SongGetActiveTrack();
	if (i<0 || i>=TRACKSNUM) return;

	TTrack& fro = g_trackcl.m_trackcopy;
	TTrack& at = *m_tracks.GetTrack(i);

	/*
	for(i=0; i<fro.len; i++)
	{
		at.note[i] = fro.note[i];
		at.instr[i] = fro.instr[i];
		at.volume[i] = fro.volume[i];
		at.speed[i] = fro.speed[i];
	}
	at.len = fro.len;
	at.go = fro.go;
	*/
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
		//prenastavi zacatek bloku na toto misto
		g_trackcl.BlockDeselect();
		g_trackcl.BlockSetBegin(m_trackactivecol,SongGetActiveTrack(),m_trackactiveline);
		g_trackcl.BlockSetEnd(lastl);
		//posune aktualni line na posledni dolni radek pastnuteho bloku
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
	if (m_instrclipboard.act<0) return;	//nebyl jeste nikdy nicim naplnen

	int i = GetActiveInstr();

	m_undo.ChangeInstrument(i,0,UETYPE_INSTRDATA,1);

	TInstrument& ai = m_instrs.m_instr[i];

	Atari_InstrumentTurnOff(i); //vypne tento instrument na vsech generatorech

	int x,y;
	BOOL bl=0,br=0,ep=0;
	BOOL bltor=0,brtol=0;

	switch (special)
	{
	case 0: //normalni paste
		memcpy((void*)(&ai),(void*)(&m_instrclipboard),sizeof(TInstrument));
		ai.act=ai.activenam=0; //aby byl cursor na zacatku nazvu instrumentu
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
	case 4: //envelope pars
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
		for(x=ENVCOLS-2; x>=ai.activeenvx; x--) //posun
		{
			int i=x+sx;
			if (i>=ENVCOLS) continue;
			for(y=0; y<ENVROWS; y++) ai.env[i][y]=ai.env[x][y];
		}
		for(x=0; x<sx; x++) //vlozeni
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
	m_instrs.ModificationInstrument(i); //promitne do Atari RAM
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
					globallytimes++; //nejaka nota timto instrumentem => zapocita
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
	{	//iinfo!=NULL =>nastavi hodnoty
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
	{	//iinfo==NULL => ukaze dialog
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
		//schova vsechny tracky a cely song
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
		int track_column[TRACKSNUM];	//prvni vyskyt ve vybrane oblasti songu
		int track_line[TRACKSNUM];		//prvni vyskyt ve vybrane oblasti songu
		for(i=0; i<TRACKSNUM; i++) track_column[i]=track_line[i]=-1; //init
		
		int onlysomething=0;
		int trackcreated=0; //pocet nove vytvorenych songu
		int songchanges=0;	//pocet zmen v songu

		if (onlychannels>=0 || (onlysonglinefrom>=0 && onlysonglineto>=0))
		{
			if (onlychannels<=0) onlychannels=0xff; //vsechny
			if (onlysonglinefrom<0) onlysonglinefrom=0; //od zacatku
			if (onlysonglineto<0) onlysonglineto=SONGLEN-1; //po konec
			onlysomething=1;
			unsigned char r;
			for(j=0; j<SONGLEN; j++)
			{
				if (m_songgo[j]>=0) continue; //je tam goto
				for(i=0; i<g_tracks4_8; i++)
				{
					t=m_song[j][i];
					if (t<0 || t>=TRACKSNUM) continue;
					r=(onlychannels&(1<<i)) && j>=onlysonglinefrom && j<=onlysonglineto;
					track_yn[t]|= (r)? 1 : 2;	//1=yes, 2=no, 3=yesno (copy)
					if (r && track_column[t]<0)
					{
						//prvni vyskyt ve vybrane oblasti songu
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
			track_changeto[i]=-1; //inicializace
			if (onlysomething && ((track_yn[i]&1)!=1) ) continue; //chce zmenu jen nekterych a tento to neni

			TTrack& st=*m_tracks.GetTrack(i);
			TTrack at; //destination track
			//udela si kopii
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
			if (changes) //provedl nejake zmeny
			{
				if (track_yn[i]&2)
				{
					//track se vyskytuje i uvnitr i mimo oblast
					//vytvori novy track
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
					//zkopiruje zmenenou kopii (at) na novy track (nt)
					TTrack *nt=m_tracks.GetTrack(k);
					memcpy((void*)nt,(void*)(&at),sizeof(TTrack));
					trackcreated++;
					//aspon jednou ho da hned do songu
					//(kvuli hledani v songu pouzitych tracku)
					m_song[track_line[i]][track_column[i]]=k;
					songchanges++;
					//bude menit vsechny vyskyty
					track_changeto[i]=k;
				}
				else
				{
					//vyskytuje se jen uvnitr oblasti
					//zkopiruje zmenenou kopii (at) na puvodni track (st)
					memcpy((void*)(&st),(void*)(&at),sizeof(TTrack));
				}
			}

		}
		//nasledne zmeny v songu
		if (onlysomething)
		{
			for(j=0; j<SONGLEN; j++)
			{
				if (m_songgo[j]>=0) continue; //je tam goto
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
		if (m_songgo[sline]>=0) continue;	//go radek se vynechava

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
	Stop();	//zastavi zvuk
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
	Stop();	//zastavi zvuk

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
	//vypocita nejvetsi pouzitou delku tracku
	int so,i,max=1;
	for(so=0; so<SONGLEN; so++)
	{
		if (m_songgo[so]>=0) continue; //go to line vynechava
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
		//min=nejratsi delka tracku na teto songline
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
		//procisti
		for(j=tt.len; j<TRACKLEN; j++)
		{
			tt.note[j]=tt.instr[j]=tt.volume[j]=tt.speed[j]=-1;
		}
		//
		if (tt.len>=maxtracklen)
		{
			tt.go=-1; //zrusi GO
			tt.len=maxtracklen; //upravi delku
		}
	}
	if (m_trackactiveline>=maxtracklen) m_trackactiveline = maxtracklen-1;
	if (m_trackplayline >= maxtracklen) m_trackplayline = maxtracklen-1;
	m_tracks.m_maxtracklen=maxtracklen;
}

void CSong::TracksAllBuildLoops(int& tracksmodified,int& beatsreduced)
{
	Stop(); //zastavi zvuk

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
	Stop(); //zastavi zvuk

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
		//inicializace
		tracklen[i]=-1;
		trackused[i]=0;
	}

	for(int sline=0; sline<SONGLEN; sline++)
	{
		if (m_songgo[sline]>=0) continue;	//go radek se vynechava

		int nejkratsi=m_tracks.m_maxtracklen;
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			trackused[n]=1;
			TTrack& tr=*m_tracks.GetTrack(n);
			if (tr.go>=0) continue;	//je tam loop => ma to maximalni delku
			if (tr.len<nejkratsi) nejkratsi=tr.len;
		}
		
		//v "nejkratsi" je delka nejkratsiho tracku ze vsech v tomto song radku
		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			if (tracklen[n]<nejkratsi) tracklen[n]=nejkratsi; //potrebuje-li delsi cast nez mel doposud poznamenano, pak protahne na tu delku kterou potrebuje
		}
	}

	int ttracks=0,tbeats=0;
	//a ted oseka ty tracky
	for(i=0; i<TRACKSNUM; i++)
	{
		int nlen=tracklen[i];
		if (nlen<1) continue;	//co nemaji mit delku ani 1 preskakuje
		TTrack& tr=*m_tracks.GetTrack(i);
		if (tr.go<0)
		{
			//neni tam loop
			if (nlen<tr.len)
			{
				//to co ma osekavat - je tam vubec neco?
				for(j=nlen; j<tr.len; j++)
				{
					if (tr.note[j]>=0 || tr.instr[j]>=0 || tr.volume[j]>=0 || tr.speed[j]>=0)
					{
						//jo, neco tam je, tak to osekne
						ttracks++;
						tbeats+=tr.len-nlen;
						tr.len=nlen; //osekne to co neni potreba
						break;
					}
				}
				//sem skoci break;
			}
		}
		else
		{
			//je tam loop
			if (tr.len>=nlen)	//zacatek loopu je dal nez je potrebna delka tracku
			{
				ttracks++;
				tbeats+=tr.len-nlen;
				tr.len=nlen;	//zkrati track
				tr.go=-1;		//zrusi loop
			}
		}
	}

	//smaze neprazdne tracky nepouzite v songu
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
		if (m_tracks.IsEmptyTrack(i)) continue;	//prazdne neporovnava
		for(j=i+1;j<TRACKSNUM; j++)
		{
			if (m_tracks.IsEmptyTrack(j)) continue;
			if (m_tracks.CompareTracks(i,j))
			{
				m_tracks.ClearTrack(j);	//j je stejny jako i, takze j smaze.
				trackto[j]=i;			//poznamena si, ze ma tracky j nahradit trackama i
				clearedtracks++;
			}
		}
	}

	//ted probere song a provede zmeny u smazanych tracku
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
		if (m_songgo[sline]>=0) continue;	//go radek se vynechava

		for(ch=0; ch<g_tracks4_8; ch++)
		{
			int n=m_song[sline][ch];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			trackused[n]=1;
		}
	}

	//smaze vsechny tracky nepouzite v songu
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

void CSong::RenumberAllTracks(int type) //1..po sloupcich, 2..po radcich
{
	int i,j,sline;
	int movetrackfrom[TRACKSNUM],movetrackto[TRACKSNUM];

	for(i=0; i<TRACKSNUM; i++) movetrackfrom[i]=movetrackto[i]=-1;

	int order=0;

	//probere song
	if (type==2)
	{
		//vodorovne po radcich
		for(sline=0; sline<SONGLEN; sline++)
		{
			if (m_songgo[sline]>=0) continue;	//go radek se vynechava
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
		//svisle po sloupcich
		for(i=0; i<g_tracks4_8; i++)
		{
			for(sline=0; sline<SONGLEN; sline++)
			{
				if (m_songgo[sline]>=0) continue;	//go radek se vynechava
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
		return;	//nezname type

	//pak jeste prida neprazdne tracky nepouzite v songu
	for(i=0; i<TRACKSNUM; i++)
	{
		if (movetrackfrom[i]<0 && !m_tracks.IsEmptyTrack(i))
		{
			movetrackfrom[i]=order;
			movetrackto[order]=i;
			order++;
		}
	}

	//precislovani cisel v songu
	for(sline=0; sline<SONGLEN; sline++)
	{
		//if (m_songgo[sline]>=0) continue;	//go radek se TED NEVYNECHAVA (zmeni i cisla zminena pod nim)
		for(i=0; i<g_tracks4_8; i++)
		{
			int n=m_song[sline][i];
			if (n<0 || n>=TRACKSNUM) continue;	//--
			m_song[sline][i]=movetrackfrom[n];
		}
	}

	//fyzicke prehazeni dat v trackach
	TTrack buft;
	for(i=0; i<order; i++)
	{
		int n=movetrackto[i];	// prohodit i <--> n
		if (n==i) continue;		//jsou stejne, takze nemusi nic prohazovat
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
	//projede vsechny existujici tracky a zjisti nepouzite instrumenty
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
			if (t>=0 && t<INSTRSNUM) instrused[t]=1;	//instrument "t" je pouzit
		}
	}

	//a ted ty nepouzite smaze
	int clearedinstruments=0;
	for(i=0; i<INSTRSNUM; i++)
	{
		if (!instrused[i])
		{
			//neni pouzit
			if (m_instrs.CalculateNoEmpty(i)) clearedinstruments++;	//byl neprazdny? ano => zapocitat jeho smazani
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

	//probere vsechny tracky
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

	//a ted jeste prida i ty neprazdne co nejsou pouzite v zadnem tracku
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
			if (moveinstrfrom[i]>=0) //tento instrument je nekde pouzit nebo je neprazdny
			{
				//presunout i na di
				if (i!=di)
				{
					memcpy((void*)(&m_instrs.m_instr[di]),(void*)(&m_instrs.m_instr[i]),sizeof(TInstrument));
					//a smaze instrument i
					m_instrs.ClearInstrument(i);
				}
				//opravi zmenove tabulky
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
		//moveinstrfrom[instr] a moveinstrto[order] ma uz pripravene,
		//takze to muze rovnou fyzicky prehazet
		for(i=0; i<order; i++)
		{
			int n=moveinstrto[i];	//prohodit i <--> n
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
		//a ted promaze ty ostatni (kvuli odpovidajicim jmenum nepouzitych prazdnych instrumentu)
		for(i=order; i<INSTRSNUM; i++) m_instrs.ClearInstrument(i);
	}
	else
	if (type==3)
	{
		//poradi podle nazvu instrumentu
		BOOL iused[INSTRSNUM];
		for(i=0; i<INSTRSNUM; i++)
		{
			iused[i]=(moveinstrfrom[i]>=0);
			moveinstrfrom[i]=i;	//default je zachovat stejne poradi
		}
		//a ted bubblesortem presklada ty co jsou iused[i]
		for(i=INSTRSNUM-1; i>0; i--)
		{
			for(j=0; j<i; j++)
			{
				k=j+1;
				//porovnat instrument j a k a bud necha nebo prehodit
				BOOL swap=0;

				if (iused[j] != iused[k])
				{
					//jeden je pouzity a jeden nepouzity 
					if (iused[k]) swap=1; //druhy je pouzity (=>ten prvni je ten nepouzity), takze prehodit
				}
				else
				{
					//oba jsou pouzite nebo oba nepouzite
					char *name1=m_instrs.GetName(j);
					char *name2=m_instrs.GetName(k);
					if (_strcmpi(name1,name2)>0) swap=1; //jsou naopak, takze prehodit
				}

				if (swap)
				{
					//prehodit j a k
					memcpy((void*)(&bufi),(void*)(&m_instrs.m_instr[j]),sizeof(TInstrument)); // j -> buffer
					memcpy((void*)(&m_instrs.m_instr[j]),(void*)(&m_instrs.m_instr[k]),sizeof(TInstrument)); // k -> j
					memcpy((void*)(&m_instrs.m_instr[k]),(void*)(&bufi),sizeof(TInstrument)); // buffer -> k
					//opravit zmenove tabulky
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
		//jeste promaze nepouzite prazdne instrumenty (kvuli jejich posunuti, takze pak neodpovidal nazev jejich cislu 20: Instrument 21)
		for(i=0; i<INSTRSNUM; i++)
		{
			if (!iused[i]) m_instrs.ClearInstrument(i);
		}
	}
	else
		return;


	//tak, a ted to musi precislovat ve vsech trackach podle moveinstrfrom[instr] tabulky
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

	//a na zaver musi vsechny instrumenty zapsat do Atarka
	for(i=0; i<INSTRSNUM; i++) m_instrs.ModificationInstrument(i); //zapise do Atarka

	//hura, hotovo
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

	if (mode==MPLAY_BOOKMARK && !IsBookmark()) return 0; //pokud neni bookmark, tak nic.

	if (m_play)
	{
		if (mode!=MPLAY_FROM) Stop(); //uz hraje a chce neco jineho nez play from edited pos.
		else
		if (!m_followplay) Stop(); //uz hraje a chce play from edited pos. ale neni followplay
	}

	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;

	switch (mode)
	{
	case MPLAY_SONG: //cely song od zacatku vcetne inicializace (kvuli portamentum atd.)
		Atari_InitRMTRoutine();
		m_songplayline = 0;					
		m_trackplayline = 0;
		m_speed = m_mainspeed;
		break;
	case MPLAY_FROM: //song od aktualniho mista
		if (m_play && m_followplay) //je hrajici s follow play
		{
			m_play = MPLAY_FROM;
			m_followplay = follow;
			//g_screenupdate=1;
			return 1;
		}
		m_songplayline = m_songactiveline;
		m_trackplayline = m_trackactiveline;
		break;
	case MPLAY_TRACK: //jen aktualni tracky porad dokola
Play3:
		m_songplayline = m_songactiveline;
		m_trackplayline = (special==0)? 0 : m_trackactiveline;
		break;
	case MPLAY_BLOCK: //jen v bloku
		if (!g_trackcl.IsBlockSelected())
		{ //neni vybran blok, tak hraje track
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
	case MPLAY_BOOKMARK: //od bookmarku
		m_songplayline = m_bookmark.songline;
		m_trackplayline = m_bookmark.trackline;
		m_speed = m_bookmark.speed;
		break;
	}

	if (m_songgo[m_songplayline]>=0)	//je tam goto
	{
		m_songplayline=m_songgo[m_songplayline];	//cilovy radek kam skace goto
		m_trackplayline=0;							//od zacatku toho tracku
		if (m_songgo[m_songplayline]>=0)
		{
			//Goto na Goto
			MessageBox(g_hwnd,"There is recursive \"Go to line\" to other \"Go to line\" in song.","Recursive \"Go to line\"...",MB_ICONSTOP);
			return 0;
		}
	}

	WaitForTimerRoutineProcessed();
	m_followplay = follow;
	g_screenupdate=1;
	PlayBeat();						//nastavuje m_speeda
	m_speeda++;						//(27.4.2003) pridava 1 k m_speeda, za to uvodni co v realu probehne v Initu
	if (m_followplay)	//nasledovani prehravaneho
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;
	}

	//Sleep(1000);

	g_playtime=0;
	m_play = mode;
	return 1;
}

BOOL CSong::Stop()
{
	m_undo.Separator();

	m_play = MPLAY_STOP;
	m_quantization_note = m_quantization_instr = m_quantization_vol = -1;
	SetPlayPressedTonesSilence();
	//Sleep(100);	//nez se stop projevi
	WaitForTimerRoutineProcessed();		//ceka nez aspon jednou probehne cela TimerRoutine
	//Atari_Silence();
	//g_screenupdate=1;
	return 1;
}

BOOL CSong::SongPlayNextLine()
{
	m_trackplayline=0;
	if (m_play==MPLAY_SONG || m_play==MPLAY_FROM || m_play==MPLAY_BOOKMARK)
	{	//normalni play a play od mista nebo od bookmark => posun
		m_songplayline++;
		if (m_songplayline>255) m_songplayline=0;
	}
	//pro vsechny (nezustane na goto radku)
	//Go to line ??
	if (m_songgo[m_songplayline]>=0)
		m_songplayline=m_songgo[m_songplayline];

	return 1;
}

BOOL CSong::PlayBeat()
{
	int t,tt,xline,len,go,speed;
	int note[SONGTRACKS],instr[SONGTRACKS],vol[SONGTRACKS];
	TTrack *tr;

	for(t=0; t<g_tracks4_8; t++)		//provadeno tady aby se to chovalo stejne jako v rutine
	{
		note[t] = -1;
		instr[t] = -1;
		vol[t] = -1;
	}

TrackLine:
	speed = m_speed;

	for(t=0; t<g_tracks4_8; t++)
	{
//		note[t] = -1;					//posunuto na zacatek
//		instr[t] = -1;					//pred TrackLine: navesti
//		vol[t] = -1;					//aby se to chovalo stejne jako v rutine
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
				//pokud je konec tracku, ale jedna se o prehravani bloku nebo prvotni volani PlayBeat (kdy je m_play=0)
				if (m_play==MPLAY_BLOCK || m_play==MPLAY_STOP) { note[t]=-1; instr[t]=-1; vol[t]=-1; continue; }
				//jinak normalni predchod na dalsi radek v songu
				SongPlayNextLine();
				goto TrackLine;
			}
		}
		else
			xline = m_trackplayline;

		if (tr->note[xline]>=0)	note[t] = tr->note[xline];
		//if (tr->instr[xline]>=0)		//kvuli stejnemu chovani jako v rutine
		instr[t] = tr->instr[xline];	//kvuli stejnemu chovani jako v rutine
		if (tr->volume[xline]>=0) vol[t] = tr->volume[xline];
		if (tr->speed[xline]>0) speed=tr->speed[xline];
	}

	//teprve ted se nastavi zmenena rychlost
	m_speeda = m_speed = speed;

	//nastaveni aktivnich not,instrumentu a volume
	for(t=0; t<g_tracks4_8; t++)
	{
		int n=note[t];
		int i=instr[t];
		int v=vol[t];
		if (v>=0 && v<16)
		{
			if (n>=0 && n<NOTESNUM /*&& i>=0 && i<INSTRSNUM*/)		//uprava kvuli kompatibilite s rutinou
			{
				if (i<0 || i>=INSTRSNUM) i=255;						//uprava kvuli kompatibilite s rutinou
				//Atari_SetTrack_NoteInstrVolume(int t,int n,int i,int v)
				Atari_SetTrack_NoteInstrVolume(t,n,i,v);
			}
			else
			{
				//Atari_SetTrack_Volume(int t,int v)
				Atari_SetTrack_Volume(t,v);
			}
		}
	}

	return 1;
}

BOOL CSong::PlayVBI()
{
	if (!m_play) return 0;

	m_speeda--;
	if (m_speeda>0) return 0;

	m_trackplayline++;

	//m_play mode 4 => hraje jen rozsah v bloku
	if (m_play==MPLAY_BLOCK && m_trackplayline>m_trackplayblockend) m_trackplayline=m_trackplayblockstart;

	//pokud zadny z tracku neni ukoncen "end"em, tak se to ukonci pri dosazeni m_maxtracklen
	if (m_trackplayline>=m_tracks.m_maxtracklen)
		SongPlayNextLine();

	PlayBeat();

	if (m_speeda==m_speed && m_followplay)	//nasledovani prehravaneho
	{
		m_trackactiveline = m_trackplayline;
		m_songactiveline = m_songplayline;

		//Quantization
		if (m_quantization_note>=0 && m_quantization_note<NOTESNUM
			&& m_quantization_instr>=0 && m_quantization_instr<INSTRSNUM
			)
		{
			//if ( TrackSetNoteActualInstrVol(m_quantization_note) )
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
		if (m_quantization_note==-2) //Specialni pripad (midi NoteOFF)
		{
			TrackSetNoteActualInstrVol(-1);
			TrackSetVol(0);
		}
		m_quantization_note=-1; //zruseni kvantizovane noty
		//konec Q
	}

	g_screenupdate=1;
	
	return 1;
}

void CSong::TimerRoutine()
{
	//MessageBeep(-1);
	//g_timerroutineprocessed=1;	//probehla TimerRoutine
	//return;

	//veci ktere se resi 1x za vbi
	PlayVBI();

	//tony na zaklade on-line stlacenych klaves
	PlayPressedTones();

	//--- Renderovani Soundu ---//
	m_pokey.RenderSound1_50(m_instrspeed);		//vyrendrovani kousku samplu (1/50s = 20ms), instrspeed

	if (m_play) g_playtime++;

	//MessageBeep(-1);
	//!!!!!!!!!!!!!!!!!!!!!!!!
	//MessageBeep(-1); g_timerroutineprocessed=1;	return; //probehla TimerRoutine

	//--- VYKRESLOVANI OBRAZU ---//

	if (g_screena>0)
		g_screena--;
	else
	{
		if (g_screenupdate) //Chce prekreslit?
		{
			//g_screenupdate=0;
			g_invalidatebytimer=1;
			AfxGetApp()->GetMainWnd()->Invalidate();
			//AfxGetApp()->GetMainWnd()->RedrawWindow(); //Invalidate();
			//MessageBeep(-1);
			//g_screena = g_screenwait se nastavuje v OnDraw
			//=>Dalsi prekresleni nejdrive po uplynuti g_screenwait
		}
	}


	g_timerroutineprocessed=1;	//probehla TimerRoutine

}


//--------------------------------------

CUndo::CUndo()
{
	for(int i=0; i<MAXUNDO; i++) m_uar[i]=NULL;
	//Init(NULL);
}

CUndo::~CUndo()
{
	//
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
	char sep=ue->separator; //uschova pro navrat
	if (ue->cursor) delete[] ue->cursor;
	if (ue->pos) delete[] ue->pos;
	if (ue->data) delete[] ue->data;
	delete ue;
	m_uar[i]=NULL;
	return sep;
}

BOOL CUndo::Undo()
{
	if (m_head==m_tail)	return 0; //neni co undovat

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
	if (m_head==m_headmax) return 0; //neni co redovat

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
		g_changes=1;	//doslo k nejake zmene
		m_song->SetRMTTitle();
	}
	//prida kurzor
	ue->part=g_activepart;
	ue->cursor=m_song->GetUECursor(g_activepart);
	//ue->separator=0; //default separator=0
	//
	if (m_uar[m_head]) DeleteEvent(m_head);
	//
	m_uar[m_head]=ue;
	//
	//je tam uz nejaky event?
	if (m_head!=m_tail)
	{
		TUndoEvent* le=m_uar[(m_head+MAXUNDO-1)%MAXUNDO];
		//CString s; s.Format("%i %i %i %i %i %i",ue->part==le->part,ue->type==le->type,!le->separator,memcmp(ue->cursor,le->cursor,_msize(ue->cursor))==0,memcmp(ue->pos,le->pos,_msize(ue->pos))==0, _msize(ue->cursor)      );
		//MessageBox(g_hwnd,s,"Msg",MB_OK);
		if (   ue->part==le->part
			&& ue->type==le->type
			&& !le->separator
			&& !ue->separator
			&& m_song->UECursorIsEqual(ue->cursor,le->cursor,ue->part)
			&& PosIsEqual(ue->pos,le->pos,ue->type)
			)
		{
			//posledni udalost je na stejnem miste kurzoru
			//a se stejnymi daty
			DeleteEvent(m_head); //vymaze ho z pameti
			//a nezapocita ho mezi undo udalosti
			//akorat ukonci maximalni undo
			m_headmax=m_head;
			m_redosteps=0;
			return;
		}
	}
	//
	if (ue->separator!=-1) m_undosteps++; //zapocitavaji se jen kompletni eventy
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
	m_undosteps--;		//odpocita tento krok
}

void CUndo::Separator(int sep)
{
	TUndoEvent* le;
	le=m_uar[(m_head+MAXUNDO-1)%MAXUNDO];
	if (!le) return;
	if (sep<0 && le->separator>=0) m_undosteps--; //odpocita pocet undo zapocitany v InsertEventu
	le->separator=sep;
}

void CUndo::ChangeTrack(int tracknum,int trackline,int type, char separator)
{
	if (tracknum<0) return;
	TTrack *tr=m_song->GetTracks()->GetTrack(tracknum);

	//Event s puvodnim stavem na menenem miste
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

	case UETYPE_TRACKDATA: //cely track
		data=(int*)new TTrack;
		memcpy((void*)data,tr,sizeof(TTrack));
		break;

	case UETYPE_TRACKSALL: //vsechny tracky
		{
		data=(int*)new TTracksAll;
		//TTracksAll* tracksall=m_song->GetTracks()->GetTracksAll();
		//memcpy((void*)data,tracksall,sizeof(TTracksAll));
		m_song->GetTracks()->GetTracksAll((TTracksAll*)data); //naplni data
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

	//Event s puvodnim stavem na menenem miste
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

	case UETYPE_SONGDATA: //cely song
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

	//Event s puvodnim stavem na menenem miste
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[2];
	ue->pos[0]=instrnum;
	ue->pos[1]=paridx;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_INSTRDATA:	//cely instrument
		{
		TInstrument* ins=new TInstrument;
		memcpy(ins,instr,sizeof(TInstrument));
		data=(int*)ins;
		}
		break;

	case UETYPE_INSTRSALL: //vsechny instrumenty
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
	//Event s puvodnim stavem na menenem miste
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[1];
	ue->pos[0]=paridx;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_INFODATA:	//cele info
		{
		TInfo* inf=new TInfo;
		m_song->GetSongInfoPars(inf); //naplni "inf" prevzatymi hodnotami z m_song
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



/*
void CUndo::ChangeSongAndTrack(int songline,int trackcol,int tracknum,int type, BOOL separator)
{
	if (songline<0 || trackcol<0) return;
	if (tracknum<0) return;
	TTrack *tr=m_song->GetTracks()->GetTrack(tracknum);

	//Event s puvodnim stavem na menenem miste
	TUndoEvent *ue = new TUndoEvent;
	ue->type=type;
	ue->pos=new int[3];
	ue->pos[0]=songline;
	ue->pos[1]=trackcol;
	ue->pos[2]=tracknum;
	ue->separator=separator;
	int *data;
	switch(type)
	{
	case UETYPE_SONGTRACKANDTRACKDATA:
		{
		TSongTrackAndTrackData* usch=new TSongTrackAndTrackData;
		memcpy((void*)&usch->trackdata,tr,sizeof(TTrack));
		usch->tracknum=tracknum;
		data=(int*)usch;
		}
		break;

	default:
		MessageBox(g_hwnd,"CUndo::ChangeSongAndTrack BAD!","Internal error",MB_ICONERROR);
		data=NULL;
	}

	ue->data=(void*)data;
	//
	InsertEvent(ue);
}
*/

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
	//uschova si separator jako navratovou hodnotu
	char sep=ue->separator;
	//nastavi tam kurzor (meni i g_activepart)
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

	case UETYPE_TRACKDATA: //cely track
		{
		int tracknum=ue->pos[0];
		TTrack* tr=(m_song->GetTracks()->GetTrack(tracknum));
		TTrack temp;
		memcpy((void*)&temp,tr,sizeof(TTrack));
		memcpy(tr,ue->data,sizeof(TTrack));
		memcpy(ue->data,(void*)&temp,sizeof(TTrack));
		}
		break;

	case UETYPE_TRACKSALL: //vsechny tracky
		{
		TTracksAll* temp = new TTracksAll;
		/*
		TTracksAll* tracksall=m_song->GetTracks()->GetTracksAll();
		memcpy((void*)temp,tracksall,sizeof(TTracksAll));
		memcpy(tracksall,ue->data,sizeof(TTracksAll));
		memcpy(ue->data,(void*)temp,sizeof(TTracksAll));
		*/
		m_song->GetTracks()->GetTracksAll(temp); //da do tempu
		m_song->GetTracks()->SetTracksAll((TTracksAll*)ue->data); //da ue->data do tracksall
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

	case UETYPE_SONGDATA: //cely song
		{
		TSong temp;
		TSong* data=(TSong*)ue->data;
		//temp <= song
		memcpy((void*)&temp.song,m_song->GetSong(),sizeof(temp.song));
		memcpy((void*)&temp.songgo,m_song->GetSongGo(),sizeof(temp.songgo));
		memcpy((void*)&temp.bookmark,m_song->GetBookmark(),sizeof(temp.bookmark));
		//song <= data z undo
		memcpy(m_song->GetSong(),data->song,sizeof(temp.song));
		memcpy(m_song->GetSongGo(),data->songgo,sizeof(temp.songgo));
		memcpy(m_song->GetBookmark(),&data->bookmark,sizeof(temp.bookmark));
		//data pro redo <= temp
		memcpy(data->song,(void*)&temp.song,sizeof(temp.song));
		memcpy(data->songgo,(void*)&temp.songgo,sizeof(temp.songgo));
		memcpy(&data->bookmark,(void*)&temp.bookmark,sizeof(temp.bookmark));
		}
		break;

	case UETYPE_INSTRDATA: //cely instrument
		{
		int instrnum=ue->pos[0];
		TInstrument* in=&(m_song->GetInstruments()->m_instr[instrnum]);
		TInstrument temp;
		memcpy((void*)&temp,in,sizeof(TInstrument));
		memcpy((void*)in,ue->data,sizeof(TInstrument));
		memcpy(ue->data,(void*)&temp,sizeof(TInstrument));
		//musi ulozit do "Atarka"
		m_song->GetInstruments()->ModificationInstrument(instrnum);
		}
		break;

	case UETYPE_INSTRSALL: //vsechny instrumenty
		{
		TInstrumentsAll* temp=new TInstrumentsAll;
		TInstrumentsAll* insall=m_song->GetInstruments()->GetInstrumentsAll();
		memcpy(temp,insall,sizeof(TInstrumentsAll));
		memcpy(insall,ue->data,sizeof(TInstrumentsAll));
		memcpy(ue->data,temp,sizeof(TInstrumentsAll));
		delete temp;
		//musi ulozit do "Atarka"
		for(i=0; i<INSTRSNUM; i++) m_song->GetInstruments()->ModificationInstrument(i);
		}
		break;

/*
	case UETYPE_SONGTRACKANDTRACKDATA: //song track + cely track
		{
		//song track
		int songline=ue->pos[0];
		int trackcol=ue->pos[1];
		int tracknum=ue->pos[2];
		TSongTrackAndTrackData* data=(TSongTrackAndTrackData*)ue->data;
		//cely track
		TTrack* tr=(m_song->GetTracks()->GetTrack(tracknum));
		TSongTrackAndTrackData temp;
		memcpy((void*)&temp.trackdata,(void*)tr,sizeof(temp.trackdata));
		memcpy((void*)tr,(void*)&data->trackdata,sizeof(temp.trackdata));
		memcpy((void*)&data->trackdata,(void*)&temp.trackdata,sizeof(temp.trackdata));
		//
		ExchangeInt((*m_song->GetSong())[songline][trackcol],data->tracknum);
		}
		break;
*/

	case UETYPE_INFODATA:
		{
		//int instrnum=ue->pos[0];
		//TInstrument* in=&(m_song->GetInstruments()->m_instr[instrnum]);
		TInfo in;
		m_song->GetSongInfoPars(&in); //naplni "in" prevzatymi hodnotami z m_song
		TInfo* data=(TInfo*)ue->data;
		m_song->SetSongInfoPars(data); //nastavi hodnoty v m_song hodnotami z "data"
		memcpy(ue->data,(void*)&in,sizeof(TInfo));
		//
		}
		break;


	default:
		MessageBox(g_hwnd,"PerformEvent BAD!","Internal error",MB_ICONERROR);

	}

	return sep; //vraci separator
}
