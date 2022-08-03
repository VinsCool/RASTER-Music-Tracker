#include "StdAfx.h"
#include <fstream>
using namespace std;

#include "GuiHelpers.h"

// MFC interface code
#include "FileNewDlg.h"
#include "ExportDlgs.h"
#include "importdlgs.h"
#include "EffectsDlg.h"
#include "MainFrm.h"


#include "Atari6502.h"
#include "XPokey.h"
#include "IOHelpers.h"

#include "Instruments.h"
#include "Song.h"
#include "Clipboard.h"


#include "global.h"

#include "Keyboard2NoteMapping.h"
#include "ChannelControl.h"
#include "RmtMidi.h"


extern CInstruments	g_Instruments;
extern CTrackClipboard g_TrackClipboard;
extern CXPokey g_Pokey;
extern CRmtMidi g_Midi;



void StrToAtariVideo(char* txt, int count)
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


void CSong::FileReload()
{
	if (!FileCanBeReloaded()) return;
	Stop();
	int r = MessageBox(g_hwnd, "Discard all changes since your last save?\n\nWarning: Undo operation won't be possible!!!", "Reload", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (r == IDYES)
	{
		CString filename = m_filename;
		FileOpen((LPCTSTR)filename, 0); //without Warning for changes
	}
}

void CSong::FileOpen(const char* filename, BOOL warnunsavedchanges)
{
	//stop the music first
	Stop();

	if (warnunsavedchanges && WarnUnsavedChanges()) return;

	CFileDialog fid(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"RMT song files (*.rmt)|*.rmt|TXT song files (*.txt)|*.txt|RMW song work files (*.rmw)|*.rmw||");
	fid.m_ofn.lpstrTitle = "Load song file";
	if (g_lastloadpath_songs != "")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
		if (g_path_songs != "") fid.m_ofn.lpstrInitialDir = g_path_songs;

	if (m_filetype == IOTYPE_RMT) fid.m_ofn.nFilterIndex = 1;
	if (m_filetype == IOTYPE_TXT) fid.m_ofn.nFilterIndex = 2;
	if (m_filetype == IOTYPE_RMW) fid.m_ofn.nFilterIndex = 3;

	CString fn = "";
	int type = 0;
	if (filename)
	{
		fn = filename;
		CString ext = fn.Right(4);
		ext.MakeLower();
		if (ext == ".rmt") type = IOTYPE_RMT;
		else
			if (ext == ".txt") type = IOTYPE_TXT;
			else
				if (ext == ".rmw") type = IOTYPE_RMW;
	}
	else
	{
		//if not ok, it's over
		if (fid.DoModal() == IDOK)
		{
			fn = fid.GetPathName();
			type = fid.m_ofn.nFilterIndex;
		}
	}

	if ((fn != "") && type) //only when a file was selected in the FileDialog or specified at startup
	{
		//uses fn what was selected in the FileDialog or what was specified when running //fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn); //direct way

		if (type < 1 || type>3) return;

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}
		//
		if (type == 2)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			MessageBox(g_hwnd, "TXT format is currently broken, this will be fixed in a future RMT version.\nSorry for the inconvenience...", "Open error", MB_ICONERROR);
			return;
		}
		//
		//deletes the current song
		ClearSong(g_tracks4_8);

		int result;
		switch (type)
		{
			case 1: //first choice in Dialog (RMT)
				result = LoadRMT(in);
				m_filetype = IOTYPE_RMT;
				break;
			case 2: //second choice in Dialog (TXT)
				result = Load(in, IOTYPE_TXT);
				m_filetype = IOTYPE_TXT;
				break;
			case 3: //third choice in Dialog (RMW)
				result = Load(in, IOTYPE_RMW);
				m_filetype = IOTYPE_RMW;
				break;
		}
		if (!result)
		{
			//something in the Load function failed
			//MessageBoxA(g_hwnd,"Failed to open file", "ERROR",MB_ICONERROR);
			ClearSong(g_tracks4_8);		//erases everything
			SetRMTTitle();
			g_screenupdate = 1;	//must refresh
			return;
		}

		in.close();
		m_filename = fn;

		//init speed
		m_speed = m_mainspeed;

		//window name
		SetRMTTitle();

		//all channels ON (unmute all)
		SetChannelOnOff(-1, 1);		//-1 = all, 1 = on

		if (m_instrspeed > 0x04)
		{
			//Allow RMT to support instrument speed up to 8, but warn when it's above 4. Pressing "No" resets the value to 1.
			int r = MessageBox(g_hwnd, "Instrument speed values above 4 are not officially supported by RMT.\nThis may cause compatibility issues.\nDo you want keep this nonstandard speed anyway?", "Warning", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
			if (r != IDYES)
			{
				MessageBox(g_hwnd, "Instrument speed has been reset to the value of 1.", "Warning", MB_ICONEXCLAMATION);
				m_instrspeed = 0x01;
			}
		}

		g_screenupdate = 1;
	}
}

void CSong::FileSave()
{
	//stop the music first
	Stop();

	//if the file does not yet exist, prompt the "save as" dialog first
	if (m_filename == "" || m_filetype == 0)
	{
		FileSaveAs();
		return;
	}

	//if the RMT module hasn't met the conditions required to be valid, it won't be saved/overwritten
	if (m_filetype == IOTYPE_RMT && !TestBeforeFileSave())
	{
		MessageBox(g_hwnd, "Warning!\nNo data has been saved!", "Warning", MB_ICONEXCLAMATION);
		SetRMTTitle();
		return;
	}

	//Allow saving files with speed values above 4, up to 8, which will also trigger a warning message, but it will save with no problem.
	if (m_instrspeed > 0x04)
	{
		int r = MessageBox(g_hwnd, "Instrument speed values above 4 are not officially supported by RMT.\nThis may cause compatibility issues.\nDo you want to save anyway?", "Warning", MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
		if (r != IDYES)
		{
			MessageBox(g_hwnd, "Warning!\nNo data has been saved!", "Warning", MB_ICONEXCLAMATION);
			return;
		}

	}

	//create the file to save, iso::binary will be assumed if the format isn't TXT
	ofstream out(m_filename, (m_filetype == IOTYPE_TXT) ? ios::out : ios::binary);
	if (!out)
	{
		MessageBox(g_hwnd, "Can't create this file", "Write error", MB_ICONERROR);
		return;
	}

	int r = 0;
	switch (m_filetype)
	{
		case IOTYPE_RMT: //RMT
			r = Export(out, IOTYPE_RMT);
			break;
		case IOTYPE_TXT: //TXT
			r = Save(out, IOTYPE_TXT);
			break;
		case IOTYPE_RMW: //RMW
			//remembers the current octave and volume for the active instrument (for saving to RMW) 
			//because it is only saved when the instrument is changed and could change the octave or volume before saving without subsequently changing the current instrument
			g_Instruments.MemorizeOctaveAndVolume(m_activeinstr, m_octave, m_volume);
			//and now saves:
			r = Save(out, IOTYPE_RMW);
			break;
	}

	//TODO: add a method to prevent deleting a valid .rmt by accident when a stripped .rmt export was aborted

	if (!r) //failed to save
	{
		out.close();
		DeleteFile(m_filename);
		MessageBox(g_hwnd, "RMT save aborted.\nFile was deleted, beware of data loss!", "Save aborted", MB_ICONEXCLAMATION);
	}
	else	//saved successfully
		g_changes = 0;	//changes have been saved

	SetRMTTitle();

	//closing only when "out" is open (because with IOTYPE_RMT it can be closed earlier)
	if (out.is_open()) out.close();
}

void CSong::FileSaveAs()
{
	//stop the music first
	Stop();

	CFileDialog fod(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"RMT song file (*.rmt)|*.rmt|TXT song files (*.txt)|*.txt|RMW song work file (*.rmw)|*.rmw||");
	fod.m_ofn.lpstrTitle = "Save song as...";

	if (g_lastloadpath_songs != "")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
		if (g_path_songs != "") fod.m_ofn.lpstrInitialDir = g_path_songs;

	//specifies the name of the file according to the last saved one
	char filenamebuff[1024];
	if (m_filename != "")
	{
		int pos = m_filename.ReverseFind('\\');
		if (pos < 0) pos = m_filename.ReverseFind('/');
		if (pos >= 0)
		{
			CString s = m_filename.Mid(pos + 1);
			memset(filenamebuff, 0, 1024);
			strcpy(filenamebuff, (char*)(LPCTSTR)s);
			fod.m_ofn.lpstrFile = filenamebuff;
			fod.m_ofn.nMaxFile = 1020;	//4 bytes less, just to make sure ;-)
		}
	}

	//prefers the type according to the last saved
	if (m_filetype == IOTYPE_RMT) fod.m_ofn.nFilterIndex = 1;
	if (m_filetype == IOTYPE_TXT) fod.m_ofn.nFilterIndex = 2;
	if (m_filetype == IOTYPE_RMW) fod.m_ofn.nFilterIndex = 3;

	//if not ok, nothing will be saved
	if (fod.DoModal() == IDOK)
	{
		int type = fod.m_ofn.nFilterIndex;
		if (type < 1 || type>3) return;

		m_filename = fod.GetPathName();
		const char* exttype[] = { ".rmt",".txt",".rmw" };
		CString ext = m_filename.Right(4);
		ext.MakeLower();
		if (ext != exttype[type - 1]) m_filename += exttype[type - 1];

		g_lastloadpath_songs = GetFilePath(m_filename); //direct way

		//TODO: fix saving the TXT format in a future version
		if (type == 2)
		{
			MessageBox(g_hwnd, "Can't save this file: " + m_filename, "Save error", MB_ICONERROR);
			MessageBox(g_hwnd, "TXT format is currently broken, this will be fixed in a future RMT version.\nSorry for the inconvenience...", "Save error", MB_ICONERROR);
			return;
		}

		switch (type)
		{
			case 1: //first choice
				m_filetype = IOTYPE_RMT;
				break;
			case 2: //second choice
				m_filetype = IOTYPE_TXT;
				break;
			case 3: //third choice
				m_filetype = IOTYPE_RMW;
				break;
			default:
				return;	//nothing will be saved if neither option was chosen
		}

		//if everything went well, the file will now be saved
		FileSave();
	}
}

void CSong::FileNew()
{
	//stop the music first
	Stop();

	//if the last changes were not saved, nothing will be created
	if (WarnUnsavedChanges()) return;

	CFileNewDlg dlg;
	if (dlg.DoModal() == IDOK)
	{
		g_Tracks.m_maxtracklen = dlg.m_maxtracklen;
		//g_cursoractview = g_Tracks.m_maxtracklen / 2;

		int i = dlg.m_combotype;
		g_tracks4_8 = (i == 0) ? 4 : 8;
		ClearSong(g_tracks4_8);
		SetRMTTitle();

		//automatically create 1 songline of empty patterns
		for (i = 0; i < g_tracks4_8; i++) m_song[0][i] = i;

		//set the goto to the first line 
		m_songgo[1] = 0;

		//all channels ON (unmute all)
		SetChannelOnOff(-1, 1);		//-1 = all, 1 = on

		//delete undo history
		g_Undo.Clear();

		//refresh the screen 
		g_screenupdate = 1;
	}
}

int l_lastimporttypeidx = -1;		//so that during the next import it has the pre-selected type that it imported last

void CSong::FileImport()
{
	//stop the music first
	Stop();

	if (WarnUnsavedChanges()) return;

	CFileDialog fid(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"ProTracker Modules (*.mod)|*.mod|TMC song files (*.tmc,*.tm8)|*.tmc;*.tm8||");
	fid.m_ofn.lpstrTitle = "Import song file";
	if (g_lastloadpath_songs != "")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
		if (g_path_songs != "") fid.m_ofn.lpstrInitialDir = g_path_songs;

	if (l_lastimporttypeidx >= 0) fid.m_ofn.nFilterIndex = l_lastimporttypeidx;

	//if not ok, nothing will be imported
	if (fid.DoModal() == IDOK)
	{
		Stop();

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_songs = GetFilePath(fn);	//direct way

		int type = fid.m_ofn.nFilterIndex;
		if (type < 1 || type>2) return;

		l_lastimporttypeidx = type;

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}

		int r = 0;

		switch (type)
		{
			case 1: //first choice in Dialog (MOD)

				r = ImportMOD(in);

				break;
			case 2: //second choice in Dialog (TMC)

				r = ImportTMC(in);

				break;
			case 3: //third choice in Dialog (nothing)
				break;
		}

		in.close();
		m_filename = "";

		if (!r)	//import failed?
			ClearSong(g_tracks4_8);			//delete everything
		else
		{
			//init speed
			m_speed = m_mainspeed;

			//window name
			AfxGetApp()->GetMainWnd()->SetWindowText("Imported " + fn);
			//SetRMTTitle();
		}
		//all channels ON (unmute all)
		SetChannelOnOff(-1, 1);		//-1 = all, 1 = on

		g_screenupdate = 1;
	}
}


void CSong::FileExportAs()
{
	//stop the music first
	Stop();

	//verify the integrity of the .rmt module to save first, so it won't be saved if it's not meeting the conditions for it
	if (!TestBeforeFileSave())
	{
		MessageBox(g_hwnd, "Warning!\nNo data has been saved!", "Warning", MB_ICONEXCLAMATION);
		return;
	}

	CFileDialog fod(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"RMT stripped song file (*.rmt)|*.rmt|"
		"ASM simple notation source (*.asm)|*.asm|"
		"SAP-R data stream (*.sapr)|*.sapr|"
		"Compressed SAP-R data stream (*.lzss)|*.lzss|"
		"SAP file + LZSS driver (*.sap)|*.sap|"
		"XEX Atari executable + LZSS driver (*.xex)|*.xex|");

	fod.m_ofn.lpstrTitle = "Export song as...";

	if (g_lastloadpath_songs != "")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_songs;
	else
		if (g_path_songs != "") fod.m_ofn.lpstrInitialDir = g_path_songs;

	if (m_exporttype == IOTYPE_RMTSTRIPPED) fod.m_ofn.nFilterIndex = 1;
	if (m_exporttype == IOTYPE_ASM) fod.m_ofn.nFilterIndex = 2;
	if (m_exporttype == IOTYPE_SAPR) fod.m_ofn.nFilterIndex = 3;
	if (m_exporttype == IOTYPE_LZSS) fod.m_ofn.nFilterIndex = 4;
	if (m_exporttype == IOTYPE_LZSS_SAP) fod.m_ofn.nFilterIndex = 5;
	if (m_exporttype == IOTYPE_LZSS_XEX) fod.m_ofn.nFilterIndex = 6;

	//if not ok, nothing will be saved
	if (fod.DoModal() == IDOK)
	{
		CString fn;
		fn = fod.GetPathName();
		int type = fod.m_ofn.nFilterIndex;

		if (type < 1 || type > 6) return;

		const char* exttype[] = { ".rmt",".asm",".sapr",".lzss",".sap",".xex" };
		int extoff = (type - 1 == 2 || type - 1 == 3) ? 5 : 4;	//fixes the "duplicate extention" bug for 4 characters extention

		CString ext = fn.Right(extoff);
		ext.MakeLower();
		if (ext != exttype[type - 1]) fn += exttype[type - 1];

		g_lastloadpath_songs = GetFilePath(fn); //direct way

		ofstream out(fn, ios::binary);
		if (!out)
		{
			MessageBox(g_hwnd, "Can't create this file: " + fn, "Write error", MB_ICONERROR);
			return;
		}

		int r;
		switch (type)
		{
			case 1: //RMT Stripped
				r = Export(out, IOTYPE_RMTSTRIPPED, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_RMTSTRIPPED;
				break;

			case 2: //ASM
				r = Export(out, IOTYPE_ASM, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_ASM;
				break;

			case 3:	//SAP-R
				r = Export(out, IOTYPE_SAPR, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_SAPR;
				break;

			case 4:	//LZSS
				r = Export(out, IOTYPE_LZSS, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_LZSS;
				break;

			case 5:	//LZSS SAP
				r = Export(out, IOTYPE_LZSS_SAP, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_LZSS_SAP;
				break;

			case 6:	//LZSS XEX
				r = Export(out, IOTYPE_LZSS_XEX, (char*)(LPCTSTR)fn);
				m_exporttype = IOTYPE_LZSS_XEX;
				break;
		}

		//file should have been successfully saved, make sure to close it
		out.close();

		//TODO: add a method to prevent accidental deletion of valid files
		if (!r)
		{
			DeleteFile(fn);
			MessageBox(g_hwnd, "Export aborted.\nFile was deleted, beware of data loss!", "Export aborted", MB_ICONEXCLAMATION);
		}

	}
}

void CSong::FileInstrumentSave()
{
	//stop the music first
	Stop();

	CFileDialog fod(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"RMT instrument file (*.rti)|*.rti||");
	fod.m_ofn.lpstrTitle = "Save RMT instrument file";

	if (g_lastloadpath_instruments != "")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_instruments;
	else
		if (g_path_instruments) fod.m_ofn.lpstrInitialDir = g_path_instruments;

	//if it's not ok, nothing is saved
	if (fod.DoModal() == IDOK)
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext = fn.Right(4);
		ext.MakeLower();
		if (ext != ".rti") fn += ".rti";

		g_lastloadpath_instruments = GetFilePath(fn);	//direct way

		ofstream ou(fn, ios::binary);
		if (!ou)
		{
			MessageBox(g_hwnd, "Can't create this file: " + fn, "Write error", MB_ICONERROR);
			return;
		}

		g_Instruments.SaveInstrument(m_activeinstr, ou, IOINSTR_RTI);

		ou.close();
	}
}

void CSong::FileInstrumentLoad()
{
	//stop the music first
	Stop();

	CFileDialog fid(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"RMT instrument files (*.rti)|*.rti||");
	fid.m_ofn.lpstrTitle = "Load RMT instrument file";
	if (g_lastloadpath_instruments != "")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_instruments;
	else
		if (g_path_instruments) fid.m_ofn.lpstrInitialDir = g_path_instruments;

	//if it's not ok, nothing will be loaded
	if (fid.DoModal() == IDOK)
	{
		g_Undo.ChangeInstrument(m_activeinstr, 0, UETYPE_INSTRDATA, 1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_instruments = GetFilePath(fn);	//direct way

		ifstream in(fn, ios::binary);
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}

		int r = g_Instruments.LoadInstrument(m_activeinstr, in, IOINSTR_RTI);
		in.close();
		g_screenupdate = 1;

		if (!r)
		{
			MessageBox(g_hwnd, "It isn't RTI format (standard version 0)", "Data error", MB_ICONERROR);
			return;
		}
	}
}

void CSong::FileTrackSave()
{
	int track = SongGetActiveTrack();
	if (track < 0 || track >= TRACKSNUM) return;

	//stop the music first
	Stop();

	CFileDialog fod(FALSE,
		NULL,
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"TXT track file (*.txt)|*.txt||");
	fod.m_ofn.lpstrTitle = "Save TXT track file";

	if (g_lastloadpath_tracks != "")
		fod.m_ofn.lpstrInitialDir = g_lastloadpath_tracks;
	else
		if (g_path_tracks != "")
			fod.m_ofn.lpstrInitialDir = g_path_tracks;

	//if not ok, nothing will be saved
	if (fod.DoModal() == IDOK)
	{
		CString fn;
		fn = fod.GetPathName();
		CString ext = fn.Right(4);
		ext.MakeLower();
		if (ext != ".txt") fn += ".txt";

		g_lastloadpath_tracks = GetFilePath(fn);	//direct way

		ofstream ou(fn);	//text mode by default
		if (!ou)
		{
			MessageBox(g_hwnd, "Can't create this file: " + fn, "Write error", MB_ICONERROR);
			return;
		}

		g_Tracks.SaveTrack(track, ou, IOTYPE_TXT);

		ou.close();
	}
}

void CSong::FileTrackLoad()
{
	int track = SongGetActiveTrack();
	if (track < 0 || track >= TRACKSNUM) return;

	//stop the music first
	Stop();

	CFileDialog fid(TRUE,
		NULL,
		NULL,
		OFN_HIDEREADONLY,
		"TXT track files (*.txt)|*.txt||");
	fid.m_ofn.lpstrTitle = "Load TXT track file";
	if (g_lastloadpath_tracks != "")
		fid.m_ofn.lpstrInitialDir = g_lastloadpath_tracks;
	else
		if (g_path_tracks != "")
			fid.m_ofn.lpstrInitialDir = g_path_tracks;

	//if not ok, nothing will be loaded
	if (fid.DoModal() == IDOK)
	{
		g_Undo.ChangeTrack(0, 0, UETYPE_TRACKSALL, 1);

		CString fn;
		fn = fid.GetPathName();
		g_lastloadpath_tracks = GetFilePath(fn);	//direct way
		ifstream in(fn);	//text mode by default
		if (!in)
		{
			MessageBox(g_hwnd, "Can't open this file: " + fn, "Open error", MB_ICONERROR);
			return;
		}

		char line[1025];
		int nt = 0;		//number of tracks
		int type = 0;		//type when loading multiple tracks
		while (NextSegment(in)) //will therefore look for the beginning of the next segment "["
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
		else
			if (nt > 1)
			{
				CTracksLoadDlg dlg;
				dlg.m_trackfrom = track;
				dlg.m_tracknum = nt;
				if (dlg.DoModal() != IDOK) return;
				type = dlg.m_radio;
			}

		in.seekg(0);	//again at the beginning
		in.clear();		//reset the flag from the end
		int nr = 0;
		NextSegment(in);	//move after the first "["
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
					g_screenupdate = 1;
				}
			}
			else
				NextSegment(in);	//move to the next "["
		}
		in.close();

		if (nr == 0 || nr > 1) //if it has not found any or more than 1
		{
			CString s;
			s.Format("%i track(s) loaded.", nr);
			MessageBox(g_hwnd, s, "Track(s) loading finished.", MB_ICONINFORMATION);
		}
	}
}


#define RMWMAINPARAMSCOUNT		31		//
#define DEFINE_MAINPARAMS int* mainparams[RMWMAINPARAMSCOUNT]= {		\
	&g_tracks4_8,												\
	(int*)&m_speed,(int*)&m_mainspeed,(int*)&m_instrspeed,		\
	(int*)&m_songactiveline,(int*)&m_songplayline,				\
	(int*)&m_trackactiveline,(int*)&m_trackplayline,			\
	(int*)&g_activepart,(int*)&g_active_ti,						\
	(int*)&g_prove,(int*)&g_respectvolume,						\
	&g_tracklinehighlight,										\
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
	&m_trackactivecol,&m_trackactivecur,						\
	&m_activeinstr,&m_volume,&m_octave,							\
	&m_infoact,&m_songnamecur									\
}

int CSong::Save(ofstream& ou, int iotype)
{
	switch (iotype)
	{
		case IOTYPE_RMW:
		{
			CString version;
			version.LoadString(IDS_RMTVERSION);
			ou << (unsigned char*)(LPCSTR)version << endl;
			//
			ou.write((char*)m_songname, sizeof(m_songname));
			//
			DEFINE_MAINPARAMS;

			int p = RMWMAINPARAMSCOUNT; //number of stored parameters
			ou.write((char*)&p, sizeof(p));		//write the number of main parameters
			for (int i = 0; i < p; i++)
				ou.write((char*)mainparams[i], sizeof(mainparams[0]));

			//write a complete song and songgo
			ou.write((char*)m_song, sizeof(m_song));
			ou.write((char*)m_songgo, sizeof(m_songgo));
		}
		break;

		case IOTYPE_TXT:
		{
			CString s, nambf;
			char bf[16];
			nambf = m_songname;
			nambf.TrimRight();
			s.Format("[MODULE]\nRMT: %X\nNAME: %s\nMAXTRACKLEN: %02X\nMAINSPEED: %02X\nINSTRSPEED: %X\nVERSION: %02X\n", g_tracks4_8, (LPCTSTR)nambf, g_Tracks.m_maxtracklen, m_mainspeed, m_instrspeed, RMTFORMATVERSION);
			ou << s << "\n"; //gap
			ou << "[SONG]\n";
			int i, j;
			//looking for the length of the song
			int lens = -1;
			for (i = 0; i < SONGLEN; i++)
			{
				if (m_songgo[i] >= 0) { lens = i; continue; }
				for (j = 0; j < g_tracks4_8; j++)
				{
					if (m_song[i][j] >= 0 && m_song[i][j] < TRACKSNUM) { lens = i; break; }
				}
			}

			//write the song
			for (i = 0; i <= lens; i++)
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

			ou << "\n"; //gap
		}
		break;
	}

	g_Instruments.SaveAll(ou, iotype);
	g_Tracks.SaveAll(ou, iotype);

	return 1;
}


int CSong::Load(ifstream& in, int iotype)
{
	ClearSong(8);	//always clear 8 tracks 

	if (iotype == IOTYPE_RMW)
	{
		//LOAD RMW
		CString version;
		version.LoadString(IDS_RMTVERSION);
		char filever[256];
		in.getline(filever, 255);
		if (strcmp((char*)(LPCTSTR)version, filever) != 0)
		{
			MessageBox(g_hwnd, CString("Incorrect version: ") + filever, "Load error", MB_ICONERROR);
			return 0;
		}
		//
		in.read((char*)m_songname, sizeof(m_songname));
		//
		DEFINE_MAINPARAMS;
		int p = 0;
		in.read((char*)&p, sizeof(p));	//read the number of main parameters
		for (int i = 0; i < p; i++)
			in.read((char*)mainparams[i], sizeof(mainparams[0]));
		//

		//read the complete song and songgo
		in.read((char*)m_song, sizeof(m_song));
		in.read((char*)m_songgo, sizeof(m_songgo));

		g_Instruments.LoadAll(in, iotype);
		g_Tracks.LoadAll(in, iotype);
	}
	else
		if (iotype == IOTYPE_TXT)
		{
			//LOAD TXT
			g_Tracks.InitTracks();

			char b;
			char line[1025];
			//read after the first "[" (inclusive)
			NextSegment(in);

			while (!in.eof())
			{
				in.getline(line, 1024);
				Trimstr(line);
				if (strcmp(line, "MODULE]") == 0)
				{
					//MODULE
					while (!in.eof())
					{
						in.read((char*)&b, 1);
						if (b == '[') break;
						line[0] = b;
						in.getline(line + 1, 1024);
						char* value = strstr(line, ": ");
						if (value)
						{
							value[1] = 0;	//gap
							value += 2;	//move to the first character after the space
						}
						else
							continue;

						if (strcmp(line, "RMT:") == 0)
						{
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
								Trimstr(value);
								memset(m_songname, ' ', SONG_NAME_MAX_LEN);
								int lname = SONG_NAME_MAX_LEN;
								if (strlen(value) <= SONG_NAME_MAX_LEN) lname = strlen(value);
								strncpy(m_songname, value, lname);
							}
							else
								if (strcmp(line, "MAXTRACKLEN:") == 0)
								{
									int v = Hexstr(value, 2);
									if (v == 0) v = 256;
									g_Tracks.m_maxtracklen = v;
									//g_cursoractview = g_Tracks.m_maxtracklen / 2;
									g_Tracks.InitTracks();	//reinitialise
								}
								else
									if (strcmp(line, "MAINSPEED:") == 0)
									{
										int v = Hexstr(value, 2);
										if (v > 0) m_mainspeed = v;
									}
									else
										if (strcmp(line, "INSTRSPEED:") == 0)
										{
											int v = Hexstr(value, 1);
											if (v > 0) m_instrspeed = v;
										}
										else
											if (strcmp(line, "VERSION:") == 0)
											{
												//int v = Hexstr(value,2);
												//the version number is not needed for TXT yet, because it only selects the parameters it knows
											}
					}
				}
				else
					if (strcmp(line, "SONG]") == 0)
					{
						//SONG 
						int idx, i;
						for (idx = 0; !in.eof() && idx < SONGLEN; idx++)
						{
							memset(line, 0, 32);
							in.read((char*)&b, 1);
							if (b == '[') break;
							line[0] = b;
							in.getline(line + 1, 1024);
							if (strncmp(line, "Go to line ", 11) == 0)
							{
								int go = Hexstr(line + 11, 2);
								if (go >= 0 && go < SONGLEN) m_songgo[idx] = go;
								continue;
							}
							for (i = 0; i < g_tracks4_8; i++)
							{
								int track = Hexstr(line + i * 3, 2);
								if (track >= 0 && track < TRACKSNUM) m_song[idx][i] = track;
							}
						}
					}
					else
						if (strcmp(line, "INSTRUMENT]") == 0)
						{
							g_Instruments.LoadInstrument(-1, in, iotype); //-1 => retrieve the instrument number from the TXT source
						}
						else
							if (strcmp(line, "TRACK]") == 0)
							{
								g_Tracks.LoadTrack(-1, in, iotype);	//-1 => retrieve the track number from TXT source
							}
							else
								NextSegment(in); //will therefore look for the beginning of the next segment
			}
		}

	return 1;
}

int CSong::TestBeforeFileSave()
{
	//it is performed on Export (everything except RMW) before the target file is overwritten
	//so if it returns 0, the export is terminated and the file is not overwritten

	//try to create a module
	unsigned char mem[65536];
	int adr_module = 0x4000;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];
	int maxadr;

	//try to make a blank RMT module
	maxadr = MakeModule(mem, adr_module, IOTYPE_RMT, instrsaved, tracksaved);
	if (maxadr < 0) return 0;	//if the module could not be created

	//and now it will be checked whether the song ends with GOTO line and if there is no GOTO on GOTO line
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

	//if the warning or error messages aren't empty, something did happen
	if (errmsg != "" || wrnmsg != "")
	{
		//if there are warnings without errors, the choice is left to ignore them
		if (errmsg == "")
		{
			wrnmsg += "\nIgnore warnings and save anyway?";
			r = MessageBox(g_hwnd, wrnmsg, "Warnings", MB_YESNO | MB_ICONQUESTION);
			if (r == IDYES) return 1;
			return 0;
		}
		//else, if there are any errors, always return 0
		MessageBox(g_hwnd, errmsg + wrnmsg, "Errors", MB_ICONERROR);
		return 0;
	}

	return 1;
}

int CSong::Export(ofstream& ou, int iotype, char* filename)
{
	//TODO: manage memory in a more dynamic way 
	unsigned char mem[65536];					//default RAM size for most 800xl/xe machines
	unsigned char buff1[65536];					//LZSS buffers for each ones of the tune parts being reconstructed
	unsigned char buff2[65536];					//they are used for parts labeled: full, intro, and loop 
	unsigned char buff3[65536];					//a LZSS export will typically make use of intro and loop only, unless specified otherwise

	//SAP-R and LZSS variables used for export binaries and/or data dumped
	int firstcount, secondcount, thirdcount, fullcount;	//4 frames counter, for complete playback, looped section, intro section, and total frames counted
	int adr_lzss_pointer = 0x3000;				//all the LZSS subtunes index will occupy this memory page
	int adr_loop_flag = 0x1C22;					//VUPlayer's address for the Loop flag
	int adr_stereo_flag = 0x1DC3;				//VUPlayer's address for the Stereo flag 
	int adr_song_speed = 0x2029;				//VUPlayer's address for setting the song speed
	int adr_region = 0x202D;					//VUPlayer's address for the region initialisation
	int adr_colour = 0x2132;					//VUPlayer's address for the rasterbar colour
	int adr_rasterbar = 0x216B;					//VUPlayer's address for the rasterbar display 
	int adr_do_play = 0x2174;					//VUPlayer's address for Play, for SAP exports bypassing the mainloop code
	int adr_rts_nop = 0x21B1;					//VUPlayer's address for JMP loop being patched to RTS NOP NOP with the SAP format
	int adr_shuffle = 0x2308;					//VUPlayer's address for the rasterbar colour shuffle (incomplete feature)
	int adr_init_sap = 0x3080;					//VUPlayer SAP initialisation hack
	int bytenum = (g_tracks4_8 == 8) ? 18 : 9;	//SAP-R bytes to copy, Stereo doubles the number
	int lzss_offset, lzss_end;

	//RMT module addresses
	int adr_module = 0x4000;					//standard RMT modules are set to $4000
	int maxadr = adr_module;

	WORD adrfrom, adrto;
	BYTE instrsaved[INSTRSNUM];
	BYTE tracksaved[TRACKSNUM];
	BOOL head_ffff = 1;							//FFFF header at the beginning of the file, it only needs to be defined once

	//create a module
	memset(mem, 0, 65536);
	maxadr = MakeModule(mem, adr_module, iotype, instrsaved, tracksaved);
	if (maxadr < 0) return 0;						//if the module could not be created, the export process is immediately aborted
	CString s;

	//first, we must dump the current module as SAP-R before LZSS conversion
	//TODO: turn this into a proper SAP-R dumper function. That way, conversions in batches could be done for multiple tunes
	if (iotype == IOTYPE_SAPR || iotype == IOTYPE_LZSS || iotype == IOTYPE_LZSS_SAP || iotype == IOTYPE_LZSS_XEX)
	{
		fullcount = firstcount = secondcount = thirdcount = 0;	//initialise them all to 0 for the first part 
		ChangeTimer((g_ntsc) ? 17 : 20);	//this helps avoiding corruption if things are running too fast
		Atari_InitRMTRoutine();	//reset the RMT routines 
		SetChannelOnOff(-1, 0);	//switch all channels off 
		SAPRDUMP = 3;	//set the SAP-r dumper initialisation flag 
		Play(MPLAY_SONG, m_followplay);	//play song from start, start before the timer changes again
		ChangeTimer(1);	//set the timer to be as fast as possible for the recording process

		while (m_play)	//the SAP-R dumper is running during that time...
		{
			if (SAPRDUMP == 2)	//ready to write data when the flag is set to 2
			{
				if (loops == 1)
				{	//first loop completed		
					SAPRDUMP = 1;								//set the flag back to dump for the looped part
					firstcount = framecount;					//from start to loop point
				}
				if (loops == 2)
				{	//second loop completed
					SAPRDUMP = 0;								//the dumper has reached its end
					secondcount = framecount - firstcount;		//from loop point to end
				}
			}
			//TODO: display progress instead of making the program look like it isn't responding 
			//it is basically writing the SAP-R data during the time the program appears frozen
			//unfortunately, I haven't figured out a good way to display the dump/compression process
			//so this is a very small annoyance, but usually it only takes about 30 seconds to export, which is definitely not the worst thing ever
		}

		ChangeTimer((g_ntsc) ? 17 : 20);			//reset the timer again, to avoid corruption from running too fast
		Stop();										//end playback now, the SAP-R data should have been dumped successfully!

		//the difference defines the length of the intro section. a size of 0 means it is virtually identical to the looped section, with few exceptions
		thirdcount = firstcount - secondcount;

		//total frames counted, from start to the end of second loop
		fullcount = framecount;
	}

	switch (iotype)
	{
		case IOTYPE_RMT:
		{
			//save the first RMT module block
			SaveBinaryBlock(ou, mem, adr_module, maxadr - 1, head_ffff);

			//the individual names are truncated by spaces and terminated by a zero
			int adrsongname = maxadr;
			s = m_songname;
			s.TrimRight();
			int lens = s.GetLength() + 1;	//including 0 after the string
			strncpy((char*)(mem + adrsongname), (LPCSTR)s, lens);
			int adrinstrnames = adrsongname + lens;
			for (int i = 0; i < INSTRSNUM; i++)
			{
				if (instrsaved[i])
				{
					s = g_Instruments.GetName(i);
					s.TrimRight();
					lens = s.GetLength() + 1;	//including 0 after the string
					strncpy((char*)(mem + adrinstrnames), s, lens);
					adrinstrnames += lens;
				}
			}
			//and now, save the 2nd block
			SaveBinaryBlock(ou, mem, adrsongname, adrinstrnames - 1, 0);
		}
		break;

		case IOTYPE_RMTSTRIPPED:
		{
			//create a variant for SFX (ie. including unused instruments and tracks)
			BYTE instrsaved2[INSTRSNUM];
			BYTE tracksaved2[TRACKSNUM];
			int maxadr2 = MakeModule(mem, adr_module, IOTYPE_RMT, instrsaved2, tracksaved2);
			if (maxadr2 < 0) return 0;	//if the module could not be created

			//Dialog for specifying the address of the RMT module in memory
			CExpRMTDlg dlg;
			dlg.m_len = maxadr - adr_module;
			dlg.m_len2 = maxadr2 - adr_module;
			dlg.m_adr = g_rmtstripped_adr_module;	//global, so that it remains the same on repeated export
			dlg.m_sfx = g_rmtstripped_sfx;
			dlg.m_gvf = g_rmtstripped_gvf;
			dlg.m_nos = g_rmtstripped_nos;
			dlg.m_song = this;
			dlg.m_filename = filename;
			dlg.m_instrsaved = instrsaved;
			dlg.m_instrsaved2 = instrsaved2;
			dlg.m_tracksaved = tracksaved;
			dlg.m_tracksaved2 = tracksaved2;
			if (dlg.DoModal() != IDOK) return 0;

			g_rmtstripped_adr_module = adr_module = dlg.m_adr;
			g_rmtstripped_sfx = dlg.m_sfx;
			g_rmtstripped_gvf = dlg.m_gvf;
			g_rmtstripped_nos = dlg.m_nos;

			//regenerates the module according to the entered address "adr"
			memset(mem, 0, 65536);
			if (!g_rmtstripped_sfx) //either without unused instruments and tracks
				maxadr = MakeModule(mem, adr_module, iotype, instrsaved, tracksaved);
			else					//or with them
				maxadr = MakeModule(mem, adr_module, IOTYPE_RMT, instrsaved, tracksaved);
			if (maxadr < 0) return 0; //if the module could not be created
			//and now save the RMT module block
			SaveBinaryBlock(ou, mem, adr_module, maxadr - 1, head_ffff);
		}
		break;

		//TODO: cleanup
		case IOTYPE_ASM:
		{
			CExpASMDlg dlg;
			CString s, snot;
			int maxova = 16;				//maximal amount of data per line
			dlg.m_labelsprefix = g_expasmlabelprefix;
			if (dlg.DoModal() != IDOK) return 0;
			//
			g_expasmlabelprefix = dlg.m_labelsprefix;
			CString lprefix = g_expasmlabelprefix;
			BYTE tracks[TRACKSNUM];
			memset(tracks, 0, TRACKSNUM); //init
			MarkTF_USED(tracks);
			ou << ";ASM notation source";
			ou << EOL << "XXX\tequ $FF\t;empty note value";
			if (!lprefix.IsEmpty())
			{
				s.Format("%s_data", lprefix);
				ou << EOL << s;
			}
			//
			if (dlg.m_type == 1)
			{
				//tracks
				int t, i, j, not, dur, ins;
				for (t = 0; t < TRACKSNUM; t++)
				{
					if (!(tracks[t] & TF_USED)) continue;
					s.Format(";Track $%02X", t);
					ou << EOL << s;
					if (!lprefix.IsEmpty())
					{
						s.Format("%s_track%02X", lprefix, t);
						ou << EOL << s;
					}
					TTrack& origtt = *g_Tracks.GetTrack(t);
					TTrack tt;	//temporary track
					memcpy((void*)&tt, (void*)&origtt, sizeof(TTrack)); //make a copy of origtt to tt
					g_Tracks.TrackExpandLoop(&tt); //expands tt due to GO loops
					int ova = maxova;
					for (i = 0; i < tt.len; i++)
					{
						if (ova >= maxova)
						{
							ou << EOL << "\tdta ";
							ova = 0;
						}
						not= tt.note[i];
						if (not>= 0)
						{
							ins = tt.instr[i];
							if (dlg.m_notes == 1) //notes
								not= g_Instruments.GetNote(ins, not);
							else				//frequencies
								not= g_Instruments.GetFrequency(ins, not);
						}
						if (not>= 0) snot.Format("$%02X", not); else snot = "XXX";
						for (dur = 1; i + dur < tt.len && tt.note[i + dur] < 0; dur++);
						if (dlg.m_durations == 1)
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
							if (dlg.m_durations == 2)
							{
								if (ova > 0) ou << ",";
								ou << snot;
								ou << "," << dur;
								ova += 2;
							}
							else
								if (dlg.m_durations == 3)
								{
									if (ova > 0) ou << ",";
									ou << dur << ",";
									ou << snot;
									ova += 2;
								}
						i += dur - 1;
					}
				}
			}
			else
				if (dlg.m_type == 2)
				{
					//song columns
					int clm;
					for (clm = 0; clm < g_tracks4_8; clm++)
					{
						BYTE finished[SONGLEN];
						memset(finished, 0, SONGLEN);
						int sline = 0;
						static char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };
						s.Format(";Song column %s", cnames[clm]);
						ou << EOL << s;
						if (!lprefix.IsEmpty())
						{
							s.Format("%s_column%s", lprefix, cnames[clm]);
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
							int trackslen = g_Tracks.m_maxtracklen;
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
								if (!lprefix.IsEmpty())
								{
									s.Format("%s_column%s_line%02X", lprefix, cnames[clm], sline);
									ou << EOL << s;
								}
								if (dlg.m_durations == 1)
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
									if (dlg.m_durations == 2)
									{
										ou << EOL << "\tdta XXX,";
										ou << trackslen;
										ova += 2;
									}
									else
										if (dlg.m_durations == 3)
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
							if (!lprefix.IsEmpty())
							{
								s.Format("%s_column%s_line%02X", lprefix, cnames[clm], sline);
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
									if (dlg.m_notes == 1) //notes
										not= g_Instruments.GetNote(ins, not);
									else				//frequencies
										not= g_Instruments.GetFrequency(ins, not);
								}
								if (not>= 0) snot.Format("$%02X", not); else snot = "XXX";
								for (dur = 1; i + dur < trackslen && tt.note[i + dur] < 0; dur++);
								if (dlg.m_durations == 1)
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
									if (dlg.m_durations == 2)
									{
										if (ova > 0) ou << ",";
										ou << snot;
										ou << "," << dur;
										ova += 2;
									}
									else
										if (dlg.m_durations == 3)
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
		}
		break;

		//TODO: clean this up better, and allow multiple export choices later
		//for now, this will simply be a full raw dump of all the bytes dumped at once, which is a full tune playback before loop
		case IOTYPE_SAPR:
		{
			CExpSAPDlg dlg;
			s = m_songname;
			s.TrimRight();			//cuts spaces after the name
			s.Replace('"', '\'');	//replaces quotation marks with an apostrophe
			dlg.m_name = s;

			dlg.m_author = "???";

			CTime time = CTime::GetCurrentTime();
			dlg.m_date = time.Format("%d/%m/%Y");

			if (dlg.DoModal() != IDOK)
			{	//clear the SAP-R dumper memory and reset RMT routines
				g_Pokey.DoneSAPR();
				return 0;
			}

			ou << "SAP" << EOL;

			s = dlg.m_author;
			s.TrimRight();
			s.Replace('"', '\'');

			ou << "AUTHOR \"" << s << "\"" << EOL;

			s = dlg.m_name;
			s.TrimRight();
			s.Replace('"', '\'');

			ou << "NAME \"" << s << " (" << firstcount << " frames)" << "\"" << EOL;	//display the total frames recorded

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

			if (m_instrspeed > 1)
			{
				ou << "FASTPLAY ";
				switch (m_instrspeed)
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
			//a double EOL is necessary for making the SAP-R export functional
			ou << EOL;

			//write the SAP-R stream to the output file defined in the path dialog with the data specified above
			g_Pokey.WriteFileToSAPR(ou, firstcount, 0);

			//clear the memory and reset the dumper to its initial setup for the next time it will be called
			g_Pokey.DoneSAPR();
		}
		break;

		//TODO: allow more LZSS choices, for now it will simply write the full tune with no loop
		case IOTYPE_LZSS:
		{
			//Just in case
			memset(buff1, 0, 65536);

			//Now, create LZSS files using the SAP-R dump created earlier
			ifstream in;

			//full tune playback up to its loop point
			int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
			if (full)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff1, sizeof(buff1))))
					full = (int)in.gcount();
				if (full < 16) full = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			g_Pokey.DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

			ou.write((char*)buff1, full);	//write the buffer 1 contents to the export file
		}
		break;

		//TODO: add more options through addional parameters from dialog box
		case IOTYPE_LZSS_SAP:
		{
			//Just in case
			memset(buff1, 0, 65536);
			memset(buff2, 0, 65536);
			memset(buff3, 0, 65536);

			//Now, create LZSS files using the SAP-R dump created earlier
			ifstream in;

			//full tune playback up to its loop point
			int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
			if (full)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff1, sizeof(buff1))))
					full = (int)in.gcount();
				if (full < 16) full = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			//intro section playback, up to the start of the detected loop point
			int intro = LZSS_SAP((char*)SAPRSTREAM, thirdcount * bytenum);
			if (intro)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff2, sizeof(buff2))))
					intro = (int)in.gcount();
				if (intro < 16) intro = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			//looped section playback, this part is virtually seamless to itself
			int loop = LZSS_SAP((char*)SAPRSTREAM + (firstcount * bytenum), secondcount * bytenum);
			if (loop)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff3, sizeof(buff3))))
					loop = (int)in.gcount();
				if (loop < 16) loop = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			g_Pokey.DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

			//some additional variables that will be used below
			adr_module = 0x3100;												//all the LZSS data will be written starting from this address
			lzss_offset = (intro) ? adr_module + intro : adr_module + full;		//calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
			lzss_end = lzss_offset + loop;										//this sets the address that defines where the data stream has reached its end

			//if the size is too big, abort the process and show an error message
			if (lzss_end > 0xBFFF)
			{
				MessageBox(g_hwnd,
					"Error, LZSS data is too big to fit in memory!\n\n"
					"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
					"Error, Buffer Overflow!", MB_ICONERROR);
				return 0;
			}

			int r;
			r = LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, adrfrom, adrto);
			if (!r)
			{
				MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
				return 0;
			}

			CExpSAPDlg dlg;

			s = m_songname;
			s.TrimRight();			//cuts spaces after the name
			s.Replace('"', '\'');	//replaces quotation marks with an apostrophe
			dlg.m_name = s;

			dlg.m_author = "???";
			GetSubsongParts(dlg.m_subsongs);

			CTime time = CTime::GetCurrentTime();
			dlg.m_date = time.Format("%d/%m/%Y");

			if (dlg.DoModal() != IDOK)
				return 0;

			ou << "SAP" << EOL;

			s = dlg.m_author;
			s.TrimRight();
			s.Replace('"', '\'');

			ou << "AUTHOR \"" << s << "\"" << EOL;

			s = dlg.m_name;
			s.TrimRight();
			s.Replace('"', '\'');

			ou << "NAME \"" << s << " (" << firstcount << " frames)" << "\"" << EOL;	//display the total frames recorded

			s = dlg.m_date;
			s.TrimRight();
			s.Replace('"', '\'');

			ou << "DATE \"" << s << "\"" << EOL;

			s = dlg.m_subsongs + " ";		//space after the last character due to parsing

			s.MakeUpper();
			int subsongs = 0;
			BYTE subpos[MAXSUBSONGS];
			subpos[0] = 0;					//start at songline 0 by default
			BYTE a, n, isn = 0;

			//parses the "Subsongs" line from the ExportSAP dialog
			for (int i = 0; i < s.GetLength(); i++)
			{
				a = s.GetAt(i);
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
			s.Format("INIT %04X", adr_init_sap);
			ou << s << EOL;
			s.Format("PLAYER %04X", adr_do_play);
			ou << s << EOL;

			if (g_tracks4_8 > 4)
			{	//stereo module
				ou << "STEREO" << EOL;
			}

			if (g_ntsc)
			{	//NTSC module
				ou << "NTSC" << EOL;
			}

			if (m_instrspeed > 1)
			{
				ou << "FASTPLAY ";
				switch (m_instrspeed)
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

			//a double EOL is necessary for making the SAP export functional
			ou << EOL;

			//patch: change a JMP [label] to a RTS with 2 NOPs
			unsigned char saprtsnop[3] = { 0x60,0xEA,0xEA };
			for (int i = 0; i < 3; i++) mem[adr_rts_nop + i] = saprtsnop[i];

			//patch: change a $00 to $FF to force the LOOP flag to be infinite
			mem[adr_loop_flag] = 0xFF;

			//SAP initialisation patch, running from address 0x3080 in Atari executable 
			unsigned char sapbytes[14] =
			{
				0x8D,0xE7,0x22,		// STA SongIdx
				0xA2,0x00,			// LDX #0
				0x8E,0x93,0x1B,		// STX is_fadeing_out
				0x8E,0x03,0x1C,		// STX stop_on_fade_end
				0x4C,0x39,0x1C		// JMP SetNewSongPtrsLoopsOnly
			};
			for (int i = 0; i < 14; i++) mem[adr_init_sap + i] = sapbytes[i];

			mem[adr_song_speed] = m_instrspeed;							//song speed
			mem[adr_stereo_flag] = (g_tracks4_8 > 4) ? 0xFF : 0x00;		//is the song stereo?

			//reconstruct the export binary 
			SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	//LZSS Driver, and some free bytes for later if needed
			SaveBinaryBlock(ou, mem, 0x2000, 0x27FF, 0);	//VUPlayer only

			//songstart pointers
			mem[0x3000] = adr_module >> 8;
			mem[0x3001] = lzss_offset >> 8;
			mem[0x3002] = adr_module & 0xFF;
			mem[0x3003] = lzss_offset & 0xFF;

			//songend pointers
			mem[0x3004] = lzss_offset >> 8;
			mem[0x3005] = lzss_end >> 8;
			mem[0x3006] = lzss_offset & 0xFF;
			mem[0x3007] = lzss_end & 0xFF;

			if (intro)
				for (int i = 0; i < intro; i++) { mem[adr_module + i] = buff2[i]; }
			else
				for (int i = 0; i < full; i++) { mem[adr_module + i] = buff1[i]; }
			for (int i = 0; i < loop; i++) { mem[lzss_offset + i] = buff3[i]; }

			//overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
			SaveBinaryBlock(ou, mem, adr_lzss_pointer, lzss_end, 0);
		}
		break;

		//TODO: add more options through dialog box parameters
		case IOTYPE_LZSS_XEX:
		{
			//Just in case
			memset(buff1, 0, 65536);
			memset(buff2, 0, 65536);
			memset(buff3, 0, 65536);

			//Now, create LZSS files using the SAP-R dump created earlier
			ifstream in;

			//full tune playback up to its loop point
			int full = LZSS_SAP((char*)SAPRSTREAM, firstcount * bytenum);
			if (full)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff1, sizeof(buff1))))
					full = (int)in.gcount();
				if (full < 16) full = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			//intro section playback, up to the start of the detected loop point
			int intro = LZSS_SAP((char*)SAPRSTREAM, thirdcount * bytenum);
			if (intro)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff2, sizeof(buff2))))
					intro = (int)in.gcount();
				if (intro < 16) intro = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			//looped section playback, this part is virtually seamless to itself
			int loop = LZSS_SAP((char*)SAPRSTREAM + (firstcount * bytenum), secondcount * bytenum);
			if (loop)
			{
				//load tmp.lzss in destination buffer 
				in.open(g_prgpath + "tmp.lzss", ifstream::binary);
				if (!(in.read((char*)buff3, sizeof(buff3))))
					loop = (int)in.gcount();
				if (loop < 16) loop = 1;
				in.close(); in.clear();
				DeleteFile(g_prgpath + "tmp.lzss");
			}

			g_Pokey.DoneSAPR();	//clear the SAP-R dumper memory and reset RMT routines

			//some additional variables that will be used below
			adr_module = 0x3100;												//all the LZSS data will be written starting from this address
			lzss_offset = (intro) ? adr_module + intro : adr_module + full;		//calculate the offset for the export process between the subtune parts, at the moment only 1 tune at the time can be exported
			lzss_end = lzss_offset + loop;										//this sets the address that defines where the data stream has reached its end

			//if the size is too big, abort the process and show an error message
			if (lzss_end > 0xBFFF)
			{
				MessageBox(g_hwnd,
					"Error, LZSS data is too big to fit in memory!\n\n"
					"High Instrument Speed and/or Stereo greatly inflate memory usage, even when data is compressed",
					"Error, Buffer Overflow!", MB_ICONERROR);
				return 0;
			}

			int r;
			r = LoadBinaryFile((char*)((LPCSTR)(g_prgpath + "RMT Binaries/VUPlayer (LZSS Export).obx")), mem, adrfrom, adrto);
			if (!r)
			{
				MessageBox(g_hwnd, "Fatal error with RMT LZSS system routines.\nCouldn't load 'RMT Binaries/VUPlayer (LZSS Export).obx'.", "Export aborted", MB_ICONERROR);
				return 0;
			}

			CExpMSXDlg dlg;
			s = m_songname;
			s.TrimRight();
			CTime time = CTime::GetCurrentTime();
			if (g_rmtmsxtext != "")
			{
				dlg.m_txt = g_rmtmsxtext;	//same from last time, making repeated exports faster
			}
			else
			{
				dlg.m_txt = s + "\x0d\x0a";
				if (g_tracks4_8 > 4) dlg.m_txt += "STEREO";
				dlg.m_txt += "\x0d\x0a" + time.Format("%d/%m/%Y");
				dlg.m_txt += "\x0d\x0a";
				dlg.m_txt += "Author: (press SHIFT key)\x0d\x0a";
				dlg.m_txt += "Author: ???";
			}
			s = "Playback speed will be adjusted to ";
			s += g_ntsc ? "60" : "50";
			s += "Hz on both PAL and NTSC systems.";
			dlg.m_speedinfo = s;

			if (dlg.DoModal() != IDOK)
			{
				return 0;
			}
			g_rmtmsxtext = dlg.m_txt;
			g_rmtmsxtext.Replace("\x0d\x0d", "\x0d");	//13, 13 => 13

			//this block of code will handle all the user input text that will be inserted in the binary during the export process
			memset(mem + 0x2EBC, 32, 40 * 5);	//5 lines of 40 characters at the user text address
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

			memset(mem + 0x2C0B, 32, 28);	//28 characters on the top line, next to the Region and VBI speed
			char framesdisplay[28] = { 0 };
			sprintf(framesdisplay, "(%i frames)", firstcount);	//total recorded frames
			for (int i = 0; i < 28; i++)
			{
				mem[0x2C0B + i] = framesdisplay[i];
			}
			StrToAtariVideo((char*)mem + 0x2C0B, 28);

			//I know the binary I have is currently set to NTSC, so I'll just convert to PAL and keep this going for now...
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
				for (int i = 0; i < 18; i++) mem[adr_region + i] = regionbytes[i];
			}

			//additional patches from the Export Dialog...
			mem[adr_song_speed] = m_instrspeed;										//song speed
			mem[adr_rasterbar] = (dlg.m_meter) ? 0x80 : 0x00;						//display the rasterbar for CPU level
			mem[adr_colour] = dlg.m_metercolor;										//rasterbar colour 
			mem[adr_shuffle] = 0x00;	// = (dlg.m_msx_shuffle) ? 0x10 : 0x00;		//rasterbar colour shuffle, incomplete feature so it is disabled
			mem[adr_stereo_flag] = (g_tracks4_8 > 4) ? 0xFF : 0x00;					//is the song stereo?
			if (!dlg.m_region_auto)													//automatically adjust speed between regions?
				for (int i = 0; i < 4; i++) mem[adr_region + 6 + i] = 0xEA;			//set the 4 bytes to NOPs to disable it

			//reconstruct the export binary 
			SaveBinaryBlock(ou, mem, 0x1900, 0x1EFF, 1);	//LZSS Driver, and some free bytes for later if needed
			SaveBinaryBlock(ou, mem, 0x2000, 0x2FFF, 0);	//VUPlayer + Font + Data + Display Lists

			//set the run address to VUPlayer 
			mem[0x2e0] = 0x2000 & 0xff;
			mem[0x2e1] = 0x2000 >> 8;
			SaveBinaryBlock(ou, mem, 0x2e0, 0x2e1, 0);

			//songstart pointers
			mem[0x3000] = adr_module >> 8;
			mem[0x3001] = lzss_offset >> 8;
			mem[0x3002] = adr_module & 0xFF;
			mem[0x3003] = lzss_offset & 0xFF;

			//songend pointers
			mem[0x3004] = lzss_offset >> 8;
			mem[0x3005] = lzss_end >> 8;
			mem[0x3006] = lzss_offset & 0xFF;
			mem[0x3007] = lzss_end & 0xFF;

			if (intro)
				for (int i = 0; i < intro; i++) { mem[adr_module + i] = buff2[i]; }
			else
				for (int i = 0; i < full; i++) { mem[adr_module + i] = buff1[i]; }
			for (int i = 0; i < loop; i++) { mem[lzss_offset + i] = buff3[i]; }

			//overwrite the LZSS data region with both the pointers for subtunes index, and the actual LZSS streams until the end of file
			SaveBinaryBlock(ou, mem, adr_lzss_pointer, lzss_end, 0);
		}
		break;

		default:
			return 0;
	}
	return 1;
}


int CSong::LoadRMT(ifstream& in)
{
	unsigned char mem[65536];
	memset(mem, 0, 65536);
	WORD bfrom, bto;
	WORD bto_mainblock;

	BYTE instrloaded[INSTRSNUM];
	BYTE trackloaded[TRACKSNUM];

	int len, i, j, k;

	//RMT header is the first main block of an RMT song

	len = LoadBinaryBlock(in, mem, bfrom, bto);

	if (len > 0)
	{
		int r = DecodeModule(mem, bfrom, bto + 1, instrloaded, trackloaded);
		if (!r)
		{
			MessageBox(g_hwnd, "Bad RMT data format or old tracker version.", "Open error", MB_ICONERROR);
			return 0;
		}
		//the main block of the module is OK => take its boot address
		g_rmtstripped_adr_module = bfrom;
		bto_mainblock = bto;
	}
	else
	{
		MessageBox(g_hwnd, "Corrupted file or unsupported format version.", "Open error", MB_ICONERROR);
		return 0;	//did not retrieve any data in the first block
	}

	//RMT - now read the second block with names)
	len = LoadBinaryBlock(in, mem, bfrom, bto);
	if (len < 1)
	{
		CString s;
		s.Format("This file appears to be a stripped RMT module.\nThe song and instruments names are missing.\n\nMemory addresses: $%04X - $%04X.", g_rmtstripped_adr_module, bto_mainblock);
		MessageBox(g_hwnd, (LPCTSTR)s, "Info", MB_ICONINFORMATION);
		return 1;
	}

	char a;
	for (j = 0; j < SONG_NAME_MAX_LEN && (a = mem[bfrom + j]); j++)
		m_songname[j] = a;

	for (k = j; k < SONG_NAME_MAX_LEN; k++) m_songname[k] = ' '; //fill in the gaps

	int adrinames = bfrom + j + 1; //+1 that's the zero behind the name
	for (i = 0; i < INSTRSNUM; i++)
	{
		if (instrloaded[i])
		{
			for (j = 0; j < INSTRUMENT_NAME_MAX_LEN && (a = mem[adrinames + j]); j++)
				g_Instruments.m_instr[i].name[j] = a;

			for (k = j; k < INSTRUMENT_NAME_MAX_LEN; k++) g_Instruments.m_instr[i].name[k] = ' '; //fill in the gaps
			adrinames += j + 1; //+1 is zero behind the name
		}
	}
	return 1;
}


BOOL CSong::ComposeRMTFEATstring(CString& dest, char* filename, BYTE* instrsaved, BYTE* tracksaved, BOOL sfx, BOOL gvf, BOOL nos)
{

#define DEST(var,str)	s.Format("%s\t\tequ %i\t\t;(%i times)\n",str,(var>0),var); dest+=s;

	dest.Format(";* --------BEGIN--------\n;* %s\n", filename);
	//
	int usedcmd[8] = { 0,0,0,0,0,0,0,0 };
	int usedcmd7volumeonly = 0;
	int usedcmd7volumeonlyongx[8] = { 0,0,0,0,0,0,0,0 };
	int usedcmd7setnote = 0;
	int usedportamento = 0;
	int usedfilter = 0;
	int usedfilterongx[8] = { 0,0,0,0,0,0,0,0 };
	int usedbass16 = 0;
	int usedbass16ongx[8] = { 0,0,0,0,0,0,0,0 };
	int usedtabtype = 0;
	int usedtabmode = 0;
	int usedtablego = 0;
	int usedaudctlmanualset = 0;
	int usedvolumemin = 0;
	int usedeffectvibrato = 0;
	int usedeffectfshift = 0;
	int speedchanges = 0;
	int i, j, g, tr;
	CString s;

	//analysis of which instruments are used on which channels and if there is a change in speed
	int instrongx[INSTRSNUM][SONGTRACKS];
	memset(&instrongx, 0, INSTRSNUM * SONGTRACKS * sizeof(instrongx[0][0]));
	//test the song
	for (int sl = 0; sl < SONGLEN; sl++)
	{
		if (m_songgo[sl] >= 0) continue;	//goto line
		for (g = 0; g < g_tracks4_8; g++)
		{
			tr = m_song[sl][g];
			if (tr < 0 || tr >= TRACKSNUM) continue;
			TTrack* tt = g_Tracks.GetTrack(tr);
			for (i = 0; i < tt->len; i++)
			{
				int inum = tt->instr[i];
				if (inum >= 0 && inum < INSTRSNUM) instrongx[inum][g]++;
				int chsp = tt->speed[i];
				if (chsp >= 0) speedchanges++;
			}
		}
	}

	//analyse the individual instruments and what they use
	for (i = 0; i < INSTRSNUM; i++)
	{
		if (instrsaved[i])
		{
			TInstrument& ai = g_Instruments.m_instr[i];
			//commands
			for (j = 0; j <= ai.parameters[PAR_ENV_LENGTH]; j++)
			{
				int cmd = ai.envelope[j][ENV_COMMAND] & 0x07;
				usedcmd[cmd]++;
				if (cmd == 7)
				{
					if (ai.envelope[j][ENV_X] == 0x08 && ai.envelope[j][ENV_Y] == 0x00)
					{
						usedcmd7volumeonly++;
						for (g = 0; g < g_tracks4_8; g++) { if (instrongx[i][g]) usedcmd7volumeonlyongx[g]++; }
					}
					else
						usedcmd7setnote++;
				}
				//portamento
				if (ai.envelope[j][ENV_PORTAMENTO]) usedportamento++;
				//filter
				if (ai.envelope[j][ENV_FILTER])
				{
					usedfilter++;
					for (g = 0; g < g_tracks4_8; g++) { if (instrongx[i][g]) usedfilterongx[g]++; }
				}

				//bass16
				if (ai.envelope[j][ENV_DISTORTION] == 6)
				{
					usedbass16++;
					for (g = 0; g < g_tracks4_8; g++) { if (instrongx[i][g]) usedbass16ongx[g]++; }
				}

			}
			//table type
			if (ai.parameters[PAR_TBL_TYPE]) usedtabtype++;
			//table mode
			if (ai.parameters[PAR_TBL_MODE]) usedtabmode++;
			//table go
			if (ai.parameters[PAR_TBL_GOTO]) usedtablego++;	//non-zero table go
			//audctl manual set
			if (ai.parameters[PAR_AUDCTL_15KHZ]
				|| ai.parameters[PAR_AUDCTL_HPF_CH2]
				|| ai.parameters[PAR_AUDCTL_HPF_CH1]
				|| ai.parameters[PAR_AUDCTL_JOIN_3_4]
				|| ai.parameters[PAR_AUDCTL_JOIN_1_2]
				|| ai.parameters[PAR_AUDCTL_179_CH3]
				|| ai.parameters[PAR_AUDCTL_179_CH1]
				|| ai.parameters[PAR_AUDCTL_POLY9]) usedaudctlmanualset++;
			//volume mininum
			if (ai.parameters[PAR_VOL_MIN]) usedvolumemin++;
			//effect vibrato and fshift
			if (ai.parameters[PAR_DELAY]) //only when the effect delay is nonzero
			{
				if (ai.parameters[PAR_VIBRATO]) usedeffectvibrato++;
				if (ai.parameters[PAR_FREQ_SHIFT]) usedeffectfshift++;
			}
		}
	}

	//generate strings

	s.Format("FEAT_SFX\t\tequ %u\n", sfx); dest += s;

	s.Format("FEAT_GLOBALVOLUMEFADE\tequ %u\t\t;RMTGLOBALVOLUMEFADE variable\n", gvf); dest += s;

	s.Format("FEAT_NOSTARTINGSONGLINE\tequ %u\n", nos); dest += s;

	s.Format("FEAT_INSTRSPEED\t\tequ %i\n", m_instrspeed); dest += s;

	s.Format("FEAT_CONSTANTSPEED\t\tequ %i\t\t;(%i times)\n", (speedchanges == 0) ? m_mainspeed : 0, speedchanges); dest += s;

	//commands 1-6
	for (i = 1; i <= 6; i++)
	{
		s.Format("FEAT_COMMAND%i\t\tequ %i\t\t;(%i times)\n", i, (usedcmd[i] > 0), usedcmd[i]);
		dest += s;
	}

	//command 7
	DEST(usedcmd7setnote, "FEAT_COMMAND7SETNOTE");
	DEST(usedcmd7volumeonly, "FEAT_COMMAND7VOLUMEONLY");

	//
	DEST(usedportamento, "FEAT_PORTAMENTO");
	//
	DEST(usedfilter, "FEAT_FILTER");
	DEST(usedfilterongx[0], "FEAT_FILTERG0L");
	DEST(usedfilterongx[1], "FEAT_FILTERG1L");
	DEST(usedfilterongx[0 + 4], "FEAT_FILTERG0R");
	DEST(usedfilterongx[1 + 4], "FEAT_FILTERG1R");
	//
	DEST(usedbass16, "FEAT_BASS16");
	DEST(usedbass16ongx[1], "FEAT_BASS16G1L");
	DEST(usedbass16ongx[3], "FEAT_BASS16G3L");
	DEST(usedbass16ongx[1 + 4], "FEAT_BASS16G1R");
	DEST(usedbass16ongx[3 + 4], "FEAT_BASS16G3R");
	//
	DEST(usedcmd7volumeonlyongx[0], "FEAT_VOLUMEONLYG0L");
	DEST(usedcmd7volumeonlyongx[2], "FEAT_VOLUMEONLYG2L");
	DEST(usedcmd7volumeonlyongx[3], "FEAT_VOLUMEONLYG3L");
	DEST(usedcmd7volumeonlyongx[0 + 4], "FEAT_VOLUMEONLYG0R");
	DEST(usedcmd7volumeonlyongx[2 + 4], "FEAT_VOLUMEONLYG2R");
	DEST(usedcmd7volumeonlyongx[3 + 4], "FEAT_VOLUMEONLYG3R");
	//
	DEST(usedtabtype, "FEAT_TABLETYPE");
	DEST(usedtabmode, "FEAT_TABLEMODE");
	DEST(usedtablego, "FEAT_TABLEGO");
	DEST(usedaudctlmanualset, "FEAT_AUDCTLMANUALSET");
	DEST(usedvolumemin, "FEAT_VOLUMEMIN");

	DEST(usedeffectvibrato, "FEAT_EFFECTVIBRATO");
	DEST(usedeffectfshift, "FEAT_EFFECTFSHIFT");

	dest += ";* --------END--------\n";
	dest.Replace("\n", "\x0d\x0a");
	return 1;
}