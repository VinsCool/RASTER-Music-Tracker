#include "StdAfx.h"
#include <fstream>
#include <memory.h>

#include "GuiHelpers.h"
#include "Song.h"

// MFC interface code
#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"
#include "EffectsDlg.h"
#include "MainFrm.h"

#include "Atari6502.h"
#include "XPokey.h"
#include "PokeyStream.h"
#include "IOHelpers.h"

#include "Instruments.h"
#include "Clipboard.h"

#include "global.h"

#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"
#include "RmtMidi.h"

#include "Wavefile.h"

#include "ModuleV2.h"

extern CInstruments	g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CXPokey g_Pokey;
extern CRmtMidi g_Midi;
extern CPokeyStream g_PokeyStream;
extern CModule g_Module;

void CSong::StrToAtariVideo(char* txt, int count)
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

int CSong::SongToAta(unsigned char* dest, int max, int adr)
{
	int j;
	int apos = 0, len = 0, go = -1;;
	for (int sline = 0; sline < SONGLEN; sline++)
	{
		apos = sline * g_tracks4_8;
		if (apos + g_tracks4_8 > max) return len;		//if it had a buffer overflow

		if ((go = m_songgo[sline]) >= 0)
		{
			//there is a goto line
			dest[apos] = 254;		//go command
			dest[apos + 1] = go;		//number where to jump
			WORD goadr = adr + (go * g_tracks4_8);
			dest[apos + 2] = goadr & 0xff;	//low byte
			dest[apos + 3] = (goadr >> 8);		//high byte
			if (g_tracks4_8 > 4)
			{
				for (int j = 4; j < g_tracks4_8; j++) dest[apos + j] = 255; //to make sure this is the correct line
			}
			len = sline * g_tracks4_8 + 4; //this is the end for now (goto has 4 bytes for 8 tracks)
		}
		else
		{
			//there are track numbers
			for (int i = 0; i < g_tracks4_8; i++)
			{
				j = m_song[sline][i];
				if (j >= 0 && j < TRACKSNUM)
				{
					dest[apos + i] = j;
					len = (sline + 1) * g_tracks4_8;		//this is the end for now
				}
				else
					dest[apos + i] = 255; //--
			}
		}
	}
	return len;
}

BOOL CSong::AtaToSong(unsigned char* sour, int len, int adr)
{
	int i = 0;
	int col = 0, line = 0;
	unsigned char b;
	while (i < len)
	{
		b = sour[i];
		if (b >= 0 && b < TRACKSNUM)
		{
			m_song[line][col] = b;
		}
		else
			if (b == 254 && col == 0)		//go command only in 0 track
			{
				//m_songgo[line]=sour[i+1];  //the driver took it by the number in channel 1
				//but more importantly, it's a vector, so it's better done that way
				int ptr = sour[i + 2] | (sour[i + 3] << 8); //goto vector
				int go = (ptr - adr) / g_tracks4_8;
				if (go >= 0 && go < (len / g_tracks4_8) && go < SONGLEN)
					m_songgo[line] = go;
				else
					m_songgo[line] = 0;	//place of invalid jump and jump to line 0
				i += g_tracks4_8;
				if (i >= len)	return 1;		//this is the end of goto 
				line++;
				if (line >= SONGLEN) return 1;
				continue;
			}
			else
				m_song[line][col] = -1;

		col++;
		if (col >= g_tracks4_8)
		{
			line++;
			if (line >= SONGLEN) return 1;	//so that it does not overflow
			col = 0;
		}
		i++;
	}
	return 1;
}

/// <summary>
/// Reload the currently loaded file
/// </summary>
void CSong::FileReload()
{
	if (!FileCanBeReloaded()) return;
	//Stop();
	int answer = MessageBox(g_hwnd, "Discard all changes since your last save?\n\nWarning: Undo operation won't be possible!!!", "Reload", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (answer == IDYES)
	{
		CString filename = m_fileName;
		FileOpen((LPCTSTR)filename, 0); // Without Warning for changes
	}
}

/// <summary>
/// Open a file for loading
/// </summary>
/// <param name="filename">path to song file to load</param>
/// <param name="warnOfUnsavedChanges">TRUE if the GUI should warn on unsaved changes</param>
void CSong::FileOpen(const char* filename, BOOL warnOfUnsavedChanges)
{
	//if (warnOfUnsavedChanges && WarnUnsavedChanges())
	//	return;

	// Open the file open dialog
	CFileDialog dlg(
		TRUE,					// Open file
		NULL,					// Default Extention
		NULL,					// Filename
		OFN_HIDEREADONLY,		// Flags, such as "hide read-only files"
		FILE_LOADSAVE_FILTERS,	// File Format filters
		NULL,					// Parent Window
		NULL,					// DWORD Size
		FALSE					// Vista Style (WTF this thing sucks so bad, the old style is much better!)
	);

	dlg.m_ofn.lpstrTitle = "Load Module file";

	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;

	else if (!g_defaultSongsPath.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	if (m_fileType == IOTYPE_RMTE)
		dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMTE;

	CString fileToLoad = "";
	int formatChoiceIndexFromDialog = 0;

	if (filename)
	{
		fileToLoad = filename;
		CString ext = fileToLoad.Right(5).MakeLower();
		if (ext == ".rmte")
			formatChoiceIndexFromDialog = FILE_LOADSAVE_FILTER_IDX_RMTE;
	}
	else
	{
		// If not ok, it's over
		if (dlg.DoModal() != IDOK)
			return;

		fileToLoad = dlg.GetPathName();
		formatChoiceIndexFromDialog = dlg.m_ofn.nFilterIndex;
	}

	// Only when a file was selected in the FileDialog or specified at startup
	if (!fileToLoad.IsEmpty() && formatChoiceIndexFromDialog)
	{
		// Use filename from the FileDialog or from the command line
		g_lastLoadPath_Songs = GetFilePath(fileToLoad);

		// Make sure .rmt, .txt or .rmw file was selected
		if (formatChoiceIndexFromDialog < FILE_LOADSAVE_FILTER_IDX_MIN || formatChoiceIndexFromDialog > FILE_LOADSAVE_FILTER_IDX_MAX)
			return;

		// Open the input file in binary format (even the text file)
		std::ifstream in(fileToLoad, std::ios::binary);

		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fileToLoad, "Open error", MB_ICONERROR);
			return;
		}

		// Deletes the current song
		ClearSong(g_tracks4_8);

		bool loadedOk = false;

		switch (formatChoiceIndexFromDialog)
		{
		case FILE_LOADSAVE_FILTER_IDX_RMTE:
			loadedOk = LoadRMTE(in);
			m_fileType = IOTYPE_RMTE;
			break;
		}

		in.close();

		if (!loadedOk)
		{
			// Something in the Load... function failed
			ClearSong(g_tracks4_8);		// Erases everything
			SetRMTTitle();
			return;
		}

		m_fileName = fileToLoad;

		SetRMTTitle();					// Window name
		SetChannelOnOff(-1, 1);			// All channels ON (unmute all) -1 = all, 1 = on
	}

/*
	// Stop the music first
	//Stop();

	if (warnOfUnsavedChanges && WarnUnsavedChanges()) return;

	// Open the file open dialog with *.rmt, *.txt and *.rmw options
	CFileDialog dlg(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		FILE_LOADSAVE_FILTERS
	);
	dlg.m_ofn.lpstrTitle = "Load song file";

	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;
	else
		if (!g_defaultSongsPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	if (m_fileType == IOTYPE_RMT) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMT;
	if (m_fileType == IOTYPE_TXT) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_TXT;
	if (m_fileType == IOTYPE_RMW) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMW;

	CString fileToLoad = "";
	int formatChoiceIndexFromDialog = 0;
	if (filename)
	{
		fileToLoad = filename;
		CString ext = fileToLoad.Right(4).MakeLower();
		if (ext == ".rmt") formatChoiceIndexFromDialog = FILE_LOADSAVE_FILTER_IDX_RMT;
		else
		if (ext == ".txt") formatChoiceIndexFromDialog = FILE_LOADSAVE_FILTER_IDX_TXT;
		else
		if (ext == ".rmw") formatChoiceIndexFromDialog = FILE_LOADSAVE_FILTER_IDX_RMW;
	}
	else
	{
		// If not ok, it's over
		if (dlg.DoModal() != IDOK)
			return;

		fileToLoad = dlg.GetPathName();
		formatChoiceIndexFromDialog = dlg.m_ofn.nFilterIndex;
	}

	// Only when a file was selected in the FileDialog or specified at startup
	if (!fileToLoad.IsEmpty() && formatChoiceIndexFromDialog) 
	{
		// Use filename from the FileDialog or from the command line
		g_lastLoadPath_Songs = GetFilePath(fileToLoad);

		// Make sure .rmt, .txt or .rmw file was selected
		if (formatChoiceIndexFromDialog < FILE_LOADSAVE_FILTER_IDX_MIN
			|| formatChoiceIndexFromDialog > FILE_LOADSAVE_FILTER_IDX_MAX)
		{
			return;
		}

		// Open the input file in binary format (even the text file)
		std::ifstream in(fileToLoad, std::ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fileToLoad, "Open error", MB_ICONERROR);
			return;
		}

		// Deletes the current song
		ClearSong(g_tracks4_8);

		bool loadedOk = false;
		switch (formatChoiceIndexFromDialog)
		{
			case FILE_LOADSAVE_FILTER_IDX_RMT: // RMT choice in Dialog
				loadedOk = LoadRMT(in);
				m_fileType = IOTYPE_RMT;
				break;

			case FILE_LOADSAVE_FILTER_IDX_TXT: // TXT choice in Dialog
				loadedOk = LoadTxt(in);
				m_fileType = IOTYPE_TXT;
				break;

			case FILE_LOADSAVE_FILTER_IDX_RMW: // RMW choice in Dialog
				loadedOk = LoadRMW(in);
				m_fileType = IOTYPE_RMW;
				break;
		}
		in.close();

		if (!loadedOk)
		{
			// Something in the Load... function failed
			ClearSong(g_tracks4_8);		// Erases everything
			SetRMTTitle();
			return;
		}

		m_fileName = fileToLoad;
		m_playSpeed = m_mainSpeed;			// Init speed
		SetRMTTitle();					// Window name
		SetChannelOnOff(-1, 1);			// All channels ON (unmute all) -1 = all, 1 = on
	}
*/

}

void CSong::FileSave()
{
	// If the song has no filename, prompt the "save as" dialog first
	if (m_fileName.IsEmpty() || m_fileType == IOTYPE_NONE)
	{
		FileSaveAs();
		return;
	}

	// Create the file to save
	std::ofstream out(m_fileName, std::ios::binary);

	if (!out)
	{
		MessageBox(g_hwnd, "Can't create this file", "Write error", MB_ICONERROR);
		return;
	}

	bool saveResult = false;

	switch (m_fileType)
	{
	case IOTYPE_RMTE:
		saveResult = SaveRMTE(out);
		break;
	}

	// Closing only when "out" is open (because with IOTYPE_RMT it can be closed earlier)
	if (out.is_open())
		out.close();

	// Changes have been saved
	g_changes = 0;

	SetRMTTitle();

/*
	// Stop the music first
	//Stop();

	// If the song has no filename, prompt the "save as" dialog first
	if (m_fileName.IsEmpty() || m_fileType == IOTYPE_NONE)
	{
		FileSaveAs();
		return;
	}

	// If the RMT module hasn't met the conditions required to be valid, it won't be saved/overwritten
	if (m_fileType == IOTYPE_RMT && !TestBeforeFileSave())
	{
		MessageBox(g_hwnd, "Warning!\nNo data has been saved!", "Warning", MB_ICONEXCLAMATION);
		SetRMTTitle();
		return;
	}

	// Create the file to save, ios::binary will be assumed if the format isn't TXT
	std::ofstream out(m_fileName, (m_fileType == IOTYPE_TXT) ? std::ios::out : std::ios::binary);
	if (!out)
	{
		MessageBox(g_hwnd, "Can't create this file", "Write error", MB_ICONERROR);
		return;
	}

	bool saveResult = false;
	switch (m_fileType)
	{
		case IOTYPE_RMT: // RMT
			saveResult = ExportV2(out, IOTYPE_RMT);
			break;

		case IOTYPE_TXT: // TXT
			saveResult = SaveTxt(out);
			break;

		case IOTYPE_RMW: // RMW
			// NOTE:
			// Remembers the current octave and volume for the active instrument (for saving to RMW) 
			// It is only saved when the instrument is changed and could change the octave or volume before saving without subsequently changing the current instrument
			g_Instruments.MemorizeOctaveAndVolume(m_activeInstrument, m_activeOctave, m_activeVolume);
			saveResult = SaveRMW(out);
			break;
	}

	// Closing only when "out" is open (because with IOTYPE_RMT it can be closed earlier)
	if (out.is_open()) out.close();

	// TODO: add a method to prevent deleting a valid .rmt by accident when a stripped .rmt export was aborted
	if (!saveResult) //failed to save
	{
		DeleteFile(m_fileName);
		MessageBox(g_hwnd, "RMT save aborted.\nFile was deleted, beware of data loss!", "Save aborted", MB_ICONEXCLAMATION);
	}
	else	//saved successfully
		g_changes = 0;	//changes have been saved

	SetRMTTitle();
*/

}

void CSong::FileSaveAs()
{
	// Open the file open dialog
	CFileDialog dlg(
		FALSE,									// Save file
		NULL,									// Default Extention
		NULL,									// Filename
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,	// Flags, such as "hide read-only files"
		FILE_LOADSAVE_FILTERS,					// File Format filters
		NULL,									// Parent Window
		NULL,									// DWORD Size
		FALSE									// Vista Style (WTF this thing sucks so bad, the old style is much better!)
	);

	dlg.m_ofn.lpstrTitle = "Save Module as...";

	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;

	else if (!g_defaultSongsPath.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	// Specifies the name of the file according to the last saved one
	char filenamebuff[1024];

	if (!m_fileName.IsEmpty())
	{
		int pos = m_fileName.ReverseFind('\\');
		if (pos < 0) pos = m_fileName.ReverseFind('/');
		if (pos >= 0)
		{
			CString s = m_fileName.Mid(pos + 1);
			memset(filenamebuff, 0, 1024);
			strcpy(filenamebuff, (char*)(LPCTSTR)s);
			dlg.m_ofn.lpstrFile = filenamebuff;
			dlg.m_ofn.nMaxFile = 1020;	// 4 bytes less, just to make sure ;-)
		}
	}

	// Set the type according to the last save
	if (m_fileType == IOTYPE_RMTE)
		dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMTE;

	//if not ok, nothing will be saved
	if (dlg.DoModal() == IDOK)
	{
		// Validate that the file type selection is valid
		int formatChoiceIndexFromDialog = dlg.m_ofn.nFilterIndex;

		if (formatChoiceIndexFromDialog < FILE_LOADSAVE_FILTER_IDX_MIN || formatChoiceIndexFromDialog > FILE_LOADSAVE_FILTER_IDX_MAX)
			return;

		m_fileName = dlg.GetPathName();
		CString ext = m_fileName.Right(5).MakeLower();
		if (ext != ".rmte" && formatChoiceIndexFromDialog == FILE_LOADSAVE_FILTER_IDX_RMTE)
			m_fileName += ".rmte";

		g_lastLoadPath_Songs = GetFilePath(m_fileName);

		switch (formatChoiceIndexFromDialog)
		{
		case FILE_LOADSAVE_FILTER_IDX_RMTE:
			m_fileType = IOTYPE_RMTE;
			break;

		default:
			// Nothing will be saved if no option was chosen
			return;
		}

		// If everything went well, the file will now be saved
		FileSave();
	}

/*
	// Stop the music first
	//Stop();

	CFileDialog dlg(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		FILE_LOADSAVE_FILTERS
	);
	dlg.m_ofn.lpstrTitle = "Save song as...";

	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;
	else
		if (!g_defaultSongsPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	// Specifies the name of the file according to the last saved one
	char filenamebuff[1024];
	if (!m_fileName.IsEmpty())
	{
		int pos = m_fileName.ReverseFind('\\');
		if (pos < 0) pos = m_fileName.ReverseFind('/');
		if (pos >= 0)
		{
			CString s = m_fileName.Mid(pos + 1);
			memset(filenamebuff, 0, 1024);
			strcpy(filenamebuff, (char*)(LPCTSTR)s);
			dlg.m_ofn.lpstrFile = filenamebuff;
			dlg.m_ofn.nMaxFile = 1020;	// 4 bytes less, just to make sure ;-)
		}
	}

	// Set the type according to the last save
	if (m_fileType == IOTYPE_RMT) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMT;
	if (m_fileType == IOTYPE_TXT) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_TXT;
	if (m_fileType == IOTYPE_RMW) dlg.m_ofn.nFilterIndex = FILE_LOADSAVE_FILTER_IDX_RMW;

	//if not ok, nothing will be saved
	if (dlg.DoModal() == IDOK)
	{
		// Validate that the file type selection is valid
		int formatChoiceIndexFromDialog = dlg.m_ofn.nFilterIndex;
		if (formatChoiceIndexFromDialog < FILE_LOADSAVE_FILTER_IDX_MIN
			|| formatChoiceIndexFromDialog > FILE_LOADSAVE_FILTER_IDX_MAX)
		{
			return;
		}

		m_fileName = dlg.GetPathName();
		const char* exttype[] = FILE_LOADSAVE_EXTENSIONS_ARRAY;
		CString ext = m_fileName.Right(4).MakeLower();
		if (ext != exttype[formatChoiceIndexFromDialog - 1]) m_fileName += exttype[formatChoiceIndexFromDialog - 1];

		g_lastLoadPath_Songs = GetFilePath(m_fileName);

		switch (formatChoiceIndexFromDialog)
		{
			case FILE_LOADSAVE_FILTER_IDX_RMT: // RMT choice
				m_fileType = IOTYPE_RMT;
				break;

			case FILE_LOADSAVE_FILTER_IDX_TXT: // TXT choice
				m_fileType = IOTYPE_TXT;
				break;

			case FILE_LOADSAVE_FILTER_IDX_RMW: // RWM choice
				m_fileType = IOTYPE_RMW;
				break;

			default:
				return;	// Nothing will be saved if no option was chosen
		}

		// If everything went well, the file will now be saved
		FileSave();
	}
*/

}

/// <summary>
/// Popup dialog asking for parameters to init a new song.
/// </summary>
void CSong::FileNew()
{
	// If the last changes were not saved, nothing will be created
	if (WarnUnsavedChanges())
		return;

	CFileNewDlg dlg;
	if (dlg.DoModal() != IDOK)
		return;

	// Apply the settings and reset the song data
	//g_Tracks.SetMaxTrackLength(dlg.m_maxTrackLength);

	g_tracks4_8 = (dlg.m_comboMonoOrStereo == 0) ? 4 : 8;
	ClearSong(g_tracks4_8);

	g_Module.InitialiseModule();
	
	//m_activeSubtune = MODULE_DEFAULT_SUBTUNE;
	//TSubtune* pSubtune = g_Module.GetSubtune(m_activeSubtune);
	//pSubtune->patternLength = dlg.m_maxTrackLength;
	//pSubtune->channelCount = g_tracks4_8;
	//pSubtune->songLength = 1;

	SetRMTTitle();

	// All channels ON (unmute all)
	SetChannelOnOff(-1, 1);		// -1 = all, 1 = on

	// Delete undo history
	g_Undo.Clear();

/*
	// Stop the music first
	//Stop();

	//if the last changes were not saved, nothing will be created
	if (WarnUnsavedChanges()) return;

	CFileNewDlg dlg;
	if (dlg.DoModal() != IDOK)
		return;
	
	// Apply the settings and reset the song data
	g_Tracks.SetMaxTrackLength(dlg.m_maxTrackLength);

	g_tracks4_8 = (dlg.m_comboMonoOrStereo == 0) ? 4 : 8;
	ClearSong(g_tracks4_8);
	SetRMTTitle();

	// Automatically create 1 songline of empty patterns
	for (int i = 0; i < g_tracks4_8; i++) m_song[0][i] = i;

	// Set the goto to the first line 
	m_songgo[1] = 0;

	// All channels ON (unmute all)
	SetChannelOnOff(-1, 1);		// -1 = all, 1 = on

	// Delete undo history
	g_Undo.Clear();
*/
}

/// <summary>
/// Import Legacy RMT song files, Protracker modules or TMC song files
/// </summary>
void CSong::FileImport()
{
	// If unsaved changes are pending, nothing will be imported
	if (WarnUnsavedChanges()) 
		return;

	std::ifstream in;
	CString s, fn;
	bool successful;

	// Create the Import Dialog for the desired formats and configuration
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, FILE_IMPORT_FILTERS);

	// Set the dialog box title as well
	dlg.m_ofn.lpstrTitle = "Import song file";

	// Either use the last Songs path if it is non-empty
	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;

	// Or use the default Songs path if it is non-empty
	else if (!g_defaultSongsPath.IsEmpty()) 
		dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	// Use the last imported file type if it is non-empty
	if (g_lastImportTypeIndex >= 0)
		dlg.m_ofn.nFilterIndex = g_lastImportTypeIndex;

	// If not ok, nothing will be imported
	if (dlg.DoModal() != IDOK)
		return;

	// Get the path name for file to import
	fn = dlg.GetPathName();

	// Open the file once it is ready to be processed
	in.open(fn, std::ios::binary);

	// If opening the file failed, no import will be done
	if (!in)
	{
		MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
		return;
	}

	// Update the last visited Songs path (without the filename)
	g_lastLoadPath_Songs = GetFilePath(fn);

	// Set the last imported file type for future imports
	g_lastImportTypeIndex = dlg.m_ofn.nFilterIndex;

	// Delete the current song before loading new data
	ClearSong(g_tracks4_8);

	// Filter the file type to import, and see if it is successful
	switch (g_lastImportTypeIndex)
	{
	case FILE_IMPORT_FILTER_IDX_RMT:
		if (successful = g_Module.ImportLegacyRMT(in))
			SetWindowText(g_hwnd, "Imported: Legacy RMT " + fn);
		break;

	case FILE_IMPORT_FILTER_IDX_MOD:
		if (successful = ImportMOD(in))
			SetWindowText(g_hwnd, "Imported: Protracker MOD " + fn);
		break;

	case FILE_IMPORT_FILTER_IDX_TMC:
		if (successful = ImportTMC(in))
			SetWindowText(g_hwnd, "Imported: TMC Module " + fn);
		break;

	default:
		successful = false;
	}

	// Close the file once it is no longer needed
	in.close();

	// If the import failed, delete everything that may have been loaded
	if (!successful)
	{
		MessageBox(g_hwnd, "Could not import this file: " + fn, "Import error", MB_ICONERROR);
		ClearSong(g_tracks4_8);
		SetRMTTitle();	// Will set the default text
		return;
	}

	m_fileName = fn;
	m_playSpeed = m_mainSpeed;	// Init speed
	SetChannelOnOff(-1, 1);	// All channels ON (unmute all) -1 = all, 1 = on
}

/// <summary>
/// Export song to one of various formats
/// </summary>
void CSong::FileExportAs()
{
	// Stop the music first
	//Stop();

	// Verify the integrity of the .rmt module to save first, so it won't be saved if it's not meeting the conditions for it
	if (!g_isRMTE && !TestBeforeFileSave())
	{
		MessageBox(g_hwnd, "Warning!\nNo data has been saved!", "Warning", MB_ICONEXCLAMATION);
		return;
	}

	CFileDialog dlg(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		FILE_EXPORT_FILTERS
	);

	dlg.m_ofn.lpstrTitle = "Export song as...";

	if (!g_lastLoadPath_Songs.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Songs;
	else
		if (!g_defaultSongsPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultSongsPath;

	if (m_lastExportType == IOTYPE_RMTSTRIPPED) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_STRIPPED_RMT;
	if (m_lastExportType == IOTYPE_ASM) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_SIMPLE_ASM;
	if (m_lastExportType == IOTYPE_SAPR) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_SAPR;
	if (m_lastExportType == IOTYPE_LZSS) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_LZSS;
	if (m_lastExportType == IOTYPE_LZSS_SAP) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_SAP;
	if (m_lastExportType == IOTYPE_LZSS_XEX) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_XEX;
	if (m_lastExportType == IOTYPE_ASM_RMTPLAYER) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_RELOC_ASM;
	if (m_lastExportType == IOTYPE_WAV) dlg.m_ofn.nFilterIndex = FILE_EXPORT_FILTER_IDX_WAV;

	// If not ok, nothing will be saved
	if (dlg.DoModal() == IDOK)
	{
		CString fn = dlg.GetPathName();
		int formatChoiceIndexFromDialog = dlg.m_ofn.nFilterIndex;

		if (formatChoiceIndexFromDialog < FILE_EXPORT_FILTER_IDX_MIN
			|| formatChoiceIndexFromDialog > FILE_EXPORT_FILTER_IDX_MAX)
		{
			return;
		}

		const char* exttype[] = FILE_EXPORT_EXTENSIONS_ARRAY;
		const int extlength[] = FILE_EXPORT_EXTENSIONS_LENGTH_ARRAY;
		int extoff = extlength[formatChoiceIndexFromDialog - 1];

		CString ext = fn.Right(extoff).MakeLower();
		if (ext != exttype[formatChoiceIndexFromDialog - 1]) fn += exttype[formatChoiceIndexFromDialog - 1];

		g_lastLoadPath_Songs = GetFilePath(fn);

		// Try and create the output file
		std::ofstream out(fn, std::ios::binary);
		if (!out)
		{
			MessageBox(g_hwnd, "Can't create this file: " + fn, "Export error", MB_ICONERROR);
			return;
		}

		bool exportResult = false;
		switch (formatChoiceIndexFromDialog)
		{
			case FILE_EXPORT_FILTER_IDX_STRIPPED_RMT:
				m_lastExportType = IOTYPE_RMTSTRIPPED;
				break;

			case FILE_EXPORT_FILTER_IDX_SIMPLE_ASM:
				m_lastExportType = IOTYPE_ASM;
				break;

			case FILE_EXPORT_FILTER_IDX_SAPR:
				m_lastExportType = IOTYPE_SAPR;
				break;

			case FILE_EXPORT_FILTER_IDX_LZSS:
				m_lastExportType = IOTYPE_LZSS;
				break;

			case FILE_EXPORT_FILTER_IDX_SAP:
				m_lastExportType = IOTYPE_LZSS_SAP;
				break;

			case FILE_EXPORT_FILTER_IDX_XEX:
				m_lastExportType = IOTYPE_LZSS_XEX;
				break;

			case FILE_EXPORT_FILTER_IDX_RELOC_ASM:	// Relocatable ASM for RMTPlayer
				m_lastExportType = IOTYPE_ASM_RMTPLAYER;
				break;

			case FILE_EXPORT_FILTER_IDX_WAV:
				m_lastExportType = IOTYPE_WAV;
				break;

		}

		// Save the file using the set parameters 
		exportResult = ExportV2(out, m_lastExportType, (LPCTSTR)fn);

		// File should have been successfully saved, make sure to close it
		out.close();

		// TODO: add a method to prevent accidental deletion of valid files
		if (!exportResult)
		{
			DeleteFile(fn);
			MessageBox(g_hwnd, "Export aborted.\nFile was deleted, beware of data loss!", "Export aborted", MB_ICONEXCLAMATION);
		}
	}
}

/// <summary>
/// Save an instrument as RTI (binary format)
/// </summary>
void CSong::FileInstrumentSave()
{
	// Stop the music first
	//Stop();

	CFileDialog dlg(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"RMT instrument file (*.rti)|*.rti||");
	dlg.m_ofn.lpstrTitle = "Save RMT instrument file";

	if (!g_lastLoadPath_Instruments.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Instruments;
	else
		if (!g_defaultInstrumentsPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultInstrumentsPath;

	// If it's not ok, nothing is saved
	if (dlg.DoModal() == IDOK)
	{
		CString fn = dlg.GetPathName();
		CString ext = fn.Right(4).MakeLower();
		if (ext != ".rti") fn += ".rti";

		g_lastLoadPath_Instruments = GetFilePath(fn);

		std::ofstream ou(fn, std::ios::binary);
		if (!ou)
		{
			MessageBox(g_hwnd, "Can't create the instrument file: " + fn, "Write error", MB_ICONERROR);
			return;
		}

		g_Instruments.SaveInstrument(m_activeInstrument, ou, IOINSTR_RTI);

		ou.close();
	}
}

/// <summary>
/// Load instrument definition RTI (binary format)
/// </summary>
void CSong::FileInstrumentLoad()
{
	// Stop the music first
	//Stop();

	CFileDialog dlg(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"RMT instrument files (*.rti)|*.rti||");
	dlg.m_ofn.lpstrTitle = "Load RMT instrument file";

	if (!g_lastLoadPath_Instruments.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Instruments;
	else
		if (!g_defaultInstrumentsPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultInstrumentsPath;

	// If it's not ok, nothing will be loaded
	if (dlg.DoModal() == IDOK)
	{
		g_Undo.ChangeInstrument(m_activeInstrument, 0, UETYPE_INSTRDATA, 1);

		CString fn = dlg.GetPathName();
		g_lastLoadPath_Instruments = GetFilePath(fn);	//direct way

		std::ifstream in(fn, std::ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}

		int loadState = g_Instruments.LoadInstrument(m_activeInstrument, in, IOINSTR_RTI);
		in.close();

		if (!loadState)
		{
			MessageBox(g_hwnd, "Failed to load RTI format (standard version 0)", "Data error", MB_ICONERROR);
			return;
		}
	}
}

/// <summary>
/// Save the active track as a text file
/// </summary>
void CSong::FileTrackSave()
{
	int track = SongGetActiveTrack();
	if (track < 0 || track >= TRACKSNUM) return;

	// Stop the music first
	//Stop();

	CFileDialog dlg(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"TXT track file (*.txt)|*.txt||");
	dlg.m_ofn.lpstrTitle = "Save TXT track file";

	if (!g_lastLoadPath_Tracks.IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Tracks;
	else
		if (!g_defaultTracksPath.IsEmpty()) dlg.m_ofn.lpstrInitialDir = g_defaultTracksPath;

	//if not ok, nothing will be saved
	if (dlg.DoModal() == IDOK)
	{
		CString fn = dlg.GetPathName();
		CString ext = fn.Right(4).MakeLower();
		if (ext != ".txt") fn += ".txt";

		g_lastLoadPath_Tracks = GetFilePath(fn);

		std::ofstream ou(fn);	// text mode by default
		if (!ou)
		{
			MessageBox(g_hwnd, "Can't create this file: " + fn, "Write error", MB_ICONERROR);
			return;
		}

		g_Tracks.SaveTrack(track, ou, IOTYPE_TXT);

		ou.close();
	}
}

/// <summary>
/// Load a text track
/// </summary>
void CSong::FileTrackLoad()
{
	int track = SongGetActiveTrack();
	if (track < 0 || track >= TRACKSNUM) return;

	// Stop the music first
	//Stop();

	CFileDialog dlg(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"TXT track files (*.txt)|*.txt||");
	dlg.m_ofn.lpstrTitle = "Load TXT track file";

	if (!g_lastLoadPath_Tracks .IsEmpty())
		dlg.m_ofn.lpstrInitialDir = g_lastLoadPath_Tracks;
	else
	if (!g_defaultTracksPath.IsEmpty())	dlg.m_ofn.lpstrInitialDir = g_defaultTracksPath;

	// If not ok, nothing will be loaded
	if (dlg.DoModal() == IDOK)
	{
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, 1);

		CString fn = dlg.GetPathName();
		g_lastLoadPath_Tracks = GetFilePath(fn);

		std::ifstream in(fn);	// text mode by default
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}

		char line[1025];
		int nt = 0;				// number of tracks
		int type = 0;			// type when loading multiple tracks
		while (NextSegment(in)) // will therefore look for the beginning of the next segment "["
		{
			in.getline(line, 1024);
			Trimstr(line);
			if (strcmp(line, "TRACK]") == 0) nt++;
		}

		if (nt == 0)
		{
			MessageBox(g_hwnd, "Sorry, this file doesn't contain any track in TXT format", "Data error", MB_ICONERROR);
			return;
		}
		else if (nt > 1)
		{
			CTracksLoadDlg dlg;
			dlg.m_trackfrom = track;
			dlg.m_tracknum = nt;
			if (dlg.DoModal() != IDOK) return;
			type = dlg.m_radio;
		}

		in.clear();			// Reset the flag from the end
		in.seekg(0);		// Again at the beginning
		int nr = 0;
		NextSegment(in);	// Move after the first "["
		while (!in.eof())
		{
			in.getline(line, 1024);
			Trimstr(line);
			if (strcmp(line, "TRACK]") == 0)
			{
				int tt = (type == 0) ? track : -1;
				if (g_Tracks.LoadTrack(tt, in, IOTYPE_TXT))
				{
					nr++;	//number of tracks loaded
					if (type == 0)
					{
						track++;	//shift by 1 to load the next track
						if (track >= TRACKSNUM)
						{
							MessageBox(g_hwnd, "Track's maximum number reached.\nLoading aborted.", "Error", MB_ICONERROR);
							break;
						}
					}
				}
			}
			else
				NextSegment(in);	//move to the next "["
		}
		in.close();

		if (nr == 0 || nr > 1) // if it has not found any or more than 1
		{
			CString s;
			s.Format("%i track(s) loaded.", nr);
			MessageBox(g_hwnd, s, "Track(s) loading finished.", MB_ICONINFORMATION);
		}
	}
}

#define RMWMAINPARAMSCOUNT		31		//
#define DEFINE_MAINPARAMS int* mainparams[RMWMAINPARAMSCOUNT]= {\
	&g_tracks4_8,												\
	(int*)&m_playSpeed,(int*)&m_mainSpeed,(int*)&m_instrumentSpeed,	\
	(int*)&m_activeSongline,(int*)&m_playSongline,				\
	(int*)&m_activeRow,(int*)&m_playRow,			\
	(int*)&g_activepart,(int*)&g_active_ti,						\
	(int*)&g_prove,(int*)&g_respectvolume,						\
	&g_trackLinePrimaryHighlight,								\
	&g_tracklinealtnumbering,									\
	&g_displayflatnotes,										\
	&g_usegermannotation,										\
	&g_cursoractview,											\
	&g_keyboard_layout,											\
	&g_keyboard_escresetatarisound,								\
	&g_keyboard_swapenter,										\
	&g_keyboard_playautofollow,									\
	&g_keyboard_updowncontinue,									\
	&g_keyboard_RememberOctavesAndVolumes,						\
	&g_keyboard_escresetatarisound,								\
	&m_activeChannel,&m_activeCursor,						\
	&m_activeInstrument,&m_activeVolume,&m_activeOctave,							\
	&m_infoact,&m_songnamecur									\
}

bool CSong::SaveRMW(std::ofstream& ou)
{
	CString version;
	version.LoadString(IDS_RMTVERSION);
	ou << (unsigned char*)(LPCSTR)version << std::endl;
	//
	ou.write((char*)m_songname, sizeof(m_songname));
	//
	DEFINE_MAINPARAMS;

	int p = RMWMAINPARAMSCOUNT;			// Number of stored parameters
	ou.write((char*)&p, sizeof(p));		// Write the number of main parameters
	for (int i = 0; i < p; i++)
		ou.write((char*)mainparams[i], sizeof(mainparams[0]));

	// Write a complete song and songgo
	ou.write((char*)m_song, sizeof(m_song));
	ou.write((char*)m_songgo, sizeof(m_songgo));

	g_Instruments.SaveAll(ou, IOTYPE_RMW);
	g_Tracks.SaveAll(ou, IOTYPE_RMW);

	return true;
}

bool CSong::LoadRMW(std::ifstream& in)
{
	ClearSong(8);	// Always clear 8 tracks 

	CString version;
	version.LoadString(IDS_RMTVERSION);
	char filever[256];
	in.getline(filever, 255);
	if (strcmp((char*)(LPCTSTR)version, filever) != 0)
	{
		MessageBox(g_hwnd, CString("Incorrect version: ") + filever, "Load error", MB_ICONERROR);
		return false;
	}
	//
	in.read((char*)m_songname, sizeof(m_songname));
	//
	DEFINE_MAINPARAMS;
	int p = 0;
	in.read((char*)&p, sizeof(p));	//read the number of main parameters
	for (int i = 0; i < p; i++)
		in.read((char*)mainparams[i], sizeof(mainparams[0]));

	// Read the complete song and songgo
	in.read((char*)m_song, sizeof(m_song));
	in.read((char*)m_songgo, sizeof(m_songgo));

	g_Instruments.LoadAll(in, IOTYPE_RMW);
	g_Tracks.LoadAll(in, IOTYPE_RMW);

	return true;
}

/// <summary>
/// Save the song as a text file
/// </summary>
/// <param name="ou">Output stream</param>
/// <returns></returns>
bool CSong::SaveTxt(std::ofstream& ou)
{
	CString s, nambf;
	char bf[16];
	nambf = m_songname;
	nambf.TrimRight();
	s.Format("[MODULE]\nRMT: %X\nNAME: %s\nMAXTRACKLEN: %02X\nMAINSPEED: %02X\nINSTRSPEED: %X\nVERSION: %02X\n", g_tracks4_8, (LPCTSTR)nambf, g_Tracks.GetMaxTrackLength(), m_mainSpeed, m_instrumentSpeed, RMTFORMATVERSION);
	ou << s << "\n"; //gap
	ou << "[SONG]\n";
	int i, j;
	// Looking for the length of the song
	int songLength = -1;
	for (i = 0; i < SONGLEN; i++)
	{
		if (m_songgo[i] >= 0) { songLength = i; continue; }
		for (j = 0; j < g_tracks4_8; j++)
		{
			if (m_song[i][j] >= 0 && m_song[i][j] < TRACKSNUM)
			{
				songLength = i;
				break;
			}
		}
	}

	// Write the song
	for (i = 0; i <= songLength; i++)
	{
		if (m_songgo[i] >= 0)
		{
			s.Format("Go to line %02X\n", m_songgo[i]);
			ou << s;
			continue;
		}
		for (j = 0; j < g_tracks4_8; j++)
		{
			int t = m_song[i][j];
			if (t >= 0 && t < TRACKSNUM)
			{
				bf[0] = CharH4(t);
				bf[1] = CharL4(t);
			}
			else
			{
				bf[0] = bf[1] = '-';
			}
			bf[2] = 0;
			ou << bf;
			if (j + 1 == g_tracks4_8)
				ou << "\n";			//for the last end of the line
			else
				ou << " ";			//between them
		}
	}

	ou << "\n"; // gap

	// Now save the instruments and tracks to the output
	g_Instruments.SaveAll(ou, IOTYPE_TXT);
	g_Tracks.SaveAll(ou, IOTYPE_TXT);

	return true;
}

/// <summary>
/// Load a text RMT file
/// </summary>
/// <param name="in">Input stream</param>
/// <returns>Returns true if the file was loaded, does not mean the resultant data is valid</returns>
bool CSong::LoadTxt(std::ifstream& in)
{
	ClearSong(8);	// Always clear 8 tracks 

	g_Tracks.InitTracks();

	char b;
	char line[1025];
	// Read until the first "[" is found. This indicates a segment [.....]
	NextSegment(in);

	while (!in.eof())
	{
		in.getline(line, 1024);			// Read the rest of the line
		Trimstr(line);					// Get rid of /r/n at the end

		if (strcmp(line, "MODULE]") == 0)
		{
			// [MODULE]
			while (!in.eof())
			{
				// Check for next segment start '['
				in.read((char*)&b, 1);
				if (b == '[') break;
				// Not a segment start so save the read character and get the rest of the line
				line[0] = b;
				in.getline(line + 1, 1024);

				// Split on the ": " (COLON + SPACE) point
				char* value = strstr(line, ": ");
				if (value)
				{
					value[1] = 0;	// Zero terminate the string on the left
					value += 2;		// move to the first character after the space
				}
				else
					continue;

				// Process each of the possible commands in a [MODULE]
				if (strcmp(line, "RMT:") == 0)
				{
					// RMT version indicator: 4 or 8
					int v = Hexstr(value, 2);
					if (v <= 4)
						v = 4;
					else
						v = 8;
					g_tracks4_8 = v;
				}
				else
				if (strcmp(line, "NAME:") == 0)
				{
					// Set the name of the song.
					Trimstr(value);
					memset(m_songname, ' ', SONG_NAME_MAX_LEN);
					int lname = SONG_NAME_MAX_LEN;
					if (strlen(value) <= SONG_NAME_MAX_LEN) lname = (int) strlen(value);
					strncpy(m_songname, value, lname);
				}
				else
				if (strcmp(line, "MAXTRACKLEN:") == 0)
				{
					// Set how long a track is: MAXTRACKLEN: 00-FF
					int v = Hexstr(value, 2);
					if (v == 0) v = 256;
					g_Tracks.SetMaxTrackLength(v);
					g_Tracks.InitTracks();		// Reinitialise
				}
				else
				if (strcmp(line, "MAINSPEED:") == 0)
				{
					// Set the play speed: MAINSPEED: 01-FF
					int v = Hexstr(value, 2);
					if (v > 0) m_mainSpeed = v;
				}
				else
				if (strcmp(line, "INSTRSPEED:") == 0)
				{
					// Set the instrument speed: INSTRSPEED: 01-FF
					int v = Hexstr(value, 1);
					if (v > 0) m_instrumentSpeed = v;
				}
				else
				if (strcmp(line, "VERSION:") == 0)
				{
					// The version number is not needed for TXT yet, because it only selects the parameters it knows
				}
			}
		}
		else
		if (strcmp(line, "SONG]") == 0)
		{
			// [SONG]
			int idx, i;
			for (idx = 0; !in.eof() && idx < SONGLEN; idx++)
			{
				// Read the song line. Dump out if its the next section
				memset(line, 0, 32);
				in.read((char*)&b, 1);
				if (b == '[') break;
				line[0] = b;
				in.getline(line + 1, 1024);

				// The line go be one of two types
				// "Go to line XX"
				// "-- -- -- --" or "-- -- -- -- -- -- -- --"
				if (strncmp(line, "Go to line ", 11) == 0)
				{
					int go = Hexstr(line + 11, 2);
					if (go >= 0 && go < SONGLEN) m_songgo[idx] = go;
					continue;
				}
				for (i = 0; i < g_tracks4_8; i++)
				{
					// Parse the track
					int track = Hexstr(line + i * 3, 2);
					if (track >= 0 && track < TRACKSNUM) m_song[idx][i] = track;
				}
			}
		}
		else
		if (strcmp(line, "INSTRUMENT]") == 0)
		{
			// [INSTRUMNENT]
			// Pass the instrument loading to the CInstruments class
			g_Instruments.LoadInstrument(-1, in, IOTYPE_TXT); //-1 => retrieve the instrument number from the TXT source
		}
		else
		if (strcmp(line, "TRACK]") == 0)
		{
			// [TRACK]
			// Pass the track loading to the CTracks class
			g_Tracks.LoadTrack(-1, in, IOTYPE_TXT);	//-1 => retrieve the track number from TXT source
		}
		else
			NextSegment(in); // Look for the beginning of the next segment
	}

	return true;
}



/// <summary>
/// Validate the song data to make sure that
/// - a RMT module could be created [Error]
/// - the song is not empty [Error]
/// - a goto statement does not go past the end of the song [Error]
/// - there is no recursive goto [Error]
/// - a goto follows another goto (which would be a waste) [Warning]
/// - Check that there is not more then one continuous blank song line [Warning]
/// - there is a goto at the end of the song, goto 0 is used by default [Warning]
/// </summary>
/// <returns>true if the song data is valid</returns>
bool CSong::TestBeforeFileSave()
{
	// It is performed on Export (everything except RMW) before the target file is overwritten
	// So if it returns 0, the export is terminated and the file is not overwritten

	// Try to create a module
	unsigned char mem[65536];
	int adr_module = 0x4000;
	BYTE instrumentSavedFlags[INSTRSNUM];
	BYTE trackSavedFlags[TRACKSNUM];

	if (MakeModule(mem, adr_module, IOTYPE_RMT, instrumentSavedFlags, trackSavedFlags) < 0)
		return false;	// Dump out if the module could not be created

	// and now it will be checked whether the song ends with GOTO line and if there is no GOTO on GOTO line
	CString errmsg, wrnmsg, s;
	int trx[SONGLEN];
	int i, j, r, go, last = -1, tr = 0, empty = 0;

	for (i = 0; i < SONGLEN; i++)
	{
		if (m_songgo[i] >= 0)
		{
			trx[i] = 2;
			last = i;
		}
		else
		{
			trx[i] = 0;
			for (j = 0; j < g_tracks4_8; j++)
			{
				if (m_song[i][j] >= 0 && m_song[i][j] < TRACKSNUM)
				{
					trx[i] = 1;
					last = i; //tracks
					tr++;
					break;
				}
			}
		}
	}

	if (last < 0)
	{
		errmsg += "Error: Song is empty.\n";
	}

	for (i = 0; i <= last; i++)
	{
		if (m_songgo[i] >= 0)
		{
			//there is a goto line
			go = m_songgo[i];	//where is goto set to?
			if (go > last)
			{
				s.Format("Error: Song line [%02X]: Go to line over last used song line.\n", i);
				errmsg += s;
			}
			if (m_songgo[go] >= 0)
			{
				s.Format("Error: Song line [%02X]: Recursive \"go to line\" to \"go to line\".\n", i);
				errmsg += s;
			}
			if (i > 0 && m_songgo[i - 1] >= 0)
			{
				s.Format("Warning: Song line [%02X]: More \"go to line\" on subsequent lines.\n", i);
				wrnmsg += s;
			}
			goto TestTooManyEmptyLines;
		}
		else
		{
			//are there tracks or empty lines?
			if (trx[i] == 0)
				empty++;
			else
			{
			TestTooManyEmptyLines:
				if (empty > 1)
				{
					s.Format("Warning: Song lines [%02X-%02X]: Too many empty song lines (%i) waste memory.\n", i - empty, i - 1, empty);
					wrnmsg += s;
				}
				empty = 0;
			}
		}
	}

	if (trx[last] == 1)
	{
		char gotoline[140];
		sprintf(gotoline, "Song line[%02X]: Unexpected end of song.\nYou have to use \"go to line\" at the end of song.\n\nSong line [00] will be used by default.", last + 1);
		MessageBox(g_hwnd, gotoline, "Warning", MB_ICONINFORMATION);
		m_songgo[last + 1] = 0;	//force a goto line to the first track line
	}

	// If the warning or error messages aren't empty, something did happen
	if (!errmsg.IsEmpty() || !wrnmsg.IsEmpty())
	{
		// If there are warnings without errors, the choice is left to ignore them
		if (errmsg.IsEmpty())
		{
			wrnmsg += "\nIgnore warnings and save anyway?";
			r = MessageBox(g_hwnd, wrnmsg, "Warnings", MB_YESNO | MB_ICONQUESTION);
			if (r == IDYES) return true;
			return false;
		}
		// Otherwise, if there are any errors, always return failure
		MessageBox(g_hwnd, errmsg + wrnmsg, "Errors", MB_ICONERROR);
		return false;
	}

	return true;
}

/// <summary>
/// Export dispatcher.
/// First build a module to make sure the data is consistent.
/// Then dispatch to the appropriate export handler
/// </summary>
/// <param name="ou">output fream</param>
/// <param name="iotype">requested output format</param>
/// <param name="filename">filename of the output</param>
/// <returns>0 if the export failed, 1 if the export is ok</returns>
bool CSong::ExportV2(std::ofstream& ou, int iotype, LPCTSTR filename)
{
	// Init the export data container
	tExportDescription exportDesc;
	memset(&exportDesc, 0, sizeof(tExportDescription));
	exportDesc.targetAddrOfModule = 0x4000;		// Standard RMT modules are set to start @ $4000

	// Create a module, if it fails stop the export
	if (!g_isRMTE)
	{
		int maxAddr = MakeModule(exportDesc.mem, exportDesc.targetAddrOfModule, iotype, exportDesc.instrumentSavedFlags, exportDesc.trackSavedFlags);
		if (maxAddr < 0)
			return false;								// If the module could not be created, the export process is immediately aborted
		exportDesc.firstByteAfterModule = maxAddr;
	}

	switch (iotype)
	{
		case IOTYPE_RMT: return ExportAsRMT(ou, &exportDesc);
		case IOTYPE_RMTSTRIPPED: return ExportAsStrippedRMT(ou, &exportDesc, filename);
		case IOTYPE_ASM: return ExportAsAsm(ou, &exportDesc);
		case IOTYPE_ASM_RMTPLAYER: return ExportAsRelocatableAsmForRmtPlayer(ou, &exportDesc);
		case IOTYPE_SAPR: return ExportSAP_R(ou);
		case IOTYPE_LZSS: return ExportLZSS(ou, filename);
		case IOTYPE_LZSS_SAP: return ExportLZSS_SAP(ou);
		case IOTYPE_LZSS_XEX: return ExportLZSS_XEX(ou);
		case IOTYPE_WAV: return ExportWav(ou, filename);
	}

	return false;	// Failed
}

/// <summary>
/// Export the song data as an RMT module with full instrument and song names.
/// Writes two data blocks.
/// </summary>
/// <param name="ou">Output stream</param>
/// <param name="exportDesc">Data about the packed RMT module</param>
/// <returns>true = saved ok</returns>
bool CSong::ExportAsRMT(std::ofstream& ou, tExportDescription *exportDesc)
{
	// Save the 1st RMT module block: Song, Tracks & Instruments
	SaveBinaryBlock(ou, exportDesc->mem, exportDesc->targetAddrOfModule, exportDesc->firstByteAfterModule - 1, TRUE);

	// Save the 2nd RMT module block: Song and Instrument Names
	// The individual names are truncated by spaces and terminated by a zero
	// Song name (0 terminated)
	CString name;
	int addrOfSongName = exportDesc->firstByteAfterModule;
	name = m_songname;
	name.TrimRight();
	int len = name.GetLength() + 1;	// including 0 after the string
	strncpy((char*)(exportDesc->mem + addrOfSongName), (LPCSTR)name, len);

	// Each saved instrument's name is written to the second module
	int addrInstrumentNames = addrOfSongName + len;
	for (int i = 0; i < INSTRSNUM; i++)
	{
		if (exportDesc->instrumentSavedFlags[i])
		{
			name = g_Instruments.GetName(i);
			name.TrimRight();
			len = name.GetLength() + 1;	//including 0 after the string
			strncpy((char*)(exportDesc->mem + addrInstrumentNames), name, len);
			addrInstrumentNames += len;
		}
	}
	// and now, save the 2nd block
	SaveBinaryBlock(ou, exportDesc->mem, addrOfSongName, addrInstrumentNames - 1, FALSE);

	return true;
}

/// <summary>
/// Export the song data as assembler source code.
/// </summary>
/// <param name="ou">Output stream</param>
/// <param name="exportStrippedDesc">Data about the packed RMT module</param>
/// <param name="filename"></param>
/// <returns>true is the save went ok</returns>
bool CSong::ExportAsStrippedRMT(std::ofstream& ou, tExportDescription* exportStrippedDesc, LPCTSTR filename)
{
	tExportDescription exportTempDescription;
	memset(&exportTempDescription, 0, sizeof(tExportDescription));
	exportTempDescription.targetAddrOfModule = 0x4000;		// Standard RMT modules are set to start @ $4000

	// Create a variant for SFX (ie. including unused instruments and tracks)
	exportTempDescription.firstByteAfterModule = MakeModule(exportTempDescription.mem, exportTempDescription.targetAddrOfModule, IOTYPE_RMT, exportTempDescription.instrumentSavedFlags, exportTempDescription.trackSavedFlags);
	if (exportTempDescription.firstByteAfterModule < 0) return false;	// if the module could not be created

	// Show the dialog to control the stripped output parameters
	CExportStrippedRMTDialog dlg;
	// Common data
	dlg.m_exportAddr = g_rmtstripped_adr_module;	//global, so that it remains the same on repeated export
	dlg.m_globalVolumeFade = g_rmtstripped_gvf;
	dlg.m_noStartingSongLine = g_rmtstripped_nos;
	dlg.m_song = this;
	dlg.m_filename = (char*)filename;
	dlg.m_sfxSupport = g_rmtstripped_sfx;
	dlg.m_assemblerFormat = g_AsmFormat;

	// Stripped RMT data
	dlg.m_moduleLengthForStrippedRMT = exportStrippedDesc->firstByteAfterModule - exportStrippedDesc->targetAddrOfModule;
	dlg.m_savedInstrFlagsForStrippedRMT = exportStrippedDesc->instrumentSavedFlags;
	dlg.m_savedTracksFlagsForStrippedRMT = exportStrippedDesc->trackSavedFlags;

	// Full RMT/SFX data
	dlg.m_moduleLengthForSFX = exportTempDescription.firstByteAfterModule - exportTempDescription.targetAddrOfModule;
	dlg.m_savedInstrFlagsForSFX = exportTempDescription.instrumentSavedFlags;
	dlg.m_savedTracksFlagsForSFX = exportTempDescription.trackSavedFlags;

	// Show the dialog and get the stripped RMT configuration parameters
	if (dlg.DoModal() != IDOK) return false;

	// Save the configurations for later reuse
	int targetAddrOfModule = dlg.m_exportAddr;

	g_rmtstripped_adr_module = dlg.m_exportAddr;
	g_rmtstripped_sfx = dlg.m_sfxSupport;
	g_rmtstripped_gvf = dlg.m_globalVolumeFade;
	g_rmtstripped_nos = dlg.m_noStartingSongLine;
	g_AsmFormat = dlg.m_assemblerFormat;

	// Now we can regenerate the RMT module with the selected configuration
	// - known start address
	// - know if we want to strip out unused instruments and tracks => IOTYPE_RMTSTRIPPED : IOTYPE_RMT
	memset(&exportTempDescription, 0, sizeof(tExportDescription));			// Clear it all again
	exportTempDescription.targetAddrOfModule = g_rmtstripped_adr_module;	// Standard RMT modules are set to start @ $4000

	exportTempDescription.firstByteAfterModule = 
		MakeModule(
			exportTempDescription.mem, 
			exportTempDescription.targetAddrOfModule, 
			g_rmtstripped_sfx ? IOTYPE_RMTSTRIPPED : IOTYPE_RMT,
			exportTempDescription.instrumentSavedFlags, 
			exportTempDescription.trackSavedFlags
		);
	if (exportTempDescription.firstByteAfterModule < 0) return false;	// if the module could not be created

	// And save the RMT module block
	SaveBinaryBlock(ou, exportTempDescription.mem, exportTempDescription.targetAddrOfModule, exportTempDescription.firstByteAfterModule, TRUE);

	return true;		// Indicate that data was saved
}

/// <summary>
/// Load a RMT file.
/// Parse the header, tracks, songs lines and instruments
/// </summary>
/// <param name="in">Input stream</param>
/// <returns>true if the load went ok</returns>
bool CSong::LoadRMT(std::ifstream& in)
{
	unsigned char mem[65536];
	memset(mem, 0, 65536);
	WORD fromAddr, toAddr;
	WORD bto_mainblock;

	BYTE instrumentLoadedFlags[INSTRSNUM];
	BYTE trackLoadedFlags[TRACKSNUM];

	int len, i, idx, k;
	int loadResult;

	// RMT header+song data is the first main block of an RMT song
	// There has to be 1 binary block with the header, song, instrument and track data
	// Optional block with instrument and song name information
	len = LoadBinaryBlock(in, mem, fromAddr, toAddr);

	if (len > 0)
	{
		loadResult = DecodeModule(mem, fromAddr, toAddr + 1, instrumentLoadedFlags, trackLoadedFlags);
		if (loadResult == 0)
		{
			MessageBox(g_hwnd, "Bad RMT data format or old tracker version.", "Open error", MB_ICONERROR);
			return false;
		}
		// The main block of the module is OK => take its boot address
		g_rmtstripped_adr_module = fromAddr;
		bto_mainblock = toAddr;
	}
	else
	{
		MessageBox(g_hwnd, "Corrupted file or unsupported format version.", "Open error", MB_ICONERROR);
		return false;	// Did not retrieve any data in the first block
	}

	// RMT - now read the second block with names
	len = LoadBinaryBlock(in, mem, fromAddr, toAddr);
	if (len < 1)
	{
		CString msg;
		msg.Format("This file appears to be a stripped RMT module.\nThe song and instruments names are missing.\n\nMemory addresses: $%04X - $%04X.", g_rmtstripped_adr_module, bto_mainblock);
		MessageBox(g_hwnd, (LPCTSTR)msg, "Info", MB_ICONINFORMATION);
		return true;
	}

	char ch;
	// Parse the song name (until we hit the terminating zero)
	for (idx = 0; idx < SONG_NAME_MAX_LEN && (ch = mem[fromAddr + idx]); idx++)
		m_songname[idx] = ch;

	for (k = idx; k < SONG_NAME_MAX_LEN; k++) m_songname[k] = ' '; // fill in the gaps

	int addrInstrumentNames = fromAddr + idx + 1; // +1 that's the zero behind the name
	for (i = 0; i < INSTRSNUM; i++)
	{
		// Check if this instrument has been loaded
		if (instrumentLoadedFlags[i])
		{
			// Yes its loaded, parse its name
			for (idx = 0; idx < INSTRUMENT_NAME_MAX_LEN && (ch = mem[addrInstrumentNames + idx]); idx++)
				//g_Instruments.m_instr[i].name[idx] = ch;
				g_Instruments.GetName(i)[idx] = ch;

			for (k = idx; k < INSTRUMENT_NAME_MAX_LEN; k++) //g_Instruments.m_instr[i].name[k] = ' '; //fill in the gaps
				g_Instruments.GetName(i)[k] = ' '; // Fill in the gaps

			// Move to source of the next instrument's name
			addrInstrumentNames += idx + 1; //+1 is zero behind the name
		}
	}

	return true;
}

bool CSong::CreateExportMetadata(int iotype, struct TExportMetadata* metadata)
{
	memcpy(metadata->songname, m_songname, SONG_NAME_MAX_LEN);
	metadata->currentTime = CTime::GetCurrentTime();
	metadata->isNTSC = g_ntsc;
	metadata->isStereo = (g_tracks4_8 == 8);
	metadata->instrspeed = m_instrumentSpeed;

	switch (iotype)
	{
	case IOTYPE_XEX:
	case IOTYPE_LZSS_XEX:
		return WriteToXEX(metadata);
	}

	return false;
}

// The XEX dialog process was split to a different function for clarity, same will be done for SAP later...
bool CSong::WriteToXEX(struct TExportMetadata* metadata)
{
	CExpMSXDlg dlg;
	CString str;

	str = metadata->songname;
	str.TrimRight();

	if (g_rmtmsxtext != "")
	{
		dlg.m_txt = g_rmtmsxtext;	// same from last time, making repeated exports faster
	}
	else
	{
		dlg.m_txt = str + EOL;
		if (metadata->isStereo) dlg.m_txt += "STEREO";
		dlg.m_txt += EOL + metadata->currentTime.Format("%d/%m/%Y");
		dlg.m_txt += EOL;
		dlg.m_txt += "Author: (press SHIFT key)" EOL;
		dlg.m_txt += "Author: ???";
	}
	str = "Playback speed will be adjusted to ";
	str += metadata->isNTSC ? "60" : "50";
	str += "Hz on both PAL and NTSC systems.";
	dlg.m_speedinfo = str;

	if (dlg.DoModal() != IDOK)
	{
		return false;
	}
	g_rmtmsxtext = dlg.m_txt;
	g_rmtmsxtext.Replace("\x0d\x0d", "\x0d");	//13, 13 => 13

	// This block of code will handle all the user input text that will be inserted in the binary during the export process
	memset(metadata->atariText, 32, 40 * 5);	// 5 lines of 40 characters at the user text address
	int p = 0, q = 0;
	char a;
	for (int i = 0; i < dlg.m_txt.GetLength(); i++)
	{
		a = dlg.m_txt.GetAt(i);
		if (a == '\n') { p += 40; q = 0; }
		else
		{
			metadata->atariText[p + q] = a;
			q++;
		}
		if (p + q >= 5 * 40) break;
	}
	StrToAtariVideo((char*)metadata->atariText, 200);

	metadata->rasterbarColour = dlg.m_metercolor;
	metadata->displayRasterbar = dlg.m_meter;
	metadata->autoRegion = dlg.m_region_auto;

	return true;
}

bool CSong::ExportWav(std::ofstream& ou, LPCTSTR filename)
{
	CWaveFile wavefile{};

	BYTE* buffer = NULL;
	BYTE* streambuffer = NULL;
	WAVEFORMATEX* wfm = NULL;
	int length = 0, frames = 0, offset = 0;
	int frameSize = (g_tracks4_8 == 8) ? 18 : 9;	// SAP-R bytes to copy, Stereo doubles the number

	ou.close();	// hack, just to be able to actually use the filename for now...

	if (!(wfm = g_Pokey.GetSoundFormat()))
	{
		MessageBox(g_hwnd, "Could not get sound format!", "ExportWav", MB_ICONWARNING);
		return false;
	}

	if (!wavefile.OpenFile((LPTSTR)filename, wfm->nSamplesPerSec, wfm->wBitsPerSample, wfm->nChannels))
	{
		MessageBox(g_hwnd, "Wav file could not be created!", "ExportWav", MB_ICONWARNING);
		return false;
	}

	// Dump the POKEY registers from full song playback
	DumpSongToPokeyBuffer();

	// Busy writing! TODO: Fix the timing overlap causing conflicts
	g_PokeyStream.SetState(CPokeyStream::WRITE);

	Atari_InitRMTRoutine();	// Reset the Atari memory 
	SetChannelOnOff(-1, 1);	// Unmute all channels

	// Create the sound buffer to copy from and to
	buffer = new BYTE[BUFFER_SIZE];
	memset(buffer, 0x80, BUFFER_SIZE);

	while (frames < g_PokeyStream.GetFirstCountPoint())
	{
		// Copy the SAP-R bytes to g_atarimem for this frame
		streambuffer = g_PokeyStream.GetStreamBuffer() + frames * frameSize;

		//for (int i = 0; i < frameSize; i++)
		//{
		//	g_atarimem[0xd200 + i] = streambuffer[i];
		//}

		g_atarimem[RMTPLAYR_TRACKN_AUDF + 0] = streambuffer[0x00];
		g_atarimem[RMTPLAYR_TRACKN_AUDF + 1] = streambuffer[0x02];
		g_atarimem[RMTPLAYR_TRACKN_AUDF + 2] = streambuffer[0x04];
		g_atarimem[RMTPLAYR_TRACKN_AUDF + 3] = streambuffer[0x06];
		g_atarimem[RMTPLAYR_TRACKN_AUDC + 0] = streambuffer[0x01];
		g_atarimem[RMTPLAYR_TRACKN_AUDC + 1] = streambuffer[0x03];
		g_atarimem[RMTPLAYR_TRACKN_AUDC + 2] = streambuffer[0x05];
		g_atarimem[RMTPLAYR_TRACKN_AUDC + 3] = streambuffer[0x07];
		g_atarimem[RMTPLAYR_V_AUDCTL] = streambuffer[0x08];

		// Fill the POKEY buffer with 1 rendered chunk
		g_Pokey.RenderSoundV2(m_instrumentSpeed, buffer, length);

		// Write the buffer to WAV file
		wavefile.WriteWave(buffer, length);

		// Update the PokeyStream offset for the next frame
		frames++;
	}

	// Clear the SAP-R dumper memory and reset RMT routines
	g_PokeyStream.FinishedRecording();

	// Finished doing WAV things...
	wavefile.CloseFile();

	// Also make sure to delete the buffer once it's no longer needed
	delete buffer;

	return true;
}

// Create a RMTE Module file
bool CSong::SaveRMTE(std::ofstream& ou)
{
/*
	TModuleHeader moduleHeader{};

	// Write the partial Module Header first
	ou.write((char*)&moduleHeader, sizeof(TModuleHeader));

	// High Header
	strncpy(moduleHeader.hiHeader.identifier, MODULE_IDENTIFIER, 4);
	moduleHeader.hiHeader.version = MODULE_VERSION;
	moduleHeader.hiHeader.region = MODULE_REGION;
	moduleHeader.hiHeader.highlightPrimary = MODULE_PRIMARY_HIGHLIGHT;
	moduleHeader.hiHeader.highlightSecondary = MODULE_SECONDARY_HIGHLIGHT;
	moduleHeader.hiHeader.baseTuning = MODULE_BASE_TUNING;
	moduleHeader.hiHeader.baseNote = MODULE_BASE_NOTE;
	moduleHeader.hiHeader.baseOctave = MODULE_BASE_OCTAVE;

	// Low Header is constructed using the written data offsets

	// Subtunes
	for (int i = 0; i < SUBTUNE_COUNT; i++)
	{
		if (TSubtune* subtune = GetSubtune(i))
		{
			moduleHeader.loHeader.subtuneIndex[i] = (UINT)ou.tellp();
			ou.write((char*)subtune, sizeof(TSubtune));
		}
	}

	// Instruments
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentV2* instrument = GetInstrument(i))
		{
			moduleHeader.loHeader.instrumentIndex[i] = (UINT)ou.tellp();
			ou.write((char*)instrument, sizeof(TInstrumentV2));
		}
	}

	// Volume Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentEnvelope* envelope = GetVolumeEnvelope(i))
		{
			moduleHeader.loHeader.volumeEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentEnvelope));
		}
	}

	// Timbre Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentEnvelope* envelope = GetTimbreEnvelope(i))
		{
			moduleHeader.loHeader.timbreEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentEnvelope));
		}
	}

	// AUDCTL Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentEnvelope* envelope = GetAudctlEnvelope(i))
		{
			moduleHeader.loHeader.audctlEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentEnvelope));
		}
	}

	// Trigger Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentTrigger* envelope = GetTriggerEnvelope(i))
		{
			moduleHeader.loHeader.triggerEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentTrigger));
		}
	}

	// Effect Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentEffect* envelope = GetEffectEnvelope(i))
		{
			moduleHeader.loHeader.effectEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentEffect));
		}
	}

	// Note Table Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentTable* envelope = GetNoteTable(i))
		{
			moduleHeader.loHeader.noteTableEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentTable));
		}
	}

	// Freq Table Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (TInstrumentTable* envelope = GetFreqTable(i))
		{
			moduleHeader.loHeader.freqTableEnvelope[i] = (UINT)ou.tellp();
			ou.write((char*)envelope, sizeof(TInstrumentTable));
		}
	}

	// Metadata
	strncpy(moduleHeader.name, GetSongName(), MODULE_SONG_NAME_MAX);
	strncpy(moduleHeader.author, GetSongAuthor(), MODULE_AUTHOR_NAME_MAX);
	strncpy(moduleHeader.copyright, GetSongCopyright(), MODULE_COPYRIGHT_INFO_MAX);

	// Write the fully constructed Module Header to file once it is ready
	ou.seekp(std::ios_base::beg);
	ou.write((char*)&moduleHeader, sizeof(TModuleHeader));
*/
	// RMTE Module file should have been successfully created
	return true;
}

// Load a RMTE Module file
bool CSong::LoadRMTE(std::ifstream& in)
{
/*
	TModuleHeader moduleHeader{};

	// Read the Module Header first
	in.read((char*)&moduleHeader, sizeof(TModuleHeader));

	// Check that the High Header starts with "RMTE", any mismatch will flag the entire file as invalid, regardless of its contents
	if (strncmp(moduleHeader.hiHeader.identifier, MODULE_IDENTIFIER, 4) != 0)
	{
		MessageBox(g_hwnd, "Invalid identifier from file header, \"RMTE\" was expected.", "LoadRMTE()", MB_ICONERROR);
		return false;
	}

	// Check the Module version number, if it is higher than current, it will not be loaded
	if (moduleHeader.hiHeader.version > MODULE_VERSION)
	{
		MessageBox(g_hwnd, "Module version is higher than expected.\nMaybe the file was created using a newer RMT version?", "LoadRMTE()", MB_ICONERROR);
		return false;
	}

	// High Header
	MODULE_REGION = moduleHeader.hiHeader.region;
	MODULE_PRIMARY_HIGHLIGHT = moduleHeader.hiHeader.highlightPrimary;
	MODULE_SECONDARY_HIGHLIGHT = moduleHeader.hiHeader.highlightSecondary;
	MODULE_BASE_TUNING = moduleHeader.hiHeader.baseTuning;
	MODULE_BASE_NOTE = moduleHeader.hiHeader.baseNote;
	MODULE_BASE_OCTAVE = moduleHeader.hiHeader.baseOctave;

	// Low Header is used to reconstruct data from file via pointers

	// Subtunes
	for (int i = 0; i < SUBTUNE_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.subtuneIndex[i])
		{
			CreateSubtune(i);
			in.seekg(offset);
			in.read((char*)GetSubtune(i), sizeof(TSubtune));
		}
	}

	// Instruments
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.instrumentIndex[i])
		{
			CreateInstrument(i);
			in.seekg(offset);
			in.read((char*)GetInstrument(i), sizeof(TInstrumentV2));
		}
	}

	// Volume Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.volumeEnvelope[i])
		{
			CreateVolumeEnvelope(i);
			in.seekg(offset);
			in.read((char*)GetVolumeEnvelope(i), sizeof(TInstrumentEnvelope));
		}
	}

	// Timbre Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.timbreEnvelope[i])
		{
			CreateTimbreEnvelope(i);
			in.seekg(offset);
			in.read((char*)GetTimbreEnvelope(i), sizeof(TInstrumentEnvelope));
		}
	}

	// AUDCTL Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.audctlEnvelope[i])
		{
			CreateAudctlEnvelope(i);
			in.seekg(offset);
			in.read((char*)GetAudctlEnvelope(i), sizeof(TInstrumentEnvelope));
		}
	}

	// Trigger Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.triggerEnvelope[i])
		{
			CreateTriggerEnvelope(i);
			in.seekg(offset);
			in.read((char*)GetTriggerEnvelope(i), sizeof(TInstrumentTrigger));
		}
	}

	// Effect Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.effectEnvelope[i])
		{
			CreateEffectEnvelope(i);
			in.seekg(offset);
			in.read((char*)GetEffectEnvelope(i), sizeof(TInstrumentEffect));
		}
	}

	// Note Table Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.noteTableEnvelope[i])
		{
			CreateNoteTable(i);
			in.seekg(offset);
			in.read((char*)GetNoteTable(i), sizeof(TInstrumentTable));
		}
	}

	// Freq Table Envelopes
	for (int i = 0; i < INSTRUMENT_COUNT; i++)
	{
		if (UINT offset = moduleHeader.loHeader.freqTableEnvelope[i])
		{
			CreateFreqTable(i);
			in.seekg(offset);
			in.read((char*)GetFreqTable(i), sizeof(TInstrumentTable));
		}
	}

	// Metadata
	SetSongName(moduleHeader.name);
	SetSongAuthor(moduleHeader.author);
	SetSongCopyright(moduleHeader.copyright);

	// Set the number of active POKEY channels from the current Subtune
	MODULE_CHANNEL_COUNT = GetChannelCount();
*/
	// Module file should have been successfully loaded
	return true;
}
