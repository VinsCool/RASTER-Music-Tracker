/*
	Atari6502.cpp
	CPU EMULATION INTERFACE + ATARI BINARY FILE FUNCTIONS
	(c) Raster/C.P.U. 2003
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>	/* need for Load/SaveBinaryFile */

using namespace std;

//#include "MainFrm.h"
//#include "global.h"

#include "Atari6502.h"

/*
	C6502_Initialise	@1
	C6502_JSR			@2
	C6502_About	 		@3

void __declspec(dllexport) C6502_Initialise(BYTE* memory );
int __declspec(dllexport) C6502_JSR(WORD* adr, BYTE* areg, BYTE* xreg, BYTE* yreg, int* maxcycles);
void __declspec(dllexport) C6502_About(char** name, char** author, char** description );
*/

typedef void (* C6502_Initialise_PROC)(BYTE*);
typedef int  (* C6502_JSR_PROC)(WORD* , BYTE* , BYTE* , BYTE* , int* );
typedef void (* C6502_About_PROC)(char**, char**, char**);

C6502_Initialise_PROC C6502_Initialise;
C6502_JSR_PROC C6502_JSR;
C6502_About_PROC C6502_About;

void Atari6502_DeInit()
{
	g_is6502 = 0;

	if (g_c6502_dll)
	{
		FreeLibrary(g_c6502_dll);
		g_c6502_dll = NULL;
	}
	g_about6502 = "No Atari 6502 CPU emulation.";
}

int Atari6502_Init()
{
	if (g_c6502_dll) Atari6502_DeInit();	//pro jistotu

	g_c6502_dll=LoadLibrary("sa_c6502.dll");
	if(!g_c6502_dll)
	{
		MessageBox(g_hwnd,"Warning:\n'sa_c6502.dll' library not found,\ntherefore the Atari sound routines can't be performed.","LoadLibrary error",MB_ICONEXCLAMATION);
		Atari6502_DeInit();
		return 1;
	}

	CString wrn="";

	C6502_Initialise = (C6502_Initialise_PROC) GetProcAddress(g_c6502_dll,"C6502_Initialise");
	if(!C6502_Initialise) wrn +="C6502_Initialise\n";

	C6502_JSR = (C6502_JSR_PROC) GetProcAddress(g_c6502_dll,"C6502_JSR");
	if(!C6502_JSR) wrn +="C6502_JSR\n";

	C6502_About = (C6502_About_PROC) GetProcAddress(g_c6502_dll,"C6502_About");
	if(!C6502_About) wrn +="C6502_About\n";

	if (wrn!="")
	{
		MessageBox(g_hwnd,"Error:\nNot compatible 'sa_c6502.dll',\ntherefore the Atari sound routines can't be performed.\nIncompatibility with:" +wrn,"C6502 library error",MB_ICONEXCLAMATION);
		Atari6502_DeInit();
		return 1;
	}

	//Text pro About dialog
	if (g_c6502_dll)
	{
		char *name, *author, *description;
		C6502_About(&name,&author,&description);
		g_about6502.Format("%s\n%s\n%s",name,author,description);
	}

	C6502_Initialise(g_atarimem);

	g_is6502 = 1;

	return 1;
}


//extern unsigned char g_atarimem[65536];
//extern void Memory_Init();
//extern int GO(int goaddr,unsigned char A,unsigned char  X,unsigned char Y);
//extern int C6502_JSR(WORD* adr, BYTE* areg, BYTE* xreg, BYTE* yreg, int* maxcycles);

#include "res/data_rmt_ata_sys.cpp"

int Atari_LoadRMTRoutines()
{
	//nacte rmt rutinu $3400, setnoteinstrvol $3d00, setvol $3e00
	WORD min,max;
	//int r = LoadBinaryFile((char*)(LPCSTR)(g_prgpath+"rmt_ata.sys"),g_atarimem,min,max);
	int r=LoadDataAsBinaryFile(data_rmt_ata_sys,sizeof(data_rmt_ata_sys),g_atarimem,min,max);
	return r;
}

int Atari_InitRMTRoutine()
{
	if (!g_is6502) return 0;
	//int r = GO(0x3400,0,0x00,0x3f);			//adr,A,X,Y
	WORD adr=RMT_INIT;
	BYTE a=0, x=0x00, y=0x3f;
	int cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y

	for(int i=0; i<SONGTRACKS; i++)
	{
		g_rmtinstr[i]=-1;
	}

	return (int)a;
}

void Atari_PlayRMT()
{
	if (!g_is6502) return;

	WORD adr=RMT_P3; //(bez SetPokey) jeden prubeh RMT rutinou, ale az od rmt_p3 (zpracovani obalek)
	BYTE a=0, x=0, y=0;
	int cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y

	//GO(0x340c,0,0,0);					//SetPokey
	adr=RMT_SETPOKEY;
	a=x=y=0;
	//cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y

	/*
	CString s;
	s.Format("CPU cycles: %i",MAXSCREENCYCLES-cycles);
	//mf->m_wndStatusBar.SetWindowText(s);
	TextXY((char*)(LPCTSTR)s,0,0,0);
	*/
}

void Atari_Silence()
{
	if (!g_is6502) return;

	//Silence rutina
	//GO(0x3409,0,0,0);
	WORD adr=RMT_SILENCE;
	BYTE a=0, x=0, y=0;
	int cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y
}

void Atari_SetTrack_NoteInstrVolume(int t,int n,int i,int v)
{
	if (!g_is6502) return;

	//GO(0x3d00,n,t,i);		//adr, nota, track, instrument
	WORD adr=RMT_ATA_SETNOTEINSTR;
	BYTE a=n, x=t, y=i;
	int cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y

	//GO(0x3e00,v,t,0);		//adr, volume, track
	adr=RMT_ATA_SETVOLUME;
	a=v; x=t; y=0;
	cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y

	g_rmtinstr[t]=i;
}

void Atari_SetTrack_Volume(int t,int v)
{
	if (!g_is6502) return;

	//GO(0x3e00,v,t,0);		//adr, volume, track
	WORD adr=RMT_ATA_SETVOLUME;
	BYTE a=v, x=t, y=0;
	int cycles=MAXSCREENCYCLES;
	C6502_JSR(&adr,&a,&x,&y,&cycles);			//adr,A,X,Y
}

void Atari_InstrumentTurnOff(int instr)
{
	if (!g_is6502) return;

	int cycles=MAXSCREENCYCLES;
	for(int i=0; i<SONGTRACKS; i++)
	{
		if (g_rmtinstr[i]==instr)
		{
			WORD adr=RMT_ATA_INSTROFF;
			BYTE a=0,x=i,y=0;
			C6502_JSR(&adr,&a,&x,&y,&cycles);			//ztisi a vypne tento instrument
			g_atarimem[0xd200+i*2+1+(i>=4)*16]=0;		//vynuluje memory POKEY audctl
			g_rmtinstr[i]=-1;
		}
	}
}


void Memory_Clear()
{
	memset(g_atarimem,0,65536);
}

bool LoadWord(ifstream& in, WORD & w)
{
	unsigned char a1, a2;
	char db,hb;
	if (in.eof()) return false;
	in.get(db);
	a1 = (unsigned char)db;
	if (in.eof()) return false;
	in.get(hb);	
	a2 = (unsigned char)hb;
	w=a1 + (a2<<8);
	return true;
}

int LoadBinaryBlock(ifstream& in,unsigned char* memory,WORD& fromadr, WORD& toadr)
//Return number of bytes red. (0 if there was some error).
{
	if (!LoadWord(in, fromadr)) return 0;
	if (fromadr==0xffff)
	{
		if (!LoadWord(in, fromadr)) return 0;
	}
	if (!LoadWord(in, toadr)) return 0;

	if (toadr<fromadr) return 0;
	in.read((char*)memory+fromadr,toadr-fromadr+1);
	return toadr-fromadr+1;
}

int LoadBinaryFile(char *fname, unsigned char *memory,WORD& minadr,WORD& maxadr)
{
	int fsize,blen;
	
	WORD bfrom,bto;

	ifstream fin(fname,ios::binary|ios::_Nocreate);
	if (!fin) return 0;
	fsize=0;
	minadr = 0xffff; maxadr=0; //opacne meze minimalni a maximalni adresy
	while(!fin.eof())
	{
		blen = LoadBinaryBlock(fin,memory,bfrom,bto);
		if (blen<=0) break;
		if (bfrom<minadr) minadr=bfrom;
		if (bto>maxadr) maxadr=bto;
		fsize+=blen;
	}
	fin.close();
	return fsize;
}

int LoadDataAsBinaryFile(unsigned char *data, WORD size, unsigned char *memory,WORD& minadr,WORD& maxadr)
{
	if (!data) return 0;

	int blen;
	int akp=0;
	WORD bfrom,bto;

	minadr = 0xffff; maxadr=0; //opacne meze minimalni a maximalni adresy
	while(akp<size)
	{
		bfrom = data[akp]|(data[akp+1]<<8);
		akp+=2;
		if (bfrom==0xffff) continue;
		bto = data[akp]|(data[akp+1]<<8);
		akp+=2;
		blen = bto-bfrom+1;
		if (blen<=0) break;
		memcpy(memory+bfrom,data+akp,blen);
		akp+=blen;
		if (bfrom<minadr) minadr=bfrom;
		if (bto>maxadr) maxadr=bto;
	}
	return akp;
}

int SaveBinaryBlock(ofstream& out,unsigned char* memory,WORD fromadr,WORD toadr,BOOL ffffhead)
{
	//od "fromadr" po "toadr" vcetne
	if (fromadr>toadr) return 0;
	if (ffffhead)
	{
		out.put((unsigned char)0xff);
		out.put((unsigned char)0xff);
	}
	out.put((unsigned char)(fromadr & 0xff));
	out.put((unsigned char)(fromadr >> 8));
	out.put((unsigned char)(toadr & 0xff));
	out.put((unsigned char)(toadr >> 8));
	out.write((char*)memory+fromadr,toadr-fromadr+1);
	return toadr-fromadr+1;	
}


