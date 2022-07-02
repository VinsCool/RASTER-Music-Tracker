/*
	Atari6502.h
	CPU EMULATION INTERFACE + ATARI BINARY FILE FUNCTIONS
	(c) Raster/C.P.U. 2003
*/

//#include "r_music.h"

#ifndef __ATARI6502__
#define __ATARI6502__

extern HWND g_hwnd;

extern unsigned char g_atarimem[65536];

#define SONGTRACKS	8
extern int g_rmtinstr[SONGTRACKS];

#define RMT_FRQTABLES	0x31c0				//basy16bit low byte, basy 0C, basy2 0e, ciste tony 0a a 0,2,4,8, basy16bit hi byte

#define RMT_INIT		0x3400
#define RMT_P3			0x3406
#define RMT_SILENCE		0x3409
#define RMT_SETPOKEY	0x340c

#define RMT_ATA_SETNOTEINSTR	0x3d00
#define RMT_ATA_SETVOLUME		0x3e00
#define RMT_ATA_INSTROFF		0x3e80

#define MAXSCREENCYCLES		114*312			//maximalni pocet taktu pro celou obrazovku

void Memory_Clear();
//int GO(int goaddr,unsigned char A,unsigned char X,unsigned char Y);
//int __declspec(dllexport) C6502_JSR(WORD* adr, BYTE* areg, BYTE* xreg, BYTE* yreg, int* maxcycles);
int LoadBinaryBlock(ifstream& in,unsigned char* memory,WORD& fromadr, WORD& toadr);
int LoadBinaryFile(char *fname, unsigned char *memory,WORD& minadr,WORD& maxadr);
int LoadDataAsBinaryFile(unsigned char *data, WORD size, unsigned char *memory,WORD& minadr,WORD& maxadr);
int SaveBinaryBlock(ofstream& out,unsigned char* memory,WORD fromadr,WORD toadr,BOOL ffffhead);

int Atari_LoadRMTRoutines();
int Atari_InitRMTRoutine();
void Atari_PlayRMT();
void Atari_Silence();
void Atari_SetTrack_NoteInstrVolume(int t,int n,int i,int v);
void Atari_SetTrack_Volume(int t,int v);
void Atari_InstrumentTurnOff(int instr);

extern HINSTANCE g_c6502_dll;
extern BOOL volatile g_is6502;
extern CString g_about6502;

extern CString g_prgpath;

extern void TextXY(char *txt,int x,int y,int c);

#endif
