#pragma once

#include "StdAfx.h"
#include <mmsystem.h>

class CWaveFile
{
public:
	bool OpenFile(LPTSTR Filename, int SampleRate, int SampleSize, int Channels);
	void CloseFile();
	void WriteWave(BYTE* Data, int Size);

private:
	PCMWAVEFORMAT WaveFormat;
	MMCKINFO ckOutRIFF, ckOut;
	MMIOINFO mmioinfoOut;
	HMMIO hmmioOut;
};