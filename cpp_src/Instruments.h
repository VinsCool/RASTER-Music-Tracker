#pragma once
#include "stdafx.h"
#include <fstream>

#include "General.h"


struct TInstrInfo
{
	int count;
	int usedintracks;
	int instrfrom, instrto;
	int minnote, maxnote;
	int minvol, maxvol;
};


struct TInstrument
{
	int act;
	char name[INSTRNAMEMAXLEN + 1];
	int activenam;
	int par[PARCOUNT];						//[24]
	int activepar;
	int env[ENVCOLS][ENVROWS];				//[32][8]
	int activeenvx, activeenvy;
	int tab[TABLEN];
	int activetab;
	int octave;
	int volume;
};


struct TInstrumentsAll		//for undo
{
	TInstrument instruments[INSTRSNUM];
};



class CInstruments
{
public:
	CInstruments();
	BOOL InitInstruments();
	BOOL ClearInstrument(int it);
	//void RandomInstrument(int it);	

	void CheckInstrumentParameters(int instr);
	BOOL RecalculateFlag(int instr);
	BOOL CalculateNoEmpty(int instr);
	void SetEnvVolume(int instr, BOOL right, int px, int py);
	int GetFrequency(int instr, int note);
	int GetNote(int instr, int note);
	void MemorizeOctaveAndVolume(int instr, int oct, int vol);
	void RememberOctaveAndVolume(int instr, int& oct, int& vol);

	BYTE GetFlag(int instr) { return m_iflag[instr]; };
	char* GetName(int it) { return m_instr[it].name; };
	TInstrumentsAll* GetInstrumentsAll() { return (TInstrumentsAll*)m_instr; };

	// GUI
	BOOL DrawInstrument(int it);

	BOOL GetInstrArea(int instr, int zone, CRect& rect);
	BOOL CursorGoto(int instr, CPoint point, int pzone);

	// IO
	BOOL ModificationInstrument(int it);

	int SaveAll(std::ofstream& ou, int iotype);
	int LoadAll(std::ifstream& in, int iotype);

	int SaveInstrument(int instr, std::ofstream& ou, int iotype);
	int LoadInstrument(int instr, std::ifstream& in, int iotype);

	BYTE InstrToAta(int instr, unsigned char* ata, int max);
	BYTE InstrToAtaRMF(int instr, unsigned char* ata, int max);
	BOOL AtaToInstr(unsigned char* ata, int instr);

	BOOL AtaV0ToInstr(unsigned char* ata, int instr);	//Due to the loading of the old version

	// Data
	TInstrument m_instr[INSTRSNUM];

private:
	BOOL DrawName(int it);
	BOOL DrawPar(int p, int it);
	void DrawEnv(int e, int it);
	BOOL DrawTab(int p, int it);

	BYTE m_iflag[INSTRSNUM];
};
