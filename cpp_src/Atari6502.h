/*
	Atari6502.h
	CPU EMULATION INTERFACE + ATARI BINARY FILE FUNCTIONS
	(c) Raster/C.P.U. 2003
*/

#ifndef __ATARI6502__
#define __ATARI6502__

#include "Tuning.h"

//bass16bit low byte, bass 0C, bass 0E, clean tones 0A and 0,2,4,8, bass16bit hi byte, this might require different addresses? What is this even used for anyway?
#define RMT_FRQTABLES	0xB000				

#define RMT_INIT		0x3400
#define RMT_PLAY		0x3403
#define RMT_P3			0x3406
#define RMT_SILENCE		0x3409
#define RMT_SETPOKEY	0x340c

#define RMT_ATA_SETNOTEINSTR	0x3d00
#define RMT_ATA_SETVOLUME		0x3e00
#define RMT_ATA_INSTROFF		0x3e80

//immediately after RMT_ATA_INSTROFF, there is some bytes left unused, these will be used as plaintext data to display the RMT driver version used
#define RMT_ATA_DRIVERVERSION	0x3e90		

//maximum clock count for the entire screen in PAL (default) and NTSC region
#define MAXSCREENCYCLES_NTSC	114*262
#define MAXSCREENCYCLES_PAL 	114*312

extern void Memory_Clear();
extern int LoadBinaryBlock(std::ifstream& in,unsigned char* memory,WORD& fromadr, WORD& toadr);
extern int LoadBinaryFile(char *fname, unsigned char *memory,WORD& minadr,WORD& maxadr);
extern int LoadDataAsBinaryFile(unsigned char *data, WORD size, unsigned char *memory,WORD& minadr,WORD& maxadr);
extern int SaveBinaryBlock(std::ofstream& out, unsigned char* memory, WORD fromAddr, WORD toAddr, BOOL withBinaryBlockHeader);

extern int Atari_LoadRMTRoutines();
extern int Atari_InitRMTRoutine();
extern void Atari_PlayRMT();
extern void Atari_Silence();
extern void Atari_SetTrack_NoteInstrVolume(int t,int n,int i,int v);
extern void Atari_SetTrack_Volume(int t,int v);
extern void Atari_InstrumentTurnOff(int instr);

extern int Atari6502_Init();
extern void Atari6502_DeInit();

//hack: optionally display the RMT driver version using the plaintext data from tracker.obx
extern void Get_Driver_Version();

#endif
