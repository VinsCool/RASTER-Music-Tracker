// POKEY Frequencies Calculator
// RMT integration by VinsCool, 2022
// Most of the tuning stuff will be defined here

#pragma once

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <fstream>
#include <cmath>
#include <limits>

#include "StdAfx.h"
#include "Rmt.h"
#include "XPokey.h"
#include "Atari6502.h"

//Thomas Young 1799's Well Temperament no.1
const double YOUNG1[13] = { 1, 1.055709, 1.119770, 1.187690, 1.253887, 1.334739, 1.407637, 1.496513, 1.583581, 1.675715, 1.781546, 1.878851, 2 };

//Thomas Young 1799's Well Temperament no.2 
const double YOUNG2[13] = { 1, 1.055880, 1.119929, 1.187865, 1.254242, 1.334839, 1.407840, 1.496616, 1.583819, 1.676105, 1.781797, 1.879240, 2 };

//Thomas Young 1807's Well Temperament
const double YOUNG3[13] = { 1, 1.053497, 1.119929, 1.185185, 1.254242, 1.333333, 1.404663, 1.496616, 1.580246, 1.676105, 1.777777, 1.877119, 2 };

//Andreas Werckmeister's temperament III (the most famous one, 1681)
const double WERCK3[13] = { 1, 1.053497, 1.117403, 1.185185, 1.252827, 1.333333, 1.404663, 1.494927, 1.580246, 1.670436, 1.777777, 1.879240, 2 };

//Tempérament Égal à Quintes Justes 
const double QUINTE[13] = { 1, 1.059634, 1.122824, 1.189782, 1.260734, 1.335916, 1.415582, 1.5, 1.589451, 1.684236, 1.784674, 1.891101, 2.003875 };

//Alembert's and Rousseau's Tempérament Ordinaire (1752/1767)
const double TEMPORD[13] = { 1, 1.051120, 1.118034, 1.181176, 1.250000, 1.331828, 1.403077, 1.495348, 1.574901, 1.671850, 1.773766, 1.872883, 2 };

//Aron - Neidhardt equal beating well temperament
const double ARONIED[13] = { 1, 1.053497, 1.118144, 1.185185, 1.250031, 1.333333, 1.404663, 1.496112, 1.580246, 1.672002, 1.777777, 1.872885, 2 };

//Atom Schisma Scale
const double ATOMSCH[13] = { 1, 1.059459, 1.122463, 1.189204, 1.259924, 1.334838, 1.414207, 1.498308, 1.587396, 1.681796, 1.781794, 1.887755, 2 };

//12 - tET approximation with minimal order 17 beats
const double APPRX12[13] = { 1, 1.058823, 1.125000, 1.187500, 1.250000, 1.333333, 1.416666, 1.500000, 1.588235, 1.666666, 1.777777, 1.888888, 2 };

//Paul Bailey's modern well temperament (2002)
const double BAILEY1[13] = { 1, 1.054892, 1.119671, 1.186774, 1.254242, 1.335183, 1.406531, 1.498302, 1.582329, 1.676250, 1.780202, 1.881407, 2 };

//John Barnes' temperament (1977) made after analysis of Wohltemperierte Klavier, 1/6 P
const double BARNES1[13] = { 1, 1.055880, 1.119929, 1.187865, 1.254242, 1.336348, 1.407840, 1.496616, 1.583819, 1.676105, 1.781797, 1.881364, 2 };

//Bethisy temperament ordinaire, see Pierre - Yves Asselin : Musique et temperament
const double BETHISY[13] = { 1, 1.051418, 1.118034, 1.181509, 1.250000, 1.331953, 1.403475, 1.495348, 1.575346, 1.671850, 1.774101, 1.872884, 2 };

//Big Gulp
const double BIGGULP[13] = { 1, 1.031250, 1.125000, 1.166666, 1.250000, 1.312500, 1.375000, 1.500000, 1.546875, 1.687500, 1.750000, 1.875000, 2 };

//12 - tone scale by Bohlen generated from the 4:7 : 10 triad, Acustica 39 / 2, 1978
const double BOHLEN12[13] = { 1, 1.100000, 1.200000, 1.304347, 1.428571, 1.571428, 1.750000, 1.909090, 2.100000, 2.300000, 2.500000, 2.750000, 3 };

//This scale may also be called the "Wedding Cake"
const double WEDDING[13] = { 1, 1.125000, 1.171875, 1.250000, 1.333333, 1.406250, 1.500000, 1.562500, 1.666666, 1.687500, 1.777777, 1.875000, 2 };

//Upside - Down Wedding Cake(divorce cake)
const double DIVORCE[13] = { 1, 1.066666, 1.125000, 1.200000, 1.280000, 1.333333, 1.500000, 1.600000, 1.687500, 1.777777, 1.800000, 1.920000, 2 };

//12 - tone Pythagorean scale
const double PYTHAG1[13] = { 1, 1.067871, 1.125000, 1.185185, 1.265625, 1.333333, 1.423828, 1.500000, 1.601806, 1.687500, 1.777777, 1.898437, 2 };

//Robert Schneider, scale of log(4) ..log(16), 1 / 1 = 264Hz
const double LOGSCALE[13] = { 1, 1.160964, 1.292481, 1.403677, 1.500000, 1.584962, 1.660964, 1.729715, 1.792481, 1.850219, 1.903677, 1.953445, 2 };

//Zarlino's Tempérament Extraordinaire, 1024 - tET mapping
const double ZARLINO1[13] = { 1, 1.041450, 1.116652, 1.180385, 1.247756, 1.337855, 1.393309, 1.494930, 1.567469, 1.669316, 1.777781, 1.865308, 2 };

//Fokker's 7-limit 12-tone just scale
const double FOKKER7[13] = { 1, 1.071428, 1.125000, 1.166666, 1.250000, 1.333333, 1.406250, 1.500000, 1.607142, 1.666666, 1.750000, 1.875000, 2 };

//Bach temperament, a'=400 Hz
const double BACH400[13] = { 1, 1.052192, 1.118997, 1.183716, 1.250521, 1.334029, 1.402922, 1.498956, 1.578288, 1.670146, 1.776618, 1.874739, 2 };

//Vallotti& Young scale(Vallotti version) also known as Tartini - Vallotti(1754)
const double VALYOUNG[13] = { 1, 1.055880, 1.119929, 1.187865, 1.254242, 1.336348, 1.407840, 1.496616, 1.583819, 1.676105, 1.781797, 1.877119, 2 };

//Vallotti - Young and Werckmeister III, 10 cents 5 - limit lesfip scale
const double VALYOWER[13] = { 1, 1.051637, 1.117298, 1.187060, 1.248356, 1.336846, 1.399836, 1.495457, 1.580099, 1.669531, 1.783574, 1.867613, 2 };

///!!!\\\ Testing some non-12 octaves scales here

//Optimally consonant major pentatonic, John deLaubenfels(2001)
const double PENTAOPT[6] = { 1, 1.118042, 1.250019, 1.496879, 1.670166, 2 };

//Ancient Greek Aeolic, also tritriadic scale of the 54:64 : 81 triad
const double AEOLIC[8] = { 1, 1.125000, 1.185185, 1.333333, 1.500000, 1.580246, 1.777777, 2 };

//African Bapare xylophone(idiophone; loose log)
const double XYLO1[11] = { 1, 1.076737, 1.200942, 1.336382, 1.497441, 1.670175, 1.932988, 2.174725, 2.285484, 2.525670, 2.738400 };

//African Yaswa xylophones(idiophone; calbash resonators with membrane)
const double XYLO2[11] = { 1, 1.128312, 1.271619, 1.486239, 1.707240, 1.936341, 2.015074, 2.215296, 2.419988, 2.871225, 3.220980 };

//19-EDO generated using Scale Workshop
const double NINTENDO[20] =
{ 1, 1.037155, 1.075690, 1.115657, 1.157110, 1.200102, 1.244692, 1.290939, 1.338904, 1.388651, 1.440246, 1.493759, 1.549259, 1.606822, 1.666524, 1.728443, 1.792664, 1.859270, 1.928352, 2 };

//Custom tuning ratio is generated in this array, so this is not a constant
static double CUSTOM[13] = { 0 };

//Table construction structure
struct TTuning
{
	int table_64khz;
	int table_15khz;
	int table_179mhz;
	int table_16bit;
};

class CTuning
{
public:
	int get_audf(double pitch, int coarse_divisor, double divisor, int cycle);
	double get_pitch(int audf, int coarse_divisor, double divisor, int cycle);
	double generate_freq(int audc, int audf, int audctl, int channel); 
	double GetTruePitch(double tuning, int temperament, int basenote, int semitone);
	void init_tuning();

private:
	void generate_table(unsigned char* table, int length, int semitone, int timbre, int audctl);
	int delta_audf(double pitch, int audf, int coarse_divisor, double divisor, int cycle, int timbre);

	//Stack of lookup tables to generate so far...
	const TTuning dist_2_bell{ 12, 0, 48, 24 };
	const TTuning dist_4_smooth{ 12, 0, 24, 24 };
	const TTuning dist_4_buzzy{ 12, 0, 12, 24 };
	const TTuning dist_a_pure{ 48, 24, 108, 24 };
	const TTuning dist_c_buzzy{ 24, 12, 84, 24 };
	const TTuning dist_c_gritty{ 12, 0, 72, 24 };
	const TTuning dist_c_unstable{ 36, 0, 96, 24 };

};

extern CTuning g_Tuning;