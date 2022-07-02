//
// global.h
// originally made by Raster, 2002-2009
// experimental changes and additions by VinsCool, 2021-2022
//

#ifndef RMT_GLOBAL_
#define RMT_GLOBAL_

unsigned char g_atarimem[65536];
char g_debugmem[65536];	//debug display of g_atarimem bytes directly, slow and terrible, do not use unless there is a purpose for it 

unsigned char SAPRSTREAM[0xFFFFFF];	//SAP-R Dumper memory, TODO: assign memory dynamically instead, however this doesn't seem to harm anything for now

int SAPRDUMP = 0;		//0, the SAPR dumper is disabled; 1, output is currently recorded to memory; 2, recording is finished and will be written to sap file; 3, flag engage the SAPR dumper
int framecount = 0;		//SAPR dumper frame counter used for calculations and memory allignment with bytes 

char CharH4(unsigned char b) { BYTE i=b>>4; return ((BYTE)i+((i<10)? 48:55)); };
char CharL4(unsigned char b) { BYTE i=b&0x0f; return ((BYTE)i + ((i<10)? 48:55)); };
int Hexstr(char* txt,int len)
{
	int r=0;
	char a;
	int i;
	for (i = 0; (a = txt[i]) && i < len; i++)
	{
		if (a>='0' && a<='9') 
			r = (r<<4) + (a-'0');
		else
		if (a>='A' && a<='F') 
			r = (r<<4) + (a-'A'+10);
		else
		{
			if (i==0) r=-1; //nothing
			return r;
		}
	}
	if (i==0) r=-1; //nothing
	return r;
}
BOOL NextSegment(ifstream& in)
{
	char b;
	while(!in.eof())
	{
		in.read((char*)&b,1);
		if (b=='[') return 1;	//end of segment (beginning of something else)
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
int g_scaling_percentage = 100;

//best known compromise for both regions, they produce identical tables
double g_basetuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
int g_basenote = 3;	//3 = A-

//ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
double g_UNISON = 1;
double g_MIN_2ND = 1;
double g_MAJ_2ND = 1;
double g_MIN_3RD = 1;
double g_MAJ_3RD = 1;
double g_PERF_4TH = 1;
double g_TRITONE = 1;
double g_PERF_5TH = 1;
double g_MIN_6TH = 1;
double g_MAJ_6TH = 1;
double g_MIN_7TH = 1;
double g_MAJ_7TH = 1;
double g_OCTAVE = 2;

//ratio left
int g_UNISON_L = 1;
int g_MIN_2ND_L = 40;
int g_MAJ_2ND_L = 10;
int g_MIN_3RD_L = 20;
int g_MAJ_3RD_L = 5;
int g_PERF_4TH_L = 4;
int g_TRITONE_L = 60;
int g_PERF_5TH_L = 3;
int g_MIN_6TH_L = 30;
int g_MAJ_6TH_L = 5;
int g_MIN_7TH_L = 30;
int g_MAJ_7TH_L = 15;
int g_OCTAVE_L = 2;

//ratio right
int g_UNISON_R = 1;
int g_MIN_2ND_R = 38;
int g_MAJ_2ND_R = 9;
int g_MIN_3RD_R = 17;
int g_MAJ_3RD_R = 4;
int g_PERF_4TH_R = 3;
int g_TRITONE_R = 43;
int g_PERF_5TH_R = 2;
int g_MIN_6TH_R = 19;
int g_MAJ_6TH_R = 3;
int g_MIN_7TH_R = 17;
int g_MAJ_7TH_R = 8;
int g_OCTAVE_R = 1;

//each preset is assigned to a number. 0 means no Temperament, any value that is not assigned defaults to custom
int g_temperament = 0;

//Thomas Young 1799's Well Temperament no.1
const double YOUNG1[13] =
{
	1,
	1.055709,
	1.119770,
	1.187690,
	1.253887,
	1.334739,
	1.407637,
	1.496513,
	1.583581,
	1.675715,
	1.781546,
	1.878851,
	2
};

//Thomas Young 1799's Well Temperament no.2 
const double YOUNG2[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.334839,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.879240,
	2
};

//Thomas Young 1807's Well Temperament
const double YOUNG3[13] =
{
	1,
	1.053497,
	1.119929,
	1.185185,
	1.254242,
	1.333333,
	1.404663,
	1.496616,
	1.580246,
	1.676105,
	1.777777,
	1.877119,
	2
};

//Andreas Werckmeister's temperament III (the most famous one, 1681)
const double WERCK3[13] =
{
	1,
	1.053497,
	1.117403,
	1.185185,
	1.252827,
	1.333333,
	1.404663,
	1.494927,
	1.580246,
	1.670436,
	1.777777,
	1.879240,
	2
};

//Tempérament Égal à Quintes Justes 
const double QUINTE[13] =
{
	1,
	1.059634,
	1.122824,
	1.189782,
	1.260734,
	1.335916,
	1.415582,
	1.5,
	1.589451,
	1.684236,
	1.784674,
	1.891101,
	2.003875
};

//d'Alembert and Rousseau tempérament ordinaire (1752/1767)
const double TEMPORD[13] =
{
	1,
	1.051120,
	1.118034,
	1.181176,
	1.250000,
	1.331828,
	1.403077,
	1.495348,
	1.574901,
	1.671850,
	1.773766,
	1.872883,
	2
};

//Aron - Neidhardt equal beating well temperament
const double ARONIED[13] =
{
	1,
	1.053497,
	1.118144,
	1.185185,
	1.250031,
	1.333333,
	1.404663,
	1.496112,
	1.580246,
	1.672002,
	1.777777,
	1.872885,
	2
};

//Atom Schisma Scale
const double ATOMSCH[13] =
{
	1,
	1.059459,
	1.122463, 
	1.189204,
	1.259924,
	1.334838,
	1.414207,
	1.498308,
	1.587396,
	1.681796,
	1.781794,
	1.887755,
	2
};

//12 - tET approximation with minimal order 17 beats
const double APPRX12[13] =
{
	1,
	1.058823,
	1.125000,
	1.187500,
	1.250000,
	1.333333,
	1.416666,
	1.500000,
	1.588235,
	1.666666,
	1.777777,
	1.888888,
	2
};

//Paul Bailey's modern well temperament (2002)
const double BAILEY1[13] =
{
	1,
	1.054892,
	1.119671,
	1.186774,
	1.254242,
	1.335183,
	1.406531,
	1.498302,
	1.582329,
	1.676250,
	1.780202,
	1.881407,
	2
};

//John Barnes' temperament (1977) made after analysis of Wohltemperierte Klavier, 1/6 P
const double BARNES1[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.336348,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.881364,
	2
};

//Bethisy temperament ordinaire, see Pierre - Yves Asselin : Musique et temperament
const double BETHISY[13] =
{
	1,
	1.051418,
	1.118034,
	1.181509,
	1.250000,
	1.331953,
	1.403475,
	1.495348,
	1.575346,
	1.671850,
	1.774101,
	1.872884,
	2,
};

//Big Gulp
const double BIGGULP[13] =
{
	1,
	1.031250,
	1.125000,
	1.166666,
	1.250000,
	1.312500,
	1.375000,
	1.500000,
	1.546875,
	1.687500,
	1.750000,
	1.875000,
	2
};

//12 - tone scale by Bohlen generated from the 4:7 : 10 triad, Acustica 39 / 2, 1978
const double BOHLEN12[13] =
{
	1,
	1.100000,
	1.200000,
	1.304347,
	1.428571,
	1.571428,
	1.750000,
	1.909090,
	2.100000,
	2.300000,
	2.500000,
	2.750000,
	3
};

//This scale may also be called the "Wedding Cake"
const double WEDDING[13] =
{
	1,
	1.125000,
	1.171875,
	1.250000,
	1.333333,
	1.406250,
	1.500000,
	1.562500,
	1.666666,
	1.687500,
	1.777777,
	1.875000,
	2
};

//Upside - Down Wedding Cake(divorce cake)
const double DIVORCE[13] =
{
	1,
	1.066666,
	1.125000,
	1.200000,
	1.280000,
	1.333333,
	1.500000,
	1.600000,
	1.687500,
	1.777777,
	1.800000,
	1.920000,
	2
};

//12 - tone Pythagorean scale
const double PYTHAG1[13] =
{
	1,
	1.067871,
	1.125000,
	1.185185,
	1.265625,
	1.333333,
	1.423828,
	1.500000,
	1.601806,
	1.687500,
	1.777777,
	1.898437,
	2
};

//Robert Schneider, scale of log(4) ..log(16), 1 / 1 = 264Hz
const double LOGSCALE[13] =
{
	1,
	1.160964,
	1.292481,
	1.403677,
	1.500000,
	1.584962,
	1.660964,
	1.729715,
	1.792481,
	1.850219,
	1.903677,
	1.953445,
	2
};

//Zarlino temperament extraordinaire, 1024 - tET mapping
const double ZARLINO1[13] =
{
	1,
	1.041450,
	1.116652,
	1.180385,
	1.247756,
	1.337855,
	1.393309,
	1.494930,
	1.567469,
	1.669316,
	1.777781,
	1.865308,
	2
};

//Fokker's 7-limit 12-tone just scale
const double FOKKER7[13] =
{
	1,
	1.071428,
	1.125000,
	1.166666, 
	1.250000,
	1.333333, 
	1.406250,
	1.500000,
	1.607142, 
	1.666666,
	1.750000, 
	1.875000, 
	2
};

//Bach temperament, a'=400 Hz
const double BACH400[13] =
{
	1,
	1.052192,
	1.118997,
	1.183716,
	1.250521,
	1.334029,
	1.402922,
	1.498956,
	1.578288,
	1.670146,
	1.776618,
	1.874739,
	2
};

//Vallotti& Young scale(Vallotti version) also known as Tartini - Vallotti(1754)
const double VALYOUNG[13] =
{
	1,
	1.055880,
	1.119929,
	1.187865,
	1.254242,
	1.336348,
	1.407840,
	1.496616,
	1.583819,
	1.676105,
	1.781797,
	1.877119,
	2
};

//Vallotti - Young and Werckmeister III, 10 cents 5 - limit lesfip scale
const double VALYOWER[13] =
{
	1,
	1.051637,
	1.117298,
	1.187060,
	1.248356,
	1.336846,
	1.399836,
	1.495457,
	1.580099,
	1.669531,
	1.783574,
	1.867613,
	2
};


///!!!\\\ Testing some non-12 octaves scales here

//Optimally consonant major pentatonic, John deLaubenfels(2001)
const double PENTAOPT[6] =
{
	1,
	1.118042,
	1.250019,
	1.496879,
	1.670166,
	2
};

//Ancient Greek Aeolic, also tritriadic scale of the 54:64 : 81 triad
const double AEOLIC[8] =
{
	1,
	1.125000,
	1.185185,
	1.333333,
	1.500000,
	1.580246,
	1.777777,
	2
};

//African Bapare xylophone(idiophone; loose log)
const double XYLO1[11] =
{
	1,
	1.076737,
	1.200942,
	1.336382,
	1.497441,
	1.670175,
	1.932988,
	2.174725,
	2.285484,
	2.525670,
	2.738400
};

//African Yaswa xylophones(idiophone; calbash resonators with membrane)
const double XYLO2[11] =
{
	1,
	1.128312,
	1.271619,
	1.486239,
	1.707240,
	1.936341,
	2.015074,
	2.215296,
	2.419988,
	2.871225,
	3.220980
};

//19-EDO generated using Scale Workshop
const double NINTENDO[20] =
{
	1,
	1.037155,
	1.075690,
	1.115657,
	1.157110,
	1.200102,
	1.244692,
	1.290939,
	1.338904,
	1.388651,
	1.440246,
	1.493759,
	1.549259,
	1.606822,
	1.666524,
	1.728443,
	1.792664,
	1.859270,
	1.928352,
	2
};


HWND g_hwnd=NULL;
HWND g_viewhwnd=NULL;

HINSTANCE g_c6502_dll;
BOOL volatile g_is6502 = 0;
CString g_aboutpokey;
CString g_about6502;
CString g_driverversion;

BOOL g_changes=0;	//have there been any changes in the module?

int g_focus;
int g_shiftkey;
int g_controlkey;
int g_altkey;	//unfinished implementation, doesn't work yet for some reason

int g_tracks4_8;
BOOL volatile g_screenupdate=0;
BOOL volatile g_invalidatebytimer=0;
int volatile g_screena;
int volatile g_screenwait;
BOOL volatile g_rmtroutine;
BOOL volatile g_timerroutineprocessed;

int volatile g_prove;			//test notes without editing (0 = off, 1 = mono, 2 = stereo)
int volatile g_respectvolume;	//does not change the volume if it is already there

WORD g_rmtstripped_adr_module;	//address for export RMT stripped file
BOOL g_rmtstripped_sfx;			//sfx offshoot RMT stripped file
BOOL g_rmtstripped_gvf;			//gvs GlobalVolumeFade for feat
BOOL g_rmtstripped_nos;			//nos NoStartingSongline for feat

CString g_rmtmsxtext;
CString g_expasmlabelprefix;	//label prefix for export ASM simple notation

int g_activepart;			//0 info, 1 edittracks, 2 editinstruments, 3 song
int g_active_ti;			//1 tracks, 2 instrs

BOOL is_editing_instr;		//0 no, 1 instrument name is edited
BOOL is_editing_infos;		//0 no, 1 song name is edited

int g_tracklinehighlight=8;	//line highlighted every x lines
BOOL g_tracklinealtnumbering=0; //alternative way of line numbering in tracks
int g_linesafter;			//number of lines to scroll after inserting a note (initializes in CSong :: Clear)
BOOL g_ntsc=0;				//NTSC (60Hz)
BOOL g_nohwsoundbuffer=0;	//Don't use hardware soundbuffer
int g_cursoractview=0;		//default position, line 0

BOOL g_displayflatnotes = 0;	//flats instead of sharps
BOOL g_usegermannotation = 0;	//H notes instead of B

int g_channelon[SONGTRACKS];
int g_rmtinstr[SONGTRACKS];

BOOL g_viewmaintoolbar=1;		//1 yes, 0 no
BOOL g_viewblocktoolbar=1;		//1 yes, 0 no
BOOL g_viewstatusbar=1;			//1 yes, 0 no
BOOL g_viewplaytimecounter=1;	//1 yes, 0 no
BOOL g_viewanalyzer=1;			//1 yes, 0 no
BOOL g_viewpokeyregs=1;			//1 yes, 0 no
BOOL g_viewinstractivehelp=1;	//1 yes, 0 no

long g_playtime=1;	//1 yes, 0 no

UINT g_mousebutt=0;			//mouse button

CTrackClipboard g_trackcl;

CString g_prgpath;					//path to the directory from which the program was started (including a slash at the end)
CString g_lastloadpath_songs;		//the path of the last song loaded
CString g_lastloadpath_instruments; //the path of the last instrument loaded
CString g_lastloadpath_tracks;		//the path of the last track loaded

CString g_path_songs;		//default path for songs
CString g_path_instruments;	//default path for instruments
CString g_path_tracks;		//default path for tracks

int g_keyboard_layout=0;	//1 yes, 0 no, not be useful anymore... should be deleted
BOOL g_keyboard_swapenter=0;	//1 yes, 0 no, probably not needed anymore but will be kept for now
BOOL g_keyboard_playautofollow=1;	//1 yes, 0 no
BOOL g_keyboard_updowncontinue=1;	//1 yes, 0 no
BOOL g_keyboard_rememberoctavesandvolumes=1;	//1 yes, 0 no
BOOL g_keyboard_escresetatarisound=1;	//1 yes, 0 no
BOOL g_keyboard_askwhencontrol_s=1;	//1 yes, 0 no

int g_midi_notech[16];			//last recorded notes on each MIDI channel
int g_midi_voluch[16];			//note volume on individual MIDI channels
int g_midi_instch[16];			//last set instrument numbers on individual MIDI channels

BOOL g_midi_tr=0;
int g_midi_volumeoffset=1;	//starts from volume 1 by default
BOOL g_midi_noteoff=0;		//by default, noteoff is turned off

#define COLOR_SELECTED	9	//highlight colour
#define COLOR_SELECTED_PROVE	3	//highlight colour in PROVE mode

//----

void TextXY(char* txt, int x, int y, int c)
{
	int sp = g_scaling_percentage;

	char a;
	c=c<<4;
	for (int i = 0; a = (txt[i]); i++, x += 8)
		if (a != 32) g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, c, 8, 16, SRCCOPY);
}

void TextXYSelN(char *txt,int n,int x,int y,int c=0)
{
	//the index character n will have a "select" color, the rest c
	int sp = g_scaling_percentage;
	char a;
	c=c<<4;

	int col = (g_prove) ? COLOR_SELECTED_PROVE : COLOR_SELECTED;

	int i;
	for (i = 0; a = (txt[i]); i++, x += 8)
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, (i == n) ? col * 16 : c, 8, 16, SRCCOPY);
	if (i == n) //the first character after the end of the string
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, 32 << 3, col * 16, 8, 16, SRCCOPY);
}

void TextXYCol(char *txt,int x,int y,char *col)
{
	int sp = g_scaling_percentage;
	char a;
	for (int i = 0; a = (txt[i]); i++, x += 8)
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, col[i] << 4, 8, 16, SRCCOPY);
}

void TextDownXY(char *txt,int x,int y,int c=0)
{
	int sp = g_scaling_percentage;
	char a;
	c=c<<4;
	for (int i = 0; a = (txt[i]); i++, y += 16)
		g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (16 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, c, 8, 16, SRCCOPY);
}

void NumberMiniXY(BYTE num,int x,int y,int c=0)
{
	int sp = g_scaling_percentage;
	c=112+(c<<3);
	g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (8 * sp) / 100, g_gfx_dc, (num & 0xf0) >> 1, c, 8, 8, SRCCOPY);
	g_mem_dc->StretchBlt(((x + 8) * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (8 * sp) / 100, g_gfx_dc, (num & 0x0f) << 3, c, 8, 8, SRCCOPY);
}

void TextMiniXY(char *txt,int x,int y,int c=0)
{
	int sp = g_scaling_percentage;
	char a;
	c=112+(c<<3);
	for (int i = 0; a = (txt[i]); i++, x += 8)
		if (a != 32) g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (8 * sp) / 100, (8 * sp) / 100, g_gfx_dc, (a & 0x7f) << 3, c, 8, 8, SRCCOPY);
}

void IconMiniXY(int icon,int x,int y)
{
	int sp = g_scaling_percentage;
	static int c=128-6;
	if (icon >= 1 && icon <= 4) g_mem_dc->StretchBlt((x * sp) / 100, (y * sp) / 100, (32 * sp) / 100, (6 * sp) / 100, g_gfx_dc, (icon - 1) * 32, c, 32, 6, SRCCOPY);
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

void UpdateShiftControlKeys()
{
	g_shiftkey=(GetAsyncKeyState(VK_SHIFT)!=NULL);
	g_controlkey=(GetAsyncKeyState(VK_CONTROL)!=NULL);
	g_altkey=(GetAsyncKeyState(VK_MENU)!=NULL);
}

BOOL IsnotMovementVKey(int vk)
{
	//returns 1 if it is not a scroll key
	return (vk!=VK_RIGHT && vk!=VK_LEFT && vk!=VK_UP && vk!=VK_DOWN && vk!=VK_TAB && vk!=13 && vk!=VK_HOME && vk!=VK_END && vk!=VK_PAGE_UP && vk!=VK_PAGE_DOWN && vk!=VK_CAPITAL);
}

int EditText(int vk,int shift,int control, char* txt,int& cur, int max)
{
	//returns 1 if TAB or ENTER was pressed
	max--;
	if (vk==8) //VK_BACKSPACE
	{
		if (cur>0)
		{
			cur--;
			for(int j=cur; j<=max-1; j++) txt[j]=txt[j+1];
			txt[max]=' ';
		}
	}
	else
	if (vk==VK_TAB || vk==13)		//VK_ENTER
	{
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
		if (control) return 0;
		char a=0;
		if (vk>=65 && vk<=90) {	a = (shift)? vk : vk+32; }		//letters - uppercase with SHIFT
		else
		if (vk>=48 && vk<=57) { a = (shift)? *(")!@#$%^&*("+vk-48): vk; }			//numbers - special characters with SHIFT
		else
		if (vk==' ')			a = ' ';	//space
		else
		if (vk==189)	a= (shift)? '_':'-';
		else
		if (vk==187)	a= (shift)? '+':'=';
		else
		if (vk == 219)	a=(shift)? '{' : '[';
		else
		if (vk == 221)	a=(shift)? '}' : ']';
		else
		if (vk==186)	a= (shift)? ':':';';
		else
		if (vk==222)	a= (shift)? '"':'\'';
		else
		if (vk == 188)	a = (shift) ? '<' : ',';
		else
		if (vk == 190)	a = (shift) ? '>' : '.';
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
			int j;
			for (j = max; j >= 0 && (txt[j] == ' '); j--);
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

void WaitForTimerRoutineProcessed()
{
	g_timerroutineprocessed=0;
	while(!g_timerroutineprocessed) Sleep(1);		//waiting
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
	{	//all channels
		if (onoff>=0)
			for(int i=0; i<SONGTRACKS; i++) g_channelon[i] = onoff;	//Settings
		else
			for(int i=0; i<SONGTRACKS; i++) g_channelon[i] ^= 1;	//invert
	}
	else
	if (ch<SONGTRACKS)
	{	//just that one
		if (onoff>=0)
			g_channelon[ch] = onoff;	//Settings
		else
			g_channelon[ch] ^= 1;	//invert
	}
}

int GetChannelOnOff(int ch)	
{ 
	return g_channelon[ch]; 
}

void SetChannelSolo(int ch)
{
	int on = GetChannelOnOff(ch);
	if (!on)
	{
Channel_SOLO:
		SetChannelOnOff(-1,0);	//all off
		SetChannelOnOff(ch,1);	//and solo only active
	}
	else
	{
		for(int i=0; i<g_tracks4_8; i++)
		{
			if(i!=ch && GetChannelOnOff(i)) goto Channel_SOLO;
		}
		SetChannelOnOff(-1,1);	//turn them all on
	}
}

//debug display of g_atarimem bytes directly, slow and terrible, do not use unless there is a purpose for it 
void GetAtariMemHexStr(int adr, int len)
{
	unsigned int a = 0;
	char c[8] = { 0 };
	memset(g_debugmem, 0, 65536);
	for (int i = 0; i < len; i++)
	{
		a = g_atarimem[adr + i];
		sprintf(c, "$%x, ", a);
		g_debugmem[i * 4] = c[0];	//$
		//force uppercase on characters "a" to "f"
		if (c[1] >= 0x61 && c[1] < 0x67) c[1] -= 0x20;
		if (c[2] >= 0x61 && c[2] < 0x67) c[2] -= 0x20;
		if (a > 0xF)	//0x10 and above
		{
			g_debugmem[(i * 4) + 1] = c[1];	//nybble 1
			g_debugmem[(i * 4) + 2] = c[2];	//nybble 2
		}
		else	//single digit hex character, add padding 0
		{
			g_debugmem[(i * 4) + 1] = '0';	//nybble 1
			g_debugmem[(i * 4) + 2] = c[1];	//nybble 2
		}
		if (i != len - 1) g_debugmem[(i * 4) + 3] = ','; 
		else g_debugmem[(i * 4) + 3] = ' ';	//, or space if last character
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

char NoteKey(int vk)	{ return keynotes[vk]; };
char NumbKey(int vk)	{ return keynumbs[vk]; };
char Numblock09Key(int vk)	{ return keynumblock09[vk]; };

#endif