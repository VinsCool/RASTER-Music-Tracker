//
// XPokey.h header file
// originally made by Raster, 2002-2009
// reworked by VinsCool, 2021-2022
//

#if !defined(_XPOKEY_H_)
#define _XPOKEY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CHANNELS		2
#define BITRESOLUTION	8
#define OUTPUTFREQ		44100		//22050		//44100
#define BUFFER_SIZE		0x8000		//must be a power of 2
#define CHUNK_SIZE_NTSC		BITRESOLUTION/8*CHANNELS*OUTPUTFREQ/60		//sixties (CHUNK_SIZE (tm) by VinsCool lol) 
#define CHUNK_SIZE_PAL		BITRESOLUTION/8*CHANNELS*OUTPUTFREQ/50		//fifties (CHUNK_SIZE (tm) by JirkaS)

#define LATENCY			3			//3/50sec
#define LATENCY_SIZE	LATENCY*CHUNK_SIZE
#define FREQ_17_NTSC	1789773		//The true clock frequency for the NTSC Atari 8-bit computer is 1.7897725 MHz
#define FREQ_17_PAL		1773447		//The true clock frequency for the PAL Atari 8-bit computer is 1.7734470 MHz

#define FRAMERATE_NTSC	60
#define FRAMERATE_PAL	50

#define CYCLESPERSCREEN ((float)CYCLESPERSECOND/FRAMERATE)
#define CYCLESPERSAMPLE	((float)CYCLESPERSECOND/44100)

class CXPokey
{
// Construction
public:
	CXPokey();
	~CXPokey();
	BOOL InitSound();
	BOOL DeInitSound();
	BOOL ReInitSound();
	BOOL RenderSound1_50(int instrspeed);
	void MemToPokey();
	bool IsSoundDriverLoaded() { return m_soundDriverId; }

private:
	int volatile		m_soundDriverId;
	HINSTANCE			m_pokey_dll;
	DWORD				m_LoadPos;
	WAVEFORMATEX*		m_SoundFormat;
	DWORD				m_LoadSize;
	DSBUFFERDESC        dsbdesc;
	LPDIRECTSOUNDBUFFER m_SoundBuffer;
	DWORD				dwSize1, dwSize2;
	LPVOID				Data1, Data2;
	BYTE				m_PlayBuffer[BUFFER_SIZE];	//rendered part of the swing CHUNK_SIZE + - something (but it can be much bigger)
	DWORD				m_PlayCursor;  
	DWORD				m_WriteCursor;
	DWORD				m_WriteCursorStart;

	int InitPokeyDll();
};

#endif
