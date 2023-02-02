#include "StdAfx.h"
#include <fstream>
#include <memory.h>

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

#define VU_PLAYER_LOOP_FLAG		LZSSP_LOOP_COUNT		// VUPlayer's address for the Loop flag
#define VU_PLAYER_STEREO_FLAG	LZSSP_IS_STEREO_FLAG	// VUPlayer's address for the Stereo flag 
#define VU_PLAYER_SONG_SPEED	LZSSP_PLAYER_SONG_SPEED	// VUPlayer's address for setting the song speed
#define VU_PLAYER_DO_PLAY_ADDR	LZSSP_DO_PLAY			// VUPlayer's address for Play, for SAP exports bypassing the mainloop code
#define VU_PLAYER_RTS_NOP		LZSSP_VU_PLAYER_RTS_NOP	// VUPlayer's address for JMP loop being patched to RTS NOP NOP with the SAP format
#define VU_PLAYER_INIT_SAP		NULL					// VUPlayer SAP initialisation hack

#define LZSS_POINTER			LZSSP_SONGINDEX			// All the LZSS subtunes index will occupy this memory page
#define VU_PLAYER_SEQUENCE		LZSSP_SONGSEQUENCE
#define VU_PLAYER_SECTION		LZSSP_SONGSECTION
#define VU_PLAYER_SONGDATA		LZSSP_LZ_DTA

#define VU_PLAYER_REGION		LZSSP_PLAYER_REGION_INIT// VUPlayer's address for the region initialisation
#define VU_PLAYER_RASTER_BAR	LZSSP_RASTERBAR_TOGGLER	// VUPlayer's address for the rasterbar display
#define VU_PLAYER_COLOUR		LZSSP_RASTERBAR_COLOUR	// VUPlayer's address for the rasterbar colour
//#define VU_PLAYER_SHUFFLE		LZSSP_PLAYER_SHUFFLE	// VUPlayer's address for the rasterbar colour shuffle (incomplete feature)
#define VU_PLAYER_SONGTOTAL		LZSSP_SONGTOTAL			// VUPlayer's address for the total number of subtunes that could be played back

#define VU_PLAYER_SOUNGTIMER	LZSSP_SONGTIMERCOUNT

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
bool CSong::ExportSAP_R(std::ofstream& ou)
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
bool CSong::ExportLZSS(std::ofstream& ou, LPCTSTR filename)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	// Now, create LZSS files using the SAP-R dump created earlier
	unsigned char compressedData[65536];

	// TODO: add a Dialog box for proper standalone LZSS exports
	// This is a hacked up method that was added only out of necessity for a project making use of song sections separately
	// I refuse to touch RMT2LZSS ever again
	CString fn = filename;
	fn = fn.Left(fn.GetLength() - 5);	// In order to keep the filename without the extention 

	// Full tune playback up to its loop point
	int full = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetFirstCountPoint() * frameSize, compressedData);
	if (full > 16)
	{
		//ou.open(fn + "_FULL.lzss", ios::binary);	// Create a new file for the Full section
		ou.write((char*)compressedData, full);	// Write the buffer contents to the export file
	}
	ou.close();	// Close the file, if successful, it should not be empty 

	// Intro section playback, up to the start of the detected loop point
	int intro = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize, compressedData);
	if (intro > 16)
	{
		ou.open(fn + "_INTRO.lzss", std::ios::binary);	// Create a new file for the Intro section
		ou.write((char*)compressedData, intro);		// Write the buffer contents to the export file
	}
	ou.close();	// Close the file, if successful, it should not be empty 

	// Looped section playback, this part is virtually seamless to itself
	int loop = LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize, compressedData);
	if (loop > 16)
	{
		ou.open(fn + "_LOOP.lzss", std::ios::binary);	// Create a new file for the Loop section
		ou.write((char*)compressedData, loop);		// Write the buffer contents to the export file
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
bool CSong::ExportLZSS_SAP(std::ofstream& ou)
{
	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	unsigned char buff1[65536];			// LZSS buffers for each ones of the tune parts being reconstructed
	unsigned char buff2[65536];			// they are used for parts labeled: full, intro, and loop 
	unsigned char buff3[65536];			// a LZSS export will typically make use of intro and loop only, unless specified otherwise

	// Now, create LZSS files using the SAP-R dump created earlier
	int full = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetFirstCountPoint() * frameSize, buff1);
	int intro = LZSS_SAP(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize, buff2);
	int loop = LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize, buff3);

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	// Some additional variables that will be used below
	int targetAddrOfModule = VU_PLAYER_SONGDATA;											// All the LZSS data will be written starting from this address
	//int lzss_offset = (intro) ? targetAddrOfModule + intro : targetAddrOfModule + full;	// Calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
	int lzss_offset = (intro > 16) ? targetAddrOfModule + intro : targetAddrOfModule;
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
		0x4C,LZSSP_SETNEWSONGPTRSFULL & 0xff, LZSSP_SETNEWSONGPTRSFULL>>8	// JMP SetNewSongPtrsLoopsOnly
	};
	memcpy(mem + VU_PLAYER_INIT_SAP, sapbytes, 14);

	mem[VU_PLAYER_SONG_SPEED] = m_instrumentSpeed;						// Song speed
	mem[VU_PLAYER_STEREO_FLAG] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		// Is the song stereo?

	// Reconstruct the export binary 
	SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	// LZSS Driver, and some free bytes for later if needed
	SaveBinaryBlock(ou, mem, 0x2000, 0x27FF, 0);	// VUPlayer only

	// SongStart pointers
	mem[LZSS_POINTER] = targetAddrOfModule >> 8;				// SongsSHIPtrs
	mem[LZSS_POINTER] = lzss_offset >> 8;					// SongsIndexEnd
	mem[LZSS_POINTER] = targetAddrOfModule & 0xFF;			// SongsSLOPtrs
	mem[LZSS_POINTER] = lzss_offset & 0xFF;					// SongsDummyEnd

	// SongEnd pointers
	mem[LZSS_POINTER] = lzss_offset >> 8;					// LoopsIndexStart
	mem[LZSS_POINTER] = lzss_end >> 8;						// LoopsIndexEnd
	mem[LZSS_POINTER] = lzss_offset & 0xFF;					// LoopsSLOPtrs
	mem[LZSS_POINTER] = lzss_end & 0xFF;						// LoopsDummyEnd

	if (intro > 16)
	{
		memcpy(mem + targetAddrOfModule, buff2, intro);
		memcpy(mem + lzss_offset, buff3, loop);
	}
	else
	{
		//memcpy(mem + targetAddrOfModule, buff1, full);
		memcpy(mem + lzss_offset, buff3, loop);
	}

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
	CString s, t;

	WORD addressFrom, addressTo;

	int subsongs = GetSubsongParts(t);
	int count = 0;

	int subtune[256];
	memset(subtune, 0, sizeof(subtune));

	int lzss_chunk = 0;	// Subtune size will be added to be used as the offset to the next one
	int lzss_total = 0;	// Final offset for LZSS bytes to export
	int framescount = 0;

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	// SAP-R bytes to copy, Stereo doubles the number
	int section = VU_PLAYER_SECTION;
	int sequence = VU_PLAYER_SEQUENCE;

	unsigned char mem[65536];					// Default RAM size for most 800xl/xe machines
	memset(mem, 0, sizeof(mem));

	// GetSubsongParts returns a CString, so the values must be converted back to int first, FIXME
	for (int i = 0; i < subsongs; i++)
	{
		char c[3];
		c[0] = t[i * 3];
		c[1] = t[i * 3 + 1];
		c[2] = '\0';
		subtune[i] = strtoul(c, NULL, 16);
	}

	// Load VUPlayerLZSS to memory
	Atari_LoadOBX(IOTYPE_LZSS_XEX, mem, addressFrom, addressTo);

	// Create the export metadata for songname, Atari text, parameters, etc
	TExportMetadata metadata;
	CreateExportMetadata(IOTYPE_LZSS_XEX, &metadata);

	while (count < subsongs)
	{
		// a LZSS export will typically make use of intro and loop only, unless specified otherwise
		int intro = 0, loop = 0;

		// LZSS buffers for each ones of the tune parts being reconstructed
		unsigned char buff2[65536], buff3[65536];

		DumpSongToPokeyBuffer(MPLAY_FROM, subtune[count], 0);

		SetStatusBarText("Compressing data ...");

		// There is an Intro section 
		if (g_PokeyStream.GetThirdCountPoint())
			intro = BruteforceOptimalLZSS(g_PokeyStream.GetStreamBuffer(), g_PokeyStream.GetThirdCountPoint() * frameSize, buff2);

		// There is a Loop section
		if (g_PokeyStream.GetFirstCountPoint())
			loop = BruteforceOptimalLZSS(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetFirstCountPoint() * frameSize), g_PokeyStream.GetSecondCountPoint() * frameSize, buff3);

		// Add the number of frames recorded to the total count
		framescount += g_PokeyStream.GetFirstCountPoint();	

		// Clear the SAP-R dumper memory and reset RMT routines
		g_PokeyStream.FinishedRecording();	

		// Some additional variables that will be used below
		int targetAddrOfModule = VU_PLAYER_SONGDATA + lzss_chunk;	// All the LZSS data will be written starting from this address
		int lzss_offset = targetAddrOfModule + intro;
		int lzss_end = lzss_offset + loop;							// this sets the address that defines where the data stream has reached its end

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

		// Set the song section and timer index 
		int index = LZSS_POINTER + count * 4;
		int timerindex = VU_PLAYER_SOUNGTIMER + count * 4;
		int subtunetimetotal = 0xFFFFFF / g_PokeyStream.GetFirstCountPoint();
		int subtunelooppoint = subtunetimetotal * g_PokeyStream.GetThirdCountPoint();
		int chunk = 0;

		mem[index + 0] = section & 0xFF;
		mem[index + 1] = section >> 8;
		mem[index + 2] = sequence & 0xFF;
		mem[index + 3] = sequence >> 8;
		mem[timerindex + 0] = subtunetimetotal >> 16;
		mem[timerindex + 1] = subtunetimetotal >> 8;
		mem[timerindex + 2] = subtunetimetotal & 0xFF;
		mem[timerindex + 3] = subtunelooppoint >> 16;

		// If there is an Intro section...
		if (intro)
		{
			memcpy(mem + targetAddrOfModule, buff2, intro);
			lzss_chunk += intro;
			mem[section + 0] = targetAddrOfModule & 0xFF;
			mem[section + 1] = targetAddrOfModule >> 8;
			mem[sequence] = chunk;
			section += 2;
			sequence += 1;
			chunk += 1;
		}

		// If there is a Loop section...
		if (loop)
		{
			memcpy(mem + lzss_offset, buff3, loop);
			lzss_chunk += loop;
			mem[section + 0] = lzss_offset & 0xFF;
			mem[section + 1] = lzss_offset >> 8;
			mem[sequence] = chunk;
			section += 2;
			sequence += 1;
			chunk += 1;
		}

		// End of data, will be overwritten if there is more data to export
		mem[section + 0] = lzss_end & 0xFF;
		mem[section + 1] = lzss_end >> 8;
		section += 2;
		mem[sequence] = (chunk | 0x80) - 1;
		sequence += 1;

		// Update the subtune offsets to export the next one
		lzss_total = lzss_end;
		count++;
	}

	// Write the Atari Video text to memory, for 5 lines of 40 characters
	memcpy(mem + LZSSP_LINE_1, metadata.atariText, 40 * 5);

	// Write the total framescount on the top line, next to the Region and VBI speed, for 28 characters
	memset(mem + LZSSP_LINE_0 + 0x0B, 32, 28);
	char framesdisplay[28] = { 0 };
	sprintf(framesdisplay, "(%i frames total)", framescount);
	for (int i = 0; i < 28; i++) mem[LZSSP_LINE_0 + 0x0B + i] = framesdisplay[i];
	StrToAtariVideo((char*)mem + LZSSP_LINE_0 + 0x0B, 28);

	// I know the binary I have is currently set to NTSC, so I'll just convert to PAL and keep this going for now...
	if (!metadata.isNTSC)
	{
		unsigned char regionbytes[18] =
		{
			0xB9,(LZSSP_TABPPPAL - 1) & 0xff,(LZSSP_TABPPPAL - 1) >> 8,			// LDA tabppPAL-1,y
			0x8D,LZSSP_ACPAPX2 & 0xFF,LZSSP_ACPAPX2 >> 8,						// STA acpapx2
			0xE0,0x9B,															// CPX #$9B
			0x30,0x05,															// BMI set_ntsc
			0xB9,(LZSSP_TABPPPALFIX - 1) & 0xff,(LZSSP_TABPPPALFIX - 1) >> 8,	// LDA tabppPALfix-1,y
			0xD0,0x03,															// BNE region_done
			0xB9,(LZSSP_TABPPNTSCFIX - 1) & 0xFF,(LZSSP_TABPPNTSCFIX - 1) >> 8	// LDA tabppNTSCfix-1,y
		};
		for (int i = 0; i < 18; i++) mem[VU_PLAYER_REGION + i] = regionbytes[i];
	}

	// Additional patches from the Export Dialog...
	mem[VU_PLAYER_SONG_SPEED] = metadata.instrspeed;						// Song speed
	mem[VU_PLAYER_RASTER_BAR] = metadata.displayRasterbar ? 0x80 : 0x00;	// Display the rasterbar for CPU level
	mem[VU_PLAYER_COLOUR] = metadata.rasterbarColour;						// Rasterbar colour 
	mem[VU_PLAYER_STEREO_FLAG] = metadata.isStereo ? 0xFF : 0x00;			// Is the song stereo?
	mem[VU_PLAYER_SONGTOTAL] = subsongs;									// Total number of subtunes
	if (!metadata.autoRegion)												// Automatically adjust speed between regions?
		for (int i = 0; i < 4; i++) mem[VU_PLAYER_REGION + 6 + i] = 0xEA;	// set the 4 bytes to NOPs to disable it
	

	// Reconstruct the export binary for the LZSS Driver, VUPlayer, and all the included data
	SaveBinaryBlock(ou, mem, LZSSP_PLAYLZ16BEGIN, LZSSP_SONGINDEX, 1);

	// Set the run address to VUPlayer 
	mem[0x2e0] = LZSSP_VUPLAYER & 0xff;
	mem[0x2e1] = LZSSP_VUPLAYER >> 8;
	SaveBinaryBlock(ou, mem, 0x2e0, 0x2e1, 0);

	// Overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
	SaveBinaryBlock(ou, mem, LZSS_POINTER, lzss_total, 0);

	return true;
}

/*
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
	// TODO: everything related to exporting the stream buffer into small files and compress them to LZSS

	DumpSongToPokeyBuffer();

	SetStatusBarText("Compressing data ...");

	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number

	int indexToSongline = 0;
	int songlineCount = g_PokeyStream.GetSonglineCount();

	// Since 0 is also a valid offset, the initial values are set to -1 to prevent conflicts
	int listOfMatches[256];
	memset(listOfMatches, -1, sizeof(listOfMatches));

	// Now, create LZSS files using the SAP-R dump created earlier
	unsigned char compressedData[65536];

	// TODO: add a Dialog box for proper standalone LZSS exports
	CString fn = filename;
	fn = fn.Left(fn.GetLength() - 5);	// In order to keep the filename without the extention 
	ou.close();

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
	indexToSongline = 0;

	// From here, data blocs based on the Songline index and offset will be written to file
	// This should strip away every duplicated chunks, but save just enough data for reconstructing everything 
	while (indexToSongline < songlineCount)
	{
		// Get the current index to Songline offset from the current position
		int index = listOfMatches[indexToSongline];

		// Find if this offset was already processed from a previous Songline
		for (int i = 0; i < songlineCount; i++)
		{
			// As soon as a match is found, increment the counter for how many times the index was referenced
			if (index == listOfMatches[i] && indexToSongline > i)
			{
				// I don't know anymore, at this point...
			}
		}

		// Process to the next songline index until they are all processed
		indexToSongline++;
	}

	// Create a new file for logging everything related to the procedure
	ou.open(fn + ".txt", ios::binary);
	ou << "This is a test that displays all duplicated SAP-R bytes from m_StreamBuffer." << endl;
	ou << "Each ones of the Buffer Chunks are indexed into memory using Songlines.\n" << endl;

	for (int i = 0; i < songlineCount; i++)
	{
		ou << "Index: " << PADHEX(2, i);
		ou << ",\t Offset (real): " << PADHEX(4, g_PokeyStream.GetOffsetPerSongline(i));
		ou << ",\t Offset (dupe): " << PADHEX(4, listOfMatches[i]);
		ou << ",\t Bytes (uncompressed): " << PADDEC(1, g_PokeyStream.GetFramesPerSongline(i) * frameSize);
		ou << ",\t Bytes (LZ16 compressed): " << PADDEC(1, LZSS_SAP(g_PokeyStream.GetStreamBuffer() + (g_PokeyStream.GetOffsetPerSongline(i) * frameSize), g_PokeyStream.GetFramesPerSongline(i) * frameSize, compressedData));
		ou << endl;
	}

	ou.close();	

	g_PokeyStream.FinishedRecording();	// Clear the SAP-R dumper memory and reset RMT routines

	SetStatusBarText("");

	return true;
}
*/

/// <summary>
/// Get the Pokey registers to be dumped to a stream buffer.
/// GUI is disabled but MFC messages are being pumped, so the screen is updated
/// </summary>
/// <returns></returns>
void CSong::DumpSongToPokeyBuffer(int playmode, int songline, int trackline)
{
	Atari_InitRMTRoutine();						// Reset the RMT routines 
	SetChannelOnOff(-1, 0);						// Switch all channels off 

	g_PokeyStream.StartRecording();

	CWnd* wnd = AfxGetApp()->GetMainWnd();
	wnd->EnableWindow(FALSE);
	SetStatusBarText("Generating Pokey stream, playing song in quick mode ...");

	int savedFollowPlay = m_followplay;
	//Play(MPLAY_SONG, TRUE);						// Play song from start, start before the timer changes again

	// Play song using the chosen playback parameters
	// If no argument was passed, Play from start will be assumed
	m_songplayline = m_songactiveline = songline;
	m_trackplayline = m_trackactiveline = trackline;
	Play(playmode, TRUE);

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

// A dumb SAP-R LZSS optimisations bruteforcer, returns the optimal value and buffer
int CSong::BruteforceOptimalLZSS(unsigned char* src, int srclen, unsigned char* dst)
{
	// Start from a high value to force the first pattern to be the best one
	int bestScore = 0xFFFFFF;
	int optimal = 0;

	for (int i = 0; i < SAPR_OPTIMISATIONS_COUNT; i++)
	{
		int bruteforced = LZSS_SAP(src, srclen, dst, i);
		if (bruteforced < bestScore)
		{
			bestScore = bruteforced;
			optimal = i;
		}
	}

	return LZSS_SAP(src, srclen, dst, optimal);
}
