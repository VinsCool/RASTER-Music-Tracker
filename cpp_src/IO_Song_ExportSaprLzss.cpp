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

extern CInstruments	g_Instruments;
extern CPokeyStream g_PokeyStream;

#define VU_PLAYER_LOOP_FLAG		0x1C22		// VUPlayer's address for the Loop flag
#define VU_PLAYER_STEREO_FLAG	0x1DC3		// VUPlayer's address for the Stereo flag 
#define VU_PLAYER_SONG_SPEED	0x2029		// VUPlayer's address for setting the song speed
#define VU_PLAYER_DO_PLAY_ADDR	0x2174		// VUPlayer's address for Play, for SAP exports bypassing the mainloop code
#define VU_PLAYER_RTS_NOP		0x21B1		// VUPlayer's address for JMP loop being patched to RTS NOP NOP with the SAP format
#define VU_PLAYER_INIT_SAP		0x3080		// VUPlayer SAP initialisation hack
#define LZSS_POINTER			0x3000		// All the LZSS subtunes index will occupy this memory page
#define VU_PLAYER_REGION		0x202D		// VUPlayer's address for the region initialisation
#define VU_PLAYER_RASTER_BAR	0x216B		// VUPlayer's address for the rasterbar display
#define VU_PLAYER_COLOUR		0x2132		// VUPlayer's address for the rasterbar colour
#define VU_PLAYER_SHUFFLE		0x2308		// VUPlayer's address for the rasterbar colour shuffle (incomplete feature)

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
/// <returns></returns>
bool CSong::ExportLZSS(ofstream& ou)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	// Now, create LZSS files using the SAP-R dump created earlier

	// Full tune playback up to its loop point
	unsigned char* compressedData = NULL;
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
	}

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	SetStatusBarText("");
	if (compressedData)
	{
		ou.write((char*)compressedData, full);		// Write the buffer 1 contents to the export file
		delete []compressedData;
	}
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
	int targetAddrOfModule = 0x3100;													// All the LZSS data will be written starting from this address
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
		0x8D,0xE7,0x22,		// STA SongIdx
		0xA2,0x00,			// LDX #0
		0x8E,0x93,0x1B,		// STX is_fadeing_out
		0x8E,0x03,0x1C,		// STX stop_on_fade_end
		0x4C,0x39,0x1C		// JMP SetNewSongPtrsLoopsOnly
	};
	memcpy(mem + VU_PLAYER_INIT_SAP, sapbytes, 14);

	mem[VU_PLAYER_SONG_SPEED] = m_instrumentSpeed;						// Song speed
	mem[VU_PLAYER_STEREO_FLAG] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		// Is the song stereo?

	// Reconstruct the export binary 
	SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	// LZSS Driver, and some free bytes for later if needed
	SaveBinaryBlock(ou, mem, 0x2000, 0x27FF, 0);	// VUPlayer only

	// SongStart pointers
	mem[0x3000] = targetAddrOfModule >> 8;
	mem[0x3001] = lzss_offset >> 8;
	mem[0x3002] = targetAddrOfModule & 0xFF;
	mem[0x3003] = lzss_offset & 0xFF;

	// SongEnd pointers
	mem[0x3004] = lzss_offset >> 8;
	mem[0x3005] = lzss_end >> 8;
	mem[0x3006] = lzss_offset & 0xFF;
	mem[0x3007] = lzss_end & 0xFF;

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
	int targetAddrOfModule = 0x3100;													// All the LZSS data will be written starting from this address
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
	memset(mem + 0x2EBC, 32, 40 * 5);	// 5 lines of 40 characters at the user text address
	int p = 0, q = 0;
	char a;
	for (int i = 0; i < dlg.m_txt.GetLength(); i++)
	{
		a = dlg.m_txt.GetAt(i);
		if (a == '\n') { p += 40; q = 0; }
		else
		{
			mem[0x2EBC + p + q] = a;
			q++;
		}
		if (p + q >= 5 * 40) break;
	}
	StrToAtariVideo((char*)mem + 0x2EBC, 200);

	memset(mem + 0x2C0B, 32, 28);	// 28 characters on the top line, next to the Region and VBI speed
	char framesdisplay[28] = { 0 };
	sprintf(framesdisplay, "(%i frames)", g_PokeyStream.GetFirstCountPoint());	// Total recorded frames
	for (int i = 0; i < 28; i++)
	{
		mem[0x2C0B + i] = framesdisplay[i];
	}
	StrToAtariVideo((char*)mem + 0x2C0B, 28);

	// I know the binary I have is currently set to NTSC, so I'll just convert to PAL and keep this going for now...
	if (!g_ntsc)
	{
		unsigned char regionbytes[18] =
		{
			0xB9,0xE6,0x26,	//LDA tabppPAL-1,y
			0x8D,0x56,0x21,	//STA acpapx2
			0xE0,0x9B,		//CPX #$9B
			0x30,0x05,		//BMI set_ntsc
			0xB9,0xF6,0x26,	//LDA tabppPALfix-1,y
			0xD0,0x03,		//BNE region_done
			0xB9,0x16,0x27	//LDA tabppNTSCfix-1,y
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
	mem[0x3000] = targetAddrOfModule >> 8;
	mem[0x3001] = lzss_offset >> 8;
	mem[0x3002] = targetAddrOfModule & 0xFF;
	mem[0x3003] = lzss_offset & 0xFF;

	// SongEnd pointers
	mem[0x3004] = lzss_offset >> 8;
	mem[0x3005] = lzss_end >> 8;
	mem[0x3006] = lzss_offset & 0xFF;
	mem[0x3007] = lzss_end & 0xFF;

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
/// Get the Pokey registers to be dumped to a stream buffer.
/// GUI is disabled but MFC messages are being pumped, so the screen is updated
/// </summary>
/// <returns></returns>
void CSong::DumpSongToPokeyBuffer()
{
	ChangeTimer(20);							// This helps avoiding corruption if things are running too fast
	Atari_InitRMTRoutine();						// Reset the RMT routines 
	SetChannelOnOff(-1, 0);						// Switch all channels off 

	g_PokeyStream.StartRecording();

	CWnd* wnd = AfxGetApp()->GetMainWnd();
	wnd->EnableWindow(FALSE);
	SetStatusBarText("Generating Pokey stream, playing song in quick mode ...");

	int savedFollowPlay = m_followplay;
	Play(MPLAY_SONG, TRUE);						// Play song from start, start before the timer changes again
	ChangeTimer(1);								// Set the timer to be as fast as possible for the recording process

	// Wait in a tight loop pumping messages until the playback stops
	MSG msg;
	while (m_play != MPLAY_STOP)				// The SAP-R dumper is running during that time...
	{
		if (::PeekMessage(&msg, wnd->m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	SetStatusBarText("");
	wnd->EnableWindow();						// Turn on the window again

	ChangeTimer((g_ntsc) ? 17 : 20);			// Reset the timer again, to avoid corruption from running too fast
	Stop();										// End playback now, the SAP-R data should have been dumped successfully!

	m_followplay = savedFollowPlay;				// Restore the foillow play flag
}