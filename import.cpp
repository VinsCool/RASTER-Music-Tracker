/********************************************************************************************/
//IMPORT 
/********************************************************************************************/

#include "importdlgs.h"
//#include "fft.h"

struct TSourceTrack
{
	int note[64];
	int instr[64];
	int volumeL[64];
	int volumeR[64];
	int speed[64];
	int len;
	BOOL stereo;
};

struct TDestinationMark
{
	int fromtrack;
	int shift;
	BYTE leftright;
};

struct TInstrumentMark
{
	int maxvolL;
	int maxvolR;
	BOOL stereo;
};

class CConvertTracks
{
 public:
	CConvertTracks();
	void SetRMTTracks(CTracks* tracks)	{ m_ctracks = tracks; };
	int Init();
	TSourceTrack* GetSTrack(int t)	{ return (t>=0 && t<TRACKSNUM) ? &m_strack[t] : (TSourceTrack*) NULL; };
	TInstrumentMark* GetIMark(int instr)	{ return (instr>=0 && instr<INSTRSNUM) ? &m_imark[instr] : (TInstrumentMark*) NULL; };

	int MakeOrFindTrackShiftLR(int from, int shift, BYTE lr);

 private:
	TSourceTrack m_strack[TRACKSNUM];
	TDestinationMark m_dmark[TRACKSNUM];
	TInstrumentMark m_imark[INSTRSNUM];
	CTracks* m_ctracks;
};

CConvertTracks::CConvertTracks()
{
	m_ctracks = 0;
	Init();
}

int CConvertTracks::Init()
{
	for(int i=0; i<TRACKSNUM; i++)
	{
		TSourceTrack* at=&m_strack[i];
		for(int j=0; j<64; j++)	at->note[j]=at->instr[j]=at->volumeL[j]=at->volumeR[j]=at->speed[j]=-1;
		at->len=-1;
		at->stereo=0;

		TDestinationMark* dm=&m_dmark[i];
		dm->fromtrack=-1;
		dm->shift=0;
		dm->leftright=0;
	}
	for(int i=0; i<INSTRSNUM; i++)
	{
		m_imark[i].maxvolL=0;
		m_imark[i].maxvolR=0;
		m_imark[i].stereo=0;
	}
	return 1;
}

#define VOLUMES_L	1
#define VOLUMES_R	2

int CConvertTracks::MakeOrFindTrackShiftLR(int from, int shift, BYTE lr)
{
	//pokusi se najit mezi jiz vytvorenymi tracky odpovidajici, nebo takovy
	//vytvori (co nejblize za puvodni cislo tracku). 
	//Vrati jeho cislo, nebo -1

	int i;

	if (m_strack[from].len<0) return -1;	//tenhle zdrojovy track je prazdny

	//najde si cislo kam predbezne vytvori ten novy
	int track=-1;

	if (m_dmark[from].fromtrack<0)
	{
		track = from;		//primo to stejne cislo je volne
	}
	else
	{
		//najde volne misto co nejbliz u "from" tracku
		for(i=from+1; i!=from; i++)
		{
			if (i>=TRACKSNUM) i=0;	//kdyz dosel na konec, zacne od zacatku
			if (m_dmark[i].fromtrack<0) break; //nasel volny track
		}
		if (i==from) return -1;		//nenasel volne misto
		track=i;	//to je prazdny, do nehoz vytvori prislusnou modifikaci zdrojoveho tracku
	}

	//prepsani z source tracku do systemoveho
	TSourceTrack* ts=&m_strack[from];
	if ( !ts->stereo ) lr=VOLUMES_L | VOLUMES_R; //pokud to neni stereo track a chce pravy kanal, da mu levy
	TTrack* td = m_ctracks->GetTrack(track);
	int activeinstr=-1;
	for(i=0; i<ts->len; i++)
	{
		int note = ts->note[i];
		if (note>=0)
		{
			note += shift;
			//while (note<0) note+=12;
			//while (note>=NOTESNUM) note-=12;
			while (note<0) note+=64;
			while (note>=64) note-=64;
			if (note>=NOTESNUM) note=NOTESNUM-1;
		}
		td->note[i]=note;

		int instr=ts->instr[i];
		td->instr[i]=instr;
		if (instr>=0) activeinstr=instr;
		
		int volume=(lr&VOLUMES_L) ? ts->volumeL[i] : ts->volumeR[i];
		if (activeinstr>=0 && volume>0)
		{
			int maxvol=(lr&VOLUMES_L) ? m_imark[activeinstr].maxvolL : m_imark[activeinstr].maxvolR;
			if (maxvol-(15-volume)<=0) volume=0;
		}
		td->volume[i]=volume;

		td->speed[i]=ts->speed[i];
	}
	td->len=ts->len;

	//hledani jestli se tenhle neshoduje s nekterym jiz existujicim
	for(i=0; i<TRACKSNUM; i++)
	{
		if (track==i) continue;	//nebude porovnavat sebe se sam sebou
		if (m_ctracks->CompareTracks(track,i))
		{
			//nasel stejny
			//takze tento nove vytvoreny smaze
			m_ctracks->ClearTrack(track);
			//a vrati cislo toho drivejsiho
		    return i; //nasel takovy
		}
	}

	//neshoduje se s zadnym predchozim

	TDestinationMark* dm=&m_dmark[track];
	dm->fromtrack=from;
	dm->shift=shift;
	dm->leftright=lr;
	return track;
}

//----------------------------------------------


int CSong::ImportTMC(ifstream& in)
{
	int originalg_tracks4_8=g_tracks4_8;

	//vymaze nynejsi song
	g_tracks4_8=8;					//standardni TMC je 8 tracku
	m_tracks.m_maxtracklen = 64;	//delka tracku 64
	ClearSong(g_tracks4_8);			//smaze vsechno

	unsigned char mem[65536];
	memset(mem,0,65536);
	WORD bfrom,bto;

	int len,i,j,k;
	char a;	

	len = LoadBinaryBlock(in,mem,bfrom,bto);

	if (len<=0)
	{
		MessageBox(g_hwnd,"Corrupted TMC file or unsupported format version.","Open error",MB_ICONERROR);
		return 0;
	}

	CConvertTracks cot;
	cot.SetRMTTracks(&m_tracks);

	//song name
	for(j=0; j<30 && (a=mem[bfrom+j]); j++)
	{
		if (a<32 || a>=127) a=' ';
		m_songname[j]=a;
	}
	for(k=j;k<SONGNAMEMAXLEN; k++) m_songname[k]=' '; //doplni mezery

	CImportTmcDlg importdlg;
	CString s=m_songname;
	s.TrimRight();
	importdlg.m_info.Format("TMC module: %s",(LPCTSTR)s);

	if (importdlg.DoModal()!=IDOK) return 0;

	BOOL x_usetable=importdlg.m_check1;
	BOOL x_optimizeloops=importdlg.m_check6;
	BOOL x_truncateunusedparts=importdlg.m_check7;


	WORD instr_ptr[64],track_ptr[128];
	BOOL instr_used[64];
	WORD adr;
	BYTE c;
	int speco=0;			//speedcorrection

	//speeds
	m_mainspeed=m_speed= mem[bfrom+30]+1;
	m_instrspeed= mem[bfrom+31];
	if (m_instrspeed>4)
	{
		speco=m_instrspeed-4;
		m_instrspeed=4;		//maximalni 4x instrspeed
	}
	else
	if (m_instrspeed<1) m_instrspeed=1;		//minimalni 1x instrspeed

	//instrument vektory
	for(i=0; i<64; i++)
	{
		instr_ptr[i]= mem[bfrom+32+i] + (mem[bfrom+32+64+i]<<8);
		instr_used[i] = 0; //zjistuje az pak pri prochazeni tracku, jestli je pouzit
	}
	//track vektory
	for(i=0; i<128; i++) track_ptr[i]= mem[bfrom+32+128+i] + (mem[bfrom+32+128+128+i]<<8);

	//tracky
	for(i=0; i<128; i++)
	{
		adr = track_ptr[i];
		if (mem[adr]==0xff) continue;	//ukazuje na FF
		int line=0;
		TSourceTrack& ts = *(cot.GetSTrack(i));
		int ains=0,note=-1,volL=15,volR=15,speed=0,space=0;
		BOOL endt=0;

		while(1)
		{
			if (line>=64) { ts.len=64; break; }

			c=mem[adr++];
			int txx=c & 0xc0;
			if (txx == 0x80)
			{
				//instrument
				ains= c & 0x3f;
				instr_used[ains]=1;
			}
			else
			if (txx == 0x00)
			{
				//nota a hlasitost
				//nota
				note = (c & 0x3f) - 1;
				if (note<0) note = -1;
				else
				{
					ts.note[line]= note;
					ts.instr[line]= ains;
				}
				//hlasitost
				c=mem[adr++];
				volL= 15 - ((c & 0xf0)>>4);
				volR= 15 - (c & 0x0f);
				if (volL!=volR) ts.stereo=1;	//je tam nejaka ruzna hlasitost pro L a R
				if (volL!=0 || volR!=0)			//v TMC nemuze existovat zapis kdy jsou volL i R rovny oba 0
				{
					ts.volumeL[line]= volL;
					ts.volumeR[line]= volR;
				}
				line++;
			}
			else
			if (txx == 0x40)
			{
				//nota speed  (+mozna hlasitost)
				//nota
				note = (c & 0x3f) - 1;
				if (note<0) note = -1;
				else
				{
					ts.note[line]= note;
					ts.instr[line]= ains;
				}
				//speed
				c=mem[adr++];
				speed = c & 0x0f;
				if (speed==0)
				{
					ts.len=line+1;
					endt=1;
					//break;			// speed=0 => konec tohoto tracku
				}
				else
				{
					ts.speed[line]= speed+1;
				}
				if (c & 0x80)		//za speedem je i hlasitost
				{
					//hlasitost
					c=mem[adr++];
					volL= 15 - ((c & 0xf0)>>4);
					volR= 15 - (c & 0x0f);
					if (volL!=volR) ts.stereo=1;	//je tam nejaka ruzna hlasitost pro L a R
					if (volL!=0 || volR!=0)			//v TMC nemuze existovat zapis kdy jsou volL i R rovny oba 0
					{
						ts.volumeL[line]= volL;
						ts.volumeR[line]= volR;
					}
				}
				if (endt) break;	//konec tohoto tracku pres speed=0
				line++;
			}
			else
			if (txx == 0xc0)
			{
				//mezera
				space=(c & 0x3f)+1;
				if (space>63 || line+space>63)
				{
					if (line>0) ts.len = 64;
					break;	//space=64 (ff) => konec tohoto tracku
				}
				line +=space;
			}
		}
		//a na dalsi track
		line=0;
	}

	//instrumenty
	int nonemptyinstruments=0;
	for(i=0; i<64; i++)
	{
		TInstrument& ai = m_instrs.m_instr[i];

		if (instr_used[i]) //je tento instrument pouzit nekde v nekterem tracku
		{
			//ano => nazev
			CString s;
			s.Format("TMC instrument imitation %02X",i);
			strncpy(ai.name,(LPCTSTR)s,s.GetLength());
		}

		int adr_e=instr_ptr[i];
		if (adr_e==0) continue;	//nedefinovan
		
		//definovan

		nonemptyinstruments++;

		int adr_t=instr_ptr[i]+63;
		int adr_p=instr_ptr[i]+63+8;

		//audctl
		BYTE audctl1 = mem[adr_p+1];
		BYTE audctl2 = mem[adr_p+2];

		//ai.par[PAR_AUDCTL0...7]=audctl1;

		//envelope
		int c1,c2,c3;
		BOOL anyrightvolisntzero=0;
		BOOL filteru=0;			//je pouzit filter?
		int lasttmccmd=-1;				//minuly command
		int lasttmcpar=0;				//minuly parametr
		int cmd1_2=0;					//pro posloupnost commandu 1 a 2
		int par1_2=0;					//parametr pro pricitani frekvenci u posloupnosti commandu 1 a 2
		int cmd2_2=0;					//posloupnost 2 a 2
		int par2_2=0;					//parametr pro posloupnost 2 a 2
		int cmd6_6=0;					//posloupnost 6 a 6
		int par6_6=0;					//parametr pro posloupnost 6 a 6
		int maxvolL=0,maxvolR=0,lastvol=0;
		for(j=0; j<21; j++)
		{
			c1=mem[adr_e+j*3];
			c2=mem[adr_e+j*3+1];
			c3=mem[adr_e+j*3+2];

			int dist08=(c1>>4) & 0x01;			//forced volume
			BYTE dist=(c1>>4) & 0x0e;					//suda c. 0,2,az,E
			if (dist==0x0e) dist=0x0a;		//ciste tony
			else
			if (dist==0x06) dist=0x02;		//sumy 2

			//nyni je dist 0,2,4,8,10,12   (bez 0x06 a 0x0e)

			int basstable = mem[adr_p+7]&0xc0;
			if (dist==0x0c && (basstable==0x80 || basstable==0xc0)) dist=0x0e;

			int com08=(c2>>4) & 0x08;		//command 8x
			BYTE audctl= com08 ? audctl2 : audctl1;

			//16 bit basy?
			if (   ((audctl&0x50)==0x50 || (audctl&0x28)==0x28) 
				&& (dist==0x0c) ) 
							dist=0x06;	//16bit bassy
			//filter
			if ( ((audctl&0x04)==0x04 || (audctl&0x02)==0x02) )
			{
				ai.env[j][ENV_FILTER] = 1;
				filteru=1;
			}
			
			ai.env[j][ENV_DISTORTION]= dist;
			int vol=c1 & 0x0f;			//volumeL 0-F;
			ai.env[j][ENV_VOLUMEL]= lastvol = vol;			//lastvol je potreba pro korekci doznivani
			if (vol>maxvolL) maxvolL=vol;	//maximalni volumeL z cele envelope
			vol = c2 & 0x0f;			//volumeR 0-F
			ai.env[j][ENV_VOLUMER]= vol;
			if (vol>maxvolR) maxvolR=vol;	//maximalni volumeR z cele envelope
			if (vol>0) anyrightvolisntzero=1;	//nejaka volumeR je >0
			
			int tmccmd=(c2>>4) & 0x07;		//command
			int tmcpar=c3;
			int rmtcmd=tmccmd,rmtpar=tmcpar;
			//prevod commandu TMC na RMT
			switch (tmccmd)
			{
			case 0: //bez efektu
				rmtcmd=0; rmtpar=0;
				break;
			case 1: //P->AUDF		(stejne s TMC)
				cmd1_2=2;
				par1_2=tmcpar;
				if (com08 || tmcpar==0x00) //ale cmd 9 nebo 1 s parametrem 00 je volume only
				{
					rmtcmd=7;		//Volume only
					rmtpar=0x80;	//
				}
				break;
			case 2: //P+A->AUDF
				if (j>0 && cmd1_2)
				{
					//predela 2 xy na 1 ab,  kde ab=hodnota u predchozi leve jednicky + prirustek
					//bude to fungovat retezove i u dalsich dvojek, coz je v poradku
					rmtcmd=1;
					rmtpar=(BYTE)(par1_2+tmcpar);
					cmd1_2=2;	//aby to vydrzelo opet pro pristi sloupec envelope
					par1_2=rmtpar;	//prictena frekvence pro priste
				}
				else
				if (j>0 && cmd2_2)
				{
					//predela 2 xy na 2 ab, kde ab=hodnota u predchozi leve dvojky + prirustek
					rmtcmd=2;
					rmtpar=(BYTE)(par2_2+tmcpar);
					cmd2_2=2;
					par2_2=rmtpar;
				}
				else
				{
					//zustane stejne
					rmtcmd=2;
					rmtpar=tmcpar;
					cmd2_2=2;
					par2_2=rmtpar;
				}
				break;
			case 3: //P+N->AUDF
				//odpovida presne TMC commandu 2
				rmtcmd=2; rmtpar=tmcpar;
				break;
			case 4: //P&RND->AUDF
				rmtcmd=1;	//prevede na pevne nastaveni nahodne zvolene frekvence
				rmtpar=rand()&0xff;
				break;
			case 5: //PN->AUDF  (hraje pevne notu s tim indexem)
				rmtcmd=1;	//prevede na pevne nastaveni frekvence odpovidajici noty (dle distortion)
				//dist=0-e
				rmtpar=(256-(tmcpar*4)) & 0xff;	//JEN PROZATIM NEZ TO BUDE PORADNE!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				break;
			case 6: //PN+A->AUDF 
				if (j>0 && cmd6_6)
				{
					rmtcmd=0; 
					rmtpar=(BYTE)(par6_6+tmcpar);
					cmd6_6=2;
					par6_6=rmtpar;
				}
				else
				{
					rmtcmd=0; 
					rmtpar=tmcpar;
					cmd6_6=2;
					par6_6=rmtpar;
				}
				break;
			case 7: //PN+N->AUDF
				rmtcmd=0;	//v RMT se notovy posun dela commandem 0
				break;
			}

			//posun basu
			if (dist==0x0c && rmtcmd==0) rmtpar+=8;

			//forced volume
			if (dist08) { rmtcmd=7; rmtpar=0x80; } //volume only

			ai.env[j][ENV_COMMAND]= rmtcmd;
			ai.env[j][ENV_X]= (rmtpar>>4) & 0x0f;
			ai.env[j][ENV_Y]= rmtpar & 0x0f;

			lasttmccmd=tmccmd;
			lasttmcpar=tmcpar;
			if (cmd1_2>0) cmd1_2--;		//odecita
			if (cmd2_2>0) cmd2_2--;		//odecita
			if (cmd6_6>0) cmd6_6--;		//odecita

		} //0-20 sloupcu envelope

		//je vsechna prava hlasitost=0 ? => prekopiruje levou na pravou
		if (!anyrightvolisntzero) 
		{
			for(j=0; j<=21; j++) ai.env[j][ENV_VOLUMER]=ai.env[j][ENV_VOLUMEL];
			maxvolR=maxvolL;
		}

		//delka envelope
		ai.par[PAR_ENVLEN]=20;			//envelope je 21 sloupcu
		ai.par[PAR_ENVGO]=20;

		TInstrumentMark* im=cot.GetIMark(i);
		im->maxvolL = maxvolL;
		im->maxvolR = maxvolR;
		im->stereo = anyrightvolisntzero;

		//table
		BOOL tableu=0;	//table used
		for(j=0; j<8; j++)
		{
			int nut=mem[adr_t+j];
			if (nut>=0x80 && nut<=0xc0) nut+=0x40;
			if (nut>=0x40 && nut<=0x7f) nut-=0x40;
			if (nut!=0) tableu=1; //table je na neco pouzita
			ai.tab[j]=nut;
		}
		
		//parametry
		//table delka a speed
		int tablen = (mem[adr_p+8]>>4) & 0x07;
		ai.par[PAR_TABLEN]= tablen;		//0-7
		ai.par[PAR_TABSPD]= mem[adr_p+8] & 0x0f;			//rychlost 0-15
		//
		//dalsi parametry
		//volume slide
		BYTE t1vslide=mem[adr_p+3];
		//speco je korekce pri zpomaleni instrumentu ze 5 a vice na 4
		BYTE t2vslide=(t1vslide<1)? 0: (BYTE)((double)15/lastvol*(double)255/((double)t1vslide * (1-((double)speco)/4) ) +0.5);
		if (t2vslide>255) t2vslide=255;
		else
		if (t2vslide<0) t2vslide=0;
		ai.par[PAR_VSLIDE] = t2vslide;
		ai.par[PAR_VMIN]= 0;
		//vibrato nebo fshift
		BYTE pvib=mem[adr_p+5] & 0x7f;
		BYTE pvib8=mem[adr_p+5] & 0x80;		//horni bit
		BYTE delay=mem[adr_p+6];
		BYTE vibspe=mem[adr_p+7] & 0x3f;	//vibrato speed
		BYTE vib=0,fshift=0;
		BOOL vpt=0;			//vibrato pres table se podarilo

		int posuntable= (int)((double)delay / (vibspe+1) +0.5);

		BOOL nobytable=0;	//jestli se ma pokouset to prevadet na table
		if (!x_usetable) nobytable=1;	//nema se pokouset
		if (filteru) nobytable=1;

		//co kdyz sice je table pouzita, ale jen pro posun na 0.miste
		if (!nobytable && tableu && tablen==0 && (pvib&0x40))		//(pvib&0x40) <--jen jestlize na neco pouziva vibrato, jinak nema smysl to predelavat
		{
			//je to tak, zoptimalizuje pres posun vsech not v envelope
			int psn=ai.tab[0];	//0.misto v table
			for(j=0; j<21; j++)
			{
				if (ai.env[j][ENV_COMMAND]==0) //notovy posun
				{
					BYTE notenum = (ai.env[j][ENV_X]<<4) + ai.env[j][ENV_Y];
					notenum += psn; //posune
					ai.env[j][ENV_X]= (notenum>>4) & 0x0f;
					ai.env[j][ENV_Y]= notenum & 0x0f;
				}
			}
			ai.tab[0]=0; //takze parametr v table se vynuluje
			tableu=0;	//a tim padem je table volna pro dalsi pouziti
		}

		//a ted jednotlive hodnoty parametru "vibrato":

		if (pvib>0x10 && pvib<0x3f)
		{
			//vibrato
			int hn=pvib>>4;		//typ vibrata 1-3
			int dn=pvib&0x0f;	//vykmit vibrata 0-f

			/*
			vib = (int) ((double) (hn+(float)dn/4)+0.5);
			if (vib<0) vib=0;
			else
			if (vib>3) vib=3;
			*/
			vib=3;
			if (hn==1) vib=1+vibspe;	//nejpodobnejsi je tohle nezavisle na velikosti vykmitu, speed to kdyztak posune na vib 2 nebo 3
			else
			if (hn==2 && dn==1) vib=2+vibspe; //pro dn==1 to odpovida presne, ostatni uz jsou uz odpovidajici vib3
			if (vib>3) vib=3;	//kdyby to po pricteni "vibspe" vyslo vic nez 3

			//jestlize ma nevyuzite table, pokusi se udelat vibrato pres table
			if (!nobytable && !tableu)
			{
				if (hn==1 && (dn>2 || vibspe>0) )							//(dn>=4 || vibspe>0) )
				{
					if (posuntable>TABLEN-4) posuntable=TABLEN-4; //nejdal co je mozne podle delay
					ai.tab[posuntable]=0;
					ai.tab[posuntable+1]=(pvib8) ? (BYTE)(256-dn) : dn;
					ai.tab[posuntable+2]=0;
					ai.tab[posuntable+3]=(pvib8) ? dn : (BYTE)(256-dn);
					ai.par[PAR_TABLEN]=posuntable+3;
					ai.par[PAR_TABGO]=posuntable;
					ai.par[PAR_TABTYPE]=1;	//tabulka frekvenci
					ai.par[PAR_TABSPD]=(vibspe<0x3f)? vibspe : 0x3f;
					vpt=1; //podarilo se
				}
				else
				if (hn==2 && (dn>2 || vibspe>0) )							//(dn>=4 || vibspe>0))
				{
					if (posuntable>TABLEN-4) posuntable=TABLEN-4; //nejdal co je mozne podle delay
					ai.tab[posuntable]=(pvib8) ? (BYTE)(256-dn) : dn;
					ai.tab[posuntable+1]=0;
					ai.tab[posuntable+2]=(pvib8) ? dn : (BYTE)(256-dn);
					ai.tab[posuntable+3]=0;
					ai.par[PAR_TABLEN]=posuntable+3;
					ai.par[PAR_TABGO]=posuntable;
					ai.par[PAR_TABTYPE]=1;	//tabulka frekvenci
					int sp=dn*(vibspe+1)-1;
					if (sp<0) sp=0;	else if (sp>0x3f) sp=0x3f;
					ai.par[PAR_TABSPD]=sp;
					vpt=1; //podarilo se
				}
				else
				if (hn==3 && (dn>2 || vibspe>0) )							//(dn>=4 || vibspe>0))
				{
					if (posuntable>TABLEN-4) posuntable=TABLEN-4; //nejdal co je mozne podle delay
					ai.tab[posuntable]=(pvib8) ? (BYTE)(dn*4) : (BYTE)(256-(dn*4)) ; //u not je to naopak (pricitani not = odecitani frekv
					ai.tab[posuntable+1]=0;
					ai.tab[posuntable+2]=(pvib8) ? (BYTE)(256-(dn*4)) : (BYTE)(dn*4); //u not je to naopak
					ai.tab[posuntable+3]=0;
					ai.par[PAR_TABLEN]=posuntable+3;
					ai.par[PAR_TABGO]=posuntable;
					ai.par[PAR_TABTYPE]=1;	//tabulka frekvenci
					int sp=dn*(vibspe+1)-1;
					if (sp<0) sp=0;	else if (sp>0x3f) sp=0x3f;
					ai.par[PAR_TABSPD]=sp;
					vpt=1; //podarilo se
				}
			}
		}
		else
		if (pvib>0x40 && pvib<=0x4f)
		{
			//fshift dolu (pridavani frq)
			fshift=pvib-0x40;
			if (pvib8) fshift = (BYTE)(256-fshift);

			//a ted zjisti, jestli by to neudelal lip pres table
			if (!nobytable && !tableu && vibspe>0)
			{
				if (posuntable>TABLEN-2) posuntable=TABLEN-2; //nejdal co to jde
				ai.tab[posuntable]=fshift;
				ai.tab[posuntable+1]=fshift;
				ai.par[PAR_TABLEN]=posuntable+1;
				ai.par[PAR_TABGO]=posuntable+1;
				ai.par[PAR_TABTYPE]=1;	//tabulka frekvenci
				ai.par[PAR_TABMODE]=1;	//pricitani
				ai.par[PAR_TABSPD]=(vibspe<0x3f)? vibspe : 0x3f;
				vpt=1; //podarilo se
			}
		}
		else
		if (pvib>0x50 && pvib<=0x5f)
		{
			//posun v notach dolu (doleva) => posun o frekvenci 255/61 * posun_v_notach 
			fshift= (int) (((double)(255/61)* (pvib-0x50))+0.5);
			if (pvib8) fshift = (BYTE)(256-fshift);

			//a ted zjisti, jestli by to neudelal lip pres table
			
			if (!nobytable && !tableu)				// && vibspe>0)
			{
				if (posuntable>TABLEN-2) posuntable=TABLEN-2; //nejdal co to jde
				int nshift=pvib-0x50;
				if (!pvib8) nshift= (BYTE)(256-nshift);		//u not je to naopak (5x je <--down, Dx je up-->)
				ai.tab[posuntable]=nshift;
				ai.tab[posuntable+1]=nshift;
				ai.par[PAR_TABLEN]=posuntable+1;
				ai.par[PAR_TABGO]=posuntable+1;
				ai.par[PAR_TABTYPE]=0;	//tabulka not
				ai.par[PAR_TABMODE]=1;	//pricitani
				ai.par[PAR_TABSPD]=(vibspe<0x3f)? vibspe : 0x3f;
				vpt=1; //podarilo se
			}
		}
		else
		if (pvib>0x60 && pvib<=0x6f)
		{
			//podladeni $60-$6f => -0 az -15 k frekvenci
			//neumime
		}


		//overeni, zda vibrato effekt udelal pres table nebo "normalne" pres vibrato a fshift
		if (vpt)
		{
			//podarilo se udelat vibrato pres table, takze nevyuzije vibrato ani shift ani delay
			vib=0;
			fshift=0;
			delay=0;
		}

		ai.par[PAR_VIBRATO]=vib;
		ai.par[PAR_FSHIFT]=fshift;
		if (vib==0 && fshift==0) delay=0;
		else
		if ((vib>0 || fshift>0) && delay==0) delay=1;
		ai.par[PAR_DELAY] = delay;

		//optimalizace
		//delka envelope
		int lastnonzerovolumecol=-1;
		int lastchangecol=0;
		for(int k=0; k<=20; k++)
		{
			if (ai.env[k][ENV_VOLUMEL]>0 || ai.env[k][ENV_VOLUMER]>0) lastnonzerovolumecol=k;
			if (k>0)
			{
				for(int m=0; m<ENVROWS; m++)
				{
					if (ai.env[k][m]!=ai.env[k-1][m])	//je neco jineho? (volumeL,R,distor,...,portamento)
					{
						lastchangecol=k;	//jo, neco je jineho.
						break;
					}
				}
				//sem skoci break
			}
		}

		if (lastnonzerovolumecol<20)
		{
			ai.par[PAR_ENVLEN]=ai.par[PAR_ENVGO]=lastnonzerovolumecol+1; //zkrati za posledni nenulovou hlasitost
		}

		if (   lastchangecol<lastnonzerovolumecol	//posledni libovolna zmena probehla ve sloupci lastchangecol
			&& ai.par[PAR_VSLIDE]==0				//jedine kdyz neubyva hlasitost
		   )
		{
			ai.par[PAR_ENVLEN]=ai.par[PAR_ENVGO]=lastchangecol; //zkrati to na sloupec kde byla posledni zmena cehokoliv
		}

		//table
		for(int v=0; v<=ai.par[PAR_TABLEN]; v++)	//jsou tam jen nuly?
		{
			if (ai.tab[v]!=0) goto NoTableOptimize;
		}
		if (ai.par[PAR_TABLEN]>=1 && ai.par[PAR_TABGO]==0)
		{
			ai.par[PAR_TABLEN]=0;
			ai.par[PAR_TABSPD]=0;
		}
NoTableOptimize:


		//promitnuti instrumentu do Atarkove RAM
		m_instrs.ModificationInstrument(i);
	} //a na dalsi instrument


	//song
	int line=0;
	BYTE lr=0;
	BOOL stereomodul=0;
	int numoftracks=0;
	int adrendsong=(instr_ptr[0]>0) ? instr_ptr[0] : track_ptr[0];

	for(adr=bfrom+32+128+256; adr<adrendsong; adr+=16,line++)
	{
		if ((mem[adr+15] & 0x80) == 0x80)	//goto radek?
		{
			//goto
			m_songgo[line]=mem[adr+14] & 0x7f;
			continue;
		}
		for(i=0; i<8; i++)
		{
			int t=mem[adr+15-i*2];
			char preladeni=(char)mem[adr+14-i*2];
			if (t>=0 && t<128)
			{
				if (i<4)
				{
					lr = VOLUMES_L;
				}
				else
				{
					//lr = VOLUMES_R;
					lr = VOLUMES_L;
					/*
					int levy=mem[adr+15+8-i*2];
					if (levy>=0 && levy<128 && cot.GetSTrack(t)->len<0)
					{
						t = levy; //pouzije pravou variaci leveho tracku
						preladeni = (char)mem[adr+14+8-i*2];	//se stejnym ladenim jako ma ten levy
					}
					*/
				}

				int vyslednytrack = cot.MakeOrFindTrackShiftLR(t,preladeni,lr);

				m_song[line][i]= vyslednytrack;

				if (vyslednytrack>numoftracks) numoftracks=vyslednytrack;	//celkovy pocet tracku

				if (vyslednytrack>=0 && i>=4) stereomodul=1;
			}
		}
	}
	//je na konci nejake goto?
	if (m_songgo[line-1]<0) m_songgo[line+1]=line;	//neni, tak prida na konec nekonecny loop
	//if (m_songgo[line-1]<0) m_songgo[line]=0; //neni, tak prida goto na zacatek

	if (!stereomodul) g_tracks4_8=4;	//mono modul

	//ZAVERECNY DIALOG PO DOKONCENI IMPORTU
	CImportTmcFinishedDlg imfdlg;
	imfdlg.m_info.Format("%i tracks, %i instruments, %i songlines",numoftracks,nonemptyinstruments,line);

	//OPTIMIZATIONS
	if (x_optimizeloops)
	{
		int optitracks=0,optibeats=0;
		TracksAllBuildLoops(optitracks,optibeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Loops in %i tracks (%i beats/lines)",optitracks,optibeats);
		imfdlg.m_info+=s;
	}

	if (x_truncateunusedparts)
	{
		int clearedtracks=0,truncatedtracks=0,truncatedbeats=0;
		SongClearUnusedTracksAndParts(clearedtracks,truncatedtracks,truncatedbeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Cleared %i, truncated %i tracks (%i beats/lines)",clearedtracks,truncatedtracks,truncatedbeats);
		imfdlg.m_info+=s;
	}

	if (imfdlg.DoModal()!=IDOK)
	{
		//nedal Ok, takze to smaze
		g_tracks4_8=originalg_tracks4_8;	//vrati puvodni hodnotu
		ClearSong(g_tracks4_8);
		MessageBox(g_hwnd,"Module import aborted.","Import...",MB_ICONINFORMATION);
	}
	
	return 1;
}


/********************************************************************************************/

//27.zari 2003 20:38 ... Prave jsem naimportoval aurora.mod, pustil to bez uprav a jsem ohromen!!!
//							To je naprosto UZASNY! UZASNY! NAPROSTO UZASNY!!!


struct TMODInstrumentMark
{
	BYTE used;			//bity urcuji v kterem sloupci (na kterem channelu) je pouzit
	int volume;
	int minnote;
	int maxnote;
	int samplen;
	int reppoint;
	int replen;
	double trackvolumeincrease;
	int trackvolumemax;
};

int AtariVolume(int volume0_64)
{
	int	avol=(int)( (double)volume0_64/4 +0.5);	//prepocet na atarkovou volume
	if (volume0_64<1) avol=0;
	else
	if (volume0_64==1) avol=1;
	else
	if (avol>0x0f) avol=0x0f;
	return avol;
}

int CSong::ImportMOD(ifstream& in)
{
	//vymaze nynejsi song
	int originalg_tracks4_8=g_tracks4_8;	//schova si puvodni hodnotu pro Abort
	g_tracks4_8=8;					//predchysta si 8 kanalovy
	m_tracks.m_maxtracklen = 64;	//delka tracku 64
	ClearSong(g_tracks4_8);			//vymaze

	int i,j;
	BYTE a;
	BYTE head[1085];

	in.read((char *)&head,1084);
	int flen=(int)in.gcount();

	head[1084]=0;	//ukonceni za hlavickou

	if (flen!=1084)
	{
		MessageBox(g_hwnd,"Bad file format.","Error",MB_ICONSTOP);
		return 0;
	}

	in.seekg(0,ios::end);
	int modulelength=(int)in.tellg();
	
	int chnls=0;
	int song=950;	//kde zacina song u 31 samploveho modulu
	int patstart=1084;	//zacatek patternu u 31 samploveho modulu
	int modsamples=31;	//31 samplovy
	if (strncmp((char*)(head+1080),"M.K.",4)==0)
		chnls=4;					//M.K.
	else
	if (strncmp((char*)(head+1081),"CHN",3)==0)
		chnls=head[1080]-'0';	//xCHN
	else
	{
		for(int i=0; i<4; i++)
		{
			a=head[1080+i];
			if (a<32 || a>90) // je to mimo " " az "Z"  (tj. neni to nejake pismeno nebo mezera)
			{
				//=>15 samplovy MOD
				chnls=4;
				modsamples=15;
				song=470;
				patstart=600;
				break;
			}
		}
	}

	if (chnls<4 || chnls>8)
	{
		CString es;
		es.Format("There isn't ProTracker identification header bytes.\nAllowed headers are \"M.K.\" or from \"4CHN\" to \"8CHN\",\nbut there is \"%s\".",head+1080);
		MessageBox(g_hwnd,(LPCTSTR)es,"Error",MB_ICONSTOP);
		return 0;
	}
	int patternsize=chnls*256;

	int songlen=head[song+0];
	int restartpos=head[song+1];
	if (restartpos>=songlen) restartpos=0;

	int maxpat=0;
	if (songlen>SONGLEN-1) songlen=SONGLEN-1;
	for(i=0; i<songlen; i++)
	{
		int patnum=head[song+2+i];
		if (patnum>maxpat) maxpat=patnum;
	}
	int memlen=song+130+patternsize*(maxpat+1);				//130= 1delka +1repeat + 128song
	if (song>=950) memlen+=4;		// navic 4 identifikacni pismena (napr. "M.K.") //song+130 (1084)

	//TED TEPRVE ALOKUJE PAMET
	BYTE* mem = new BYTE[memlen];
	if (!mem)
	{
		MessageBox(g_hwnd,"Can't allocate memory for module.","Error",MB_ICONSTOP);
		return 0;
	}
	
	in.clear();
	in.seekg(0);		//znovu na zacatek
	in.read((char*)mem,memlen);	//nacte modul
	flen=(int)in.gcount();
	if (flen!=memlen)
	{
		MessageBox(g_hwnd,"Bad file.","Error",MB_ICONSTOP);
		if (mem) delete[] mem;
		return 0;
	}

	BYTE trackorder[8]={0,1,2,3,4,5,6,7};	//usporadani tracku
	int rmttype=0;

	CImportModDlg importdlg;
	importdlg.m_info.Format("%i channels %i samples ProTracker module detected.\n(Header bytes \"%s\".)",chnls,modsamples,head+1080);
	if (chnls==4)
	{	//4 kanalovy modul
		importdlg.m_txtradio1="RMT4 with 1,2,3,4 tracks order";
		importdlg.m_txtradio2="RMT8 with 1,4 / 2,3 tracks order";
		if (importdlg.DoModal()==IDOK)
		{
			if (importdlg.m_txtradio1!="")
			{	//prvni volba
				rmttype=4;
			}
			else
			{	//druha volba
				rmttype=8;
				trackorder[0]=0;
				trackorder[1]=4;
				trackorder[2]=5;
				trackorder[3]=1;
			}
		}
	}
	else
	{	//5-8 kanalovy modul
		importdlg.m_txtradio1="RMT8 with 1,4,5,8 / 2,3,6,7 tracks order";
		importdlg.m_txtradio2="RMT8 with 1,2,3,4 / 5,6,7,8 tracks order";
		if (importdlg.DoModal()==IDOK)
		{
			if (importdlg.m_txtradio1!="")
			{	//prvni volba
				rmttype=8;
				trackorder[0]=0;
				trackorder[1]=4;
				trackorder[2]=5;
				trackorder[3]=1;
				trackorder[4]=2;
				trackorder[5]=6;
				trackorder[6]=7;
				trackorder[7]=3;
			}
			else
			{	//druha volba
				rmttype=8;
			}
		}
	}

	if (rmttype!=4 && rmttype!=8)
	{	//nevybral zadnou moznost (cancel v dialogu)
		if (mem) delete[] mem;
		return 0;
	}

	BOOL x_shiftdownoctave=importdlg.m_check1;
	BOOL x_portamento=importdlg.m_check5;
	BOOL x_fullvolumerange=importdlg.m_check2;
	BOOL x_volumeincrease=importdlg.m_check3;
	BOOL x_decreaseinstrument=importdlg.m_check4;
	BOOL x_optimizeloops=importdlg.m_check6;
	BOOL x_truncateunusedparts=importdlg.m_check7;
	BOOL x_fourier=importdlg.m_check8;

	g_tracks4_8=rmttype;	//vyrobi RMT4 nebo RMT8

	//jmeno songu
	for(j=0; j<20 && (a=mem[j]); j++) m_songname[j]=a;

	//speeds
	m_mainspeed=m_speed= 6;			//default speed
	m_instrspeed=1;

	int maxsmplen=0;			//maximalni delka samplu

	//instrumenty 1-31
	TMODInstrumentMark imark[32];
	for(i=1; i<=modsamples; i++)
	{
		//jmeno 
		BYTE *sdata=mem+(20+(i-1)*30);	//hlavickova data samplu
		TInstrument *ti = &m_instrs.m_instr[i];
		char *dname= ti->name;
		for(j=0; j<22; j++)	//0-21 jmeno
		{
			a=sdata[j];
			if (a>=32 && a<=126) 
				dname[j]=a;
			else
				dname[j]=' ';
		}
		for(;j<INSTRNAMEMAXLEN; j++) dname[j]=' '; //smaze zbytek nazvu instrumentu
		int samplen=(sdata[23] | (sdata[22]<<8))*2;
		//BYTE finetune=sdata[24]&0x0f;		//0-15
		//if (finetune>=8) finetune+=240;		//0-7 nebo 248-255
		
		int volume=sdata[25];
		if (volume>0x3f) volume=0x3f;		//00-3f

		int reppoint=(sdata[27] | (sdata[26]<<8))*2;

		int replen=(sdata[29] | (sdata[28]<<8))*2;

		//ti->env[0][ENV_VOLUMEL]= (volume>>2);	//0-15
		//ti->env[0][ENV_VOLUMER]= (volume>>2);	//0-15
		//ti->env[0][ENV_DISTORTION]= 0x0a;		//cisty ton
		/*if (finetune!=0)
		{
			ti->env[0][ENV_COMMAND]=0x02;		//frekvencni posun
			ti->env[0][ENV_X]=(finetune>>4);
			ti->env[0][ENV_Y]=(finetune&0x0f);
		}
		*/
		//if (!reppoint) ti->par[PAR_VSLIDE]=255-(samplen>>8);

		imark[i].volume=volume;
		imark[i].minnote=NOTESNUM-1;
		imark[i].maxnote=0;
		imark[i].used=0;			//nepouzit na zadnem kanalu
		imark[i].samplen=samplen;
		imark[i].reppoint=reppoint;
		imark[i].replen=replen;
		imark[i].trackvolumeincrease=1;
		imark[i].trackvolumemax=0;

		if (samplen>maxsmplen) maxsmplen=samplen;	//maximalni delka samplu

		//promitnuti do Atarka je na konci pri doplnovani veci do instrumentu
	}
	imark[0].trackvolumeincrease=1; //kvuli volume slide, pokud ji provadi bez udani samplu (tj. samplem cislo 0)

	const int TABLENOTES=73;
	const int pertable[TABLENOTES]={															//period table
	0x06B0,0x0650,0x05F4,0x05A0,0x054C,0x0500,0x04B8,0x0474,0x0434,0x03F8,0x03C0,0x038B,	//C3-B3
	0x0358,0x0328,0x02FA,0x02D0,0x02A6,0x0280,0x025C,0x023A,0x021A,0x01FC,0x01E0,0x01C5,	//C4-B4
	0x01AC,0x0194,0x017D,0x0168,0x0153,0x0140,0x012E,0x011D,0x010D,0x00FE,0x00F0,0x00E2,	//C5-B5
	0x00D6,0x00CA,0x00BE,0x00B4,0x00AA,0x00A0,0x0097,0x008F,0x0087,0x007F,0x0078,0x0071,	//C6-B6
	0x006B,0x0065,0x005F,0x005A,0x0055,0x0050,0x004B,0x0047,0x0043,0x003F,0x003C,0x0038,	//C7-B7
	0x0035,0x0032,0x002F,0x002D,0x002A,0x0028,0x0025,0x0023,0x0021,0x001F,0x001E,0x001C,	//C8-B8
	0x001B }; //dopocitana hodnota															//C9
	//vyrobi si tabulku
	BYTE pertonote[4096];	//prevod periody na notu
	int n1=0,n2=0,n12=0,lastp=4095;
	for(i=0; i<TABLENOTES; i++)
	{
		n1=pertable[i];
		if (i<TABLENOTES-1)
		{
			n2= pertable[i+1];
			n12= (int)(((float)(n1+n2))/2+0.5);
		}
		else
		{
			n12=0;
		}
		BYTE note=(BYTE)i;
		while (note>=NOTESNUM) note-=12;
		for (j=lastp; j>=n12; j--) pertonote[j]=note;
		lastp=n12-1;
	}

	//ZACATEK ZPRACOVAVANI CELEHO SONGU A PATTERNU
	int dsline;		//destination song line
	int destnum;	//destination track num

	int nofpass=(x_fullvolumerange)? 1 : 0;
	for(int pass=0; pass<=nofpass; pass++)
	{ //---PRUCHOD 0/1---

	destnum=0;		//destination track num
	int tnot[8]={-1,-1,-1,-1,-1,-1,-1,-1};	//posledne hrana nota (pouzije se pro portamento)
	int tporon[8]={0,0,0,0,0,0,0,0};	//portamento yes/no
	int tporperiod[8]={0,0,0,0,0,0,0,0};	//cilova period pro portamento
	int tporspeed[8]={0,0,0,0,0,0,0,0};		//portamento speed
	int tper[8]={0,0,0,0,0,0,0,0};	//aktualni period
	int tvol[8]={0,0,0,0,0,0,0,0};	//hlasitosti jednotlivych tracku (pouzije se pro volume slide)
	int tvolslidedebt[8]={0,0,0,0,0,0,0,0};		//dluh pri volume slide
	int tins[8]={0,0,0,0,0,0,0,0};	//instrumenty jednotlivych tracku (pouzije se kdyz samplenum je ==0)
	int ticks=m_mainspeed;		//songspeed (pro volume slide), default 6
	int beats=125;				//default beats/min speed

	int LastRowTicks=ticks;
	int LastRowBeats=beats;

	//song
	dsline=0;		//destination song line
	int ThisPatternFromRow=0;	//pocatecni radek patternu (kvuli Dxx effektu)
	int ThisPatternFromColumn=-1;	//zadny
	int NextPatternFromRow=0;
	int NextPatternFromColumn=-1;
	for(i=0; i<songlen; i++)
	{
		int patnum=mem[song+2+i];	//probira song

		BYTE *pdata=mem+(patstart+patnum*patternsize);	//zacatek patternu

		int songjump=-1;	//inicializace pro song jump
		ThisPatternFromRow=NextPatternFromRow;	//prebere si z minuleho patternu
		ThisPatternFromColumn=NextPatternFromColumn;	//prebere si z minula
		NextPatternFromRow=0;	//inicializovano na 0 pro pristi pattern
		NextPatternFromColumn=-1;	//inicializovano na -1 pro pristi pattern

		//predpocitani tickrow[0..63] a speedrow[0..63]
		int tickrow[64],speedrow[64];
		int m,n;
		BOOL lrow=0;
		ticks=LastRowTicks;		//posledni ticks z minula
		beats=LastRowBeats;		//posledni beats z minula
		LastRowTicks=-1;		//init
		LastRowBeats=-1;		//init
		for(m=ThisPatternFromRow; m<64; m++)
		{
			for(n=0; n<chnls; n++)
			{
				BYTE *bdata=pdata+m*chnls*4+n*4;
				int effect=bdata[2]&0x0f;
				int param=bdata[3];
				if (effect==0x0f)
				{	//speed
					if (param<=0x20) 
						ticks=(param>0)? param : 1; //speed 0 nelze
					else
						beats=param;
				}
				else
				if (effect==0x0b || effect==0x0d)
				{	//pattern break nebo song jump
					lrow=1;	//musi si to zapsat az dokonci cely tento radek 0..chnls
				}
			}
			//prepocet beats a ticks na ss
			int ss= (int)(((double)(125 * ticks)) / ((double)beats) + 0.5);
			if (ss<1) ss=1;
			else
			if (ss>255) ss=255;

			tickrow[m]=ticks;
			speedrow[m]=ss;

			if ((lrow || m==63) && LastRowTicks<0 && LastRowBeats<0)
			{
				LastRowTicks=ticks;	//v pristim patternu zacne touto hodnotou ticks
				LastRowBeats=beats;	//v pristim patternu zacne touto hodnotou beats
			}
		}

		//4 tracky v patternu
		for(int ch=0; ch<chnls; ch++)
		{
			BYTE *tdata=pdata+ch*4;	//zacatek zdroje dat tracku
			TTrack *tr=m_tracks.GetTrack(destnum);	//cilovy track
			m_tracks.ClearTrack(destnum);	//vycisti ho nejdriv
			int dline;
			int sline;
			for(sline=ThisPatternFromRow,dline=0; sline<64; sline++,dline++)
			{
				BYTE *bdata=tdata+sline*chnls*4;		//ukazatel na 4 bytovy blok

				ticks=tickrow[sline];	//spolecna predpocitana hodnota ticks pro cely radek

				int period = bdata[1] | ((bdata[0]&0x0f)<<8);			//12 bitu
				int sample = (((bdata[2]&0xf0)>>4) | (bdata[0]&0x10)) & modsamples;	//0-31/0-15, nikoliv 0-255 !
				int effect = bdata[2]&0x0f;
				int param = bdata[3];
				int sampleorig = sample;		//original jak je v tracku
				int pspeed;

				if (sample==0) sample=tins[ch];	//sampl cislo 0 znamena stejny jako posledne pouzity
				else
					tins[ch]=sample;	//ulozi si posledne pouzity sampl v tomto "sloupci"
				
				int note=-1,vol=-1;
				if (period<1 || period>=4096)
				{
					//prazdne misto (neni tam nota)
TonePortamento:
					if (x_portamento && tporon[ch])
					{	//naposled bylo portamento
						int pnote = pertonote[tper[ch]];	//jake note odpovida perioda
						if (pnote != tnot[ch])
						{
							//portamento efekt posunul frekvenci az na uroven jine noty
							note = pnote;
							//hlasitost bude dle aktualni hlasitosti co je na tomto kanalu, tj. tvol[ch]
							vol=tvol[ch];	//0-64
							tporon[ch]=0;	//prozatim hotovo (pridana nota odpovida zmene portamentem)
							goto NoteByPortamento;
						}
					}
				}
				else
				{
					//je tam nota
					note = pertonote[period];
					if (x_portamento && (effect==0x03 || effect==0x05))
					{
						//tone portamento 3xx  nebo continue tone portamento 5
						tporperiod[ch]=period;	//cilova portamento perioda
						tporon[ch]=1;
						//navic
						int aper = tper[ch];
						int hfper= aper;
						if (effect==0x03)
						{
							if (param) 
								pspeed=tporspeed[ch]=param;	//TONE portamento speed jen u parametru 3xx
							else
								pspeed=tporspeed[ch];	//je-li 300 => continue portamento posledne pouzitou rychlosti
						}
						else //effect==0x05
							pspeed=tporspeed[ch];	//eff 0x05 je portamento continue

						if (aper<period)
						{
							hfper=aper + pspeed*(ticks-1)/2;
							if (hfper>period) hfper=period;
						}
						else
						{
							hfper=aper - pspeed*(ticks-1)/2;
							if (hfper<period) hfper=period;
						}
						if (pertonote[aper]!=pertonote[hfper])
						{
							note = pertonote[hfper];
							vol = tvol[ch];
							tporon[ch]=0;	//prozatim hotovo (pridana nota odpovida zmene portamentem)
							goto NoteByPortamento;
						}
						//
						goto TonePortamento;	//a resi to jako by tam bylo prazdne misto
					}

					tper[ch]=period;	//aktualni period
					tporon[ch]=0;		//zadne portamento
					vol=0x40;			//defaultni je plna hlasitost (kdyztak to pak prepise)
					tvolslidedebt[ch]=0; //dluh volume slide=0
NoteByPortamento:
					tnot[ch]=note;		//posledne hrana nota
					tr->note[dline]=note;
					tr->instr[dline]=sample;
					int	avol=AtariVolume(vol);		//prepocet na atarkovou volume
					tr->volume[dline]=avol;	//pripadne to nasledne prepise u parametru Cxx
					imark[sample].used|=(1<<trackorder[ch]);	//sampl je pouzit na kanalu "ch"
					if (note>imark[sample].maxnote) imark[sample].maxnote=note;
					if (note<imark[sample].minnote) imark[sample].minnote=note;
				}

				//efekty: PORTAMENTO
				pspeed=0;
				if (effect==0x01)	//1xx	Portamento Up
				{
					int cpor=pertable[NOTESNUM-1];	//nejvyssi nota jakou RMT umi
					tporperiod[ch]=cpor;		//cilove portamento si ulozi pro tento channel
					pspeed=param;		//portamento speed
					goto Effect3;
				}
				else
				if (effect==0x02)	//2xx	Portamento Down
				{
					int cpor=pertable[0];	//nejnizsi nota jakou RMT umi
					tporperiod[ch]=cpor;		//cilove portamento si ulozi pro tento channel
					pspeed=param;		//portamento speed
					goto Effect3;
				}
				else
				if (effect==0x05)	//5xx	continue toneportamento (+soucasne volume slide, to dela nize)
				{
					pspeed=tporspeed[ch]; //prebere speed z minula
					goto Effect3;	//stejne jako effekt 3, ale bez nastavovani portamento speed
				}
				else
				if (effect==0x03)	//3xx	TonePortamento
				{
					if (param) 
						pspeed=tporspeed[ch]=param;	//TONE portamento speed jen u parametru 3xx
					else
						pspeed=tporspeed[ch];	//je-li 300 => continue portamento posledne pouzitou rychlosti
Effect3:
					int cpor=tporperiod[ch]; //cilove portamento
					int aper=tper[ch];	//aktualni perioda
					if (aper>cpor)
					{
						//portamento smerem k mensim hodnotam, tj. nahoru k vyssim tonum
						aper -= pspeed*(ticks-1);
						if (aper<cpor) aper=cpor;	//pokud by ho prebehlo, tak srovnat
					}
					else
					if (aper<cpor)
					{
						//portamento smerem k vetsim hodnotam, tj. dolu k nizsim tonum
						aper += pspeed*(ticks-1);
						if (aper>cpor) aper=cpor;	//pokud by ho prebehlo, tak srovnat
					}

					tper[ch]=aper;
					tporon[ch]=1;	//v pristim kroku bude resit portamento
				}



				//efekty: HLASITOST
				if (effect==0x0c)	//Cxx   setvolume xx=$00-$40
				{
					if (pass==1) 
						param=(int)((double)param*imark[sample].trackvolumeincrease +0.5);
					if (param>0x40) param=0x40;	//maximalni hlasitost
					tvol[ch]=param;
					tvolslidedebt[ch]=0;		//zadny dluh
					if (param>imark[sample].trackvolumemax) imark[sample].trackvolumemax=param;

					int avol=AtariVolume(param);	//atarkova volume
					tr->volume[dline]=avol;
				}
				else
				{	//neni to Cxx => nedefinovana hlasitost
					if (note>=0 || sampleorig>0)
					{
						//nota bez hlasitosti nebo sampl bez noty => defaultni maximalni hlasitost
						int v=(vol>=0)? vol : 0x40;	//vezme bud "vol" z portamenta, nebo defaultni plnou
						tvol[ch]=v;
						imark[sample].trackvolumemax=v;
					}
				}

				//efekty: DALSI...
				if (   effect==0x0a		//Axx	volumeslide	xx=0x decrease, xx=x0 increase
					|| effect==0x05		//5xx	continue toneportamento (to uz udelal vyse)+ volumeslide xx
					|| effect==0x06		//6xx	continue vibrato + volumeslide xx => jen volumeslide xx
					)	
				{
					int vol=tvol[ch];
					double slidedivide = (note>=0 || sampleorig>0)? 2 : 1; //prvni ubytek je polovicni
					if ((param&0xf0)==0)
					{
						int voldec = (int)((double)(param&0x0f)*(ticks-1)*imark[sample].trackvolumeincrease/slidedivide +0.5);
						vol-= voldec;	//decrease
						if (slidedivide==2)	tvolslidedebt[ch]= -voldec; //dluh volume slide
					}
					else
					{
						int volinc = (int)((double)((param&0xf0)>>4)*(ticks-1)*imark[sample].trackvolumeincrease/slidedivide +0.5);
						vol+= volinc;	//increase
						if (slidedivide==2)	tvolslidedebt[ch]= volinc; //dluh volume slide
					}
					
					if (vol<0) vol=0;
					else
					if (vol>0x40) vol=0x40;
					
					tvol[ch]=vol;
					if (vol>imark[sample].trackvolumemax) imark[sample].trackvolumemax=vol;

					int avol=AtariVolume(vol);		//atarkova volume
					tr->volume[dline]=avol;
				}
				else
				if (effect==0x0f)	//Fxx   setspeed/tempo
				{
					/*
					if (param<=0x20) 
						ticks=(param>0)? param : 1; //speed 0 nelze
					else
						beats=param;
					
					int ss= (int)(((double)(125 * ticks)) / ((double)beats) + 0.5);
					if (ss<1) ss=1;
					else
					if (ss>255) ss=255;
					*/
					int ss=speedrow[sline];		//dava tam tu spolecnou predpocitanou pro cely tento radek
					tr->speed[dline]=ss;
				}
				else
				if (effect==0x0d)	//Dxx	pattern break (xx = goto to xx line in next pattern <- hm, to neumime)
				{
					//ukonci track na prvnim vyskytu Dxx shora
					//a ma pokracovat nasledujicim od pozice xx
					if (tr->len==64)
					{
						tr->len=dline+1;
						int nxp= ((int)(param/16))*10 + (param % 16);		//je to tam v 10kove soustave
						if (nxp>=0 && nxp<64 && NextPatternFromColumn==-1)
						{
							NextPatternFromRow=nxp;
							NextPatternFromColumn=ch;
						}
					}
				}
				else
				if (effect==0x0b)	//Bxx	song jump
				{
					if (tr->len==64) tr->len=dline+1;	//track break
					if (songjump<0) songjump=param;
				}

				//doresi dluh jestli nejaky je a je tam volne misto
				if (tr->volume[dline]<0)	//hlasitost neni udana
				{
					int mvol=tvol[ch];
					mvol+=tvolslidedebt[ch]; //upravi volume o dluh
					if (mvol<0) mvol=0;
					else
					if (mvol>0x40) mvol=0x40;
					int avol=AtariVolume(mvol);
					if (avol!=AtariVolume(tvol[ch])) tr->volume[dline]=avol; //pokud vyjde jina nez byla, prida ji
					tvol[ch]=mvol;
					tvolslidedebt[ch]= 0; //dluh volume slide vyresen
				}


			}//line 0-64

			//pokud se zacinalo s posunem (effekt Dxx), pak musi zkratit delku prislusneho tracku
			if (ThisPatternFromColumn==ch && tr->len==64 && dline<64) tr->len=dline;

			int cit=-1;	//cislo tracku pro song
			if (!m_tracks.IsEmptyTrack(destnum)) //je neprazdny
			{
				//Odstrani nadbytecne volume 0
				m_tracks.TrackOptimizeVol0(destnum);

				//podiva se, jestli uz takovy track neexistuje
				for(int k=0; k<destnum; k++)
				{
					if (m_tracks.CompareTracks(k,destnum))
					{
						cit=k;	//nasel takovy
						break;
					}
				}
				if (cit<0)
				{
					cit=destnum;	//nenasel
					destnum++;		//predchysta pro pristi
				}
			}

			m_song[dsline][trackorder[ch]]=cit;
			if (destnum>=TRACKSNUM)
			{
				if (pass==0) MessageBox(g_hwnd,"Out of RMT tracks. Tracks converting terminated.","Warning",MB_ICONWARNING);
				goto OutOfTracks;	//dosly tracky
			}

		}//4 tracky v patternu
		dsline++; //dalsi radek ciloveho (RMT) songu
		if (dsline>=SONGLEN)
		{
			if (pass==0) MessageBox(g_hwnd,"Out of song lines. Song converting terminated.","Warning",MB_ICONWARNING);
			goto OutOfSongLines;	//dosly Song radky
		}

		if (songjump>=0 && songjump<SONGLEN)
		{
			m_songgo[dsline]=songjump;
			dsline++;
			if (dsline>=SONGLEN)
			{
				if (pass==0) MessageBox(g_hwnd,"Out of song lines. Song converting terminated.","Warning",MB_ICONWARNING);
				goto OutOfSongLines;
			}
		}
	}
OutOfTracks:
OutOfSongLines:

	//VSECHNY PATTERNY SONGU DODELANY

	//pripravi trackvolumeincrease pro jednotlive samply, aby tim v druhem pruchodu
	//navysil hlasitosti v trackach
	if (pass==0)
	{
		for(i=1; i<=modsamples; i++)
		{
			TMODInstrumentMark *it=&imark[i];
			if (it->trackvolumemax>0) it->trackvolumeincrease=(double)0x40/(it->trackvolumemax);
		}
	}

	//---KONEC PRUCHODU 0/1---
	} //pass=0/1

	//zkoriguje jumpy v songu
	int nog=0;
	int k;
	for(i=0; i<dsline; i++)
	{
		int go=m_songgo[i];
		if (go<0) continue;
		for(k=0; k<=go && k<SONGLEN; k++)	//hleda kolik je od zacatku jumpu a za kazdy nalezeny posune go o 1 dal
		{
			if (m_songgo[k]>=0) go++;
		}
		m_songgo[i]=go;	//zapise ten posunuty skok
	}
	if (m_songgo[dsline-1]<0) { m_songgo[dsline]=restartpos; dsline++; } //smycka na zacatek nebo kam chce dle head[951]

	//doplni k instrumentum do popisu MIN MAX rozsah
	//a najde globalne nejnizsi a nejvyssi pouzitou notu ze vsech instrumentu a v celem songu
	int glonomin=NOTESNUM-1;
	int glonomax=0;
	for(i=1; i<=modsamples; i++)
	{
		if (!imark[i].used) continue;
		int minnote = imark[i].minnote;
		int maxnote = imark[i].maxnote;
		if (minnote<glonomin) glonomin=minnote;
		if (maxnote>glonomax) glonomax=maxnote;
		//maximalni hlasitost v trackach
		int avol=AtariVolume(imark[i].trackvolumemax);	//atarkova volume
		CString s;
		s.Format("%s%s%X%02X",notes[minnote],notes[maxnote],avol,imark[i].used);
		strncpy(m_instrs.m_instr[i].name+23,s,9);	//9 znaku!! 23+9 = 32
	}

	if (x_shiftdownoctave) //posun o oktavu dolu u prilis vysoko ladenych songu (pokud je to mozne)
	{
		if (glonomin>=12 && glonomax>36+5)
		{
			int noteshift=256-12;	//o oktavu niz
			for(i=1; i<=modsamples; i++)
			{
				if (!imark[i].used) continue;
				m_instrs.m_instr[i].tab[0]=(BYTE)(noteshift);
			}
		}
	}

	//imitace volume dle samplu
	int smpfrom=patstart + (maxpat+1)*patternsize;
	int samplen=0;
	int nonemptysamples=0;
	for(i=1; i<=modsamples; i++,smpfrom+=samplen)	//na konci smycky vzdycky posun na dalsi sampl
	{
		TMODInstrumentMark *im = &imark[i];
		TInstrument *rmti= &m_instrs.m_instr[i];

		samplen=im->samplen;

		if (samplen<=2)	continue;		//nulova delka => prazdny sampl

		BYTE *smpdata = new BYTE[samplen];

		if (!smpdata) continue;		//nepodarilo se alokovat
		memset(smpdata,0,samplen);	//vynuluje

		in.clear();		//kvuli dosahovani konce, kdy se nastavi eof
		in.seekg(smpfrom,ios::beg);
		if ((int)in.tellg()!=smpfrom)
		{
			CString s;
			s.Format("Can't seek sample #%02X data.",i);
			MessageBox(g_hwnd,(LPCTSTR)s,"Warning",MB_ICONWARNING);
		}
		else
		{
			in.read((char*)smpdata,samplen);
			if (in.gcount()!=samplen)
			{
				CString s;
				s.Format("Can't read fully sample #%02X data.",i);
				MessageBox(g_hwnd,(LPCTSTR)s,"Warning",MB_ICONWARNING);
			}
		}

		int minnote=im->minnote;
		int period=pertable[minnote];
		int parts=samplen/period;
		if (parts<1) parts=1;
		if (im->replen>2 || im->reppoint>0 )
		{
			//je tam smycka
			if (parts>32) parts=32;
		}
		else
		{
			//neni tam smycka
			if (parts>31) parts=31; //32 dilek si rezervuje pro ticho
		}
		int blocksize=samplen/parts;
		int blockp=blocksize-1;		//-1 aby se hranice mezi oddily posunula o 1 doleva
									//a provedlo se i zapocitani posledniho oddilu
		long sum=0;
		BYTE lastsd=0;
		long maxsum=1;	//maximalne dosazitelna suma v bloku samplu (je to 1 protoze se tim deli)

		int ix=0;
		long sumtab[32];
		for(int k=0; k<samplen; k++)
		{
			sum+= abs((int)smpdata[k]-(int)lastsd);
			lastsd=smpdata[k];		//posledni stav krivky
			if (k==blockp)			//hranice mezi oddily
			{
				sumtab[ix]= sum;
				if (sum>maxsum) maxsum=sum;	//nejvyssi
				sum=0;
				blockp+=blocksize;	//posunuti hranice o delku oddilu
				ix++;
				if (ix>=parts) break;
			}
		}

		double sampvol= (x_decreaseinstrument)? ((double)im->volume/0x3f) : 1;	//desetinne cislo 0 az 1
		if (x_volumeincrease) sampvol/=im->trackvolumeincrease;		//snizi dle toho jak zvysil volume v trackach
		for(int k=0; k<ix; k++)
		{
			int avol = (int) ((double)16*((double)sumtab[k]/maxsum)*sampvol+0.5);
			if (avol>15) avol=15;
			rmti->env[k][ENV_VOLUMEL]=rmti->env[k][ENV_VOLUMER]=avol;
			rmti->env[k][ENV_DISTORTION]=0x0a;	//cisty ton
		}

		rmti->par[PAR_ENVLEN]=ix-1;
		if (im->replen>2 || im->reppoint>0 ) //je tam nejaka smycka?
		{
			//smycka
			int ego= (int)((double)im->reppoint/blocksize +0.5);
			if (ego>ix-1) ego=ix-1;
			rmti->par[PAR_ENVGO]=ego;
			//a predela konec instrumentu podle delky loopu
			int lopend= (int)((double)(im->reppoint+im->replen)/blocksize +0.5);
			if (lopend>ix-1) lopend=ix-1;
			rmti->par[PAR_ENVLEN];
		}
		else
		{
			//bez smycky (ix je max 31, aby mohl pridat na konec "tichou smycku")
			rmti->env[ix][ENV_VOLUMEL]=rmti->env[ix][ENV_VOLUMER]=0; //ticho na konci
			rmti->par[PAR_ENVLEN]=ix;	//delka o 1 vic
			rmti->par[PAR_ENVGO]=ix;	//skok na to samo
		}

		//Fourier
		/*

		if (x_fourier)
		{
#define	F_BLOCK		4096
#define F_SAMPLELEN	5*F_BLOCK
			Fft *myFFT=NULL;
			BYTE fsample[F_SAMPLELEN];	//bere prvnich 5 bloku => 20480 => cca 1 sekunda
			int sapos=0,safro=0,salen=im->samplen;
			while(sapos<F_SAMPLELEN)	// && im->replen>2
			{
				int lenb = (sapos+salen<F_SAMPLELEN)? salen : F_SAMPLELEN-sapos;
				memcpy(fsample+sapos,smpdata+safro,lenb);
				sapos+=lenb;
				if (im->replen>2 || im->reppoint>0)
				{
					safro = im->reppoint;
					salen= im->replen;
				}
			}

			int zaknota=0;
			if (rmti->par[PAR_TABLEN]==0) zaknota=rmti->tab[0];


			for(int i=0; i<flen/F_BLOCK; i++)
			{
				BYTE *bsample=fsample+i*F_BLOCK;
				if (myFFT) delete myFFT;
				myFFT = new Fft(4096,22050);
				if (!myFFT) break;
				for(int j=0; j<F_BLOCK; j++) myFFT->PutAt(j,bsample[j]-128);
				myFFT->Transform();

				int no[3];
				int p=myFFT->GetNotes(no[0],no[1],no[2]);
				if (p>0)
				{
					for(int j=0; j<p; j++) rmti->tab[j]=zaknota+no[j]%12;
					rmti->par[PAR_TABLEN]=p-1;
					rmti->par[PAR_TABGO]=0;
					rmti->par[PAR_TABTYPE]=0;	//tabulka not
					rmti->par[PAR_TABMODE]=0;	//nastavovani
					rmti->par[PAR_TABSPD]=1;
				}
				if (myFFT) 
				{ 
					delete myFFT;
					myFFT=NULL;
				}
			}
		}
		//konec Fouriera		
		*/

		//pocet neprazdnych samplu
		nonemptysamples++;

		//uvolni pamet
		if (smpdata) { delete[] smpdata; smpdata=NULL; }

	}

	//kontrola konce modulu s koncem posledniho samplu
	if (smpfrom!=modulelength)
	{
		//lisi se
		CString s;
		s.Format("Bad length of module.\n(Last sample's end is at %i, but length of module is %i.)",smpfrom,modulelength);
		MessageBox(g_hwnd, s,"Warning",MB_ICONWARNING);
	}

	//a az tedka na konci
	for(i=1; i<=modsamples; i++)
	{
		//promitnout do Atarka
		m_instrs.ModificationInstrument(i);
	}

	//UVOLNENI PAMETI
	if (mem) {	delete[] mem;	mem=NULL; }

	//ZAVERECNY DIALOG PO DOKONCENI IMPORTU
	CImportModFinishedDlg imfdlg;
	imfdlg.m_info.Format("%i tracks, %i instruments, %i songlines",destnum,nonemptysamples,dsline);

	//OPTIMIZATIONS
	if (x_optimizeloops)
	{
		int optitracks=0,optibeats=0;
		TracksAllBuildLoops(optitracks,optibeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Loops in %i tracks (%i beats/lines)",optitracks,optibeats);
		imfdlg.m_info+=s;
	}

	if (x_truncateunusedparts)
	{
		int clearedtracks=0,truncatedtracks=0,truncatedbeats=0;
		SongClearUnusedTracksAndParts(clearedtracks,truncatedtracks,truncatedbeats);
		CString s;
		s.Format("\x0d\x0aOptimization: Cleared %i, truncated %i tracks (%i beats/lines)",clearedtracks,truncatedtracks,truncatedbeats);
		imfdlg.m_info+=s;
	}

	if (imfdlg.DoModal()!=IDOK)
	{
		//nedal Ok, takze to smaze
		g_tracks4_8=originalg_tracks4_8;	//vrati puvodni hodnotu
		ClearSong(g_tracks4_8);
		MessageBox(g_hwnd,"Module import aborted.","Import...",MB_ICONINFORMATION);
	}

	return 1;
}