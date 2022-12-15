// Original code by Raster, 2002-2009
// Experimental changes and additions by VinsCool, 2021-2022
// TODO: fix apokeysnd support

#include "stdafx.h"
#include "Rmt.h"
#include "XPokey.h"
#include "RmtView.h"
#include "Atari6502.h"
#include "global.h"
#include "ChannelControl.h"
#include "PokeyStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef enum 
{
	ASAP_FORMAT_U8 = 8,       /* unsigned char */
	ASAP_FORMAT_S16_LE = 16,  /* signed short, little-endian */
	ASAP_FORMAT_S16_BE = -16  /* signed short, big-endian */
} ASAP_SampleFormat;

typedef enum
{
	SOUND_DRIVER_NONE,
	SOUND_DRIVER_APOKEYSND,
	SOUND_DRIVER_SA_POKEY
} POKEY_SoundDriver;

typedef int abool;

typedef void (* APokeySound_Initialize_PROC)(abool stereo);
typedef void (* APokeySound_PutByte_PROC)(int addr, int data);
typedef int  (* APokeySound_GetRandom_PROC)(int addr, int cycle);
typedef int  (* APokeySound_Generate_PROC)(int cycles, byte buffer[], ASAP_SampleFormat format);
typedef void (* APokeySound_About_PROC)(const char **name, const char **author, const char **description);

APokeySound_Initialize_PROC APokeySound_Initialize;
APokeySound_PutByte_PROC APokeySound_PutByte;
APokeySound_GetRandom_PROC APokeySound_GetRandom;	// Unused?
APokeySound_Generate_PROC APokeySound_Generate;
APokeySound_About_PROC APokeySound_About;

typedef void (* Pokey_Initialise_PROC)(int*, char**);
typedef void (* Pokey_SoundInit_PROC)(DWORD, WORD, BYTE);
typedef void (* Pokey_Process_PROC)(BYTE*, const WORD);
typedef BYTE (* Pokey_GetByte_PROC)(WORD);
typedef void (* Pokey_PutByte_PROC)(WORD, BYTE);
typedef void (* Pokey_About_PROC)(char**, char**, char**);

Pokey_Initialise_PROC Pokey_Initialise;
Pokey_SoundInit_PROC Pokey_SoundInit;
Pokey_Process_PROC Pokey_Process;
Pokey_GetByte_PROC Pokey_GetByte;	// Unused?
Pokey_PutByte_PROC Pokey_PutByte;
Pokey_About_PROC Pokey_About;

// Needed for proper Machine Region and Stereo detection with POKEY plugins
int numTracksSetOnDriver = g_tracks4_8;
int ntscRegionSetOnDriver = g_ntsc;

static LPDIRECTSOUND          g_lpds;
static LPDIRECTSOUNDBUFFER    g_lpdsbPrimary;

extern CPokeyStream g_PokeyStream;

CXPokey::CXPokey()
{
	m_soundDriverId = SOUND_DRIVER_NONE;
	m_pokey_dll = NULL;
	m_SoundBuffer = NULL;
}

CXPokey::~CXPokey()
{
	DeInitSound();
}

BOOL CXPokey::DeInitSound()
{
	m_soundDriverId = SOUND_DRIVER_NONE;
	if (m_pokey_dll)
	{
		FreeLibrary(m_pokey_dll);
		m_pokey_dll = NULL;
	}
	g_aboutpokey = "No Pokey sound emulation.";

	if (m_SoundBuffer)
	{
		m_SoundBuffer->Stop();
		m_SoundBuffer->Release();
	}
	m_SoundBuffer = NULL;

	if (g_lpdsbPrimary) g_lpdsbPrimary->Release();
	g_lpdsbPrimary = NULL;

	if (g_lpds) g_lpds->Release();
	g_lpds = NULL;

	return 1;
}

BOOL CXPokey::ReInitSound()
{
	DeInitSound();
	return InitSound();
}

BOOL CXPokey::RenderSound1_50(int instrspeed)
{
	if (!m_soundDriverId) return 0;
	if (!m_SoundBuffer) return 0;

	m_SoundBuffer->GetCurrentPosition(&m_PlayCursor, &m_WriteCursor);

	//|||||||||||||||||||||||||||||||||||||||||
	//           ^|-------delta------->^
	//      m_WriteCursor           m_LoadPos

	int delta = (m_LoadPos - m_WriteCursor) & (BUFFER_SIZE - 1);

	m_LoadSize = CHUNK_SIZE;	//1764  (882 samples * 2 channels)
	if (delta > LATENCY_SIZE)
	{
		if (delta > (BUFFER_SIZE / 2))
		{
			//we missed it, we're more than half a buffer late, so get to it and move on to what we should be right
			m_LoadPos = (m_WriteCursor + LATENCY_SIZE) & (BUFFER_SIZE - 1);
		}
		else
		{
			//we ran too far ahead so we slow down a bit (we will render smaller pieces than CHUNK_SIZE)
			m_LoadSize = CHUNK_SIZE - (((delta - LATENCY_SIZE) / 16) & (BUFFER_SIZE - 1 - 1));	//-1-1 <=Just the numbers!
			//watched
			if (m_LoadSize <= 0) return 0; //we are so far ahead that it will not render at all
		}
	}
	else // delta <=LATENCY_SIZE
	{
		//we are closer than the required latency, that's great, but we'd rather speed up so that m_WriteCursor doesn't catch up with us (we'll render bigger chunks than CHUNK_SIZE)
		m_LoadSize = CHUNK_SIZE + (((LATENCY_SIZE - delta) / 16) & (BUFFER_SIZE - 1 - 1));	//-1-1 <=Just the numbers!

		//watched
		if (m_LoadSize >= BUFFER_SIZE / 2) return 0; //that would be a bigger piece than the size of a buffer pulse
	}

	int rendersize = m_LoadSize;
	int renderpartsize = 0;
	int renderoffset = 0;

	for (; instrspeed > 0; instrspeed--)
	{
		//--- RMT - instrument play ---/
		if (g_rmtroutine) Atari_PlayRMT();	//one run RMT routine (instruments)
		MemToPokey();			//transfer from g_atarimem to POKEY (mono or stereo)
		renderpartsize = (rendersize / instrspeed) & 0xfffe;	//just the numbers

		switch (m_soundDriverId)
		{
		case SOUND_DRIVER_APOKEYSND:	// FIXME: Mono POKEY sound generation is broken, currently the reason for this is unclear...
			{
				int cycles = (unsigned short)((float)renderpartsize / CHANNELS * CYCLESPERSAMPLE);
				while (cycles > 0 && renderpartsize > 0)
				{
					// The maximum number of cycles that can be generated is CYCLESPERSCREEN
					int rencyc = (cycles > CYCLESPERSCREEN) ? (int)CYCLESPERSCREEN : cycles;
					renderpartsize = APokeySound_Generate(rencyc, (unsigned char*)&m_PlayBuffer + renderoffset, ASAP_FORMAT_U8);
					rendersize -= renderpartsize;
					renderoffset += renderpartsize;
					cycles -= rencyc;
				}
			}
			break;

		case SOUND_DRIVER_SA_POKEY:
			Pokey_Process((unsigned char*)&m_PlayBuffer + renderoffset, (unsigned short)renderpartsize);
			rendersize -= renderpartsize;
			renderoffset += renderpartsize;
			break;
		}
	}

	if (!m_SoundBuffer) return 0;	// Should help preventing crashes from reading NULL pointer when data is read faster than it could be processed

	m_LoadSize = renderoffset; // Actually generated sample data

	int r = m_SoundBuffer->Lock(m_LoadPos, m_LoadSize, &Data1, &dwSize1, &Data2, &dwSize2, 0);

	if (r == DS_OK)
	{
		// Render the whole m_LoadSize into the buffer at once
		m_LoadPos = (m_LoadPos + m_LoadSize) & (BUFFER_SIZE - 1);

		// Transfer the first bit from the buffer to Data1
		memcpy(Data1, m_PlayBuffer, dwSize1);

		if (Data2)
		{
			// If it is divided, now transfer the remaining bits
			memcpy(Data2, m_PlayBuffer + dwSize1, dwSize2);
		}
		m_SoundBuffer->Unlock(Data1, dwSize1, Data2, dwSize2);

		return 1;
	}

	return 0;
}

BOOL CXPokey::InitSound()
{
	if (m_soundDriverId || m_pokey_dll) DeInitSound();	// Just in case, everything must be cleared before initialising

	if (DirectSoundCreate(NULL, &g_lpds, NULL) != DS_OK)
	{
		MessageBox(g_hwnd, "Error: DirectSoundCreate", "DirectSound Error!", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// Set cooperative level
	if (g_lpds->SetCooperativeLevel(AfxGetApp()->GetMainWnd()->m_hWnd, DSSCL_PRIORITY) != DS_OK)
	{
		MessageBox(g_hwnd, "Error: SetCooperativeLevel", "DirectSound Error!", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	// Set the emulated Machine Region and if Stereo is used
	numTracksSetOnDriver = g_tracks4_8;
	ntscRegionSetOnDriver = g_ntsc;

	WAVEFORMATEX wfm;

	// Set primary buffer format
	ZeroMemory(&wfm, sizeof(WAVEFORMATEX));
	wfm.wFormatTag = WAVE_FORMAT_PCM;
	wfm.nChannels = CHANNELS;			//2
	wfm.nSamplesPerSec = OUTPUTFREQ;	//44100
	wfm.wBitsPerSample = BITRESOLUTION;	//8 
	wfm.nBlockAlign = wfm.wBitsPerSample / 8 * wfm.nChannels;
	wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
	wfm.cbSize = 0;

	// Create primary buffer.
	ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

	if (g_lpds->CreateSoundBuffer(&dsbdesc, &g_lpdsbPrimary, NULL) != DS_OK)
	{
		MessageBox(g_hwnd, "Error: CreatePrimarySoundBuffer", "DirectSound Error!", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	if (g_lpdsbPrimary->SetFormat(&wfm) != DS_OK)
	{
		MessageBox(g_hwnd, "Error: SetFormat", "DirectSound Error!", MB_OK | MB_ICONSTOP);
		return FALSE;
	}

	ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCHARDWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS;
	dsbdesc.dwBufferBytes = BUFFER_SIZE;
	dsbdesc.lpwfxFormat = &wfm;

	if (g_nohwsoundbuffer ||
		g_lpds->CreateSoundBuffer(&dsbdesc, &m_SoundBuffer, NULL) != DS_OK)
	{
		dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS;
		if (g_lpds->CreateSoundBuffer(&dsbdesc, &m_SoundBuffer, NULL) != DS_OK)
		{
			MessageBox(g_hwnd, "Error: CreateSoundBuffer", "DirectSound Error!", MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}

	DSBCAPS bc;
	bc.dwSize = sizeof(bc);
	m_SoundBuffer->GetCaps(&bc);

	int r = m_SoundBuffer->Lock(0, BUFFER_SIZE, &Data1, &dwSize1, &Data2, &dwSize2, DSBLOCK_FROMWRITECURSOR);
	if (r == DS_OK)
	{
		memset(Data1, 0x80, dwSize1);
		if (Data2) memset(Data2, 0x80, dwSize2);
		m_SoundBuffer->Unlock(Data1, dwSize1, Data2, dwSize2);
	}

	m_SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
	m_SoundBuffer->GetCurrentPosition(&m_PlayCursor, &m_WriteCursorStart);
	m_LoadPos = (m_WriteCursorStart + LATENCY_SIZE) & (BUFFER_SIZE - 1);  //initial latency (in hundredths of a second)

	// Initialise the POKEY emulation plugin once the sound interface is ready
	m_soundDriverId = InitPokeyDll();

	return 1;
}

/// <summary>
/// Transfer 9 Pokey registers values into the sound driver.
/// Mono: D200-D208
/// Stereo: D200-D208 and D210-D218
/// </summary>
void CXPokey::MemToPokey()
{
	// If the variabes no longer match the last known parameters, the POKEY plugins must be re-initialised first
	bool resetPokey = false;
	if (numTracksSetOnDriver != g_tracks4_8 || ntscRegionSetOnDriver != g_ntsc)
	{
		numTracksSetOnDriver = g_tracks4_8;
		ntscRegionSetOnDriver = g_ntsc;
		resetPokey = true;
		//ReInitSound();
		//return;
	}

	// Check for which POKEY plugin to use, and process whichever is currently active
	switch (m_soundDriverId)
	{
	case SOUND_DRIVER_APOKEYSND:
		if (resetPokey) 
			APokeySound_Initialize(g_tracks4_8 == 8);
		for (int i = 0; i <= 8; i++)	// 0-7 + 8 (AUDCTL)
		{
			APokeySound_PutByte(i, (i & 0x01) && !GetChannelOnOff(i / 2) ? 0 : g_atarimem[0xd200 + i]);
			if (numTracksSetOnDriver == 8)
				APokeySound_PutByte(i + 16, (i & 0x01) && !GetChannelOnOff(i / 2 + 4) ? 0 : g_atarimem[0xd210 + i]);	// Stereo
		}
		break;

	case SOUND_DRIVER_SA_POKEY:
		if (resetPokey) 
			Pokey_SoundInit(FREQ_17, OUTPUTFREQ, (g_tracks4_8 == 8) + 1);
		for (int i = 0; i <= 8; i++)	// 0-7 + 8 (AUDCTL)
		{
			Pokey_PutByte(i, (i & 0x01) && !GetChannelOnOff(i / 2) ? 0 : g_atarimem[0xd200 + i]);
			if (numTracksSetOnDriver == 8)
				Pokey_PutByte(i + 16, (i & 0x01) && !GetChannelOnOff(i / 2 + 4) ? 0 : g_atarimem[0xd210 + i]);		// Stereo
		}
		break;
	}
}

//TODO: Add a method for letting the user chose which plugin they would like to use instead of the current default/fallback setup
int CXPokey::InitPokeyDll()
{
	// apokeysnd.dll is first loaded, will be used in priority if it is found
	if (m_pokey_dll = LoadLibrary("apokeysnd.dll"))
	{
		CString warningMessage = "";

		APokeySound_Initialize = (APokeySound_Initialize_PROC)GetProcAddress(m_pokey_dll, "APokeySound_Initialize");
		if (!APokeySound_Initialize) warningMessage += "APokeySound_Initialize\n";

		APokeySound_PutByte = (APokeySound_PutByte_PROC)GetProcAddress(m_pokey_dll, "APokeySound_PutByte");
		if (!APokeySound_PutByte) warningMessage += "APokeySound_PutByte\n";

		APokeySound_GetRandom = (APokeySound_GetRandom_PROC)GetProcAddress(m_pokey_dll, "APokeySound_GetRandom");
		if (!APokeySound_GetRandom) warningMessage += "APokeySound_GetRandom\n";

		APokeySound_Generate = (APokeySound_Generate_PROC)GetProcAddress(m_pokey_dll, "APokeySound_Generate");
		if (!APokeySound_Generate) warningMessage += "APokeySound_Generate\n";

		APokeySound_About = (APokeySound_About_PROC)GetProcAddress(m_pokey_dll, "APokeySound_About");
		if (!APokeySound_About) warningMessage += "APokeySound_About\n";

		// Get "About" data from apokeysnd driver, then finalise the inisialisation
		if (warningMessage.IsEmpty())
		{
			const char* name, * author, * description;
			APokeySound_About(&name, &author, &description);
			g_aboutpokey.Format("%s\n%s\n%s", name, author, description);
			APokeySound_Initialize(g_tracks4_8 == 8);	// STEREO enabled
			return SOUND_DRIVER_APOKEYSND;
		}

		// If an error is caught, the plugin will be unloaded with an error message showing the problematic procedures
		MessageBox(g_hwnd, "Error:\nNo compatible 'apokeysnd.dll',\ntherefore the Pokey sound can't be performed.\nIncompatibility with:" + warningMessage, "Pokey library error", MB_ICONEXCLAMATION);
		FreeLibrary(m_pokey_dll);
	}

	// sa_pokey.dll will be loaded next if apokeysnd.dll was not found or had an error, as a fallback
	if (m_pokey_dll = LoadLibrary("sa_pokey.dll"))
	{
		CString warningMessage = "";

		Pokey_Initialise = (Pokey_Initialise_PROC)GetProcAddress(m_pokey_dll, "Pokey_Initialise");
		if (!Pokey_Initialise) warningMessage += "Pokey_Initialise\n";

		Pokey_SoundInit = (Pokey_SoundInit_PROC)GetProcAddress(m_pokey_dll, "Pokey_SoundInit");
		if (!Pokey_SoundInit) warningMessage += "Pokey_SoundInit\n";

		Pokey_Process = (Pokey_Process_PROC)GetProcAddress(m_pokey_dll, "Pokey_Process");
		if (!Pokey_Process) warningMessage += "Pokey_Process\n";

		Pokey_GetByte = (Pokey_GetByte_PROC)GetProcAddress(m_pokey_dll, "Pokey_GetByte");
		if (!Pokey_GetByte) warningMessage += "Pokey_GetByte\n";

		Pokey_PutByte = (Pokey_PutByte_PROC)GetProcAddress(m_pokey_dll, "Pokey_PutByte");
		if (!Pokey_PutByte) warningMessage += "Pokey_PutByte\n";

		Pokey_About = (Pokey_About_PROC)GetProcAddress(m_pokey_dll, "Pokey_About");
		if (!Pokey_About) warningMessage += "Pokey_About\n";

		// Get "About" data from sa_pokey driver, then finalise the inisialisation
		if (warningMessage.IsEmpty())
		{
			char* name, * author, * description;
			Pokey_About(&name, &author, &description);
			g_aboutpokey.Format("%s\n%s\n%s", name, author, description);
			Pokey_Initialise(0, 0);

			// Specify the machine region and if it uses Stereo or Mono, as well as the frequency for the sound output
			Pokey_SoundInit(FREQ_17, OUTPUTFREQ, (g_tracks4_8 == 8) + 1);
			return SOUND_DRIVER_SA_POKEY;
		}

		// If an error is caught, the plugin will be unloaded with an error message showing the problematic procedures
		MessageBox(g_hwnd, "Error:\nNo compatible 'sa_pokey.dll',\ntherefore the Pokey sound can't be performed.\nIncompatibility with:" + warningMessage, "Pokey library error", MB_ICONEXCLAMATION);
		FreeLibrary(m_pokey_dll);
	}

	// If no POKEY emulation plugin was found, no sound emulation will be output
	MessageBox(g_hwnd, "Warning:\nNone of 'apokeysnd.dll' or 'sa_pokey.dll' found,\ntherefore the Pokey sound can't be performed.", "LoadLibrary error", MB_ICONEXCLAMATION);
	return SOUND_DRIVER_NONE;
}
