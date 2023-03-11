
#include "StdAfx.h"
#include "WaveFile.h"

bool CWaveFile::OpenFile(LPTSTR Filename, int SampleRate, int SampleSize, int Channels)
{
	int nError;

	WaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.wf.nChannels = Channels;	
	WaveFormat.wf.nSamplesPerSec = SampleRate;
	WaveFormat.wBitsPerSample = SampleSize;
	WaveFormat.wf.nBlockAlign = (WaveFormat.wBitsPerSample / 8) * WaveFormat.wf.nChannels;
	WaveFormat.wf.nAvgBytesPerSec = WaveFormat.wf.nSamplesPerSec * WaveFormat.wf.nBlockAlign;

	hmmioOut = mmioOpen(Filename, NULL, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);

	ckOutRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	ckOutRIFF.cksize = 0;

	nError = mmioCreateChunk(hmmioOut, &ckOutRIFF, MMIO_CREATERIFF);

	if (nError != MMSYSERR_NOERROR)
		return false;

	ckOut.ckid = mmioFOURCC('f', 'm', 't', ' ');
	ckOut.cksize = sizeof(PCMWAVEFORMAT);

	nError = mmioCreateChunk(hmmioOut, &ckOut, 0);

	if (nError != MMSYSERR_NOERROR)
		return false;

	mmioWrite(hmmioOut, (HPSTR)&WaveFormat, sizeof(PCMWAVEFORMAT));
	mmioAscend(hmmioOut, &ckOut, 0);

	ckOut.ckid = mmioFOURCC('d', 'a', 't', 'a');
	ckOut.cksize = 0;

	nError = mmioCreateChunk(hmmioOut, &ckOut, 0);

	if (nError != MMSYSERR_NOERROR)
		return false;

	mmioGetInfo(hmmioOut, &mmioinfoOut, 0);

	return true;
}

void CWaveFile::CloseFile()
{
	mmioinfoOut.dwFlags |= MMIO_DIRTY;
	mmioSetInfo(hmmioOut, &mmioinfoOut, 0);
	mmioAscend(hmmioOut, &ckOut, 0);
	mmioAscend(hmmioOut, &ckOutRIFF, 0);
	mmioSeek(hmmioOut, 0, SEEK_SET);
	mmioDescend(hmmioOut, &ckOutRIFF, NULL, 0);
	mmioClose(hmmioOut, 0);
}

void CWaveFile::WriteWave(BYTE* Data, int Size)
{
	for (int i = 0; i < Size; i++)
	{
		if (mmioinfoOut.pchNext == mmioinfoOut.pchEndWrite)
		{
			mmioinfoOut.dwFlags |= MMIO_DIRTY;
			mmioAdvance(hmmioOut, &mmioinfoOut, MMIO_WRITE);
		}
		*((BYTE*)mmioinfoOut.pchNext) = *((BYTE*)Data + i);
		mmioinfoOut.pchNext++;
	}
}