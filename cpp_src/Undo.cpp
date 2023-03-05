#include "stdafx.h"
#include "resource.h"
#include "General.h"

#include "Undo.h"
#include "Song.h"
#include "Clipboard.h"

#include "global.h"


extern CSong g_Song;
extern CInstruments g_Instruments;
extern CTrackClipboard g_TrackClipboard;


CUndo::CUndo()
{
	for (int i = 0; i < MAXUNDO; i++) m_uar[i] = NULL;
}

CUndo::~CUndo()
{
	for (int i = 0; i < MAXUNDO; i++) DeleteEvent(i);
}

void CUndo::Init()
{
	Clear();
}

void CUndo::Clear()
{
	m_head = 0;
	m_tail = 0;
	m_headmax = 0;
	m_undosteps = m_redosteps = 0;
	for (int i = 0; i < MAXUNDO; i++) DeleteEvent(i);
}

char CUndo::DeleteEvent(int i)
{
	TUndoEvent* ue = m_uar[i];
	if (!ue) return 1;
	char sep = ue->separator; //storage for return
	if (ue->cursor) delete[] ue->cursor;
	if (ue->pos) delete[] ue->pos;
	if (ue->data) delete[] ue->data;
	delete ue;
	m_uar[i] = NULL;
	return sep;
}

BOOL CUndo::Undo()
{
	if (m_head == m_tail)	return 0; //nothing to keep

	g_Song.Stop();

	int prev;
	char sep;
	do
	{
		m_head = (m_head + MAXUNDO - 1) % MAXUNDO;
		PerformEvent(m_head);
		if (m_head == m_tail) break;
		prev = (m_head + MAXUNDO - 1) % MAXUNDO;
		sep = m_uar[prev]->separator;
	} while (sep == -1);

	m_undosteps--;
	m_redosteps++;

	return 1;
}

BOOL CUndo::Redo()
{
	if (m_head == m_headmax) return 0; //nothing to return

	g_Song.Stop();

	char sep;
	do
	{
		sep = PerformEvent(m_head);
		m_head = (m_head + 1) % MAXUNDO;
	} while (sep == -1);

	m_redosteps--;
	m_undosteps++;

	return 1;
}

void CUndo::InsertEvent(TUndoEvent* ue)
{
	if (!g_changes)
	{
		g_changes = 1;	//there has been some change
		g_Song.SetRMTTitle();
	}
	//add cursor
	ue->part = g_activepart;
	ue->cursor = g_Song.GetUECursor(g_activepart);
	if (m_uar[m_head]) DeleteEvent(m_head);
	m_uar[m_head] = ue;
	//is there an event already?
	if (m_head != m_tail)
	{
		TUndoEvent* le = m_uar[(m_head + MAXUNDO - 1) % MAXUNDO];
		if (ue->part == le->part
			&& ue->type == le->type
			&& !le->separator
			&& !ue->separator
			&& g_Song.UECursorIsEqual(ue->cursor, le->cursor, ue->part)
			&& PosIsEqual(ue->pos, le->pos, ue->type)
			)
		{
			//the last event is at the same cursor position and with the same data
			DeleteEvent(m_head); //erases it from memory
			//and will not count it among undo events, just end the maximum undo
			m_headmax = m_head;
			m_redosteps = 0;
			return;
		}
	}
	//
	if (ue->separator != -1) m_undosteps++; //only complete events are included
	m_head = (m_head + 1) % MAXUNDO;
	if ((m_undosteps > UNDOSTEPS)
		|| ((m_head + 1) % MAXUNDO == m_tail))
	{
		char sep;
		do
		{
			sep = DeleteEvent(m_tail);
			m_tail = (m_tail + 1) % MAXUNDO;
		} while (sep == -1);
		m_undosteps--;
	}
	m_headmax = m_head;
	m_redosteps = 0;
}

void CUndo::DropLast()
{
	if (m_head == m_tail) return;
	m_head = (m_head + MAXUNDO - 1) % MAXUNDO;
	DeleteEvent(m_head);
	m_undosteps--;		//will count this step
}

void CUndo::Separator(int sep)
{
	TUndoEvent* le;
	le = m_uar[(m_head + MAXUNDO - 1) % MAXUNDO];
	if (!le) return;
	if (sep < 0 && le->separator >= 0) m_undosteps--; //the number of undo counted in InsertEvent
	le->separator = sep;
}

void CUndo::ChangeTrack(int tracknum, int trackline, int type, char separator)
{
	if (!g_Tracks.IsValidTrack(tracknum) || !g_Tracks.IsValidLine(trackline)) return;

	TTrack* tr = g_Tracks.GetTrack(tracknum);

	// An event with the original status at a different place
	TUndoEvent* ue = new TUndoEvent;
	ue->type = type;
	ue->pos = new int[2];
	ue->pos[0] = tracknum;
	ue->pos[1] = trackline;
	ue->separator = separator;
	int* data;

	switch (type)
	{
		case UETYPE_NOTEINSTRVOL:
			data = new int[3];
			data[0] = tr->note[trackline];
			data[1] = tr->instr[trackline];
			data[2] = tr->volume[trackline];
			break;

		case UETYPE_NOTEINSTRVOLSPEED:
			data = new int[4];
			data[0] = tr->note[trackline];
			data[1] = tr->instr[trackline];
			data[2] = tr->volume[trackline];
			data[3] = tr->speed[trackline];
			break;

		case UETYPE_SPEED:
			data = new int[1];
			data[0] = tr->speed[trackline];
			break;

		case UETYPE_LENGO:
			data = new int[2];
			data[0] = tr->len;
			data[1] = tr->go;
			break;

		case UETYPE_TRACKDATA: // Whole track
			data = (int*)new TTrack;
			memcpy((void*)data, (void*)tr, sizeof(TTrack));
			break;

		case UETYPE_TRACKSALL: // All tracks
			data = (int*)new TTracksAll;
			g_Tracks.GetTracksAll((TTracksAll*)data);
			break;

		default:
			MessageBox(g_hwnd, "CUndo::ChangeTrack BAD!", "Internal error", MB_ICONERROR);
			data = NULL;
	}

	ue->data = (void*)data;
	InsertEvent(ue);
}

void CUndo::ChangeSong(int songline, int trackcol, int type, char separator)
{
	if (songline < 0 || trackcol < 0) return;

	TSong* song;

	// An event with the original status at a different place
	TUndoEvent* ue = new TUndoEvent;
	ue->type = type;
	ue->pos = new int[2];
	ue->pos[0] = songline;
	ue->pos[1] = trackcol;
	ue->separator = separator;
	int* data;

	switch (type)
	{
	case UETYPE_SONGTRACK:
		data = new int[1];
		data[0] = g_Song.SongGetTrack(songline, trackcol);
		break;

	case UETYPE_SONGGO:
		data = new int[1];
		data[0] = g_Song.SongGetGo(songline);
		break;

	case UETYPE_SONGDATA: // Whole song
		song = new TSong;
		memcpy((void*)song->song, (void*)g_Song.GetSong(), sizeof(song->song));
		memcpy((void*)song->songgo, (void*)g_Song.GetSongGo(), sizeof(song->songgo));
		data = (int*)song;
		break;

	default:
		MessageBox(g_hwnd, "CUndo::ChangeSong BAD!", "Internal error", MB_ICONERROR);
		data = NULL;
	}

	ue->data = (void*)data;
	InsertEvent(ue);
}

void CUndo::ChangeInstrument(int instrnum, int paridx, int type, char separator)
{
	TInstrument* instr = g_Instruments.GetInstrument(instrnum);
	TInstrumentsAll* insall = g_Instruments.GetInstrumentsAll();
	if (!instr || !insall) return;

	// An event with the original status at a different place
	TUndoEvent* ue = new TUndoEvent;
	ue->type = type;
	ue->pos = new int[2];
	ue->pos[0] = instrnum;
	ue->pos[1] = paridx;
	ue->separator = separator;
	int* data;

	switch (type)
	{
	case UETYPE_INSTRDATA:	// Whole instrument
		data = (int*)new TInstrument;
		memcpy((void*)data, (void*)instr, sizeof(TInstrument));
		break;

	case UETYPE_INSTRSALL: // All instruments
		data = (int*)new TInstrumentsAll;
		memcpy((void*)data, (void*)insall, sizeof(TInstrumentsAll));
		break;

	default:
		MessageBox(g_hwnd, "CUndo::ChangeInstrument BAD!", "Internal error", MB_ICONERROR);
		data = NULL;
	}

	ue->data = (void*)data;
	InsertEvent(ue);
}

void CUndo::ChangeInfo(int paridx, int type, char separator)
{
	// An event with the original status at a different place
	TUndoEvent* ue = new TUndoEvent;
	ue->type = type;
	ue->pos = new int[1];
	ue->pos[0] = paridx;
	ue->separator = separator;
	int* data;

	switch (type)
	{
	case UETYPE_INFODATA:	// Whole info
		data = (int*)new TInfo;
		g_Song.GetSongInfoPars((TInfo*)data); // Fill with values taken from g_Song
		break;

	default:
		MessageBox(g_hwnd, "CUndo::ChangeInfo BAD!", "Internal error", MB_ICONERROR);
		data = NULL;
	}

	ue->data = (void*)data;
	InsertEvent(ue);
}

BOOL CUndo::PosIsEqual(int* pos1, int* pos2, int type)
{
	int len;
	switch (type >> 6)	//  /64
	{
		case 0:		len = POSGROUPTYPE0_63SIZE;
			break;
		case 1:		len = POSGROUPTYPE64_127SIZE;
			break;
		case 2:		len = POSGROUPTYPE128_191SIZE;
			break;
		default:
			return 0;
	}
	for (int i = 0; i < len; i++) if (pos1[i] != pos2[i]) return 0;
	return 1;
}

void ExchangeInt(int& a, int& b)
{
	int c = a;
	a = b;
	b = c;
}

char CUndo::PerformEvent(int i)
{
	TUndoEvent* ue = m_uar[i];
	if (!ue) return 1;

	TTrack* tr;
	TInstrument* in;
	TInstrumentsAll* insall;
	TInfo* info;

	int tracknum, trackline, songline, trackcol, instrnum;
	int* data, * temp;

	// Keeps the separator as a return value
	char sep = ue->separator;

	// Set cursor there (change and g_activepart)
	g_Song.SetUECursor(ue->part, ue->cursor);

	// ue->separator = 0; // Default separator = 0
	BLOCKDESELECT;

	switch (ue->type)
	{
	case UETYPE_NOTEINSTRVOL:
		tracknum = ue->pos[0];
		trackline = ue->pos[1];
		tr = g_Tracks.GetTrack(tracknum);
		data = (int*)ue->data;
		ExchangeInt(tr->note[trackline], data[0]);
		ExchangeInt(tr->instr[trackline], data[1]);
		ExchangeInt(tr->volume[trackline], data[2]);
		break;

	case UETYPE_NOTEINSTRVOLSPEED:
		tracknum = ue->pos[0];
		trackline = ue->pos[1];
		tr = g_Tracks.GetTrack(tracknum);
		data = (int*)ue->data;
		ExchangeInt(tr->note[trackline], data[0]);
		ExchangeInt(tr->instr[trackline], data[1]);
		ExchangeInt(tr->volume[trackline], data[2]);
		ExchangeInt(tr->speed[trackline], data[3]);
		break;

	case UETYPE_SPEED:
		tracknum = ue->pos[0];
		trackline = ue->pos[1];
		tr = g_Tracks.GetTrack(tracknum);
		data = (int*)ue->data;
		ExchangeInt(tr->speed[trackline], data[0]);
		break;

	case UETYPE_LENGO:
		tracknum = ue->pos[0];
		trackline = ue->pos[1];
		tr = g_Tracks.GetTrack(tracknum);
		data = (int*)ue->data;
		ExchangeInt(tr->len, data[0]);
		ExchangeInt(tr->go, data[1]);
		break;

	case UETYPE_TRACKDATA: // Whole track
		temp = (int*)new TTrack;
		tracknum = ue->pos[0];
		tr = g_Tracks.GetTrack(tracknum);
		data = (int*)ue->data;
		memcpy((void*)temp, (void*)tr, sizeof(TTrack));
		memcpy((void*)tr, (void*)data, sizeof(TTrack));
		memcpy((void*)data, (void*)temp, sizeof(TTrack));
		delete temp;
		break;

	case UETYPE_TRACKSALL: // All tracks
		temp = (int*)new TTracksAll;
		g_Tracks.GetTracksAll((TTracksAll*)temp); //from temp
		data = (int*)ue->data;
		g_Tracks.SetTracksAll((TTracksAll*)data); //data to tracksall
		memcpy((void*)data, (void*)temp, sizeof(TTracksAll));
		delete temp;
		break;

	case UETYPE_SONGTRACK:
		songline = ue->pos[0];
		trackcol = ue->pos[1];
		data = (int*)ue->data;
		ExchangeInt((*g_Song.GetSong())[songline][trackcol], data[0]);
		break;

	case UETYPE_SONGGO:
		songline = ue->pos[0];
		trackcol = ue->pos[1];
		data = (int*)ue->data;
		ExchangeInt((*g_Song.GetSongGo())[songline], data[0]);
		break;

	case UETYPE_SONGDATA: // Whole song
	{
		TSong temp;
		TSong* data = (TSong*)ue->data;
		// Temp <= song
		memcpy((void*)&temp.song, g_Song.GetSong(), sizeof(temp.song));
		memcpy((void*)&temp.songgo, g_Song.GetSongGo(), sizeof(temp.songgo));
		memcpy((void*)&temp.bookmark, g_Song.GetBookmark(), sizeof(temp.bookmark));
		// Song <= data from undo
		memcpy(g_Song.GetSong(), data->song, sizeof(temp.song));
		memcpy(g_Song.GetSongGo(), data->songgo, sizeof(temp.songgo));
		memcpy(g_Song.GetBookmark(), &data->bookmark, sizeof(temp.bookmark));
		// Data for redo <= temp
		memcpy(data->song, (void*)&temp.song, sizeof(temp.song));
		memcpy(data->songgo, (void*)&temp.songgo, sizeof(temp.songgo));
		memcpy(&data->bookmark, (void*)&temp.bookmark, sizeof(temp.bookmark));
	}
	break;

	case UETYPE_INSTRDATA: // Whole instrument
		temp = (int*)new TInstrument;
		instrnum = ue->pos[0];
		in = g_Instruments.GetInstrument(instrnum);
		data = (int*)ue->data;
		memcpy((void*)temp, (void*)in, sizeof(TInstrument));
		memcpy((void*)in, (void*)data, sizeof(TInstrument));
		memcpy((void*)data, (void*)temp, sizeof(TInstrument));
		delete temp;
		// Must save to Atari
		g_Instruments.WasModified(instrnum);
		break;

	case UETYPE_INSTRSALL: // All instruments
		temp = (int*)new TInstrumentsAll;
		insall = g_Instruments.GetInstrumentsAll();
		data = (int*)ue->data;
		memcpy((void*)temp, (void*)insall, sizeof(TInstrumentsAll));
		memcpy((void*)insall, (void*)data, sizeof(TInstrumentsAll));
		memcpy((void*)data, (void*)temp, sizeof(TInstrumentsAll));
		delete temp;
		// Must save to Atari
		for (i = 0; i < INSTRSNUM; i++) g_Instruments.WasModified(i);
		break;

	case UETYPE_INFODATA:
		temp = (int*)new TInfo;
		info = new TInfo;
		g_Song.GetSongInfoPars((TInfo*)info); // Fill "in" with values taken from g_song
		data = (int*)ue->data;
		memcpy((void*)temp, (void*)info, sizeof(TInfo));
		memcpy((void*)info, (void*)data, sizeof(TInfo));
		memcpy((void*)data, (void*)temp, sizeof(TInfo));
		g_Song.SetSongInfoPars((TInfo*)info); // Set values in g_song with values from "data"
		delete temp;
		delete info;
		break;

	default:
		MessageBox(g_hwnd, "PerformEvent BAD!", "Internal error", MB_ICONERROR);
	}

	g_Song.TrackRespectBoundaries();
	return sep; // Returns separator
}
