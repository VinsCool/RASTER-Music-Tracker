// XPokey.h header file
//

#if !defined(_XPOKEY_H_)
#define _XPOKEY_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CMy2PokeysDlg dialog

//#include <dsound.h>
//#include "Pokey.h"
//#include "Pokeysnd.h"

extern HWND g_hwnd;

extern int g_tracks4_8;

extern BOOL g_nohwsoundbuffer;

extern BOOL volatile g_rmtroutine;
extern void Atari_PlayRMT();

extern unsigned char g_atarimem[65536];
extern void TextXY(char *txt,int x,int y,int c);


#define CHANNELS		2
#define BITRESOLUTION	8
#define OUTPUTFREQ		44100		//22050		//44100
#define BUFFER_SIZE		0x8000		//musi byt mocnina 2
#define CHUNK_SIZE		BITRESOLUTION/8*CHANNELS*OUTPUTFREQ/50		//padesatiny (CHUNK_SIZE (tm) by JirkaS) 
#define LATENCY			3			//3/50sec
#define LATENCY_SIZE	LATENCY*CHUNK_SIZE

#define FREQ_17_EXACT     1789790	/* exact 1.79 MHz clock freq */
#define FREQ_17_APPROX    1787520	/* approximate 1.79 MHz clock freq */

extern CString g_aboutpokey;

extern int GetChannelOnOff(int ch);


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
	BOOL MemToPokey(int tracks4_8);
	BOOL GetRenderSound()		{ return m_rendersound; };

	//BOOL Test();

private:
	BOOL volatile m_rendersound;
	HINSTANCE m_pokey_dll;

	DWORD				m_LoadPos;
	WAVEFORMATEX*		m_SoundFormat;
	DWORD				m_LoadSize;
	DSBUFFERDESC        dsbdesc;
	LPDIRECTSOUNDBUFFER m_SoundBuffer;
	DWORD				dwSize1, dwSize2;
	LPVOID				Data1, Data2;
	BYTE				m_PlayBuffer[BUFFER_SIZE];	//renderovana cast kolisa CHUNK_SIZE +- neco (ale muze byt i daleko vetsi)
	DWORD				m_PlayCursor;  
	DWORD				m_WriteCursor;
	DWORD				m_WriteCursorStart;
};

#endif
