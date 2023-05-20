// EffectsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Rmt.h"
#include "EffectsDlg.h"

#include "Song.h"
#include "IOHelpers.h"
#include "global.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CInstruments	g_Instruments;
extern CTracks g_Tracks;
extern CSong g_Song;

/////////////////////////////////////////////////////////////////////////////
// CEffectsDlg dialog


CEffectsDlg::CEffectsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEffectsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEffectsDlg)
	m_info = _T("");
	//}}AFX_DATA_INIT
}


void CEffectsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEffectsDlg)
	DDX_Control(pDX, IDC_EFF_P3, m_p3);
	DDX_Control(pDX, IDC_EFF_P2, m_p2);
	DDX_Control(pDX, IDC_EFF_P1, m_p1);
	DDX_Control(pDX, IDC_EFF_EDIT3, m_edit3);
	DDX_Control(pDX, IDC_EFF_EDIT2, m_edit2);
	DDX_Control(pDX, IDC_EFF_EDIT1, m_edit1);
	DDX_Control(pDX, IDC_EFF_COMBO, m_eff_combo);
	DDX_Text(pDX, IDC_INFO, m_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEffectsDlg, CDialog)
	//{{AFX_MSG_MAP(CEffectsDlg)
	ON_CBN_SELCHANGE(IDC_EFF_COMBO, OnSelchangeEffCombo)
	ON_BN_CLICKED(IDDEFAULT, OnDefault)
	ON_BN_CLICKED(IDTRY, OnTry)
	ON_BN_CLICKED(IDRESTORE, OnRestore)
	ON_BN_CLICKED(IDPLAYSTOP, OnPlaystop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEffectsDlg message handlers

#define NUMBEROFEFFECT	6

struct TEffs {
	const char* name;
	const char* p1;
	const char* e1;
	const char* p2;
	const char* e2;
	const char* p3;
	const char* e3;
};

TEffs effects[NUMBEROFEFFECT]={
	{	
		"Fade in/out",
		"Initial volume level, 0-100 (%)",
		"100",
		"Final volume level, 0-100 (%)",
		"100",
		"Line step",
		"1"
	},
	{
		"Modify notes, instruments and volume values",
		"Notes tuning (+- semitones)",
		"0",
		"Instruments used (+- offset value)",
		"0",
		"Volume changes (%)",
		"100"
	},
	{
		"Echo",
		"Delay (lines)",
		"3",
		"Fade out level 0-100 (%), or V1-V15 for linear volume subtraction",
		"20",
		"Minimal volume 0-15, or !0-!15 for ending echo on minimal volume",
		"1"
	},
	{
		"Expand/shrink lines",
		"From step (negative values for bottom-up way)",
		"1",
		"To step (negative values for bottom-up way)",
		"2",
		"",
		""
	},
	{
		"Volume humanize",
		"Random level 0-100 (%)",
		"30",
		"Minimal volume 0-15",
		"1",
		"Line step",
		"1"
	},
	{
		"Volume set/remove",
		"Volume range - minimum 0-15",
		"0",
		"Volume range - maximum 0-15",
		"15",
		"Set volume to 0-15, or 'X' to remove whole note events",
		"15"
	}
};

int g_effai=0;
CString eff_ed[NUMBEROFEFFECT][3];

BOOL CEffectsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	//a copy of the original track to m_trackorig
	memcpy(m_trackorig,m_trackptr,sizeof(TTrack));

	m_effai = g_effai;

	for(int i=0; i<NUMBEROFEFFECT; i++)
	{
		m_eff_combo.AddString(effects[i].name);
	}

	m_eff_combo.SetCurSel(m_effai);
	OnSelchangeEffCombo();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEffectsDlg::OnSelchangeEffCombo() 
{
	m_effai=m_eff_combo.GetCurSel();
	m_p1.SetWindowText(effects[m_effai].p1);

	m_p2.SetWindowText(effects[m_effai].p2);
	m_edit2.EnableWindow(effects[m_effai].p2[0]!=0);

	m_p3.SetWindowText(effects[m_effai].p3);
	m_edit3.EnableWindow(effects[m_effai].p3[0]!=0);

	if (eff_ed[m_effai][0]=="") 
		OnDefault();	//if the P1 parameter is empty, then set all defaults
	else
	{
		m_edit1.SetWindowText(eff_ed[m_effai][0]);
		m_edit2.SetWindowText(eff_ed[m_effai][1]);
		m_edit3.SetWindowText(eff_ed[m_effai][2]);
	}
}

void ZpracujChPar(CString& s,char& ch,int& par) //"ProcessChPar" in English lol
{
	//s is, for example, "0" , "15", "-5", "100", "E10", "E -5", "X"
	//split it into the first character (char) and number (integer)
	int len=s.GetLength();
	int i;
	char a;
	ch=0;
	par=0;
	for(i=0; i<len; i++)
	{
		a=s[i];
		if (a>='a' && a<='z') a-='a'-'A';
		if ((a>='0' && a<='9') || a=='-')
		{
			par=atoi( ((LPCTSTR)s)+i );
			return;
		}
		if (ch==0 && a!=' ') ch=a;
	}
}

void CEffectsDlg::OnOK() 
{
	PerformEffect();
	g_effai = m_effai;
	CDialog::OnOK();
}

void CEffectsDlg::OnCancel() 
{
	OnRestore();
	CDialog::OnCancel();
}

void CEffectsDlg::OnDefault() 
{
	m_edit1.SetWindowText(effects[m_effai].e1);
	m_edit2.SetWindowText(effects[m_effai].e2);
	m_edit3.SetWindowText(effects[m_effai].e3);
	eff_ed[m_effai][0]=effects[m_effai].e1;
	eff_ed[m_effai][1]=effects[m_effai].e2;
	eff_ed[m_effai][2]=effects[m_effai].e3;
}

void CEffectsDlg::OnTry() 
{
	PerformEffect();	
}

void CEffectsDlg::OnRestore() 
{
	memcpy(m_trackptr,m_trackorig,sizeof(TTrack));
}

void CEffectsDlg::OnPlaystop() 
{
	if (g_Song.GetPlayMode() != MPLAY_STOP)
		g_Song.Stop();
	else
		g_Song.Play(MPLAY_BLOCK, g_Song.GetFollowPlayMode());
}

void CEffectsDlg::PerformEffect()
{
	int i,j,h;

	int bfro = m_bfro;
	int bto = m_bto;
	int p1,p2,p3;
	char ch1,ch2,ch3;
	
	CString s;
	m_edit1.GetWindowText(s);
	eff_ed[m_effai][0]=s;
	ZpracujChPar(s,ch1,p1);
	m_edit2.GetWindowText(s);
	eff_ed[m_effai][1]=s;
	ZpracujChPar(s,ch2,p2);
	m_edit3.GetWindowText(s);
	eff_ed[m_effai][2]=s;
	ZpracujChPar(s,ch3,p3);

	TTrack td;
	memcpy(&td,m_trackorig,sizeof(TTrack));
	
	float fvolume[TRACKLEN]; //volume in real numbers
	for(i=bfro; i<=bto; i++) fvolume[i]=(float)td.volume[i];

	int continstr[TRACKLEN]; //the instrument numbers are continuous
	int lasti=-1;
	for(i=0; i<=bto; i++)
	{
		if (td.instr[i]>=0) lasti=td.instr[i];
		if (i>=bfro) continstr[i]=lasti;
	}

	TTrack tempt;	//auxiliary empty track
	for (i=0;i<TRACKLEN;i++) tempt.note[i]=tempt.instr[i]=tempt.volume[i]=tempt.speed[i]=-1;

	switch(m_effai)
	{
	case 0:	//fade in/out: initial volume level %, final vol.level %, line step
		{
			if (p3<=0) break;
			for(i=bfro; i<=bto; i+=p3)	//line step p3
			{
				if (!m_all && m_ainstr!=continstr[i]) continue;
				if (td.volume[i]<0) continue;		//never without volume
				float proc=(float)p1/100;
				if (i>0) proc+=(float)(p2-p1)/(bto-bfro)*(i-bfro)/100;
				h=(int)(proc*td.volume[i]+0.5);	//volume change (rounded)
				if (h<0) h=0;
				else
				if (h>15) h=15;
				td.volume[i]=h;
			}
		}
		break;

	case 1: //change notes, instruments and volumes: note+-, instr+-, volume%
		{
			int ai= (m_all)? -1 : m_ainstr;
			g_Tracks.ModifyTrack(&td,bfro,bto,ai,p1,p2,p3);
		}
		break;

	case 2: //echo: delay, fadeout level %, minimal volume 0..15 or !1..!15 echo ending volume
		{
			float dvol=0;
			if (ch2=='V')
			{	//linear calculations
				if (p2<-15) p2=-15;
				if (p2>15) p2=15;
			}
			else
			{	//percentage calculations
				dvol = (1-((float)p2/100));
				if (dvol<-15) dvol=-15;
				if (dvol>15) dvol=15;
			}
			float nv;
			int ph;

			for(i=bfro; i<=bto; i++)
			{
				if (td.note[i]<0) continue;	//there is no note
				if (!m_all && td.instr[i]!=m_ainstr) continue;	//not for this instrument
				j=i+p1;	//echo for p1
				if (j<bfro || j>bto) continue;		//echo is coming out of the block
				if (td.note[j]>=0) continue;	//there is already a note in the final place

				if (ch2=='V')
					nv=fvolume[i]-p2;	//linear calculations
				else
					nv=fvolume[i]*dvol; //percentage calculations
				ph=(int)(fvolume[i]+0.5); //original volume (rounded to the nearest)
				h=(int)(nv+0.5); //new volume (rounded to the nearest)

				if (ch3=='!')
				{	//ending volume
					if ( ph<=p3 ) continue;
				}

				if (h<p3) h=p3;	//minimal volume p3
				if (h<0) h=0;
				else
				if (h>15) h=15;

				fvolume[j]=nv;				//volume in real numbers
				td.note[j]=td.note[i];		//copies the note
				td.instr[j]=td.instr[i];	//the same instrument
				td.volume[j]=h;				//corresponding volume
			}
		}
		break;

	case 3: //expand/shrink lines: from step, to step
		{
			if (p1==0 && p2==0) break;
			int lenb=bto-bfro;
			for(i=(p1>=0)?0:lenb,j=(p2>=0)?0:lenb; i>=0 && i<=lenb && j>=0 && j<=lenb; i+=p1,j+=p2)
			{
				if (!m_all && td.instr[bfro+i]!=m_ainstr) continue;	//not for this instrument
				tempt.note[j]=td.note[bfro+i];
				tempt.instr[j]=td.instr[bfro+i];
				tempt.volume[j]=td.volume[bfro+i];
				tempt.speed[j]=td.speed[bfro+i];
			}
			for(i=0; i<=lenb; i++)
			{
				td.note[bfro+i]=tempt.note[i];
				td.instr[bfro+i]=tempt.instr[i];
				td.volume[bfro+i]=tempt.volume[i];
				td.speed[bfro+i]=tempt.speed[i];
			}
		}
		break;

	case 4: //volume humanize: random level %, minimal volume, line step
		{

			if (p1<0) p1=0;
			if (p1>100) p1=100;
			if (p2<0) p2=0;
			if (p3<=0) break;

			int vol;
			for(i=bfro; i<=bto; i+=p3)
			{
				vol=td.volume[i];
				if (!m_all && m_ainstr!=continstr[i]) continue;	//not for this instrument
				if (vol<0) continue;	//if there is no volume

				float dol= ( vol - (float)p1/100*15 );
				if (dol<p2) dol=(float)p2;
				dol-=0.5;

				float hor= ( vol + (float)p1/100*15 );
				if (hor>15) hor=15;
				hor+=0.5;

				if ( hor <= dol ) continue;
				
				int nv=(int)( 0.5 + dol + (float)((hor-dol)*(((float)(rand()%1000))/1000)) );

				if (nv<p2) nv=p2;
				if (nv>15) nv=15;
				td.volume[i]=nv;
			}
		}
		break;

	case 5: //Volume set / remove (ch3=='X')
		{
			if (p1<0) p1=0;
			if (p1>15) p1=15;
			if (p2<0) p2=0;
			if (p2>15) p2=15;
			if (p3<0) p3=0;
			if (p3>15) p3=15;
			int vol;
			for(i=bfro; i<=bto; i++)
			{
				vol=td.volume[i];
				if (!m_all && m_ainstr!=continstr[i]) continue;	//not for this instrument
				if (vol<0) continue;	//if there is no volume

				if (p1<=vol && vol<=p2)
				{
					if (ch3=='X')
						td.note[i]=td.instr[i]=td.volume[i]=-1; //clear
					else
					if (p3>=0 && p3<=15)
						td.volume[i]=p3;
				}
			}
		}
		break;

	default:
		break;	
	}

	//copies to the actual track
	memcpy(m_trackptr,&td,sizeof(TTrack));
}

/////////////////////////////////////////////////////////////////////////////
// CSongTracksOrderDlg dialog


CSongTracksOrderDlg::CSongTracksOrderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSongTracksOrderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSongTracksOrderDlg)
	m_songlinefrom = _T("");
	m_songlineto = _T("");
	//}}AFX_DATA_INIT
}


void CSongTracksOrderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSongTracksOrderDlg)
	DDX_Text(pDX, IDC_SONGLINEFROM, m_songlinefrom);
	DDX_Text(pDX, IDC_SONGLINETO, m_songlineto);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSongTracksOrderDlg, CDialog)
	//{{AFX_MSG_MAP(CSongTracksOrderDlg)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
	ON_BN_CLICKED(IDC_MONOSTEREO, OnMonostereo)
	ON_BN_CLICKED(IDC_L1, OnL1R4)
	ON_BN_CLICKED(IDC_STEREOMONO, OnStereomono)
	ON_BN_CLICKED(IDC_NOTHING, OnNothing)
	ON_BN_CLICKED(IDC_L2, OnL1R4)
	ON_BN_CLICKED(IDC_L3, OnL1R4)
	ON_BN_CLICKED(IDC_L4, OnL1R4)
	ON_BN_CLICKED(IDC_R1, OnL1R4)
	ON_BN_CLICKED(IDC_R2, OnL1R4)
	ON_BN_CLICKED(IDC_R3, OnL1R4)
	ON_BN_CLICKED(IDC_R4, OnL1R4)
	ON_BN_CLICKED(IDC_L1D, OnL1R4)
	ON_BN_CLICKED(IDC_L2D, OnL1R4)
	ON_BN_CLICKED(IDC_L3D, OnL1R4)
	ON_BN_CLICKED(IDC_L4D, OnL1R4)
	ON_BN_CLICKED(IDC_R1D, OnL1R4)
	ON_BN_CLICKED(IDC_R2D, OnL1R4)
	ON_BN_CLICKED(IDC_R3D, OnL1R4)
	ON_BN_CLICKED(IDC_R4D, OnL1R4)
	ON_BN_CLICKED(IDC_COPYLEFTRIGHT, OnCopyleftright)
	ON_BN_CLICKED(IDC_COPYRIGHTLEFT, OnCopyrightleft)
	ON_BN_CLICKED(IDC_CLEARALL, OnClearall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSongTracksOrderDlg message handlers

BOOL CSongTracksOrderDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_fromtrack=m_totrack=-1;
	if (g_tracks4_8<=4)
	{
		for(int i=0; i<4; i++)
		{
			GetDlgItem(IDC_R1+i)->EnableWindow(0);
			GetDlgItem(IDC_R1D+i)->EnableWindow(0);
		}
		GetDlgItem(IDC_MONOSTEREO)->EnableWindow(0);
		GetDlgItem(IDC_STEREOMONO)->EnableWindow(0);
		GetDlgItem(IDC_COPYLEFTRIGHT)->EnableWindow(0);
		GetDlgItem(IDC_COPYRIGHTLEFT)->EnableWindow(0);
	}
	OnDefault();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSongTracksOrderDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	const int tids[8]={ IDC_L1,IDC_L2,IDC_L3,IDC_L4,IDC_R1,IDC_R2,IDC_R3,IDC_R4 };
	const int tidd[8]={ IDC_L1D,IDC_L2D,IDC_L3D,IDC_L4D,IDC_R1D,IDC_R2D,IDC_R3D,IDC_R4D };

	for(int i=0; i<g_tracks4_8; i++)
	{
		int z=m_tracksorder[i];
		if (z>=0)
		{
			CRect rects,rectd;
			((CWnd*)GetDlgItem(tids[z]))->GetWindowRect(&rects);
			ScreenToClient(&rects);
			((CWnd*)GetDlgItem(tidd[i]))->GetWindowRect(&rectd);
			ScreenToClient(&rectd);
			dc.MoveTo((rects.left+rects.right)/2,rects.bottom);		//rects.CenterPoint()
			dc.LineTo((rectd.left+rectd.right)/2,rectd.top);		//rectd.CenterPoint());
		}
	}
	
	// Do not call CDialog::OnPaint() for painting messages
}

void CSongTracksOrderDlg::OnDefault() 
{
	for(int i=0; i<8; i++) m_tracksorder[i]=i;
	Invalidate();
}

void CSongTracksOrderDlg::OnMonostereo() 
{
	const int ttt[8]={0,3,4,7, 1,2,5,6};
	for(int i=0; i<8; i++) m_tracksorder[i]=ttt[i];
	Invalidate();
}

void CSongTracksOrderDlg::OnStereomono() 
{
	const int ttt[8]={0,4,5,1, 2,6,7,3};
	for(int i=0; i<8; i++) m_tracksorder[i]=ttt[i];
	Invalidate();
}

void CSongTracksOrderDlg::OnCopyleftright() 
{
	for(int i=0; i<8; i++) m_tracksorder[i]=i%4;
	Invalidate();
}

void CSongTracksOrderDlg::OnCopyrightleft() 
{
	for(int i=0; i<8; i++) m_tracksorder[i]=i%4+4;
	Invalidate();
}

void CSongTracksOrderDlg::OnClearall() 
{
	for(int i=0; i<8; i++) m_tracksorder[i]=-1;
	Invalidate();
}


void CSongTracksOrderDlg::OnL1R4() 
{
	CWnd* wnd=GetFocus();
	int num = wnd->GetDlgCtrlID() - IDC_L1;
	if (num>=0 && num<8)
	{
		m_fromtrack=num;
	}
	else
	if (num>=8 && num<16)
	{
		m_totrack=num- (IDC_L1D-IDC_L1);
		m_tracksorder[m_totrack]=m_fromtrack;
		Invalidate();
	}
}

void CSongTracksOrderDlg::OnNothing() 
{
	m_fromtrack=-1;	
}


/////////////////////////////////////////////////////////////////////////////
// CInstrumentChangeDlg dialog


CInstrumentChangeDlg::CInstrumentChangeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentChangeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInstrumentChangeDlg)
	m_combo1 = -1;
	m_combo2 = -1;
	m_combo3 = -1;
	m_combo4 = -1;
	m_combo5 = -1;
	m_combo6 = -1;
	m_combo7 = -1;
	m_combo8 = -1;
	m_combo9 = -1;
	m_combo10 = -1;
	m_combo11 = -1;
	m_combo12 = -1;
	//}}AFX_DATA_INIT
}


void CInstrumentChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstrumentChangeDlg)
	DDX_Control(pDX, IDC_CHECK12, m_check6);
	DDX_Control(pDX, IDC_EDIT2, m_edit2);
	DDX_Control(pDX, IDC_EDIT1, m_edit1);
	DDX_Control(pDX, IDC_CHECK9, m_check5);
	DDX_Control(pDX, IDC_CHECKONEINSTRUMENT, m_checkoneinstr);
	DDX_Control(pDX, IDC_TITLE2, m_ctitle2);
	DDX_Control(pDX, IDC_TITLE, m_ctitle);
	DDX_Control(pDX, IDC_CHECK4, m_check4);
	DDX_Control(pDX, IDC_CHECK3, m_check3);
	DDX_Control(pDX, IDC_CHECK2, m_check2);
	DDX_Control(pDX, IDC_CHECK1, m_check1);
	DDX_CBIndex(pDX, IDC_COMBO1, m_combo1);
	DDX_CBIndex(pDX, IDC_COMBO2, m_combo2);
	DDX_CBIndex(pDX, IDC_COMBO3, m_combo3);
	DDX_CBIndex(pDX, IDC_COMBO4, m_combo4);
	DDX_CBIndex(pDX, IDC_COMBO5, m_combo5);
	DDX_CBIndex(pDX, IDC_COMBO6, m_combo6);
	DDX_CBIndex(pDX, IDC_COMBO7, m_combo7);
	DDX_CBIndex(pDX, IDC_COMBO8, m_combo8);
	DDX_CBIndex(pDX, IDC_COMBO9, m_combo9);
	DDX_CBIndex(pDX, IDC_COMBO10, m_combo10);
	DDX_CBIndex(pDX, IDC_COMBO11, m_combo11);
	DDX_CBIndex(pDX, IDC_COMBO12, m_combo12);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInstrumentChangeDlg, CDialog)
	//{{AFX_MSG_MAP(CInstrumentChangeDlg)
	ON_BN_CLICKED(IDC_BUTTON1, OnDefault)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO11, OnSelchangeComboInstrs)
	ON_BN_CLICKED(IDC_CHECK1, OnSameNoteRanges)
	ON_BN_CLICKED(IDC_CHECK2, OnSameVolumeRange)
	ON_BN_CLICKED(IDC_CHECK3, OnSameInstrRange)
	ON_BN_CLICKED(IDC_BUTTON2, OnFullRanges)
	ON_BN_CLICKED(IDC_CHECKONEINSTRUMENT, OnCheckoneinstrument)
	ON_BN_CLICKED(IDC_CHECK9, OnCheckSomeChannelsOnly)
	ON_BN_CLICKED(IDC_CHECK4, OnCheckTrackOnly)
	ON_CBN_SELCHANGE(IDC_COMBO2, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO3, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO4, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO5, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO6, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO7, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO8, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO9, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO10, OnSelchangeComboX)
	ON_CBN_SELCHANGE(IDC_COMBO12, OnSelchangeComboInstrs)
	ON_BN_CLICKED(IDC_CHECK12, OnCheckSomeSonglinesOnly)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstrumentChangeDlg message handlers

BOOL CInstrumentChangeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int i;
	for(i=0; i<NOTESNUM; i++)
	{
		((CComboBox*)GetDlgItem(IDC_COMBO1))->AddString(notes[i]);
		((CComboBox*)GetDlgItem(IDC_COMBO2))->AddString(notes[i]);
		((CComboBox*)GetDlgItem(IDC_COMBO5))->AddString(notes[i]);
		((CComboBox*)GetDlgItem(IDC_COMBO6))->AddString(notes[i]);
	}
	((CComboBox*)GetDlgItem(IDC_COMBO6))->AddString("---");
	CString s;
	for(i=0; i<=15; i++)
	{
		s.Format("%X",i);
		((CComboBox*)GetDlgItem(IDC_COMBO3))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO4))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO7))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO8))->AddString((LPCTSTR)s);
	}
	((CComboBox*)GetDlgItem(IDC_COMBO8))->AddString("---");
	for(i=0; i<INSTRSNUM; i++)
	{
		s.Format("%02X",i);
		((CComboBox*)GetDlgItem(IDC_COMBO9))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO10))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO11))->AddString((LPCTSTR)s);
		((CComboBox*)GetDlgItem(IDC_COMBO12))->AddString((LPCTSTR)s);
	}
	((CComboBox*)GetDlgItem(IDC_COMBO10))->AddString("---");

	if (m_onlytrack>=0)
		s.Format("Only in current track ($%02X)",m_onlytrack);
	else
	{
		s="Only in current track";
		m_check4.EnableWindow(0);
	}
	m_check4.SetWindowText(s);

	m_onlychannels=-1;

	s.Format("%02X",m_onlysonglinefrom);
	m_edit1.SetWindowText(s);
	s.Format("%02X",m_onlysonglineto);
	m_edit2.SetWindowText(s);
	m_edit1.EnableWindow(0);
	m_edit2.EnableWindow(0);

	((CComboBox*)GetDlgItem(IDC_COMBO11))->SetCurSel(m_combo11);
	((CComboBox*)GetDlgItem(IDC_COMBO12))->SetCurSel(m_combo12);

	m_checkoneinstr.SetCheck(1);

	OnDefault();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentChangeDlg::OnDefault() 
{
	int instrfrom,instrto;
	instrfrom=((CComboBox*)GetDlgItem(IDC_COMBO11))->GetCurSel();
	instrto=((CComboBox*)GetDlgItem(IDC_COMBO12))->GetCurSel();

	if (m_checkoneinstr.GetCheck() || instrto<instrfrom)	instrto=instrfrom;

	TInstrInfo iinfo;
	//m_song->InstrInfo(instrfrom,&iinfo,instrto);
	g_Song.InstrInfo(instrfrom, &iinfo, instrto);
	int count = iinfo.count;

	if (!count)
	{
		iinfo.minnote=0;
		iinfo.maxnote=NOTESNUM-1;
		iinfo.minvol=0;
		iinfo.maxvol=MAXVOLUME;
	}

	int noftracks = iinfo.usedintracks;
	CString s;
	if (instrfrom==instrto)
		s.Format("Instrument: %02X\tName: %s",instrfrom, g_Instruments.GetName(instrfrom));
	else
		s.Format("Instruments %02X-%02X (%u)",instrfrom,instrto,instrto-instrfrom+1);
	m_ctitle.SetWindowText(s);
	s.Format("Used in %u tracks, globally %u times.",noftracks,count);
	m_ctitle2.SetWindowText(s);

	m_check1.SetCheck(1);
	m_check2.SetCheck(1);
	m_check3.SetCheck(1);

	((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(iinfo.minnote);
	((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(iinfo.maxnote);
	((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(iinfo.minvol);
	((CComboBox*)GetDlgItem(IDC_COMBO4))->SetCurSel(iinfo.maxvol);

	((CComboBox*)GetDlgItem(IDC_COMBO5))->SetCurSel(iinfo.minnote);
	((CComboBox*)GetDlgItem(IDC_COMBO6))->SetCurSel(iinfo.maxnote);
	((CComboBox*)GetDlgItem(IDC_COMBO7))->SetCurSel(iinfo.minvol);
	((CComboBox*)GetDlgItem(IDC_COMBO8))->SetCurSel(iinfo.maxvol);

	((CComboBox*)GetDlgItem(IDC_COMBO9))->SetCurSel(instrfrom);
	((CComboBox*)GetDlgItem(IDC_COMBO10))->SetCurSel(instrto);
	((CComboBox*)GetDlgItem(IDC_COMBO11))->SetCurSel(instrfrom);
	((CComboBox*)GetDlgItem(IDC_COMBO12))->SetCurSel(instrto);

	SelChangeComboX();
}

void CInstrumentChangeDlg::OnFullRanges() 
{
	//All instruments

	m_check1.SetCheck(1);
	m_check2.SetCheck(1);
	m_check3.SetCheck(1);
	m_checkoneinstr.SetCheck(0);

	TInstrInfo iinfo;
	g_Song.InstrInfo(0, &iinfo, INSTRSNUM - 1);
	if (!iinfo.count)
	{
		((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(NOTESNUM-1);
		((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO4))->SetCurSel(15);
		
		((CComboBox*)GetDlgItem(IDC_COMBO5))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO6))->SetCurSel(NOTESNUM-1);
		((CComboBox*)GetDlgItem(IDC_COMBO7))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO8))->SetCurSel(15);
		
		((CComboBox*)GetDlgItem(IDC_COMBO11))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO12))->SetCurSel(INSTRSNUM-1);
		((CComboBox*)GetDlgItem(IDC_COMBO9))->SetCurSel(0);
		((CComboBox*)GetDlgItem(IDC_COMBO10))->SetCurSel(INSTRSNUM-1);
	}
	else
	{
		((CComboBox*)GetDlgItem(IDC_COMBO11))->SetCurSel(iinfo.instrfrom);
		((CComboBox*)GetDlgItem(IDC_COMBO12))->SetCurSel(iinfo.instrto);
	}
	OnDefault();
}

void CInstrumentChangeDlg::OnSelchangeComboX()
{
	SelChangeComboX();
}

void CInstrumentChangeDlg::OnSelchangeComboInstrs()
{
	OnDefault();
}

void CInstrumentChangeDlg::SelChangeComboX()
{
	int i,c[12];
	static int idcombo[12]={IDC_COMBO1,IDC_COMBO2,IDC_COMBO3,IDC_COMBO4,IDC_COMBO5,IDC_COMBO6,IDC_COMBO7,IDC_COMBO8,IDC_COMBO9,IDC_COMBO10,IDC_COMBO11,IDC_COMBO12};
	for(i=0; i<12; i++) c[i]=((CComboBox*)GetDlgItem(idcombo[i]))->GetCurSel();

	if (c[1]<c[0]) c[1]=c[0];	//noteto<notefrom
	if (c[3]<c[2]) c[3]=c[2];	//volumemax<volumemin
	if (c[5]<c[4]) c[5]=c[4];	//noteto<notefrom
	if (c[7]<c[6]) c[7]=c[6];	//volumemax<volumemin
	if (c[9]<c[8]) c[9]=c[8];	//instrto<instrfrom
	if (c[11]<c[10]) c[11]=c[10]; //instrto<instrfrom

	if (m_check1.GetCheck())
	{	//same volume range
		int j=c[1]-c[0]+c[4];
		if (j>NOTESNUM) j=NOTESNUM;		//an "---" item is added at the end
		c[5]=j;	
	}

	if (m_check2.GetCheck())
	{	//same volume range
		int j=c[3]-c[2]+c[6];
		if (j>16) j=16;					//16th item "---" is added at the end of 0-15
		c[7]=j;	
	}

	if (m_check3.GetCheck() || m_checkoneinstr.GetCheck())
	{
		//same instrument range || only one instrument
		if (m_checkoneinstr.GetCheck()) c[11]=c[10];
		int j=c[11]-c[10]+c[8];
		if (j>INSTRSNUM) j=INSTRSNUM;	//an "---" item is added at the end
		c[9]=j;
	}

	((CComboBox*)GetDlgItem(IDC_COMBO6))->EnableWindow(!m_check1.GetCheck());
	((CComboBox*)GetDlgItem(IDC_COMBO8))->EnableWindow(!m_check2.GetCheck());
	((CComboBox*)GetDlgItem(IDC_COMBO10))->EnableWindow(!m_check3.GetCheck() && !m_checkoneinstr.GetCheck());

	int ch=m_checkoneinstr.GetCheck();
	((CComboBox*)GetDlgItem(IDC_COMBO12))->EnableWindow(!ch);
	m_check3.EnableWindow(!ch);

	for(i=0; i<12; i++) ((CComboBox*)GetDlgItem(idcombo[i]))->SetCurSel(c[i]);
}

void CInstrumentChangeDlg::OnSameNoteRanges() 
{
	SelChangeComboX();
}

void CInstrumentChangeDlg::OnSameVolumeRange() 
{
	SelChangeComboX();
}

void CInstrumentChangeDlg::OnSameInstrRange() 
{
	SelChangeComboX();
}

void CInstrumentChangeDlg::OnCheckoneinstrument() 
{
	if (m_checkoneinstr.GetCheck())
	{
		OnDefault();
	}
	else
		SelChangeComboX();
}

void CInstrumentChangeDlg::OnCheckTrackOnly() 
{
	//mutually excluded check4 and check5+6
	if (m_check4.GetCheck())
	{
		m_check5.SetCheck(0);
		m_check5.SetWindowText("Only in some channels");
		m_onlychannels=-1; //all
		m_check6.SetCheck(0);
		m_edit1.EnableWindow(0);
		m_edit2.EnableWindow(0);
	}
}

void CInstrumentChangeDlg::OnCheckSomeChannelsOnly() 
{
	//mutually excluded check4 and check5+6
	if (m_check5.GetCheck())
	{
		m_check4.SetCheck(0);
		CChannelsSelectionDlg dlg;
		if (dlg.DoModal()==IDOK && dlg.m_channelyes>0)
		{
			m_onlychannels=dlg.m_channelyes;
			const char* cnames[] = { "L1","L2","L3","L4","R1","R2","R3","R4" };
			CString s;
			s="Only in ";
			int j=0;
			for(int i=0; i<g_tracks4_8; i++)
			{
				if (m_onlychannels&(1<<i))
				{ 
					if (j) s+=",";
					s+=cnames[i];
					j++;
				}
			}
			m_check5.SetWindowText(s);
		}
		else
			m_check5.SetCheck(0);
	}
	if (!m_check5.GetCheck())
	{
		m_check5.SetWindowText("Only in some channels");
		m_onlychannels=-1; //all
	}
}

void CInstrumentChangeDlg::OnCheckSomeSonglinesOnly() 
{
	int check6=m_check6.GetCheck();
	if (check6)
	{
		m_check4.SetCheck(0);
	}
	m_edit1.EnableWindow(check6);
	m_edit2.EnableWindow(check6);
}


void CInstrumentChangeDlg::OnOK() 
{
	if (!m_check4.GetCheck()) m_onlytrack=-1;
	if (!m_check5.GetCheck()) m_onlychannels=-1;
	if (m_check6.GetCheck())
	{
		CString s;
		m_edit1.GetWindowText(s);
		m_onlysonglinefrom=Hexstr((char*)(LPCTSTR)s,4);
		m_edit2.GetWindowText(s);
		m_onlysonglineto=Hexstr((char*)(LPCTSTR)s,4);
	}
	else
		m_onlysonglinefrom=m_onlysonglineto=-1;
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CRenumberTracksDlg dialog


CRenumberTracksDlg::CRenumberTracksDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRenumberTracksDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRenumberTracksDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRenumberTracksDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenumberTracksDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenumberTracksDlg, CDialog)
	//{{AFX_MSG_MAP(CRenumberTracksDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenumberTracksDlg message handlers

BOOL CRenumberTracksDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_radio = 0;	
	((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRenumberTracksDlg::OnOK() 
{
	if (((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck()) m_radio=1;
	else
	if (((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck()) m_radio=2;
	
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CRenumberInstrumentsDlg dialog


CRenumberInstrumentsDlg::CRenumberInstrumentsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRenumberInstrumentsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRenumberInstrumentsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRenumberInstrumentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenumberInstrumentsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenumberInstrumentsDlg, CDialog)
	//{{AFX_MSG_MAP(CRenumberInstrumentsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenumberInstrumentsDlg message handlers

BOOL CRenumberInstrumentsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_radio = 0;	
	((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRenumberInstrumentsDlg::OnOK() 
{
	if (((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck()) m_radio=1;
	else
	if (((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck()) m_radio=2;
	else
	if (((CButton*)GetDlgItem(IDC_RADIO3))->GetCheck()) m_radio=3;

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CInsertCopyOrCloneOfSongLinesDlg dialog


CInsertCopyOrCloneOfSongLinesDlg::CInsertCopyOrCloneOfSongLinesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInsertCopyOrCloneOfSongLinesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInsertCopyOrCloneOfSongLinesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInsertCopyOrCloneOfSongLinesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertCopyOrCloneOfSongLinesDlg)
	DDX_Control(pDX, IDC_INFO, m_c_info);
	DDX_Control(pDX, IDC_TEXT2, m_c_text2);
	DDX_Control(pDX, IDC_TEXT1, m_c_text1);
	DDX_Control(pDX, IDC_VOLUMEP, m_c_volumep);
	DDX_Control(pDX, IDC_TUNING, m_c_tuning);
	DDX_Control(pDX, IDC_SONGLINETO, m_c_lineto);
	DDX_Control(pDX, IDC_SONGLINEFROM, m_c_linefrom);
	DDX_Control(pDX, IDC_CLONETRACKS, m_c_clone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInsertCopyOrCloneOfSongLinesDlg, CDialog)
	//{{AFX_MSG_MAP(CInsertCopyOrCloneOfSongLinesDlg)
	ON_BN_CLICKED(IDC_CLONETRACKS, OnClonetracks)
	ON_EN_CHANGE(IDC_SONGLINEFROM, OnChangeSonglinerange)
	ON_EN_CHANGE(IDC_SONGLINETO, OnChangeSonglinerange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertCopyOrCloneOfSongLinesDlg message handlers

BOOL CInsertCopyOrCloneOfSongLinesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString s;
	s.Format("%02X",m_linefrom);
	m_c_linefrom.SetWindowText(s);
	s.Format("%02X",m_lineto);
	m_c_lineto.SetWindowText(s);
	m_c_clone.SetCheck(m_clone);
	s.Format("%i",m_tuning);
	m_c_tuning.SetWindowText(s);
	s.Format("%i",m_volumep);
	m_c_volumep.SetWindowText(s);

	ValuesTest();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertCopyOrCloneOfSongLinesDlg::OnOK() 
{
	// TODO: Add extra validation here
	m_clone = m_c_clone.GetCheck();

	if (!ValuesTest())
	{
		MessageBox("Some parameters need to be corrected.\nPlease re-verify their values.","Warning",MB_ICONWARNING);
		return;
	}
	
	CDialog::OnOK();
}

BOOL CInsertCopyOrCloneOfSongLinesDlg::ValuesTest()
{
	BOOL r=1;
	int c = m_c_clone.GetCheck();
	m_clone=c;
	m_c_text1.EnableWindow(c);
	m_c_text2.EnableWindow(c);
	m_c_tuning.EnableWindow(c);
	m_c_volumep.EnableWindow(c);
	CString s;

	m_c_linefrom.GetWindowText(s);
	c=Hexstr((char*)(LPCTSTR)s,4);
	if (c<0) { c=0; r=0; }
	else
	if (c>=SONGLEN) { c=SONGLEN-1; r=0; }
	m_linefrom=c;
	s.Format("%02X",c);
	m_c_linefrom.SetWindowText(s);

	m_c_lineto.GetWindowText(s);
	c=Hexstr((char*)(LPCTSTR)s,4);
	if (c<0) { c=0; r=0; }
	else
	if (c>=SONGLEN) { c=SONGLEN-1; r=0; }
	if (c<m_linefrom) { c=m_linefrom; r=0; }	//it can't be smaller
	m_lineto=c;
	s.Format("%02X",c);
	m_c_lineto.SetWindowText(s);

	OnChangeSonglinerange();

	m_c_tuning.GetWindowText(s);
	c=atoi((LPCTSTR)s);
	m_tuning=c;

	m_c_volumep.GetWindowText(s);
	c=atoi((LPCTSTR)s);
	if (c<0) { c=0; r=0; }
	else
	if (c>=1600) { c=1600; r=0; }
	m_volumep=c;
	s.Format("%i",c);
	m_c_volumep.SetWindowText(s);

	return r;
}

void CInsertCopyOrCloneOfSongLinesDlg::OnClonetracks() 
{
	ValuesTest();	
}

void CInsertCopyOrCloneOfSongLinesDlg::OnChangeSonglinerange() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	CString s;
	m_c_linefrom.GetWindowText(s);
	int f=Hexstr((char*)(LPCTSTR)s,4);
	m_c_lineto.GetWindowText(s);
	int t=Hexstr((char*)(LPCTSTR)s,4);
	if (t<f) t=f;
	int n=t-f+1;
	if (n>1)
		s.Format("%i lines will be inserted into $%02X song line.",n,m_lineinto);
	else
		s.Format("1 line will be inserted into $%02X song line.",m_lineinto);
	m_c_info.SetWindowText(s);
}

/////////////////////////////////////////////////////////////////////////////
// COctaveSelectDlg dialog


COctaveSelectDlg::COctaveSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COctaveSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COctaveSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void COctaveSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COctaveSelectDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COctaveSelectDlg, CDialog)
	//{{AFX_MSG_MAP(COctaveSelectDlg)
	ON_BN_CLICKED(IDC_OCTAVE1, OnOctave)
	ON_BN_CLICKED(IDC_OCTAVE2, OnOctave)
	ON_BN_CLICKED(IDC_OCTAVE3, OnOctave)
	ON_BN_CLICKED(IDC_OCTAVE4, OnOctave)
	ON_BN_CLICKED(IDC_OCTAVE5, OnOctave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COctaveSelectDlg message handlers

BOOL COctaveSelectDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message==0x0118 && pMsg->wParam==0xfff6)
	{
		//P message:0x0118 [Unknown] wParam:0000FFF6 lParam:17677DBA
		//(related to WM_NCHITTEST, WM_NCMOUSEMOVE and HTCLOSE events)
		//The message that causes the "Close" bubble prompt. And because of that, it stops playing for a moment and it annoys it there unnecessarily. ;-)
		return 1;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL COctaveSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	SetWindowPos(0,m_pos.x,m_pos.y,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);
	GetDlgItem(IDC_OCTAVE1+m_octave)->SetFocus();
	
	return FALSE;
	//return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COctaveSelectDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void COctaveSelectDlg::OnOctave() 
{
	// TODO: Add your control notification handler code here
	m_octave = GetFocus()->GetDlgCtrlID() - IDC_OCTAVE1;
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CInstrumentSelectDlg dialog


CInstrumentSelectDlg::CInstrumentSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInstrumentSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInstrumentSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInstrumentSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstrumentSelectDlg)
	DDX_Control(pDX, IDC_LIST1, m_list1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInstrumentSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CInstrumentSelectDlg)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstrumentSelectDlg message handlers

BOOL CInstrumentSelectDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message==0x0118 && pMsg->wParam==0xfff6)
	{
		//P message:0x0118 [Unknown] wParam:0000FFF6 lParam:17677DBA
		//(related to WM_NCHITTEST, WM_NCMOUSEMOVE and HTCLOSE events)
		//The message that causes the "Close" bubble prompt. And because of that, it stops playing for a moment and it annoys it there unnecessarily. ;-)
		return 1;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CInstrumentSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	int i;
	CString s;

	// TODO: Add extra initialization here
	SetWindowPos(0,m_pos.x,m_pos.y,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);	

	for(i=0; i<INSTRSNUM; i++)
	{
		s.Format("%02X: %s", i, g_Instruments.GetName(i));
		m_list1.AddString(s);
	}

	m_list1.SetCurSel(m_selected);
	if (m_selected>16)
	{
		if (m_selected<64-16) m_list1.SetTopIndex(m_selected-16);
		else
			m_list1.SetTopIndex(64-16);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentSelectDlg::OnSelchangeList1() 
{
	// TODO: Add your control notification handler code here
	m_selected = m_list1.GetCurSel();
	CDialog::OnOK();
}
/////////////////////////////////////////////////////////////////////////////
// CVolumeSelectDlg dialog


CVolumeSelectDlg::CVolumeSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVolumeSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVolumeSelectDlg)
	m_respectvolume = FALSE;
	//}}AFX_DATA_INIT
}


void CVolumeSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVolumeSelectDlg)
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Check(pDX, IDC_CHECK1, m_respectvolume);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVolumeSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CVolumeSelectDlg)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeSelectDlg message handlers

BOOL CVolumeSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	SetWindowPos(0,m_pos.x,m_pos.y,0,0,SWP_NOZORDER|SWP_NOSIZE|SWP_SHOWWINDOW);

	m_list1.SetTabStops(10);
	const char *vs[]={"F\t|||||||||||||||","E\t||||||||||||||","D\t|||||||||||||","C\t||||||||||||",
				"B\t|||||||||||","A\t||||||||||","9\t|||||||||","8\t||||||||",
				"7\t|||||||","6\t||||||","5\t|||||","4\t||||",
				"3\t|||","2\t||","1\t|","0\t"};
	for(int i=0; i<16; i++) m_list1.AddString(vs[i]);
	
	m_list1.SetCurSel(15-m_volume);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CVolumeSelectDlg::OnSelchangeList1() 
{
	// TODO: Add your control notification handler code here
	m_volume = 15-m_list1.GetCurSel();
	CDialog::OnOK();	
}


void CVolumeSelectDlg::OnOK() 
{
	// TODO: Add extra validation here
	m_volume = 15-m_list1.GetCurSel();
	CDialog::OnOK();
}

BOOL CVolumeSelectDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message==0x0118 && pMsg->wParam==0xfff6)
	{
		//P message:0x0118 [Unknown] wParam:0000FFF6 lParam:17677DBA
		//(related to WM_NCHITTEST, WM_NCMOUSEMOVE and HTCLOSE events)
		//The message that causes the "Close" bubble prompt. And because of that, it stops playing for a moment and it annoys it there unnecessarily. ;-)
		return 1;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CChannelsSelectionDlg dialog


CChannelsSelectionDlg::CChannelsSelectionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChannelsSelectionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChannelsSelectionDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CChannelsSelectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChannelsSelectionDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChannelsSelectionDlg, CDialog)
	//{{AFX_MSG_MAP(CChannelsSelectionDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChannelsSelectionDlg message handlers

const int idcchan[8] = { IDC_CHECK1,IDC_CHECK2,IDC_CHECK3,IDC_CHECK4,IDC_CHECK10,IDC_CHECK11,IDC_CHECK12,IDC_CHECK13 };

BOOL CChannelsSelectionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_channelyes=0;
	if (g_tracks4_8<=4)
	{
		for(int i=4; i<8; i++) ((CButton*)GetDlgItem(idcchan[i]))->EnableWindow(0);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CChannelsSelectionDlg::OnOK() 
{
	// TODO: Add extra validation here
	for(int i=0; i<8; i++)
	{
		if (((CButton*)GetDlgItem(idcchan[i]))->GetCheck())
			m_channelyes|=(1<<i);
	}
	CDialog::OnOK();
}

