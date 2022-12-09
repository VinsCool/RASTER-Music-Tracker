#include "StdAfx.h"
#include <fstream>
#include <memory.h>
using namespace std;

#include "GuiHelpers.h"
#include "Song.h"
#include "Instruments.h"

#include "ExportDlgs.h"

#include "Atari6502.h"
#include "XPokey.h"
#include "PokeyStream.h"

#include "global.h"

#include "ChannelControl.h"

#include "lzssp.h"
#include "lzss_sap.h"

extern CInstruments	g_Instruments;
extern CPokeyStream g_PokeyStream;

#define VU_PLAYER_LOOP_FLAG		LZSSP_LOOP_TOGGLE		// VUPlayer's address for the Loop flag
#define VU_PLAYER_STEREO_FLAG	LZSSP_IS_STEREO_FLAG	// VUPlayer's address for the Stereo flag 
#define VU_PLAYER_SONG_SPEED	LZSSP_PLAYER_SONG_SPEED	// VUPlayer's address for setting the song speed
#define VU_PLAYER_DO_PLAY_ADDR	LZSSP_DO_PLAY			// VUPlayer's address for Play, for SAP exports bypassing the mainloop code
#define VU_PLAYER_RTS_NOP		LZSSP_VU_PLAYER_RTS_NOP	// VUPlayer's address for JMP loop being patched to RTS NOP NOP with the SAP format
#define VU_PLAYER_INIT_SAP		LZSSP_SAPINDEX			// VUPlayer SAP initialisation hack
#define LZSS_POINTER			LZSSP_SONGSINDEXSTART	// All the LZSS subtunes index will occupy this memory page
#define VU_PLAYER_REGION		LZSSP_PLAYER_REGION_INIT// VUPlayer's address for the region initialisation
#define VU_PLAYER_RASTER_BAR	LZSSP_RASTERBAR_TOGGLER	// VUPlayer's address for the rasterbar display
#define VU_PLAYER_COLOUR		LZSSP_RASTERBAR_COLOUR	// VUPlayer's address for the rasterbar colour
#define VU_PLAYER_SHUFFLE		LZSSP_PLAYER_SHUFFLE	// VUPlayer's address for the rasterbar colour shuffle (incomplete feature)

static void StrToAtariVideo(char* txt, int count)
{
	char a;
	for (int i = 0; i < count; i++)
	{
		a = txt[i] & 0x7f;
		if (a < 32) a = 0;
		else
			if (a < 96) a -= 32;
		txt[i] = a;
	}
}

/// <summary>
/// Export the Pokey registers to the SAP Type R format (data stream)
/// </summary>
/// <param name="ou">Output stream</param>
/// <returns>true if the file was written</returns>
bool CSong::ExportSAP_R(ofstream& ou)
{
	DumpSongToPokeyBuffer();

	CString s;
	CExpSAPDlg dlg;
	s = m_songname;
	s.TrimRight();			//cuts spaces after the name
	s.Replace('"', '\'');	//replaces quotation marks with an apostrophe
	dlg.m_name = s;

	dlg.m_author = "???";

	CTime time = CTime::GetCurrentTime();
	dlg.m_date = time.Format("%d/%m/%Y");

	if (dlg.DoModal() != IDOK)
	{
		// Clear the Pokey dumper memory and reset RMT routines
		g_PokeyStream.FinishedRecording();
		return false;
	}

	ou << "SAP" << EOL;

	s = dlg.m_author;
	s.TrimRight();
	s.Replace('"', '\'');

	ou << "AUTHOR \"" << s << "\"" << EOL;

	s = dlg.m_name;
	s.TrimRight();
	s.Replace('"', '\'');

	ou << "NAME \"" << s << " (" << g_PokeyStream.GetFirstCountPoint() << " frames)" << "\"" << EOL;	//display the total frames recorded

	s = dlg.m_date;
	s.TrimRight();
	s.Replace('"', '\'');

	ou << "DATE \"" << s << "\"" << EOL;

	s.MakeUpper();

	ou << "TYPE R" << EOL;

	if (g_tracks4_8 > 4)
	{	//stereo module
		ou << "STEREO" << EOL;
	}

	if (g_ntsc)
	{	//NTSC module
		ou << "NTSC" << EOL;
	}

	if (m_instrumentSpeed > 1)
	{
		ou << "FASTPLAY ";
		switch (m_instrumentSpeed)
		{
		case 2:
			ou << ((g_ntsc) ? "131" : "156");
			break;

		case 3:
			ou << ((g_ntsc) ? "87" : "104");
			break;

		case 4:
			ou << ((g_ntsc) ? "66" : "78");
			break;

		default:
			ou << ((g_ntsc) ? "262" : "312");
			break;
		}
		ou << EOL;
	}
	// A double EOL is necessary for making the SAP-R export functional
	ou << EOL;

	// Write the SAP-R stream to the output file defined in the path dialog with the data specified above
	g_PokeyStream.WriteToFile(ou, g_PokeyStream.GetFirstCountPoint(), 0);

	// Clear the memory and reset the dumper to its initial setup for the next time it will be called
	g_PokeyStream.FinishedRecording();

	return true;
}

/// <summary>
/// Generate a SAP-R data stream and compress it with LZSS
/// </summary>
/// <param name="ou">File to output the compressed data to</param>
/// <param name="filename">Filename to output additional files, required for splitting the Intro and Loop sections of a song</param>
/// <returns></returns>
bool CSong::ExportLZSS(ofstream& ou, LPCTSTR filename)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	// Now, create LZSS files using the SAP-R dump created earlier
	unsigned char* compressedData = NULL;

	// TODO: add a Dialog box for proper standalone LZSS exports
	// This is a hacked up method that was added only out of necessity for a project making use of song sections separately
	// I refuse to touch RMT2LZSS ever again
	CString fn = filename;
	fn = fn.Left(fn.GetLength() - 5);	// In order to keep the filename without the extention 

	// Full tune playback up to its loop point
	int full = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetFirstCountPoint() * frameSize);
	if (full)
	{
		// Load tmp.lzss in destination buffer 
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);

		// Find the file size
		in.seekg(0, in.end);
		int fileSize = (int)in.tellg();

		// Go back to the beginning
		in.seekg(0, in.beg);

		full = 0;

		if (fileSize > 16) 
		{
			compressedData = new unsigned char[fileSize];

			in.read((char*)compressedData, fileSize);
			full = (int)in.gcount();
		}

		if (full < 16) full = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");

		if (compressedData)
		{
			ou.write((char*)compressedData, full);		// Write the buffer contents to the export file
			delete[]compressedData;
		}
	}
	ou.close();	// Close the file, if successful, it should not be empty 

	// Intro section playback, up to the start of the detected loop point
	int intro = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize);
	if (intro)
	{
		// Load tmp.lzss in destination buffer 
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);

		// Find the file size
		in.seekg(0, in.end);
		int fileSize = (int)in.tellg();

		// Go back to the beginning
		in.seekg(0, in.beg);

		intro = 0;

		if (fileSize > 16)
		{
			compressedData = new unsigned char[fileSize];

			in.read((char*)compressedData, fileSize);
			intro = (int)in.gcount();
		}

		if (intro < 16) intro = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");

		if (compressedData)
		{
			ou.open(fn + "_INTRO.lzss", ios::binary);	// Create a new file for the Intro section
			ou.write((char*)compressedData, intro);		// Write the buffer contents to the export file
			delete[]compressedData;
		}
	}
	ou.close();	// Close the file, if successful, it should not be empty 

	// Looped section playback, this part is virtually seamless to itself
	int loop = LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize);
	if (loop)
	{
		// Load tmp.lzss in destination buffer 
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);

		// Find the file size
		in.seekg(0, in.end);
		int fileSize = (int)in.tellg();

		// Go back to the beginning
		in.seekg(0, in.beg);

		loop = 0;

		if (fileSize > 16)
		{
			compressedData = new unsigned char[fileSize];

			in.read((char*)compressedData, fileSize);
			loop = (int)in.gcount();
		}

		if (loop < 16) loop = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");

		if (compressedData)
		{
			ou.open(fn + "_LOOP.lzss", ios::binary);	// Create a new file for the Loop section
			ou.write((char*)compressedData, loop);		// Write the buffer contents to the export file
			delete[]compressedData;
		}
	}
	ou.close();	// Close the file, if successful, it should not be empty

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	SetStatusBarText("");

	return true;
}

/// <summary>
/// Export the Pokey registers to a SAP TYPE B format
/// </summary>
/// <param name="ou">Output stream</param>
/// <returns>true if the file was written</returns>
bool CSong::ExportLZSS_SAP(ofstream& ou)
{
	unsigned char buff1[65536];			// LZSS buffers for each ones of the tune parts being reconstructed
	unsigned char buff2[65536];			// they are used for parts labeled: full, intro, and loop 
	unsigned char buff3[65536];			// a LZSS export will typically make use of intro and loop only, unless specified otherwise

	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	// Now, create LZSS files using the SAP-R dump created earlier
	memset(buff1, 0, sizeof(buff1));
	memset(buff2, 0, sizeof(buff2));
	memset(buff3, 0, sizeof(buff3));

	// Full tune playback up to its loop point
	int full = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetFirstCountPoint() * frameSize);
	if (full)
	{
		// Load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff1, sizeof(buff1))))
			full = (int)in.gcount();
		if (full < 16) full = 1;
		in.close(); 
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	// Intro section playback, up to the start of the detected loop point
	int intro = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize);
	if (intro)
	{
		// Load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff2, sizeof(buff2))))
			intro = (int)in.gcount();
		if (intro < 16) intro = 1;
		in.close(); 
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	// Looped section playback, this part is virtually seamless to itself
	int loop = LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize);
	if (loop)
	{
		// Load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff3, sizeof(buff3))))
			loop = (int)in.gcount();
		if (loop < 16) loop = 1;
		in.close(); 
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	// Some additional variables that will be used below
	int targetAddrOfModule = LZSSP_SONGDATA;											// All the LZSS data will be written starting from this address
	int lzss_offset = (intro) ? targetAddrOfModule + intro : targetAddrOfModule + full;	// Calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
	int lzss_end = lzss_offset + loop;													// this sets the address that defines where the data stream has reached its end

	SetStatusBarText("");

	// If the size is too big, abort the process and show an error message
	if (lzss_end > 0xBFFF)
	{
		MessageBox(g_hwnd,
			"Error, LZSS data is too big to fit in memory!\n\n"
			"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
			"Error, Buffer Overflow!", MB_ICONERROR);
		return false;
	}

	unsigned char mem[65536];					// Default RAM size for most 800xl/xe machines
	memset(mem, 0, sizeof(mem));

	WORD addressFrom;
	WORD addressTo;

	if (!LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, addressFrom, addressTo))
	{
		MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
		return false;
	}

	CExpSAPDlg dlg;
	CString str;
	str = m_songname;
	str.TrimRight();			//cuts spaces after the name
	str.Replace('"', '\'');	//replaces quotation marks with an apostrophe
	dlg.m_name = str;

	dlg.m_author = "???";
	GetSubsongParts(dlg.m_subsongs);

	CTime time = CTime::GetCurrentTime();
	dlg.m_date = time.Format("%d/%m/%Y");

	if (dlg.DoModal() != IDOK)
		return false;

	// Output things in SAP format
	ou << "SAP" << EOL;

	str = dlg.m_author;
	str.TrimRight();
	str.Replace('"', '\'');

	ou << "AUTHOR \"" << str << "\"" << EOL;

	str = dlg.m_name;
	str.TrimRight();
	str.Replace('"', '\'');

	ou << "NAME \"" << str << " (" << g_PokeyStream.GetFirstCountPoint() << " frames)" << "\"" << EOL;	//display the total frames recorded

	str = dlg.m_date;
	str.TrimRight();
	str.Replace('"', '\'');

	ou << "DATE \"" << str << "\"" << EOL;

	str = dlg.m_subsongs + " ";		//space after the last character due to parsing

	str.MakeUpper();
	int subsongs = 0;
	BYTE subpos[MAXSUBSONGS];
	subpos[0] = 0;					//start at songline 0 by default
	BYTE a, n = 0, isn = 0;

	// Parses the "Subsongs" line from the ExportSAP dialog
	for (int i = 0; i < str.GetLength(); i++)
	{
		a = str.GetAt(i);
		if (a >= '0' && a <= '9') { n = (n << 4) + (a - '0'); isn = 1; }
		else
		if (a >= 'A' && a <= 'F') { n = (n << 4) + (a - 'A' + 10); isn = 1; }
		else
		{
			if (isn)
			{
				subpos[subsongs] = n;
				subsongs++;
				if (subsongs >= MAXSUBSONGS) break;
				isn = 0;
			}
		}
	}
	if (subsongs > 1)
		ou << "SONGS " << subsongs << EOL;

	ou << "TYPE B" << EOL;
	str.Format("INIT %04X", VU_PLAYER_INIT_SAP);
	ou << str << EOL;
	str.Format("PLAYER %04X", VU_PLAYER_DO_PLAY_ADDR);
	ou << str << EOL;

	if (g_tracks4_8 > 4)
	{
		ou << "STEREO" << EOL;
	}

	if (g_ntsc)
	{
		ou << "NTSC" << EOL;
	}

	if (m_instrumentSpeed > 1)
	{
		ou << "FASTPLAY ";
		switch (m_instrumentSpeed)
		{
		case 2:
			ou << ((g_ntsc) ? "131" : "156");
			break;

		case 3:
			ou << ((g_ntsc) ? "87" : "104");
			break;

		case 4:
			ou << ((g_ntsc) ? "66" : "78");
			break;

		default:
			ou << ((g_ntsc) ? "262" : "312");
			break;
		}
		ou << EOL;
	}

	// A double EOL is necessary for making the SAP export functional
	ou << EOL;

	// Patch: change a JMP [label] to a RTS with 2 NOPs
	unsigned char saprtsnop[3] = { 0x60,0xEA,0xEA };
	for (int i = 0; i < 3; i++) mem[VU_PLAYER_RTS_NOP + i] = saprtsnop[i];

	// Patch: change a $00 to $FF to force the LOOP flag to be infinite
	mem[VU_PLAYER_LOOP_FLAG] = 0xFF;

	// SAP initialisation patch, running from address 0x3080 in Atari executable 
	unsigned char sapbytes[14] =
	{
		0x8D,LZSSP_SONGIDX & 0xff,LZSSP_SONGIDX>>8,									// STA SongIdx
		0xA2,0x00,																	// LDX #0
		0x8E,LZSSP_IS_FADEING_OUT & 0xff, LZSSP_IS_FADEING_OUT>>8,					// STX is_fadeing_out
		0x8E,LZSSP_STOP_ON_FADE_END & 0xff, LZSSP_STOP_ON_FADE_END>>8,				// STX stop_on_fade_end
		0x4C,LZSSP_SETNEWSONGPTRSLOOPSONLY & 0xff, LZSSP_SETNEWSONGPTRSLOOPSONLY>>8	// JMP SetNewSongPtrsLoopsOnly
	};
	memcpy(mem + VU_PLAYER_INIT_SAP, sapbytes, 14);

	mem[VU_PLAYER_SONG_SPEED] = m_instrumentSpeed;						// Song speed
	mem[VU_PLAYER_STEREO_FLAG] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		// Is the song stereo?

	// Reconstruct the export binary 
	SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	// LZSS Driver, and some free bytes for later if needed
	SaveBinaryBlock(ou, mem, 0x2000, 0x27FF, 0);	// VUPlayer only

	// SongStart pointers
	mem[LZSSP_SONGSSHIPTRS] = targetAddrOfModule >> 8;				// SongsSHIPtrs
	mem[LZSSP_SONGSINDEXEND] = lzss_offset >> 8;					// SongsIndexEnd
	mem[LZSSP_SONGSSLOPTRS] = targetAddrOfModule & 0xFF;			// SongsSLOPtrs
	mem[LZSSP_SONGSDUMMYEND] = lzss_offset & 0xFF;					// SongsDummyEnd

	// SongEnd pointers
	mem[LZSSP_LOOPSINDEXSTART] = lzss_offset >> 8;					// LoopsIndexStart
	mem[LZSSP_LOOPSINDEXEND] = lzss_end >> 8;						// LoopsIndexEnd
	mem[LZSSP_LOOPSSLOPTRS] = lzss_offset & 0xFF;					// LoopsSLOPtrs
	mem[LZSSP_LOOPSDUMMYEND] = lzss_end & 0xFF;						// LoopsDummyEnd

	if (intro)
	{
		memcpy(mem + targetAddrOfModule, buff2, intro);
	}
	else
	{
		memcpy(mem + targetAddrOfModule, buff1, full);
	}
	memcpy(mem + lzss_offset, buff3, loop);

	// Overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
	SaveBinaryBlock(ou, mem, LZSS_POINTER, lzss_end, 0);

	return true;
}

/// <summary>
/// Generate a SAP-R data stream, compress it and export to VUPlayer xex
/// </summary>
/// <param name="ou">File to output the compressed data to</param>
/// <returns></returns>
bool CSong::ExportLZSS_XEX(std::ofstream& ou)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	unsigned char buff1[65536];			// LZSS buffers for each ones of the tune parts being reconstructed
	unsigned char buff2[65536];			// they are used for parts labeled: full, intro, and loop 
	unsigned char buff3[65536];			// a LZSS export will typically make use of intro and loop only, unless specified otherwise
	memset(buff1, 0, sizeof(buff1));
	memset(buff2, 0, sizeof(buff2));
	memset(buff3, 0, sizeof(buff3));

	// Now, create LZSS files using the SAP-R dump created earlier

	// Full tune playback up to its loop point
	int full = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetFirstCountPoint() * frameSize);
	if (full)
	{
		// Load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff1, sizeof(buff1))))
			full = (int)in.gcount();
		if (full < 16) full = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	// Intro section playback, up to the start of the detected loop point
	int intro = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize);
	if (intro)
	{
		// load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff2, sizeof(buff2))))
			intro = (int)in.gcount();
		if (intro < 16) intro = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	// Looped section playback, this part is virtually seamless to itself
	int loop = LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize);
	if (loop)
	{
		// Load tmp.lzss in destination buffer
		ifstream in;
		in.open(g_prgpath + "tmp.lzss", ifstream::binary);
		if (!(in.read((char*)buff3, sizeof(buff3))))
			loop = (int)in.gcount();
		if (loop < 16) loop = 1;
		in.close();
		DeleteFile(g_prgpath + "tmp.lzss");
	}

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	// Some additional variables that will be used below
	int targetAddrOfModule = LZSSP_SONGDATA;											// All the LZSS data will be written starting from this address
	int lzss_offset = (intro) ? targetAddrOfModule + intro : targetAddrOfModule + full;	// Calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
	int lzss_end = lzss_offset + loop;													// this sets the address that defines where the data stream has reached its end

	SetStatusBarText("");

	// If the size is too big, abort the process and show an error message
	if (lzss_end > 0xBFFF)
	{
		MessageBox(g_hwnd,
			"Error, LZSS data is too big to fit in memory!\n\n"
			"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
			"Error, Buffer Overflow!", MB_ICONERROR);
		return false;
	}

	unsigned char mem[65536];					// Default RAM size for most 800xl/xe machines
	memset(mem, 0, sizeof(mem));

	WORD addressFrom;
	WORD addressTo;

	if (!LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, addressFrom, addressTo))
	{
		MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
		return false;
	}

	CExpMSXDlg dlg;
	CString str;
	str = m_songname;
	str.TrimRight();
	CTime time = CTime::GetCurrentTime();
	if (g_rmtmsxtext != "")
	{
		dlg.m_txt = g_rmtmsxtext;	// same from last time, making repeated exports faster
	}
	else
	{
		dlg.m_txt = str + "\x0d\x0a";
		if (g_tracks4_8 > 4) dlg.m_txt += "STEREO";
		dlg.m_txt += "\x0d\x0a" + time.Format("%d/%m/%Y");
		dlg.m_txt += "\x0d\x0a";
		dlg.m_txt += "Author: (press SHIFT key)\x0d\x0a";
		dlg.m_txt += "Author: ???";
	}
	str = "Playback speed will be adjusted to ";
	str += g_ntsc ? "60" : "50";
	str += "Hz on both PAL and NTSC systems.";
	dlg.m_speedinfo = str;

	if (dlg.DoModal() != IDOK)
	{
		return false;
	}
	g_rmtmsxtext = dlg.m_txt;
	g_rmtmsxtext.Replace("\x0d\x0d", "\x0d");	//13, 13 => 13

	// This block of code will handle all the user input text that will be inserted in the binary during the export process
	memset(mem + LZSSP_LINE_1, 32, 40 * 5);	// 5 lines of 40 characters at the user text address
	int p = 0, q = 0;
	char a;
	for (int i = 0; i < dlg.m_txt.GetLength(); i++)
	{
		a = dlg.m_txt.GetAt(i);
		if (a == '\n') { p += 40; q = 0; }
		else
		{
			mem[LZSSP_LINE_1 + p + q] = a;
			q++;
		}
		if (p + q >= 5 * 40) break;
	}
	StrToAtariVideo((char*)mem + LZSSP_LINE_1, 200);

	memset(mem + LZSSP_LINE_0 + 0x0B, 32, 28);	// 28 characters on the top line, next to the Region and VBI speed
	char framesdisplay[28] = { 0 };
	sprintf(framesdisplay, "(%i frames)", g_PokeyStream.GetFirstCountPoint());	// Total recorded frames
	for (int i = 0; i < 28; i++)
	{
		mem[LZSSP_LINE_0 + 0x0B + i] = framesdisplay[i];
	}
	StrToAtariVideo((char*)mem + LZSSP_LINE_0 + 0x0B, 28);

	// I know the binary I have is currently set to NTSC, so I'll just convert to PAL and keep this going for now...
	if (!g_ntsc)
	{
		unsigned char regionbytes[18] =
		{
			0xB9,(LZSSP_TABPPPAL-1) & 0xff,(LZSSP_TABPPPAL-1) >> 8,			//LDA tabppPAL-1,y
			0x8D,LZSSP_ACPAPX2 & 0xFF,LZSSP_ACPAPX2 >> 8,					//STA acpapx2
			0xE0,0x9B,														//CPX #$9B
			0x30,0x05,														//BMI set_ntsc
			0xB9,(LZSSP_TABPPPALFIX-1) & 0xff,(LZSSP_TABPPPALFIX-1) >> 8,	//LDA tabppPALfix-1,y
			0xD0,0x03,														//BNE region_done
			0xB9,(LZSSP_TABPPNTSCFIX-1) & 0xFF,(LZSSP_TABPPNTSCFIX-1) >> 8	//LDA tabppNTSCfix-1,y
		};
		for (int i = 0; i < 18; i++) mem[VU_PLAYER_REGION + i] = regionbytes[i];
	}

	// Additional patches from the Export Dialog...
	mem[VU_PLAYER_SONG_SPEED] = m_instrumentSpeed;						// Song speed
	mem[VU_PLAYER_RASTER_BAR] = (dlg.m_meter) ? 0x80 : 0x00;			// Display the rasterbar for CPU level
	mem[VU_PLAYER_COLOUR] = dlg.m_metercolor;							// Rasterbar colour 
	mem[VU_PLAYER_SHUFFLE] = 0x00;	// = (dlg.m_msx_shuffle) ? 0x10 : 0x00;		// Rasterbar colour shuffle, incomplete feature so it is disabled
	mem[VU_PLAYER_STEREO_FLAG] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		// Is the song stereo?
	if (!dlg.m_region_auto)												// Automatically adjust speed between regions?
	{
		for (int i = 0; i < 4; i++) mem[VU_PLAYER_REGION + 6 + i] = 0xEA;	//set the 4 bytes to NOPs to disable it
	}

	// Reconstruct the export binary 
	SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	// LZSS Driver, and some free bytes for later if needed
	SaveBinaryBlock(ou, mem, 0x2000, 0x2FFF, 0);	// VUPlayer + Font + Data + Display Lists

	// Set the run address to VUPlayer 
	mem[0x2e0] = 0x2000 & 0xff;
	mem[0x2e1] = 0x2000 >> 8;
	SaveBinaryBlock(ou, mem, 0x2e0, 0x2e1, 0);

	// SongStart pointers
	mem[LZSSP_SONGSSHIPTRS] = targetAddrOfModule >> 8;				// SongsSHIPtrs
	mem[LZSSP_SONGSINDEXEND] = lzss_offset >> 8;					// SongsIndexEnd
	mem[LZSSP_SONGSSLOPTRS] = targetAddrOfModule & 0xFF;			// SongsSLOPtrs
	mem[LZSSP_SONGSDUMMYEND] = lzss_offset & 0xFF;					// SongsDummyEnd

	// SongEnd pointers
	mem[LZSSP_LOOPSINDEXSTART] = lzss_offset >> 8;					// LoopsIndexStart
	mem[LZSSP_LOOPSINDEXEND] = lzss_end >> 8;						// LoopsIndexEnd
	mem[LZSSP_LOOPSSLOPTRS] = lzss_offset & 0xFF;					// LoopsSLOPtrs
	mem[LZSSP_LOOPSDUMMYEND] = lzss_end & 0xFF;						// LoopsDummyEnd

	if (intro)
	{
		memcpy(mem + targetAddrOfModule, buff2, intro);
	}
	else
	{
		memcpy(mem + targetAddrOfModule, buff1, full);
	}
	memcpy(mem + lzss_offset, buff3, loop);

	// Overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
	SaveBinaryBlock(ou, mem, LZSS_POINTER, lzss_end, 0);

	return true;
}

/// <summary>
/// Generate a SAP-R data stream, compress it and optimise the compression further by removing redundancy.
/// Ideally, this will be used to find and merge duplicated songline buffers, for a compromise between speed and memory.
/// The output streams may get considerably smaller, at the cost of having an index to keep track of, in order to reconstruct the stream properly.
/// </summary>
/// <param name="ou">File to output the compressed data to</param>
/// <param name="filename">Filename to use for saving the data output</param>
/// <returns></returns>
bool CSong::ExportCompactLZSS(std::ofstream& ou, LPCTSTR filename)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	int indexToSongline = 0;
	int songlineCount = g_PokeyStream.GetSonglineCount();

	// Since 0 is also a valid offset, the initial values are set to -1 to prevent conflicts
	int listOfMatches[256];
	memset(listOfMatches, -1, sizeof(listOfMatches));

	// Now, create LZSS files using the SAP-R dump created earlier
	unsigned char* compressedData = NULL;

	// TODO: add a Dialog box for proper standalone LZSS exports
	CString fn = filename;
	fn = fn.Left(fn.GetLength() - 5);	// In order to keep the filename without the extention 

	// For all songlines to index, process with comparisons and find duplicates 
	while (indexToSongline < songlineCount)
	{
		int bytesCount = g_PokeyStream.GetFramesPerSongline(indexToSongline) * frameSize;

		int index1 = g_PokeyStream.GetOffsetPerSongline(indexToSongline);
		unsigned char* buff1 = g_PokeyStream.GetStreamBuffer() + (index1 * frameSize);

		// If there is no index already, assume the Index1 to be the first occurence 
		if (listOfMatches[indexToSongline] == -1)
			listOfMatches[indexToSongline] = index1;

		// Compare all indexes available and overwrite matching songline streams with index1's offset
		for (int i = 0; i < songlineCount; i++)
		{
			// If the bytes count between 2 songlines isn't matching, don't even bother trying
			if (bytesCount != g_PokeyStream.GetFramesPerSongline(i) * frameSize)
				continue;

			int index2 = g_PokeyStream.GetOffsetPerSongline(i);
			unsigned char* buff2 = g_PokeyStream.GetStreamBuffer() + (index2 * frameSize);

			// If there is a match, the second index will adopt the offset of the first index
			if (!memcmp(buff1, buff2, bytesCount))
				listOfMatches[i] = index1;
		}

		// Process to the next songline index until they are all processed
		indexToSongline++;
	}

	// TODO: everything related to exporting the stream buffer into small files and compress them to LZSS
	ou << "This is a test that displays all duplicated SAP-R bytes from m_StreamBuffer." << endl;
	ou << "Each ones of the Buffer Chunks are indexed into memory using Songlines.\n" << endl;

	for (int i = 0; i < songlineCount; i++)
	{
		ou << "Index: 0x" << hex << uppercase << i << ", Offset (real): " << dec << g_PokeyStream.GetOffsetPerSongline(i);
		ou << ", Offset (dupe): " << listOfMatches[i] << ", Frames: " << g_PokeyStream.GetFramesPerSongline(i);
		ou << ", Bytes (uncompressed): " << g_PokeyStream.GetFramesPerSongline(i) * frameSize << endl;
	}

	ou.close();	// Close the file, if successful, it should not be empty

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	SetStatusBarText("");

	return true;
}

/// <summary>
/// Get the Pokey registers to be dumped to a stream buffer.
/// GUI is disabled but MFC messages are being pumped, so the screen is updated
/// </summary>
/// <returns></returns>
void CSong::DumpSongToPokeyBuffer()
{
	Atari_InitRMTRoutine();						// Reset the RMT routines 
	SetChannelOnOff(-1, 0);						// Switch all channels off 

	g_PokeyStream.StartRecording();

	CWnd* wnd = AfxGetApp()->GetMainWnd();
	wnd->EnableWindow(FALSE);
	SetStatusBarText("Generating Pokey stream, playing song in quick mode ...");

	int savedFollowPlay = m_followplay;
	Play(MPLAY_SONG, TRUE);						// Play song from start, start before the timer changes again

	// Wait in a tight loop pumping messages until the playback stops
	MSG msg;

	// The SAP-R dumper is running during that time...
	while (m_play != MPLAY_STOP)
	{
		// 1 VBI of module playback
		PlayVBI();
		int xvbi = 0;

		// Multiple calls per VBI will be processed as well
		while (xvbi < m_instrumentSpeed)
		{
			// 1xVBI of RMT routine (for instruments)
			if (g_rmtroutine)
				Atari_PlayRMT();

			// Transfer from g_atarimem to POKEY buffer
			g_PokeyStream.Record();

			// 1xVBI was played
			xvbi++;
		}

		// The song is currently playing, increment the timer
		g_playtime++;

		// Update the screen only once every few frames
		// Displaying everything in real time slows things down considerably!
		if (g_timerGlobalCount % 10 != 0)
			continue;

		if (::PeekMessage(&msg, wnd->m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	SetStatusBarText("");
	wnd->EnableWindow();						// Turn on the window again

	Stop();										// End playback now, the SAP-R data should have been dumped successfully!

	m_followplay = savedFollowPlay;				// Restore the follow play flag
}