// POKEY Frequencies Calculator
// RMT integration by VinsCool, 2022
// Most of the global stuff will be defined here

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <cmath>
#include <limits>

#include "StdAfx.h"
#include "Rmt.h"

#define RMT_FRQTABLES	0xB000
#define FREQ_17_NTSC 1789773
#define FREQ_17_PAL 1773447

extern unsigned char g_atarimem[65536];
extern BOOL g_ntsc;
extern HWND g_hwnd;

extern double g_basetuning;
extern int g_basenote;
extern int g_temperament;

//ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
extern double g_UNISON;
extern double g_MIN_2ND;
extern double g_MAJ_2ND;
extern double g_MIN_3RD;
extern double g_MAJ_3RD;
extern double g_PERF_4TH;
extern double g_TRITONE;
extern double g_PERF_5TH;
extern double g_MIN_6TH;
extern double g_MAJ_6TH;
extern double g_MIN_7TH;
extern double g_MAJ_7TH;
extern double g_OCTAVE;

//ratio left
extern int g_UNISON_L;
extern int g_MIN_2ND_L;
extern int g_MAJ_2ND_L;
extern int g_MIN_3RD_L;
extern int g_MAJ_3RD_L;
extern int g_PERF_4TH_L;
extern int g_TRITONE_L;
extern int g_PERF_5TH_L;
extern int g_MIN_6TH_L;
extern int g_MAJ_6TH_L;
extern int g_MIN_7TH_L;
extern int g_MAJ_7TH_L;
extern int g_OCTAVE_L;

//ratio right
extern int g_UNISON_R;
extern int g_MIN_2ND_R;
extern int g_MAJ_2ND_R;
extern int g_MIN_3RD_R;
extern int g_MAJ_3RD_R;
extern int g_PERF_4TH_R;
extern int g_TRITONE_R;
extern int g_PERF_5TH_R;
extern int g_MIN_6TH_R;
extern int g_MAJ_6TH_R;
extern int g_MIN_7TH_R;
extern int g_MAJ_7TH_R;
extern int g_OCTAVE_R;

void real_freq();
double generate_freq(int i_audc, int i_audf, int i_audctl, int i_ch_index);
void generate_table(int note, double freq, int distortion, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT);
int get_audf(double freq, int coarse_divisor, double divisor, int modoffset);
double get_pitch(int audf, int coarse_divisor, double divisor, int modoffset);
int delta_audf(int audf, double freq, int coarse_divisor, double divisor, int modoffset, int distortion);
void macro_table_gen(int distortion, int note_offset, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT);
void init_tuning();