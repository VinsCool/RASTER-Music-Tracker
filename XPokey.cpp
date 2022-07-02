// 2PokeysDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XPokey.h"
//#include "Pokey.h"
//#include "Pokeysnd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

/*
	Pokey_Initialise	@1
	Pokey_SoundInit 	@2
	Pokey_Process		@3
	Pokey_GetByte		@4
	Pokey_PutByte		@5
	Pokey_About 		@6
*/

typedef enum {
	ASAP_FORMAT_U8 = 8,       /* unsigned char */
	ASAP_FORMAT_S16_LE = 16,  /* signed short, little-endian */
	ASAP_FORMAT_S16_BE = -16  /* signed short, big-endian */
} ASAP_SampleFormat;

typedef int abool;


typedef void (* APokeySound_Initialize_PROC)(abool stereo);
typedef void (* APokeySound_PutByte_PROC)(int addr, int data);
typedef int  (* APokeySound_GetRandom_PROC)(int addr, int cycle);
typedef int  (* APokeySound_Generate_PROC)(int cycles, byte buffer[], ASAP_SampleFormat format);
typedef void (* APokeySound_About_PROC)(const char **name, const char **author, const char **description);

/*
void APokeySound_Initialize(abool stereo);
void APokeySound_PutByte(int addr, int data);
int APokeySound_GetRandom(int addr, int cycle);
int APokeySound_Generate(int cycles, byte buffer[], ASAP_SampleFormat format);
void APokeySound_About(const char **name, const char **author, const char **description);
*/

APokeySound_Initialize_PROC APokeySound_Initialize;
APokeySound_PutByte_PROC APokeySound_PutByte;
APokeySound_GetRandom_PROC APokeySound_GetRandom;
APokeySound_Generate_PROC APokeySound_Generate;
APokeySound_About_PROC APokeySound_About;

//44100 Hz
//882 samples/screen
//35568 cycles/screen
//1778400 cycles/s
//=>
//
//FoxPokey
//Hardcoded to 1773447 Hz
//=> 35469 cycles/screen	(35468,94)

#define CYCLESPERSECOND	1773447
#define CYCLESPERSCREEN ((float)CYCLESPERSECOND/50)
#define CYCLESPERSAMPLE	((float)CYCLESPERSECOND/44100)


typedef void (* Pokey_Initialise_PROC)(int*, char**);
typedef void (* Pokey_SoundInit_PROC)(DWORD, WORD, BYTE);
typedef void (* Pokey_Process_PROC)(BYTE*, const WORD);
typedef BYTE (* Pokey_GetByte_PROC)(WORD);
typedef void (* Pokey_PutByte_PROC)(WORD, BYTE);
typedef void (* Pokey_About_PROC)(char**, char**, char**);

Pokey_Initialise_PROC Pokey_Initialise;
Pokey_SoundInit_PROC Pokey_SoundInit;
Pokey_Process_PROC Pokey_Process;
Pokey_GetByte_PROC Pokey_GetByte;
Pokey_PutByte_PROC Pokey_PutByte;
Pokey_About_PROC Pokey_About;



int stereo_enabled=1;

static LPDIRECTSOUND          g_lpds;
static LPDIRECTSOUNDBUFFER    g_lpdsbPrimary;


//#include <fstream.h>
//ofstream g_log;


CXPokey::CXPokey()
{
//	InitSound();
	m_rendersound=0;
	m_SoundBuffer=NULL;
}

CXPokey::~CXPokey()
{
	// TODO: Add your specialized code here and/or call the base class
	DeInitSound();
}

BOOL CXPokey::DeInitSound()
{
	m_rendersound = 0;

	if (m_pokey_dll)
	{
		FreeLibrary(m_pokey_dll);
		m_pokey_dll = NULL;
	}

	g_aboutpokey = "No Pokey sound emulation.";

	//g_log.close();

	if (m_SoundBuffer )
	{
		m_SoundBuffer->Stop();
		m_SoundBuffer->Release();
	}
	m_SoundBuffer = NULL;

	if(g_lpdsbPrimary) g_lpdsbPrimary->Release();
	g_lpdsbPrimary=NULL;
	if(g_lpds) g_lpds->Release();
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
	if (!m_rendersound) return 0;
	if (!m_SoundBuffer) return 0;

	m_SoundBuffer->GetCurrentPosition(&m_PlayCursor,&m_WriteCursor);

	//|||||||||||||||||||||||||||||||||||||||||
	//           ^|-------delta------->^
	//      m_WriteCursor           m_LoadPos

	int delta = (m_LoadPos - m_WriteCursor) & (BUFFER_SIZE-1);

	m_LoadSize = CHUNK_SIZE;	//1764  (882samplu *2kanaly)
	if ( delta > LATENCY_SIZE )
	{
		if (delta > (BUFFER_SIZE/2))
		{
			//uteklo nam to (jsme vic nez pul bufferu pozde
			//takze se na to vyprdnem a posunem se tam co bychom meli spravne byt
			m_LoadPos = (m_WriteCursor+LATENCY_SIZE) & (BUFFER_SIZE-1);
		}
		else 
		{
			//utekli jsme tomu moc dopredu
			//takze trosku pribrzdime (budeme renderovat mensi kousky nez CHUNK_SIZE)
			m_LoadSize = CHUNK_SIZE - ( ((delta-LATENCY_SIZE) /16) & (BUFFER_SIZE-1-1) );	//-1-1 <=JEN SUDA CISLA!!!
			
			//hlidani
			if (m_LoadSize <= 0 ) return 0; //jsme tak moc vepredu, ze vubec nebudem renderovat
			//MessageBeep(-1);
		}
	}
	else // delta <=LATENCY_SIZE
	{
		//jsme bliz nez pozadovana latence, to je sice super, ale radej
		//trosicku zrychlime, aby nas nedohnal m_WriteCursor (budeme renderovat vetsi kousky nez CHUNK_SIZE)
		m_LoadSize = CHUNK_SIZE + ( ((LATENCY_SIZE-delta) /16) & (BUFFER_SIZE-1-1) );	//-1-1 <=JEN SUDA CISLA!!!		

		//hlidani
		if (m_LoadSize >= BUFFER_SIZE/2 ) return 0; //to by byl uz vetsi kus nez velikost pulky bufferu
	}

	
	//g_log << m_LoadSize << endl;

	int rendersize=m_LoadSize;
	int renderpartsize;
	int renderoffset=0;
	for( ; instrspeed>0 ; instrspeed--)
	{
		//--- RMT - playovani instrumentuuu ---/
		if (g_rmtroutine) Atari_PlayRMT();			//jeden prubeh RMT rutinou (instrumenty)
		MemToPokey(g_tracks4_8);			//prenos z g_atarimem do POKEYe (mono ci stereo)
		renderpartsize=(rendersize/instrspeed) & 0xfffe;	//jen suda cisla

		if (m_rendersound==1)
		{
			//apokeysnd
			int cycles = (unsigned short) ((float)renderpartsize/CHANNELS * CYCLESPERSAMPLE);
			while (cycles>0 && renderpartsize>0)
			{
				//maximalni pocet cyklu co se da generovat je CYCLESPERSCREEN
				int rencyc = (cycles>CYCLESPERSCREEN)? (int)CYCLESPERSCREEN : cycles;
				renderpartsize = APokeySound_Generate(rencyc, (unsigned char*)&m_PlayBuffer + renderoffset , ASAP_FORMAT_U8);
				rendersize-=renderpartsize;
				renderoffset+=renderpartsize;
				cycles -= rencyc;
			}
		}
		else
		if (m_rendersound==2)
		{
			//sa_pokey
			Pokey_Process((unsigned char*)&m_PlayBuffer + renderoffset,(unsigned short) renderpartsize);
			rendersize-=renderpartsize;
			renderoffset+=renderpartsize;
		}
	}

	m_LoadSize = renderoffset; //skutecne vygenerovana data sampluuu

	int r;

	r = m_SoundBuffer->Lock(m_LoadPos, m_LoadSize, &Data1, &dwSize1, &Data2, &dwSize2,0 ); 

	//if (dwSize1 != m_LoadSize) MessageBeep(-1);

	if (r==DS_OK)
	{
		//vyrendrujeme do bufferu najednou cele m_LoadSize
		//Pokey_Process((unsigned char*)&m_PlayBuffer,(unsigned short) m_LoadSize);
		m_LoadPos = (m_LoadPos + m_LoadSize) & (BUFFER_SIZE-1);

		//prvni kousek preneseme z bufferu na Data1
		memcpy(Data1,m_PlayBuffer,dwSize1);

		if (Data2)
		{
			//pokud je to rozdeleno, tak ted ten druhy zbyvajici kousek
			memcpy(Data2,m_PlayBuffer+dwSize1,dwSize2);
		}

		m_SoundBuffer->Unlock(Data1, dwSize1, Data2, dwSize2);
		return 1;
	}

	return 0;
}

BOOL CXPokey::InitSound()
{
	if (m_rendersound || m_pokey_dll) DeInitSound();	//pro jistotu

	int pokeytype=0;
	CString wrn="";

	m_pokey_dll=LoadLibrary("apokeysnd.dll");

	if (m_pokey_dll)
	{
		//apokeysnd.dll

		APokeySound_Initialize = (APokeySound_Initialize_PROC) GetProcAddress(m_pokey_dll,"APokeySound_Initialize");
		if (!APokeySound_Initialize) wrn +="APokeySound_Initialize\n";

		APokeySound_PutByte = (APokeySound_PutByte_PROC) GetProcAddress(m_pokey_dll,"APokeySound_PutByte");
		if (!APokeySound_PutByte) wrn +="APokeySound_PutByte\n";

		APokeySound_GetRandom = (APokeySound_GetRandom_PROC) GetProcAddress(m_pokey_dll,"APokeySound_GetRandom");
		if (!APokeySound_GetRandom) wrn +="APokeySound_GetRandom\n";

		APokeySound_Generate = (APokeySound_Generate_PROC) GetProcAddress(m_pokey_dll,"APokeySound_Generate");
		if (!APokeySound_Generate) wrn +="APokeySound_Generate\n";

		APokeySound_About = (APokeySound_About_PROC) GetProcAddress(m_pokey_dll,"APokeySound_About");
		if (!APokeySound_About) wrn +="APokeySound_About\n";

		if (wrn!="")
		{
			MessageBox(g_hwnd,"Error:\nNo compatible 'apokeysnd.dll',\ntherefore the Pokey sound can't be performed.\nIncompatibility with:" +wrn,"Pokey library error",MB_ICONEXCLAMATION);
			DeInitSound();
			return 1;
		}

		//about apokeysnd
		const char *name, *author, *description;
		APokeySound_About(&name,&author,&description);
		g_aboutpokey.Format("%s\n%s\n%s",name,author,description);

		APokeySound_Initialize(1);	//STEREO YES

		pokeytype=1;	//apokeysnd
	}
	else //neni apokeysnd.dll
	{
		m_pokey_dll=LoadLibrary("sa_pokey.dll");
		if(!m_pokey_dll)
		{
			//neni ani sa_pokey.dll
			MessageBox(g_hwnd,"Warning:\nNone of 'apokeysnd.dll' or 'sa_pokey.dll' found,\ntherefore the Pokey sound can't be performed.","LoadLibrary error",MB_ICONEXCLAMATION);
			DeInitSound();
			return 1;
		}

		//sa_pokey.dll

		Pokey_Initialise = (Pokey_Initialise_PROC) GetProcAddress(m_pokey_dll,"Pokey_Initialise");
		if(!Pokey_Initialise) wrn +="Pokey_Initialise\n";

		Pokey_SoundInit = (Pokey_SoundInit_PROC) GetProcAddress(m_pokey_dll,"Pokey_SoundInit");
		if(!Pokey_SoundInit) wrn +="Pokey_SoundInit\n";

		Pokey_Process = (Pokey_Process_PROC) GetProcAddress(m_pokey_dll,"Pokey_Process");
		if(!Pokey_Process) wrn +="Pokey_Process\n";

		Pokey_GetByte = (Pokey_GetByte_PROC) GetProcAddress(m_pokey_dll,"Pokey_GetByte");
		if(!Pokey_GetByte) wrn +="Pokey_GetByte\n";

		Pokey_PutByte = (Pokey_PutByte_PROC) GetProcAddress(m_pokey_dll,"Pokey_PutByte");
		if(!Pokey_PutByte) wrn +="Pokey_PutByte\n";

		Pokey_About = (Pokey_About_PROC) GetProcAddress(m_pokey_dll,"Pokey_About");
		if(!Pokey_About) wrn +="Pokey_About\n";

		if (wrn!="")
		{
			MessageBox(g_hwnd,"Error:\nNo compatible 'sa_pokey.dll',\ntherefore the Pokey sound can't be performed.\nIncompatibility with:" +wrn,"Pokey library error",MB_ICONEXCLAMATION);
			DeInitSound();
			return 1;
		}

		//about sa_pokey.dll
		char *name, *author, *description;
		Pokey_About(&name,&author,&description);
		g_aboutpokey.Format("%s\n%s\n%s",name,author,description);

		Pokey_Initialise(0,0);
		Pokey_SoundInit(FREQ_17_EXACT, OUTPUTFREQ, 2);//22050,(stereo_enabled)?2:1);

		pokeytype=2;	//sa_pokey
	}

	//g_log.open("log.log");	//,ios::binary);

    WAVEFORMATEX        wfm;

	if (DirectSoundCreate( NULL, &g_lpds, NULL ) != DS_OK)
	{
		MessageBox(g_hwnd,"Error: DirectSoundCreate","DirectSound Error!",MB_OK|MB_ICONSTOP);
		return FALSE;		
	}
	
	// Set cooperative level.
	//if(g_lpds->SetCooperativeLevel( AfxGetApp()->GetMainWnd()->m_hWnd, DSSCL_WRITEPRIMARY ) !=DS_OK)
	//if(g_lpds->SetCooperativeLevel( AfxGetApp()->GetMainWnd()->m_hWnd, DSSCL_EXCLUSIVE) !=DS_OK)
	if(g_lpds->SetCooperativeLevel( AfxGetApp()->GetMainWnd()->m_hWnd, DSSCL_PRIORITY) !=DS_OK)
	{
		MessageBox(g_hwnd,"Error: SetCooperativeLevel","DirectSound Error!",MB_OK|MB_ICONSTOP);
		return FALSE;
	}

  // Set primary buffer format.
    ZeroMemory( &wfm, sizeof( WAVEFORMATEX ) ); 
    wfm.wFormatTag      = WAVE_FORMAT_PCM; 
    wfm.nChannels       = CHANNELS;			//2
    wfm.nSamplesPerSec  = OUTPUTFREQ;		//44100
    wfm.wBitsPerSample  = BITRESOLUTION;	//8 
    wfm.nBlockAlign     = wfm.wBitsPerSample / 8 * wfm.nChannels;
    wfm.nAvgBytesPerSec = wfm.nSamplesPerSec * wfm.nBlockAlign;
	wfm.cbSize			= 0;
	
	
    // Create primary buffer.
    ZeroMemory( &dsbdesc, sizeof( DSBUFFERDESC ) );
    dsbdesc.dwSize  = sizeof( DSBUFFERDESC );
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
//	dsbdesc.dwBufferBytes       = BUFFER_SIZE;
    //dsbdesc.lpwfxFormat         = &wfm;


	if (g_lpds->CreateSoundBuffer(&dsbdesc, &g_lpdsbPrimary, NULL) != DS_OK )
	{
		MessageBox(g_hwnd,"Error: CreatePrimarySoundBuffer","DirectSound Error!",MB_OK|MB_ICONSTOP);
		return FALSE;
	}


	if (g_lpdsbPrimary->SetFormat( &wfm ) != DS_OK )
	{
		MessageBox(g_hwnd,"Error: SetFormat","DirectSound Error!",MB_OK|MB_ICONSTOP);
		return FALSE;
	}


    ZeroMemory( &dsbdesc, sizeof( DSBUFFERDESC ) );
    dsbdesc.dwSize  = sizeof( DSBUFFERDESC );
	dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2| DSBCAPS_LOCHARDWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS;
	dsbdesc.dwBufferBytes       = BUFFER_SIZE;
    dsbdesc.lpwfxFormat         = &wfm;

	if (	g_nohwsoundbuffer ||
			g_lpds->CreateSoundBuffer(&dsbdesc, &m_SoundBuffer, NULL) != DS_OK )
	{
		dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2| DSBCAPS_LOCSOFTWARE | DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS;
		if (g_lpds->CreateSoundBuffer(&dsbdesc, &m_SoundBuffer, NULL) != DS_OK )
		{
			MessageBox(g_hwnd,"Error: CreateSoundBuffer","DirectSound Error!",MB_OK|MB_ICONSTOP);
			return FALSE;
		}
	}

	DSBCAPS bc;
	bc.dwSize = sizeof(bc);
	m_SoundBuffer->GetCaps(&bc);//IDirectSoundBuffer_GetCaps(pDSB, &bc);
	//sbufsize = bc.dwBufferBytes;



	int r = m_SoundBuffer->Lock(0, BUFFER_SIZE, &Data1, &dwSize1, &Data2, &dwSize2,DSBLOCK_FROMWRITECURSOR); 
	if (r==DS_OK)
	{
		memset(Data1,0,dwSize1);
		if (Data2) memset(Data2,0,dwSize2);
		m_SoundBuffer->Unlock(Data1, dwSize1, Data2, dwSize2);
	}

	m_SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

	m_SoundBuffer->GetCurrentPosition(&m_PlayCursor,&m_WriteCursorStart);

	m_LoadPos = (m_WriteCursorStart+LATENCY_SIZE) & (BUFFER_SIZE-1);  //uvodni latence (v setinach sekundy)

	m_rendersound = pokeytype;	//1 or 2
	//m_rendersound = 0;	//DEBUG !!!!!!!!!!!!!!!!!!!!

	return 1;
}


BOOL CXPokey::MemToPokey(int tracks4_8)
{
	//if (!m_rendersound) return 0;
	int i;
	if (m_rendersound==1)
	{
		//apokey
		for(i=0; i<=8; i++)	//0-7 + 8audctl
		{
			APokeySound_PutByte(i,(i&0x01) && !GetChannelOnOff(i/2)? 0 : g_atarimem[0xd200+i]);
			if (tracks4_8 == 8) 
				APokeySound_PutByte(i+16,(i&0x01) && !GetChannelOnOff(i/2+4)? 0 : g_atarimem[0xd210+i]);	//stereo
			else
				APokeySound_PutByte(i+16,(i&0x01) && !GetChannelOnOff(i/2)? 0 : g_atarimem[0xd200+i]);		//mono - do obou pokeyu to samo
		}
	}
	else
	if (m_rendersound==2)
	{
		//sa_pokey
		for(i=0; i<=8; i++)	//0-7 + 8audctl
		{
			Pokey_PutByte(i,(i&0x01) && !GetChannelOnOff(i/2)? 0 : g_atarimem[0xd200+i]);
			if (tracks4_8 == 8) 
				Pokey_PutByte(i+16,(i&0x01) && !GetChannelOnOff(i/2+4)? 0 : g_atarimem[0xd210+i]);		//stereo
			else
				Pokey_PutByte(i+16,(i&0x01) && !GetChannelOnOff(i/2)? 0 : g_atarimem[0xd200+i]);		//mono - do obou pokeyu to samo
		}
	}
	else //rendersound==0
		return 0;

	return 1;
}

/*
BOOL CXPokey::Test()
{
	//DEBUG HACK pro KLAVESU "F1" !!!
	//a v Initu je 	m_rendersound = 0;	//DEBUG !!!!!!!!!!!!!!!!!!!!
	// ^ nutno oboje vratit do spravneho stavu

	unsigned char buff[2048];
	memset(buff,255,sizeof(buff));		//buff init
	APokeySound_Initialize(1);			//stereo
	APokeySound_PutByte(0x08,0);		//audctl
	APokeySound_PutByte(0x0f,3);		//
	APokeySound_PutByte(0x08,0);		//audctl
	APokeySound_PutByte(0x00,15);		//frq=15
	APokeySound_PutByte(0x01,175);		//distor=160+15 pure tone
	int renderpartsize = 882;	//882 samples (for 1/50 sec)
	int cycles = (int)((float)renderpartsize * CYCLESPERSAMPLE);
	int generated = APokeySound_Generate(cycles, buff, ASAP_FORMAT_U8);
	int testrandom;
	testrandom=APokeySound_GetRandom(53770,1);
	testrandom=APokeySound_GetRandom(53770,1);
	testrandom=APokeySound_GetRandom(53770,1);
	return 0;
}
*/