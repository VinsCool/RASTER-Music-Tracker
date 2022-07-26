#pragma once


//Undo operation (one can consume up to 3 records)
#define UNDOSTEPS		100
#define MAXUNDO			(UNDOSTEPS*3+8)	//302	//2 extra separating gap

#define POSGROUPTYPE0_63SIZE 2
#define	UETYPE_NOTEINSTRVOL			1
#define UETYPE_NOTEINSTRVOLSPEED	2
#define	UETYPE_SPEED				3
#define UETYPE_LENGO				4
#define UETYPE_TRACKDATA			5

#define UETYPE_SONGTRACK			33
#define UETYPE_SONGGO				34

#define POSGROUPTYPE64_127SIZE 1
#define UETYPE_SONGDATA				65
#define UETYPE_INSTRDATA			66
#define UETYPE_TRACKSALL			67
#define UETYPE_INSTRSALL			68
#define UETYPE_INFODATA				69

#define POSGROUPTYPE128_191SIZE 3

struct TInfo
{
	char songname[SONGNAMEMAXLEN + 1];
	int speed;
	int mainspeed;
	int instrspeed;
	int songnamecur; //to return the cursor to the appropriate position when undo changes in the song name
};

struct TUndoEvent
{
	int part;		//the part in which the editing is performed
	int* cursor;	//cursor
	int type;		//type of changed data
	int* pos;		//position of changed data
	void* data;		//change data
	char separator;	//= 0 accumulate continuous changes, = 1 completed change, = -1 more events for one step
};



//-----------------------------------------------------

class CUndo
{
public:
	CUndo();
	~CUndo();

	void Init();
	void Clear();
	char DeleteEvent(int i);
	char PerformEvent(int i);
	void DropLast();

	BOOL Undo();
	BOOL Redo();
	int GetUndoSteps() { return m_undosteps; };
	int GetRedoSteps() { return m_redosteps; };

	BOOL PosIsEqual(int* pos1, int* pos2, int type);

	void Separator(int sep = 1);
	void ChangeTrack(int tracknum, int trackline, int type, char separator = 0);
	void ChangeSong(int songline, int trackcol, int type, char separator = 0);
	void ChangeInstrument(int instrnum, int paridx, int type, char separator = 0);
	void ChangeInfo(int paridx, int type, char separator = 0);

private:
	void InsertEvent(TUndoEvent* ue);

	TUndoEvent* m_uar[MAXUNDO];
	int m_head, m_tail, m_headmax;
	int m_undosteps, m_redosteps;
};

extern CUndo g_Undo;