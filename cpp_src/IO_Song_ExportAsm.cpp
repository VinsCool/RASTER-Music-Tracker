#include "StdAfx.h"
#include <fstream>
#include <memory.h>
using namespace std;

#include "GuiHelpers.h"
#include "Song.h"
#include "Instruments.h"

#include "ExportDlgs.h"

#include "global.h"

extern CInstruments	g_Instruments;

// ============================================================================
// Code to export RMT song data as
// - simple notation assembler
// - fully reloctable assembler source code for RMTPlayer
// ============================================================================

/// <summary>
/// Export the RMT module as assembler
/// </summary>
/// <param name="ou">Output stream</param>
/// <param name="exportStrippedDesc">Data about the packed RMT module</param>
/// <returns>true if the save went ok</returns>
bool CSong::ExportAsAsm(ofstream& ou, tExportDescription* exportStrippedDesc)
{
	// Setup the ASM export dialog
	CExportAsmDlg dlg;
	dlg.m_prefixForAllAsmLabels = g_PrefixForAllAsmLabels;

	if (dlg.DoModal() != IDOK) return false;

	// Save for future ASM exports
	g_PrefixForAllAsmLabels = dlg.m_prefixForAllAsmLabels;

	CString s, snot;
	int maxova = 16;				// maximal amount of data per line

	BYTE tracksFlags[TRACKSNUM];
	memset(tracksFlags, 0, TRACKSNUM); // init
	MarkTF_USED(tracksFlags);

	ou << ";ASM notation source";
	ou << EOL << "XXX\tequ $FF\t;empty note value";
	if (!g_PrefixForAllAsmLabels.IsEmpty())
	{
		s.Format("%s_data", g_PrefixForAllAsmLabels);
		ou << EOL << s;
	}
	//
	if (dlg.m_exportType == 1 /* Tracks only*/)
	{
		// Tracks
		int j, note, dur, instrumentNr;
		for (int trackNr = 0; trackNr < TRACKSNUM; trackNr++)
		{
			// Only process if the track is used
			if (!(tracksFlags[trackNr] & TF_USED))
				continue;

			s.Format(";Track $%02X", trackNr);
			ou << EOL << s;
			if (!g_PrefixForAllAsmLabels.IsEmpty())
			{
				s.Format("%s_track%02X", g_PrefixForAllAsmLabels, trackNr);
				ou << EOL << s;
			}

			TTrack& originalTrack = *g_Tracks.GetTrack(trackNr);
			TTrack tempTrack;	// temporary track
			memcpy((void*)&tempTrack, (void*)&originalTrack, sizeof(TTrack)); // make a copy of origtt to tt
			g_Tracks.TrackExpandLoop(&tempTrack); //expands tt due to GO loops
			int ova = maxova;

			for (int idx = 0; idx < tempTrack.len; idx++)
			{
				if (ova >= maxova)
				{
					ou << EOL << "\tdta ";
					ova = 0;
				}
				note = tempTrack.note[idx];
				if (note >= 0)
				{
					instrumentNr = tempTrack.instr[idx];
					if (dlg.m_notesIndexOrFreq == 1) //notes
						note = g_Instruments.GetNote(instrumentNr, note);
					else				//frequencies
						note = g_Instruments.GetFrequency(instrumentNr, note);
				}
				if (note >= 0) snot.Format("$%02X", note); else snot = "XXX";
				for (dur = 1; idx + dur < tempTrack.len && tempTrack.note[idx + dur] < 0; dur++);
				if (dlg.m_durationsType == 1 /* Notes only */)
				{
					if (ova > 0) ou << ",";
					ou << snot; ova++;
					for (j = 1; j < dur; j++, ova++)
					{
						if (ova >= maxova)
						{
							ova = 0;
							ou << EOL << "\tdta XXX";
						}
						else
							ou << ",XXX";
					}
				}
				else if (dlg.m_durationsType == 2 /* Note, duration */)
				{
					if (ova > 0) ou << ",";
					ou << snot;
					ou << "," << dur;
					ova += 2;
				}
				else if (dlg.m_durationsType == 3 /* Duration, Note */)
				{
					if (ova > 0) ou << ",";
					ou << dur << ",";
					ou << snot;
					ova += 2;
				}
				idx += dur - 1;
			}
		}
	}
	else if (dlg.m_exportType == 2 /* Whole song */)
	{
		// song columns
		int clm;
		for (clm = 0; clm < g_tracks4_8; clm++)
		{
			BYTE finished[SONGLEN];
			memset(finished, 0, SONGLEN);
			int sline = 0;
			static char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };
			s.Format(";Song column %s", cnames[clm]);
			ou << EOL << s;
			if (!g_PrefixForAllAsmLabels.IsEmpty())
			{
				s.Format("%s_column%s", g_PrefixForAllAsmLabels, cnames[clm]);
				ou << EOL << s;
			}
			while (sline >= 0 && sline < SONGLEN && !finished[sline])
			{
				finished[sline] = 1;
				s.Format(";Song line $%02X", sline);
				ou << EOL << s;
				if (m_songgo[sline] >= 0)
				{
					sline = m_songgo[sline]; //GOTO line
					s.Format(" Go to line $%02X", sline);
					ou << s;
					continue;
				}
				int trackslen = g_Tracks.m_maxTrackLength;
				for (int i = 0; i < g_tracks4_8; i++)
				{
					int at = m_song[sline][i];
					if (at < 0 || at >= TRACKSNUM) continue;
					if (g_Tracks.GetGoLine(at) >= 0) continue;
					int al = g_Tracks.GetLastLine(at) + 1;
					if (al < trackslen) trackslen = al;
				}
				int t, i, j, not, dur, ins;
				int ova = maxova;
				t = m_song[sline][clm];
				if (t < 0)
				{
					ou << " Track --";
					if (!g_PrefixForAllAsmLabels.IsEmpty())
					{
						s.Format("%s_column%s_line%02X", g_PrefixForAllAsmLabels, cnames[clm], sline);
						ou << EOL << s;
					}
					if (dlg.m_durationsType == 1)
					{
						for (i = 0; i < trackslen; i++, ova++)
						{
							if (ova >= maxova)
							{
								ova = 0;
								ou << EOL << "\tdta XXX";
							}
							else
								ou << ",XXX";
						}
					}
					else
						if (dlg.m_durationsType == 2)
						{
							ou << EOL << "\tdta XXX,";
							ou << trackslen;
							ova += 2;
						}
						else
							if (dlg.m_durationsType == 3)
							{
								ou << EOL << "\tdta ";
								ou << trackslen << ",XXX";
								ova += 2;
							}
					sline++;
					continue;
				}

				s.Format(" Track $%02X", t);
				ou << s;
				if (!g_PrefixForAllAsmLabels.IsEmpty())
				{
					s.Format("%s_column%s_line%02X", g_PrefixForAllAsmLabels, cnames[clm], sline);
					ou << EOL << s;
				}

				TTrack& origtt = *g_Tracks.GetTrack(t);
				TTrack tt; //temporary track
				memcpy((void*)&tt, (void*)&origtt, sizeof(TTrack)); //make a copy of origtt to tt
				g_Tracks.TrackExpandLoop(&tt); //expands tt due to GO loops
				for (i = 0; i < trackslen; i++)
				{
					if (ova >= maxova)
					{
						ova = 0;
						ou << EOL << "\tdta ";
					}

					not= tt.note[i];
					if (not>= 0)
					{
						ins = tt.instr[i];
						if (dlg.m_notesIndexOrFreq == 1) //notes
							not= g_Instruments.GetNote(ins, not);
						else				//frequencies
							not= g_Instruments.GetFrequency(ins, not);
					}
					if (not>= 0) snot.Format("$%02X", not); else snot = "XXX";
					for (dur = 1; i + dur < trackslen && tt.note[i + dur] < 0; dur++);
					if (dlg.m_durationsType == 1)
					{
						if (ova > 0) ou << ",";
						ou << snot; ova++;
						for (j = 1; j < dur; j++, ova++)
						{
							if (ova >= maxova)
							{
								ova = 0;
								ou << EOL << "\tdta XXX";
							}
							else
								ou << ",XXX";
						}
					}
					else
						if (dlg.m_durationsType == 2)
						{
							if (ova > 0) ou << ",";
							ou << snot;
							ou << "," << dur;
							ova += 2;
						}
						else
							if (dlg.m_durationsType == 3)
							{
								if (ova > 0) ou << ",";
								ou << dur << ",";
								ou << snot;
								ova += 2;
							}
					i += dur - 1;
				}
				sline++;
			}
		}
	}
	ou << EOL;

	return true;
}

/// <summary>
/// Export the RMT data for the RMTPlayer assembler player.
/// All data is exported: instruments, tracks, song lines
/// </summary>
/// <param name="ou">Output stream</param>
/// <param name="exportDescStripped">Data about the packed RMT module</param>
/// <returns>true if the save went ok</returns>
bool CSong::ExportAsRelocatableAsmForRmtPlayer(std::ofstream& ou, tExportDescription* exportDescStripped)
{
	tExportDescription exportDescWithSFX;
	memset(&exportDescWithSFX, 0, sizeof(tExportDescription));
	exportDescWithSFX.targetAddrOfModule = 0x4000;		// Standard RMT modules are set to start @ $4000

	// Create a variant for SFX (ie. including unused instruments and tracks)
	exportDescWithSFX.firstByteAfterModule = MakeModule(exportDescWithSFX.mem, exportDescWithSFX.targetAddrOfModule, IOTYPE_RMT, exportDescWithSFX.instrumentSavedFlags, exportDescWithSFX.trackSavedFlags);
	if (exportDescWithSFX.firstByteAfterModule < 0) return false;	// if the module could not be created

	CExportRelocatableAsmForRmtPlayer dlg;
	dlg.m_exportDescStripped = exportDescStripped;
	dlg.m_exportDescWithSFX = &exportDescWithSFX;

	dlg.m_strAsmLabelForStartOfSong = g_AsmLabelForStartOfSong;
	dlg.m_wantRelocatableInstruments = g_AsmWantRelocatableInstruments;
	dlg.m_wantRelocatableTracks = g_AsmWantRelocatableTracks;
	dlg.m_wantRelocatableSongLines = g_AsmWantRelocatableSongLines;
	dlg.m_strAsmInstrumentsLabel = g_AsmInstrumentsLabel;
	dlg.m_strAsmTracksLabel = g_AsmTracksLabel;
	dlg.m_strAsmSongLinesLabel = g_AsmSongLinesLabel;
	dlg.m_assemblerFormat = g_AsmFormat;

	dlg.m_sfxSupport = g_rmtstripped_sfx;
	dlg.m_globalVolumeFade = g_rmtstripped_gvf;
	dlg.m_noStartingSongLine = g_rmtstripped_nos;
	dlg.m_song = this;
	dlg.m_filename = "";

	if (dlg.DoModal() != IDOK) return false;

	// Save the dialog settings for future exports
	g_AsmLabelForStartOfSong = dlg.m_strAsmLabelForStartOfSong;
	g_AsmWantRelocatableInstruments = dlg.m_wantRelocatableInstruments;
	g_AsmWantRelocatableTracks = dlg.m_wantRelocatableTracks;
	g_AsmWantRelocatableSongLines = dlg.m_wantRelocatableSongLines;
	g_AsmInstrumentsLabel = dlg.m_strAsmInstrumentsLabel;
	g_AsmTracksLabel = dlg.m_strAsmTracksLabel;
	g_AsmSongLinesLabel = dlg.m_strAsmSongLinesLabel;
	g_AsmFormat = dlg.m_assemblerFormat;

	g_rmtstripped_sfx = dlg.m_sfxSupport;
	g_rmtstripped_gvf = dlg.m_globalVolumeFade;
	g_rmtstripped_nos = dlg.m_noStartingSongLine;

	// Which one is to be exported?
	// Full or stripped down version (names are never exported)
	CString strAsmForModule;

	BOOL isGood = BuildRelocatableAsm(
		strAsmForModule,
		dlg.m_sfxSupport ? &exportDescWithSFX : exportDescStripped,
		dlg.m_strAsmLabelForStartOfSong,
		dlg.m_wantRelocatableTracks ? dlg.m_strAsmTracksLabel : "",
		dlg.m_wantRelocatableSongLines ? dlg.m_strAsmSongLinesLabel : "",
		dlg.m_wantRelocatableInstruments ? dlg.m_strAsmInstrumentsLabel : "",
		dlg.m_assemblerFormat,
		dlg.m_sfxSupport,
		dlg.m_globalVolumeFade,
		dlg.m_noStartingSongLine,
		false
	);

	if (!isGood)
		return false;

	ou << strAsmForModule << EOL;

	return true;
}

// ============================================================================
// The code below handles the export of the RMT song data as relocatable
// assembler source code.

static int rword(unsigned char* data, int i)
{
	return data[i] | (data[i + 1] << 8);
}

/// <summary>
/// Based on the rtm2atasm code
/// </summary>
/// <param name="dest"></param>
/// <param name="exportDesc"></param>
/// <param name="strAsmStartLabel"></param>
/// <param name="strTracksLabel"></param>
/// <param name="strSongLinesLabel"></param>
/// <param name="strInstrumentsLabel"></param>
/// <param name="assemblerFormat"></param>
/// <param name="sfx"></param>
/// <param name="gvf"></param>
/// <param name="nos"></param>
/// <returns></returns>
BOOL CSong::BuildRelocatableAsm(
	CString& addCodeHere,
	tExportDescription* exportDesc,
	CString strAsmStartLabel,
	CString strTracksLabel,
	CString strSongLinesLabel,
	CString strInstrumentsLabel,
	int assemblerFormat,
	BOOL sfx,
	BOOL gvf,
	BOOL nos,
	bool bWantSizeInfoOnly
)
{
	CString strModuleSequence = "\nSequence of modules:\n  Header\n";
	CString strCode;

	int sizeIntro = 16;
	int sizeInstruments = 0;
	int sizeTrack = 0;
	int sizeSongLines = 0;

	// .byte		|	dta
	// .word		|   dta a()
	// .byte "str"	|	dta c'str'
	// ?localvar	|   __localvar

	// Assembler type setup
	BOOL hasDotLocal = 0;
	char* _byte = "dta";
	if (assemblerFormat == ASSEMBLER_FORMAT_ATASM)
	{
		hasDotLocal = 1;
		_byte = ".byte";
	}

	// Sanity checks
	strAsmStartLabel.Trim();
	if (strAsmStartLabel.IsEmpty())
		strAsmStartLabel = "RMT_SONG_DATA";

	strCode.Format("; Cut and paste the data from the line ';* --------BEGIN--------'  to the line ';* --------END--------'\n; into rmt_feat%s\n",
		assemblerFormat == ASSEMBLER_FORMAT_ATASM ? ".asm" : "./65"
	);

	CString str;
	ComposeRMTFEATstring(str, "", exportDesc->instrumentSavedFlags, exportDesc->trackSavedFlags, sfx, gvf, nos, assemblerFormat);
	strCode += str;
	//
	unsigned char* buf = &exportDesc->mem[exportDesc->targetAddrOfModule];
	int start = exportDesc->targetAddrOfModule;
	int end = exportDesc->firstByteAfterModule;
	int len = end - start;
	str.Format("; RMT%c file exported as relocatable source code\n"
		"; Original size: $%04x bytes @ $%04x\n", buf[3], len, start);
	strCode += str;

	// Read parameters from file:
	int numTracks = buf[3] - '0';				// RMTx - x is 4 or 8
	int offsetInstrumentPtrTable = rword(buf, 8) - start;	// This is where the instruments are stored (always directly after this table)
	int offsetTrackPtrTableLow = rword(buf, 10) - start;	// Track ptrs are broken up into lo and hi bytes
	int offsetTrackPtrTableHigh = rword(buf, 12) - start;
	int offsetSong = rword(buf, 14) - start;				// The song tracks start here

	int numInstruments = offsetTrackPtrTableLow - offsetInstrumentPtrTable;
	int numtrk = offsetTrackPtrTableHigh - offsetTrackPtrTableLow;

	// Read all tracks addresses searching for the lowest address
	int first_track = 0xFFFF;
	for (int i = 0; i < numtrk; i++)
	{
		int x = buf[offsetTrackPtrTableLow + i] + (buf[offsetTrackPtrTableHigh + i] << 8);
		if (x)
		{
			x -= start;
			if (x < 0 || x >= offsetSong)
			{
				return 0;
			}
			if (x < first_track)
				first_track = x;
		}
	}
	// Read all instrument addresses searching for the lowest address
	int first_instr = 0xFFFF;
	for (int i = 0; i < numInstruments; i += 2)
	{
		int x = rword(buf, offsetInstrumentPtrTable + i);
		if (x)
		{
			x -= start;
			if (x < 0 || x >= first_track)
			{
				return 0;
			}
			if (x < first_instr)
				first_instr = x;
		}
	}
	if (first_instr < 0 || first_instr >= len ||
		first_track < 0 || first_track >= len)
	{
		if (first_instr == 0xFFFF)
		{
			strCode += "; No instrument data!\n";
		}
		if (first_track)
			return 0;
	}
	if (offsetTrackPtrTableHigh + numtrk != first_instr)
	{
		return 0;
	}
	if (first_track < first_instr)
	{
		return 0;
	}

	// Write assembly output
	str.Format(
		".local\n"
		"%s\n"					// This is the start of the song data
		"?start\n"
		,
		strAsmStartLabel
	);
	strCode += str;

	if (assemblerFormat == ASSEMBLER_FORMAT_ATASM)
		str.Format("    {{byte}} \"RMT%c\"\n", buf[3]);
	else
		str.Format("    dta c'RMT%c'\n", buf[3]);
	strCode += str;

	str.Format("?song_info\n"
		"    {{byte}} $%02x            ; Track length = %d\n"
		"    {{byte}} $%02x            ; Song speed\n"
		"    {{byte}} $%02x            ; Player Frequency\n"
		"    {{byte}} $%02x            ; Format version\n"
		, buf[4], buf[4]// max track length
		, buf[5]		// song speed
		, buf[6]		// player frequency
		, buf[7]		// format version
	);
	strCode += str;
	strCode += "; ptrs to tables\n";
	if (assemblerFormat == ASSEMBLER_FORMAT_ATASM)
	{
		// Atasm
		str.Format(
			"?ptrInstrumentTbl\n    .word ?InstrumentsTable       ; start + $%04x\n"
			"?ptrTracksTblLo\n    .word ?TracksTblLo            ; start + $%04x\n"
			"?ptrTracksTblHi\n    .word ?TracksTblHi            ; start + $%04x\n"
			"?ptrSong\n    .word ?SongData               ; start + $%04x\n",
			offsetInstrumentPtrTable, offsetTrackPtrTableLow, offsetTrackPtrTableHigh, offsetSong);
	}
	else
	{
		// Xasm
		str.Format(
			"__ptrInstrumentTbl\n    dta a(__InstrumentsTable)       ; start + $%04x\n"
			"__ptrTracksTblLo\n    dta a(__TracksTblLo)            ; start + $%04x\n"
			"__ptrTracksTblHi\n    dta a(__TracksTblHi)            ; start + $%04x\n"
			"__ptrSong\n    dta a(__SongData)               ; start + $%04x\n",
			offsetInstrumentPtrTable, offsetTrackPtrTableLow, offsetTrackPtrTableHigh, offsetSong);
	}
	strCode += str;

	// List of ptrs to instruments
	strCode += "\n; List of ptrs to instruments\n";
	int instr_pos[65536];
	memset(instr_pos, 0, sizeof(instr_pos));
	strCode += "?InstrumentsTable";

	for (int i = 0; i < numInstruments; i += 2)
	{
		int loc = rword(buf, i + offsetInstrumentPtrTable) - start;
		if (loc >= first_instr && loc < first_track && loc < len)
		{
			instr_pos[loc] = (i >> 1) + 1;
			str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "?Instrument_%d" : "a(?Instrument_%d)", i >> 1);
		}
		else if (loc == -start)
			str = (assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "  $0000" : " a($0000)");
		else
		{
			return 0;
		}

		sizeIntro += 2;		// 2 bytes per used instrument

		strCode += (assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "\n    .word " : "\n    dta ");
		strCode += str;
		str.Format("\t\t; %s", g_Instruments.GetName(i >> 1));
		strCode += str;
	}
	strCode += "\n";
	// List of tracks:
	int track_pos[65536];
	memset((void*)track_pos, 0, sizeof(track_pos));
	strCode += "\n?TracksTblLo";
	for (int i = 0; i < numtrk; i++)
	{
		int loc = buf[i + offsetTrackPtrTableLow] + (buf[i + offsetTrackPtrTableHigh] << 8) - start;
		if (i % 8 == 0)
			strCode += (assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "\n    .byte " : "\n    dta ");
		else
			strCode += ",";
		if (loc >= first_track && loc < offsetSong && loc < len)
		{
			track_pos[loc] = i + 1;
			str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "<?Track_%02x" : "l(__Track_%02x)", i);
			strCode += str;
		}
		else if (loc == -start)
			strCode += "$00";
		else
		{
			return 0;
		}

		sizeIntro += 1;		// 1 byte per used track
	}
	strCode += "\n?TracksTblHi";
	for (int i = 0; i < numtrk; i++)
	{
		int loc = buf[i + offsetTrackPtrTableLow] + (buf[i + offsetTrackPtrTableHigh] << 8) - start;
		if (i % 8 == 0)
			strCode += (assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "\n    .byte " : "\n    dta ");
		else
			strCode += ",";
		if (loc >= first_track && loc < offsetSong && loc < len)
		{
			str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? ">?Track_%02x" : "h(__Track_%02x)", i);
			strCode += str;
		}
		else if (loc == -start)
			strCode += "$00";
		else
		{
			return 0;
		}

		sizeIntro += 1;		// 1 byte per used track
	}

	strCode += "\n";

	// First dump all the sequencial code, relocated data is dumped after this
	if (strInstrumentsLabel.IsEmpty())
	{
		sizeInstruments = BuildInstrumentData(str, "", buf, first_instr, first_track, instr_pos, assemblerFormat);
		strCode += str;

		strModuleSequence += "  Instruments\n";
	}
	if (strTracksLabel.IsEmpty())
	{
		sizeTrack = BuildTracksData(str, "", buf, first_track, offsetSong, track_pos, assemblerFormat);
		if (sizeTrack == 0)
			return 0;
		strCode += str;
		strModuleSequence += "  Tracks\n";
	}
	if (strSongLinesLabel.IsEmpty())
	{
		sizeSongLines = BuildSongData(str, "", buf, offsetSong, len, start, numTracks, assemblerFormat);
		strCode += str;
		strModuleSequence += "  Song Lines\n";
	}

	// Dump the relocated data
	if (strInstrumentsLabel.IsEmpty() == false)
	{
		sizeInstruments = BuildInstrumentData(str, strInstrumentsLabel, buf, first_instr, first_track, instr_pos, assemblerFormat);
		strCode += str;
		strModuleSequence += "Relocated Instruments\n";
	}
	if (strTracksLabel.IsEmpty() == false)
	{
		sizeTrack = BuildTracksData(str, strTracksLabel, buf, first_track, offsetSong, track_pos, assemblerFormat);
		if (sizeTrack == 0)
			return 0;
		strCode += str;
		strModuleSequence += "Relocated Tracks\n";
	}
	if (strSongLinesLabel.IsEmpty() == false)
	{
		sizeSongLines = BuildSongData(str, strSongLinesLabel, buf, offsetSong, len, start, numTracks, assemblerFormat);
		strCode += str;
		strModuleSequence += "Relocated Song Lines\n";
	}

	strCode.Replace("{{byte}}", _byte);
	strCode.Replace("?", "__");

	if (assemblerFormat == ASSEMBLER_FORMAT_XASM)
		strCode.Replace(".local", "");


	if (bWantSizeInfoOnly)
	{
		addCodeHere.Format(
			"Header\t\t= $%04x (%d) bytes\n"
			"Instruments\t= $%04x (%d) bytes\n"
			"Tracks\t\t= $%04x (%d) bytes\n"
			"Song Lines\t= $%04x (%d) bytes\n"
			,
			sizeIntro, sizeIntro,
			sizeInstruments, sizeInstruments,
			sizeTrack, sizeTrack,
			sizeSongLines, sizeSongLines
		);
		addCodeHere += strModuleSequence;
		addCodeHere.Replace("\n", "\x0d\x0a");
	}
	else
	{
		addCodeHere = strCode;
	}

	return 1;
}

int CSong::BuildInstrumentData(
	CString& strCode,
	CString strInstrumentsLabel,
	unsigned char* buf,
	int from,
	int to,
	int* info,
	int assemblerFormat
)
{
	strCode = "\n\n; Instrument data\n";

	int sizeInstruments = 0;
	CString str;
	if (strInstrumentsLabel.IsEmpty() == false)
	{
		// Make the instruments relocatable
		str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "* = %s\n" : "org %s\n", (LPCTSTR)strInstrumentsLabel);
		strCode += str;
	}

	for (int i = from, l = 0; i < to; i++, l++)
	{
		if (info[i])
		{
			str.Format("\n?Instrument_%d", info[i] - 1);
			strCode += str;
			info[i] = 0;
			l = 0;
		}
		if (l % 16 == 0)
			strCode += "\n    {{byte}} ";
		else
			strCode += ",";
		str.Format("$%02x", buf[i]);
		strCode += str;

		++sizeInstruments;
	}

	return sizeInstruments;
}

int CSong::BuildTracksData(
	CString& strCode,
	CString strTracksLabel,
	unsigned char* buf,
	int from,
	int to,
	int* track_pos,
	int assemblerFormat
)
{
	strCode = "\n\n; Track data";

	int sizeTrack = 0;
	CString str;

	if (strTracksLabel.IsEmpty() == false)
	{
		// Make the track data relocatable
		str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "\n* = %s\n" : "\norg %s\n", (LPCTSTR)strTracksLabel);
	}
	strCode += str;
	for (int i = from, l = 0; i < to; i++, l++)
	{
		if (track_pos[i])
		{
			str.Format("\n?Track_%02x", track_pos[i] - 1);
			strCode += str;
			track_pos[i] = 0;
			l = 0;
		}
		if (l % 16 == 0)
			strCode += "\n    {{byte}} ";
		else
			strCode += ",";
		str.Format("$%02x", buf[i]);
		strCode += str;

		++sizeTrack;
	}
	for (int i = 0; i < 65536; i++)
	{
		if (track_pos[i] != 0)
			return 0;
	}
	return sizeTrack;
}

int CSong::BuildSongData(
	CString& strCode,
	CString strSongLinesLabel,
	unsigned char* buf,
	int offsetSong,
	int len,
	int start,
	int numTracks,
	int assemblerFormat
)
{
	strCode = "\n\n; Song data\n";

	int sizeSongLines = 0;
	CString str;
	if (strSongLinesLabel.IsEmpty() == false)
	{
		str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? "\n* = %s\n" : "\norg %s\n", (LPCTSTR)strSongLinesLabel);
	}
	strCode += str;

	strCode += "?SongData";
	int jmp = 0, l = 0;
	for (int i = offsetSong; i < len; i++, l++)
	{
		if (jmp == -2)
		{
			jmp = 0x10000 + buf[i];
			continue;
		}
		else if (jmp > 0)
		{
			jmp = (0xFFFF & (jmp | (buf[i] << 8))) - start;
			if (0 == ((jmp - offsetSong) % numTracks) && jmp >= offsetSong && jmp < len)
			{
				int lnum = (jmp - offsetSong) / numTracks;
				str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? ",<?line_%02x,>?line_%02x" : ",l(__line_%02x),h(__line_%02x)", lnum, lnum);
				strCode += str;
			}
			else
			{
				str.Format("; ERROR malformed file(song jump bad $ % 04x[% x:% x])\n", jmp, offsetSong, len);
				strCode += str;
				str.Format(assemblerFormat == ASSEMBLER_FORMAT_ATASM ? ",<($%x+?SongData),>($%x+?SongData)" : ",l($%x+__SongData),h($%x+__SongData)", jmp, jmp);
				strCode += str;
			}
			jmp = 0;
			// Allows terminating song on last JUMP
			if (i + 1 == len && numTracks == 8)
				l += 4;

			continue;
		}
		else if (jmp == -1)
			jmp = -2;

		if (l % numTracks == 0)
		{
			str.Format("\n?Line_%02x  {{byte}} ", l / numTracks);
			strCode += str;
		}
		else
			strCode += ",";
		str.Format("$%02x", buf[i]);
		strCode += str;

		if (buf[i] == 0xfe)
		{
			if ((l % numTracks) != 0)
				return 0;
			else
				jmp = -1;
		}

		++sizeSongLines;
	}
	strCode += "\n";

	return sizeSongLines;
}

/// <summary>
/// Create Assembler code to describe which features
/// of the RMTPlayer are to be used
/// </summary>
/// <param name="dest">Output string is appended here</param>
/// <param name="filename">Filename of the song</param>
/// <param name="instrumentSavedFlags">Flags indicating if an instrument is used</param>
/// <param name="trackSavedFlags">Flags indicating if a track is used</param>
/// <param name="soundFXSupport">Flag to indicate if sound effects are to be supported</param>
/// <param name="globalVolumeFade">Flag to indicate support for global volume fading</param>
/// <param name="noStartingSongLine">Flag to indicate if there is a starting song line</param>
/// <param name="assemblerFormat">Which assembler format is to be used ATASM or XASM</param>
void CSong::ComposeRMTFEATstring(
	CString& dest,
	char* filename,
	BYTE* instrumentSavedFlags,
	BYTE* trackSavedFlags,
	BOOL soundFXSupport,
	BOOL globalVolumeFade,
	BOOL noStartingSongLine,
	int assemblerFormat
)
{
	// Depending on the assembler format: equ or =
	char* equal = "equ";
	if (assemblerFormat == ASSEMBLER_FORMAT_ATASM)
		equal = "=";

#define DEST(var,str)	s.Format("%s\t\t%s %i\t\t;(%i times)\n",str,equal,(var>0),var); dest+=s;

	dest.Format(";* --------BEGIN--------\n;* %s\n", filename);
	//
	int usedCommand[8] = { 0,0,0,0,0,0,0,0 };
	int usedCommand7_VolumeOnly = 0;
	int usedCommand7_VolumeOnlyOnChannelX[8] = { 0,0,0,0,0,0,0,0 };
	int usedCommand7_SetNote = 0;
	int usedPortamento = 0;
	int usedFilter = 0;
	int usedFilterOnChannelX[8] = { 0,0,0,0,0,0,0,0 };
	int usedBass16 = 0;
	int usedBass16OnChannelX[8] = { 0,0,0,0,0,0,0,0 };
	int usedTableType = 0;
	int usedTableMode = 0;
	int usedTableGoto = 0;
	int usedAudctlManualSet = 0;
	int usedVolumeMin = 0;
	int usedEffectVibrato = 0;
	int usedEffectFrequencyShift = 0;
	int howManySpeedChanges = 0;

	// Analysis of which instruments are used on which channel and if there is a change in speed
	int instrumentUsedOnChannelX[INSTRSNUM][SONGTRACKS];
	memset(&instrumentUsedOnChannelX, 0, sizeof(instrumentUsedOnChannelX));

	for (int songLineNr = 0; songLineNr < SONGLEN; songLineNr++)
	{
		if (m_songgo[songLineNr] >= 0) continue;	// goto line
		for (int channelNr = 0; channelNr < g_tracks4_8; channelNr++)
		{
			// Get the track# that is at the song line and at the channel
			int trackNr = m_song[songLineNr][channelNr];
			if (trackNr < 0 || trackNr >= TRACKSNUM) continue;

			TTrack* tt = g_Tracks.GetTrack(trackNr);		// Get the track data
			for (int i = 0; i < tt->len; i++)
			{
				// Track which instruments are used
				int instrumentNr = tt->instr[i];
				if (instrumentNr >= 0 && instrumentNr < INSTRSNUM) 
					instrumentUsedOnChannelX[instrumentNr][channelNr]++;

				// Track how often the speed is changed
				int chsp = tt->speed[i];
				if (chsp >= 0) howManySpeedChanges++;
			}
		}
	}

	// Analyse the individual instruments and what they use
	for (int instrumentNr = 0; instrumentNr < INSTRSNUM; instrumentNr++)
	{
		if (instrumentSavedFlags[instrumentNr])
		{
			// Get access to the instrument
			TInstrument& ai = g_Instruments.m_instr[instrumentNr];

			// Run over all commands that an instrument uses
			for (int j = 0; j <= ai.parameters[PAR_ENV_LENGTH]; j++)
			{
				int cmd = ai.envelope[j][ENV_COMMAND] & 0x07;
				usedCommand[cmd]++;
				if (cmd == 7) // AUDCTL
				{
					if (ai.envelope[j][ENV_X] == 0x08 && ai.envelope[j][ENV_Y] == 0x00)
					{
						usedCommand7_VolumeOnly++;
						for (int channelNr = 0; channelNr < g_tracks4_8; channelNr++) 
						{ 
							if (instrumentUsedOnChannelX[instrumentNr][channelNr]) 
								usedCommand7_VolumeOnlyOnChannelX[channelNr]++; 
						}
					}
					else
						usedCommand7_SetNote++;
				}

				// Portamento
				if (ai.envelope[j][ENV_PORTAMENTO]) usedPortamento++;

				// Filter
				if (ai.envelope[j][ENV_FILTER])
				{
					usedFilter++;
					for (int channelNr = 0; channelNr < g_tracks4_8; channelNr++) 
					{ 
						if (instrumentUsedOnChannelX[instrumentNr][channelNr]) 
							usedFilterOnChannelX[channelNr]++; 
					}
				}

				// Bass16
				if (ai.envelope[j][ENV_DISTORTION] == 6)
				{
					usedBass16++;
					for (int channelNr = 0; channelNr < g_tracks4_8; channelNr++) 
					{ 
						if (instrumentUsedOnChannelX[instrumentNr][channelNr]) 
							usedBass16OnChannelX[channelNr]++; 
					}
				}

			}
			// table type
			if (ai.parameters[PAR_TBL_TYPE]) usedTableType++;
			// table mode
			if (ai.parameters[PAR_TBL_MODE]) usedTableMode++;
			// table go
			if (ai.parameters[PAR_TBL_GOTO]) usedTableGoto++;	//non-zero table go
			// AUDCTL manual set
			if (ai.parameters[PAR_AUDCTL_15KHZ]
				|| ai.parameters[PAR_AUDCTL_HPF_CH2]
				|| ai.parameters[PAR_AUDCTL_HPF_CH1]
				|| ai.parameters[PAR_AUDCTL_JOIN_3_4]
				|| ai.parameters[PAR_AUDCTL_JOIN_1_2]
				|| ai.parameters[PAR_AUDCTL_179_CH3]
				|| ai.parameters[PAR_AUDCTL_179_CH1]
				|| ai.parameters[PAR_AUDCTL_POLY9]) usedAudctlManualSet++;
			// Volume mininum
			if (ai.parameters[PAR_VOL_MIN]) usedVolumeMin++;
			// Effect vibrato and frequency shift
			if (ai.parameters[PAR_DELAY]) // only when the effect delay is non-zero
			{
				if (ai.parameters[PAR_VIBRATO]) usedEffectVibrato++;
				if (ai.parameters[PAR_FREQ_SHIFT]) usedEffectFrequencyShift++;
			}
		}
	}

	// Generate strings
	CString s;

	s.Format("FEAT_SFX\t\t%s %u\n", equal, soundFXSupport); dest += s;

	s.Format("FEAT_GLOBALVOLUMEFADE\t%s %u\t\t;RMTGLOBALVOLUMEFADE variable\n", equal, globalVolumeFade); dest += s;

	s.Format("FEAT_NOSTARTINGSONGLINE\t%s %u\n", equal, noStartingSongLine); dest += s;

	s.Format("FEAT_INSTRSPEED\t\t%s %i\n", equal, m_instrumentSpeed); dest += s;

	s.Format("FEAT_CONSTANTSPEED\t\t%s %i\t\t;(%i times)\n", equal, (howManySpeedChanges == 0) ? m_mainSpeed : 0, howManySpeedChanges); dest += s;

	// Commands 1-6
	for (int i = 1; i <= 6; i++)
	{
		s.Format("FEAT_COMMAND%i\t\t%s %i\t\t;(%i times)\n", i, equal, (usedCommand[i] > 0), usedCommand[i]);
		dest += s;
	}

	// Command 7
	DEST(usedCommand7_SetNote, "FEAT_COMMAND7SETNOTE");
	DEST(usedCommand7_VolumeOnly, "FEAT_COMMAND7VOLUMEONLY");
	//
	DEST(usedPortamento, "FEAT_PORTAMENTO");
	//
	DEST(usedFilter, "FEAT_FILTER");
	DEST(usedFilterOnChannelX[0], "FEAT_FILTERG0L");
	DEST(usedFilterOnChannelX[1], "FEAT_FILTERG1L");
	DEST(usedFilterOnChannelX[0 + 4], "FEAT_FILTERG0R");
	DEST(usedFilterOnChannelX[1 + 4], "FEAT_FILTERG1R");
	//
	DEST(usedBass16, "FEAT_BASS16");
	DEST(usedBass16OnChannelX[1], "FEAT_BASS16G1L");
	DEST(usedBass16OnChannelX[3], "FEAT_BASS16G3L");
	DEST(usedBass16OnChannelX[1 + 4], "FEAT_BASS16G1R");
	DEST(usedBass16OnChannelX[3 + 4], "FEAT_BASS16G3R");
	//
	DEST(usedCommand7_VolumeOnlyOnChannelX[0], "FEAT_VOLUMEONLYG0L");
	DEST(usedCommand7_VolumeOnlyOnChannelX[2], "FEAT_VOLUMEONLYG2L");
	DEST(usedCommand7_VolumeOnlyOnChannelX[3], "FEAT_VOLUMEONLYG3L");
	DEST(usedCommand7_VolumeOnlyOnChannelX[0 + 4], "FEAT_VOLUMEONLYG0R");
	DEST(usedCommand7_VolumeOnlyOnChannelX[2 + 4], "FEAT_VOLUMEONLYG2R");
	DEST(usedCommand7_VolumeOnlyOnChannelX[3 + 4], "FEAT_VOLUMEONLYG3R");
	//
	DEST(usedTableType, "FEAT_TABLETYPE");
	DEST(usedTableMode, "FEAT_TABLEMODE");
	DEST(usedTableGoto, "FEAT_TABLEGO");
	DEST(usedAudctlManualSet, "FEAT_AUDCTLMANUALSET");
	DEST(usedVolumeMin, "FEAT_VOLUMEMIN");

	DEST(usedEffectVibrato, "FEAT_EFFECTVIBRATO");
	DEST(usedEffectFrequencyShift, "FEAT_EFFECTFSHIFT");

	dest += ";* --------END--------\n";
	dest.Replace("\n", "\x0d\x0a");
}

