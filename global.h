//
// global.h
//

//#include "stdafx.h"

#ifndef RMT_GLOBAL_
#define RMT_GLOBAL_


unsigned char g_atarimem[65536];

char CharH4(unsigned char b) { BYTE i=b>>4; return ((BYTE)i+((i<10)? 48:55)); };
char CharL4(unsigned char b) { BYTE i=b&0x0f; return ((BYTE)i + ((i<10)? 48:55)); };
int Hexstr(char* txt,int len)
{
	int r=0;
	char a;
	int i;
	for(i=0; (a=txt[i]) && i<len; i++)
	{
		if (a>='0' && a<='9') 
			r = (r<<4) + (a-'0');
		else
		if (a>='A' && a<='F') 
			r = (r<<4) + (a-'A'+10);
		else
		{
			if (i==0) r=-1; //nic
			return r;
		}
	}
	if (i==0) r=-1; //nic
	return r;
}
BOOL NextSegment(ifstream& in)
{
	char b;
	while(!in.eof())
	{
		in.read((char*)&b,1);
		if (b=='[') return 1;	//konec segmentu (zacatek neceho dalsiho)
	}
	return 0;
}
void Trimstr(char* txt)
{
	char a;
	for(int i=0; (a=txt[i]); i++)
	{
		if (a==13 || a==10)
		{
			txt[i]=0;
			return;
		}
	}
}


BOOL g_closeapplication=0;
CDC* g_mem_dc=NULL;
CDC* g_gfx_dc=NULL;

int g_width = 0;
int g_height = 0;
int g_tracklines = 8;
int g_songlines = 3;
int g_song_x    = SONG_X;

HWND g_hwnd=NULL;
HWND g_viewhwnd=NULL;

HINSTANCE g_c6502_dll;
BOOL volatile g_is6502 = 0;
CString g_aboutpokey;
CString g_about6502;

BOOL g_changes=0;	//doslo k nejakym zmenam v modulu?
//int g_rmtexit=0;
int g_focus;
int g_shiftkey;
int g_controlkey;
int g_numlock;
int g_capslock_rmt;
int g_capslock_other;
int g_scrolllock_rmt;
int g_scrolllock_other;

//
int g_tracks4_8;
BOOL volatile g_screenupdate=0;
BOOL volatile g_invalidatebytimer=0;
int volatile g_screena;
int volatile g_screenwait;
BOOL volatile g_rmtroutine;
BOOL volatile g_timerroutineprocessed;

int volatile g_prove;		//testovaci hra bez editace (0=vyp,1=mono,2=stereo)
int volatile g_respectvolume;	//nemeni hlasitost pokud uz tam je

WORD g_rmtstripped_adr_module;	//adresa pro export RMT stripped file
BOOL g_rmtstripped_sfx;			//sfx odnoz RMT stripped file
BOOL g_rmtstripped_gvf;			//gvs GlobalVolumeFade pro feat
BOOL g_rmtstripped_nos;			//nos NoStartingSongline pro feat
CString g_rmtmsxtext;
CString g_expasmlabelprefix;	//label prefix pro export ASM simple notation

int g_activepart;			//0 info, 1 edittracks, 2 editinstruments, 3 song
int g_active_ti;			//1 tracks, 2 instrs

int g_tracklinehighlight=8;	//kazdy 8-radek zvyrazneny
BOOL g_tracklinealtnumbering=0; //alternativni zpusob cislovani radku v trackach
int g_linesafter;			//pocet radku pro posun po vlozeni noty (inicializuje se v CSong::Clear)
BOOL g_ntsc=0;				//NTSC (60Hz)
BOOL g_nohwsoundbuffer=0;	//Don't use hardware soundbuffer
int g_trackcursorverticalrange=6;	//maximalni volnost kurzoru (default 6)
int g_cursoractview=0;		//
int g_track_scroll_margin = 3;
int g_channelon[SONGTRACKS];
int g_rmtinstr[SONGTRACKS];

BOOL g_viewmaintoolbar=1;	//1 ano, 0 ne
BOOL g_viewblocktoolbar=1;	//1 ano, 0 ne
BOOL g_viewstatusbar=1;	//1 ano, 0 ne
BOOL g_viewplaytimecounter=1;	//1 ano, 0 ne
BOOL g_viewanalyzer=1;		//1 ano, 0 ne
BOOL g_viewpokeyregs=0;		//1 ano, 0 ne
BOOL g_viewinstractivehelp=1;	//1 ano, 0 ne

long g_playtime=0;

UINT g_mousebutt=0;			//mouse buttony

CTrackClipboard g_trackcl;

CString g_prgpath;			//cesta do adresare odkud byl program spusten (vcetne lomitka na konci)
CString g_lastloadpath_songs;		//cesta posledniho loadovani songu
CString g_lastloadpath_instruments; //cesta posledniho loadovani instrumentu
CString g_lastloadpath_tracks;		//cesta posledniho loadovani tracky

CString g_path_songs;		//default cesta pro songy
CString g_path_instruments;	//default cesta pro instrumenty
CString g_path_tracks; //default cesta pro tracky

int g_keyboard_layout=0;
BOOL g_keyboard_swapenter=0;
BOOL g_keyboard_playautofollow=0;
BOOL g_keyboard_usenumlock=1;
BOOL g_keyboard_updowncontinue=1;
BOOL g_keyboard_rememberoctavesandvolumes=1;
BOOL g_keyboard_escresetatarisound=0;
BOOL g_keyboard_askwhencontrol_s=1;

int g_midi_notech[16];			//posledne stlacene noty na jednotlivych MIDI kanalech
int g_midi_voluch[16];			//hlasitost not na jednotlivych MIDI kanalech
int g_midi_instch[16];			//posledne nastavena cisla instrumentu na jednotlivych MIDI kanalech

BOOL g_midi_tr=0;
int g_midi_volumeoffset=1;	//standardne zacina od hlasitosti 1
BOOL g_midi_noteoff=0;		//defaultne je noteoff vypnuto

#define COLOR_SELECTED	3	//3.sada jsou selektovane znaky


//----

void TextXY(char *txt,int x,int y,int c=0)
{
	char a;
	c=c<<4;
	for(int i=0; a=(txt[i]); i++,x+=8)
		if (a!=32) g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,(a & 0x7f)<<3,c,SRCCOPY);
}

void TextXYSelN(char *txt,int n,int x,int y,int c=0)
{
	//znak indexu n bude mit "select" barvu, ostatni c
	char a;
	c=c<<4;
	int i;
	for(i=0; a=(txt[i]); i++,x+=8)
		g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,(a & 0x7f)<<3,(i==n)? COLOR_SELECTED*16:c,SRCCOPY);
	if (i==n) //prvni znak za koncem retezce
		g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,32<<3,COLOR_SELECTED*16,SRCCOPY);
}

void TextXYCol(char *txt,int x,int y,char *col)
{
	char a;
	for(int i=0; a=(txt[i]); i++,x+=8)
		g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,(a & 0x7f)<<3,col[i]<<4,SRCCOPY);
}

void TextDownXY(char *txt,int x,int y,int c=0)
{
	char a;
	c=c<<4;
	for(int i=0; a=(txt[i]); i++,y+=16)
		g_mem_dc->BitBlt(x,y,8,16,g_gfx_dc,(a & 0x7f)<<3,c,SRCCOPY);
}

void NumberMiniXY(BYTE num,int x,int y,int c=0)
{
	c=112+(c<<3);
	g_mem_dc->BitBlt(x,y,8,8,g_gfx_dc,(num & 0xf0)>>1,c,SRCCOPY);
	g_mem_dc->BitBlt(x+8,y,8,8,g_gfx_dc,(num & 0x0f)<<3,c,SRCCOPY);
}

void TextMiniXY(char *txt,int x,int y,int c=0)
{
	char a;
	c=112+(c<<3);
	for(int i=0; a=(txt[i]); i++,x+=8)
		if (a!=32) g_mem_dc->BitBlt(x,y,8,8,g_gfx_dc,(a & 0x7f)<<3,c,SRCCOPY);
}

void IconMiniXY(int icon,int x,int y)
{
	static int c=128-6;
	if (icon>=1 && icon<=4) g_mem_dc->BitBlt(x,y,32,6,g_gfx_dc,(icon-1) * 32,c,SRCCOPY);
}

void GetTracklineText(char *dest,int line)
{
	if (line<0 || line>0xff) { dest[0]=0; return; }
	if (g_tracklinealtnumbering)
	{
		int a=line/g_tracklinehighlight;
		if (a>=35) a=(a-35)%26+'a'-'9'+1;
		int b=line%g_tracklinehighlight;
		if (b>=35) b=(b-35)%26+'a'-'9'+1;
		if (a<=8)
			a='1'+a;
		else
			a='A'-9+a;
		if (b<=8)
			b='1'+b;
		else
			b='A'-9+b;
		dest[0]=a;
		dest[1]=b;
		dest[2]=0;
	}
	else
		sprintf(dest,"%02X",line);
}

void SetLockKeys(int vkey, BOOL onoff)
{
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize=sizeof(info);
	GetVersionEx(&info);
	if (info.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
	{	//Win32 on Windows 95 or Windows 98
		BYTE keys[256];
		GetKeyboardState(keys);
		keys[vkey]=onoff;
		SetKeyboardState(keys);
	}
	else
	{	//Win NT/2000
		keybd_event(vkey,0,0,0);
		keybd_event(vkey,0,KEYEVENTF_KEYUP,0);
	}

	//
	//Beep(500,10); Sleep(300);
}

BOOL GetCapsLock()
{
	BYTE keys[256];
	GetKeyboardState(keys);
	return (keys[VK_CAPITAL]!=0); //CapsLock
}

void SetCapsLock(BOOL onoff)
{
	/*
	BYTE keyboard[256];
	GetKeyboardState(keyboard);
	keyboard[20]=onoff;		//CapsLock off
	SetKeyboardState(keyboard);
	*/
	BOOL state=GetCapsLock();
	if (state != onoff) SetLockKeys(VK_CAPITAL,onoff);	//CapsLock on/off
}

BOOL GetScrollLock()
{
	BYTE keys[256];
	GetKeyboardState(keys);
	return (keys[VK_SCROLL]!=0); //ScrollLock
}

void SetScrollLock(BOOL onoff)
{
	/*
	BYTE keyboard[256];
	GetKeyboardState(keyboard);
	keyboard[20]=onoff;		//CapsLock off
	SetKeyboardState(keyboard);
	*/
	BOOL state=GetScrollLock();
	if (state != onoff) SetLockKeys(VK_SCROLL,onoff);	//ScrollLock on/off
}

void UpdateShiftControlKeys()
{
	g_shiftkey=(GetAsyncKeyState(VK_SHIFT)!=NULL);
	g_controlkey=(GetAsyncKeyState(VK_CONTROL)!=NULL);
}


BOOL IsnotMovementVKey(int vk)
{
	//vraci 1 kdyz nejde o posunovou klavesu
	return (vk!=VK_RIGHT && vk!=VK_LEFT && vk!=VK_UP && vk!=VK_DOWN && vk!=VK_TAB && vk!=13 && vk!=VK_HOME && vk!=VK_END && vk!=VK_PAGE_UP && vk!=VK_PAGE_DOWN && vk!=VK_CAPITAL);
}

int EditText(int vk,int shift,int control, char* txt,int& cur, int max)
{
	//vraci 1, pokud byl stisknut TAB nebo ENTER
	
	//short capslock = GetKeyState(20);	//VK_CAPS_LOCK

	max--;
	if (vk==8) //VK_BACKSPACE
	{
		if (cur>0)
		{
			cur--;
			for(int j=cur; j<=max-1; j++) txt[j]=txt[j+1];
			txt[max]=' ';
			//if (cur==max) txt[cur]=' ';
			//txt[cur]=' ';
		}
	}
	else
	if (vk==VK_TAB || vk==13)		//VK_ENTER
	{
		g_capslock_rmt=0;
		SetCapsLock(0); //CapsLock off
		return 1;
	}
	else
	if (vk==VK_INSERT)
	{
		for(int j=max-1; j>=cur; j--) txt[j+1]=txt[j];
		txt[cur]=' ';
	}
	else
	if (vk==VK_DELETE)
	{
		for(int j=cur; j<=max-1; j++) txt[j]=txt[j+1];
		txt[max]=' ';
	}
	else
	{
		if ( control ) return 0;
		char a=0;
		if (vk>=65 && vk<=90) {	a = (shift)? vk : vk+32; }		//pismena - velka pres Control
		else
		if (vk>=48 && vk<=57) { a = (shift)? *(")!@#$%^&*("+vk-48): vk; }			//cislice - pres Control znaky pod nimi
		else
		if (vk==' ')			a = ' ';	//mezera
		else
		if (vk==189)	a= (shift)? '_':'-';
		else
		if (vk==187)	a= (shift)? '+':'=';
		else
		if (vk==219)	a='[';
		else
		if (vk==221)	a=']';
		else
		if (vk==186)	a= (shift)? ':':';';
		else
		if (vk==222)	a= (shift)? '"':'\'';
		else
		if (vk==188)	a= (shift)? '<':',';
		else
		if (vk==190)	a= (shift)? '>':'.';
		else
		if (vk==191)	a= (shift)? '?':'/';
		else
		if (vk==220)	a= (shift)? '|':'\\';
		else
		if (vk==VK_RIGHT)
		{
			if (cur<max) cur++;
		}
		else
		if (vk==VK_LEFT)
		{
			if (cur>0) cur--;
		}
		else
		if (vk==VK_HOME) cur=0;
		else
		if (vk==VK_END)
		{
			//CString s; s.Format("'%s'",txt); MessageBox(g_hwnd,s,"Show",MB_OK);
			int j;
			for(j=max; j>=0 && (txt[j]==' '); j--);
			cur=(j<max)? j+1:max;
		}


		if (a>0)
		{
			for(int j=max-1; j>=cur; j--) txt[j+1]=txt[j];
			txt[cur]=a;
			if (cur<max) cur++;
		}
	}
	return 0;
}

/*
void ClearXY(int x,int y,int n)
{
	x<<=3; y<<=4;
	g_mem_dc->FillSolidRect(x,y,x+(n<<8),y+15,RGB(96,96,96));
}
*/

void WaitForTimerRoutineProcessed()
{
	g_timerroutineprocessed=0;
	while(!g_timerroutineprocessed) Sleep(1);		//ceka
}

void StrToAtariVideo(char* txt,int count)
{
	char a;
	for(int i=0; i<count; i++)
	{
		a=txt[i] & 0x7f;
		if (a<32) a=0;
		else
		if (a<96) a-=32;
		txt[i]=a;
	}
}

//----

void SetChannelOnOff(int ch,int onoff)
{
	if (ch<0)
	{	//vsechny kanaly
		if (onoff>=0)
			for(int i=0; i<SONGTRACKS; i++) g_channelon[i] = onoff;	//nastaveni
		else
			for(int i=0; i<SONGTRACKS; i++) g_channelon[i] ^= 1;	//inverze
	}
	else
	if (ch<SONGTRACKS)
	{	//jen ten jeden
		if (onoff>=0)
			g_channelon[ch] = onoff;	//nastaveni
		else
			g_channelon[ch] ^= 1;	//inverze
	}
}

int GetChannelOnOff(int ch)	
{ 
	//if (ch<0 || ch>7) MessageBeep(-1);
	return g_channelon[ch]; 
}

void SetChannelSolo(int ch)
{
	int on = GetChannelOnOff(ch);
	if (!on)
	{
Channel_SOLO:
		SetChannelOnOff(-1,0);	//vsechny vyp
		SetChannelOnOff(ch,1);	//a solo jen aktivniho
	}
	else
	{
		for(int i=0; i<g_tracks4_8; i++)
		{
			if(i!=ch && GetChannelOnOff(i)) goto Channel_SOLO;
		}
		SetChannelOnOff(-1,1);	//vsechny zapnout
	}
}


//----

const char keynotes[256]=
{
//0
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, 27, -1,
//50
	13, 15, -1, 18, 20, 22, -1, 25, -1, -1,
	-1, -1, -1, -1, -1, -1,  7,  4,  3, 16,
	-1,  6,  8, 24, 10, -1, 13, 11,  9, 26,
	28, 12, 17,  1, 19, 23,  5, 14,  2, 21,
	 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//100
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//150
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, 15, 30, 12, -1,
	14, 16, -1, -1, -1, -1, -1, -1, -1, -1,
//200
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, 29,
	-1, 31, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//250
	-1, -1, -1, -1, -1, -1
};

const char keynumbs[256]=
{
//0
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
//64
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//128
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//192
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

const char keynumblock09[256]=
{
//0
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//64
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//128
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//192
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


//#define NoteKey(vk)		keynotes[vk]
//#define NumbKey(vk)		keynumbs[vk]
//#define Numblock09Key(vk)	keynumblock09[vk]
char NoteKey(int vk)	{ return keynotes[vk]; };
char NumbKey(int vk)	{ return keynumbs[vk]; };
char Numblock09Key(int vk)	{ return keynumblock09[vk]; };

#endif