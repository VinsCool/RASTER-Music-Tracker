// POKEY Frequencies Calculator
// by VinsCool
// Based on the code originally used for the Pitch Calculations in Raster Music Tracker 1.31+
// Backported to RMT with additional improvements

#include "tuning.h"
#include "global.h"

//Tuning tables to generate...
//double ref_pitch[2200] = { 0 };	//delete soon, this won't be useful anymore

//PAGE_DISTORTION_2 => Address 0xB000
int tab_64khz_2[64] = { 0 };
int tab_179mhz_2[64] = { 0 };
int tab_16bit_2[128] = { 0 };

//PAGE_DISTORTION_4 => Address 0xB100
int tab_64khz_4_smooth[64] = { 0 };
int tab_179mhz_4_smooth[64] = { 0 };
int tab_16bit_4_smooth[128] = { 0 };
int tab_64khz_4_buzzy[64] = { 0 };
int tab_179mhz_4_buzzy[64] = { 0 };
int tab_16bit_4_buzzy[128] = { 0 };

//PAGE_DISTORTION_A => Address 0xB200
int tab_64khz_a_pure[64] = { 0 };
int tab_179mhz_a_pure[64] = { 0 };
int tab_16bit_a_pure[128] = { 0 };

//PAGE_DISTORTION_C => Address 0xB300
int tab_64khz_c_buzzy[64] = { 0 };
int tab_179mhz_c_buzzy[64] = { 0 };
int tab_16bit_c_buzzy[128] = { 0 };

//PAGE_DISTORTION_E => Address 0xB400
int tab_64khz_c_gritty[64] = { 0 };
int tab_179mhz_c_gritty[64] = { 0 };
int tab_16bit_c_gritty[128] = { 0 };

//PAGE_EXTRA_0 => Address 0xB500, 0xB580 for 15khz Pure and 0xB5C0 for 15khz Buzzy
//Croissant Sawtooth ch1	//no generator yet
//Croissant Sawtooth ch3	//no generator yet
int tab_15khz_a_pure[64] = { 0 };
int tab_15khz_c_buzzy[64] = { 0 };

//PAGE_EXTRA_1 => Address 0xB600
//Clarinet Lo	//no generator yet
//Clarinet Hi	//no generator yet
//unused
//unused

//PAGE_EXTRA_2 => Address 0xB700
int tab_64khz_c_unstable[64] = { 0 };
int tab_179mhz_c_unstable[64] = { 0 };
int tab_16bit_c_unstable[128] = { 0 };



//AUDCTL bits
bool CLOCK_15 = 0;				//0x01
bool HPF_CH24 = 0;				//0x02
bool HPF_CH13 = 0;				//0x04
bool JOIN_34 = 0;				//0x08
bool JOIN_12 = 0;				//0x10
bool CH3_179 = 0;				//0x20
bool CH1_179 = 0;				//0x40
bool POLY9 = 0;					//0x80

//combined AUDCTL bits for special cases
bool JOIN_16BIT = 0;			//valid 16-bit mode
bool CLOCK_179 = 0;				//valid 1.79mhz mode
bool SAWTOOTH = 0;				//valid Sawtooth mode
bool SAWTOOTH_INVERTED = 0;		//valid Sawtooth mode (inverted)

//SKCTL bits
bool TWO_TONE = 0;				//0x8B, Two-Tone Filter

//combined AUDC and AUDF bits for special cases
bool IS_SMOOTH_DIST_4 = 0;
bool IS_BUZZY_DIST_4 = 0;
bool IS_UNSTABLE_DIST_4_1 = 0;
bool IS_UNSTABLE_DIST_4_2 = 0;
bool IS_BUZZY_DIST_C = 0;		//Distortion C, MOD3 AUDF and not MOD5 AUDF
bool IS_GRITTY_DIST_C = 0;		//Distortion C, neither MOD3 nor MOD5 AUDF
bool IS_UNSTABLE_DIST_C = 0;	//Distortion C, MOD5 AUDF and not MOD3 AUDF
bool IS_METALLIC_POLY9 = 0;		//AUDCTL 0x80, Distortion 0 and 8, MOD7 AUDF


//Parts of this code was rewritten for POKEY Frequencies Calculator, then backported to RMT 1.31+
double CTuning::generate_freq(int audc, int audf, int audctl, int channel)
{
	//register variables
	int skctl = 0;						//not yet implemented in calculations
	int distortion = audc & 0xf0;

	//variables for pitch calculation
	double divisor = 1;					//divisors must never be 0!
	int coarse_divisor = 1;
	int cycle = 1;
	int modulo = 0;

	IS_SMOOTH_DIST_4 = 0;
	IS_BUZZY_DIST_4 = 0;
	IS_UNSTABLE_DIST_4_1 = 0;
	IS_UNSTABLE_DIST_4_2 = 0;

	IS_BUZZY_DIST_C = 0;
	IS_GRITTY_DIST_C = 0;
	IS_UNSTABLE_DIST_C = 0;
	IS_METALLIC_POLY9 = 0;

	CLOCK_15 = audctl & 0x01;
	HPF_CH24 = audctl & 0x02;
	HPF_CH13 = audctl & 0x04;
	JOIN_34 = audctl & 0x08;
	JOIN_12 = audctl & 0x10;
	CH3_179 = audctl & 0x20;
	CH1_179 = audctl & 0x40;
	POLY9 = audctl & 0x80;

	TWO_TONE = (skctl == 0x8B) ? 1 : 0;

	//combined modes for some special output...
	JOIN_16BIT = ((JOIN_12 && CH1_179 && (channel == 1 || channel == 5)) || (JOIN_34 && CH3_179 && (channel == 3 || channel == 7))) ? 1 : 0;
	CLOCK_179 = ((CH1_179 && (channel == 0 || channel == 4)) || (CH3_179 && (channel == 2 || channel == 6))) ? 1 : 0;
	if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode if they are enabled at the same time

	/*
	//TODO: Sawtooth generation needs to be optimal in order to compromise the high pitched hiss versus the tuning accuracy
	SAWTOOTH = (CH1_179 && CH3_179 && HPF_CH13 && (dist == 0xA0 || dist == 0xE0) && (i == 0 || i == 4)) ? 1 : 0;
	SAWTOOTH_INVERTED = 0;
	if (i % 4 == 0)	//only in valid sawtooth channels
	audf3 = g_atarimem[idx[i + 2]];
	*/

	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	if (JOIN_16BIT) cycle = 7;
	else if (CLOCK_179) cycle = 4;
	else coarse_divisor = (CLOCK_15) ? 114 : 28;

	switch (distortion)
	{
	case 0x60:	//Duplicate of Distortion 2
	case 0x20:
		divisor = 31;
		modulo = 31;
		if ((audf + cycle) % modulo == 0) return 0;
		break;

	case 0x40:
		divisor = 232.5;		//Buzzy
		modulo = (CLOCK_15) ? 5 : 15;
		IS_UNSTABLE_DIST_4_1 = ((audf + cycle) % 5 == 0) ? 1 : 0;
		IS_SMOOTH_DIST_4 = ((audf + cycle) % 3 == 0 || CLOCK_15) ? 1 : 0;
		IS_UNSTABLE_DIST_4_2 = ((audf + cycle) % 31 == 0) ? 1 : 0;
		if (IS_UNSTABLE_DIST_4_1) divisor = 46.5;	//Unstable #1
		if (IS_SMOOTH_DIST_4) divisor = 77.5;	//Smooth
		if (IS_UNSTABLE_DIST_4_2) divisor = (IS_SMOOTH_DIST_4 || IS_UNSTABLE_DIST_4_1) ? 2.5 : 7.5;	//Unstable #2 and #3
		if ((audf + cycle) % modulo == 0) return 0;
		break;

	case 0x00:
	case 0x80:
		if (POLY9)
		{
			divisor = 255.5;	//Metallic Buzzy
			modulo = 73;
			if (CLOCK_179 || JOIN_16BIT)
				IS_METALLIC_POLY9 = ((audf + cycle) % 7 == 0) ? 1 : 0;
			else
				IS_METALLIC_POLY9 = 1;

			if (IS_METALLIC_POLY9) divisor = 36.5;
			if ((audf + cycle) % modulo == 0) return 0;
			if (distortion == 0x00 && ((audf + cycle) % 31 == 0)) return 0;	//MOD31 values are invalid with Distortion 0
		}
		break;

	case 0xC0:
		divisor = 7.5;		//Gritty
		modulo = (CLOCK_15) ? 5 : 15;
		IS_UNSTABLE_DIST_C = ((audf + cycle) % 5 == 0) ? 1 : 0;
		IS_BUZZY_DIST_C = ((audf + cycle) % 3 == 0 || CLOCK_15) ? 1 : 0;
		if (IS_UNSTABLE_DIST_C) divisor = 1.5;	//Unstable
		if (IS_BUZZY_DIST_C) divisor = 2.5;	//Buzzy
		if ((audf + cycle) % modulo == 0) return 0;
		//IS_VALID = ((audf + cycle) % modulo == 0) ? 0 : 1;
		break;
	}
	return get_pitch(audf, coarse_divisor, divisor, cycle);
}

//this code was originally added in POKEY Frequencies Calculator, and adapted for RMT 1.31+
void CTuning::generate_table(int note, double freq, int distortion, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT)
{
	int audf = 0;
	int modoffset = 1;
	int coarse_divisor = 1;
	int v_modulo = 0;
	double divisor = 1;
	double PITCH = 0;

	//since globals are specifically wanted, the parameters are used to define these flags here
	CLOCK_15 = IS_15KHZ;
	CLOCK_179 = IS_179MHZ;
	JOIN_16BIT = IS_16BIT;

	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	if (JOIN_16BIT) modoffset = 7;
	else if (CLOCK_179) modoffset = 4;
	else coarse_divisor = (CLOCK_15) ? 114 : 28;

	switch (distortion)
	{
	case 0x20:
		divisor = 31;
		v_modulo = 31;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((audf + modoffset) % v_modulo == 0)	//invalid values
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion);
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT) tab_16bit_2[note * 2] = audf;
		else if (CLOCK_179) tab_179mhz_2[note] = audf;
		else tab_64khz_2[note] = audf;
		break;

	case 0x40:
		divisor = (IS_SMOOTH_DIST_4 || CLOCK_15) ? 77.5 : 232.5;		//Smooth
		v_modulo = (CLOCK_15) ? 5 : 15;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((CLOCK_15 && (audf + modoffset) % v_modulo == 0) ||
			(IS_SMOOTH_DIST_4 && ((audf + modoffset) % 3 != 0 || (audf + modoffset) % 5 == 0 || (audf + modoffset) % 31 == 0)) ||
			(IS_BUZZY_DIST_4 && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 == 0 || (audf + modoffset) % 31 == 0))) 
		{
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion); 
		}
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT)
		{
			if (IS_SMOOTH_DIST_4) tab_16bit_4_smooth[note * 2] = audf;
			else if (IS_BUZZY_DIST_4) tab_16bit_4_buzzy[note * 2] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_179)
		{
			if (IS_SMOOTH_DIST_4) tab_179mhz_4_smooth[note] = audf;
			else if (IS_BUZZY_DIST_4) tab_179mhz_4_buzzy[note] = audf;
			else break;	//invalid parameter
		}
		else	//64khz mode
		{
			if (IS_SMOOTH_DIST_4) tab_64khz_4_smooth[note] = audf;
			else if (IS_BUZZY_DIST_4) tab_64khz_4_buzzy[note] = audf;
			else break;	//invalid parameter
		}
		break;

	case 0xA0:
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT) tab_16bit_a_pure[note * 2] = audf;
		else if (CLOCK_179) tab_179mhz_a_pure[note] = audf;
		else if (CLOCK_15) tab_15khz_a_pure[note] = audf;
		else tab_64khz_a_pure[note] = audf;
		break;

	case 0xC0:
		divisor = (IS_BUZZY_DIST_C || CLOCK_15) ? 2.5 : 7.5;
		v_modulo = (CLOCK_15) ? 5 : 15;
		if (IS_UNSTABLE_DIST_C) divisor = 1.5;
		audf = get_audf(freq, coarse_divisor, divisor, modoffset);
		if ((CLOCK_15 && (audf + modoffset) % v_modulo == 0) ||
			(IS_BUZZY_DIST_C && ((audf + modoffset) % 3 != 0 || (audf + modoffset) % 5 == 0)) ||
			(IS_GRITTY_DIST_C && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 == 0)) ||
			(IS_UNSTABLE_DIST_C && ((audf + modoffset) % 3 == 0 || (audf + modoffset) % 5 != 0)))
		{
			audf = delta_audf(audf, freq, coarse_divisor, divisor, modoffset, distortion); //aaaaaa
		}
		if (!JOIN_16BIT)	//not 16-bit mode
		{
			if (audf > 0xFF) audf = 0xFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		else
		{
			if (audf > 0xFFFF) audf = 0xFFFF;	//lowest possible pitch
			else if (audf < 0x00) audf = 0x00;	//highest possible pitch
		}
		if (JOIN_16BIT)
		{
			if (IS_BUZZY_DIST_C) tab_16bit_c_buzzy[note * 2] = audf;
			else if (IS_GRITTY_DIST_C) tab_16bit_c_gritty[note * 2] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_16bit_c_unstable[note * 2] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_179)
		{
			if (IS_BUZZY_DIST_C) tab_179mhz_c_buzzy[note] = audf;
			else if (IS_GRITTY_DIST_C) tab_179mhz_c_gritty[note] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_179mhz_c_unstable[note] = audf;
			else break;	//invalid parameter
		}
		else if (CLOCK_15) tab_15khz_c_buzzy[note] = audf;
		else	//64khz mode
		{
			if (IS_BUZZY_DIST_C) tab_64khz_c_buzzy[note] = audf;
			else if (IS_GRITTY_DIST_C) tab_64khz_c_gritty[note] = audf;
			else if (IS_UNSTABLE_DIST_C) tab_64khz_c_unstable[note] = audf;
			else break;	//invalid parameter
		}
		break;
	}
}

/// <summary> Calculate the POKEY audio pitch using the given parameters </summary>
/// <param name = "audf"> POKEY Frequency, either 8-bit or 16-bit </param>
/// <param name = "coarse_divisor"> Coarse division, 28 in 64kHz mode, 114 in 15kHz mode, 1 for no division </param>
/// <param name = "divisor"> Fine division, variable relative to Distortion, Cycle, and frequency modulo, 1 for no division </param> 
/// <param name = "cycle"> Offset added to AUDF, 4 for 1.79mHz mode, 7 for 16-bit+1.79mHz mode, 1 for neither </param>
/// <returns> POKEY audio pitch (in Hertz) </returns> 
double CTuning::get_pitch(int audf, int coarse_divisor, double divisor, int cycle)
{
	return ((((g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL) / (coarse_divisor * divisor)) / (audf + cycle)) / 2;
}

/// <summary> Find the nearest POKEY Frequency (AUDF) using the given parameters </summary>
/// <param name = "pitch"> Source audio pitch (in Hertz) </param>
/// <param name = "coarse_divisor"> Coarse division, 28 in 64kHz mode, 114 in 15kHz mode, 1 for no division </param>
/// <param name = "divisor"> Fine division, variable relative to Distortion, Cycle, and frequency modulo, 1 for no division </param> 
/// <param name = "cycle"> Offset added to AUDF, 4 for 1.79mHz mode, 7 for 16-bit+1.79mHz mode, 1 for neither </param>
/// <returns> POKEY Frequency (AUDF) </returns> 
int CTuning::get_audf(double pitch, int coarse_divisor, double divisor, int cycle)
{
	return (int)round(((((g_ntsc) ? FREQ_17_NTSC : FREQ_17_PAL) / (coarse_divisor * divisor)) / (2 * pitch)) - cycle);
}

//code originally written for POKEY Frequencies Calculator
int CTuning::delta_audf(int audf, double freq, int coarse_divisor, double divisor, int modoffset, int distortion)
{
	int tmp_audf_up = audf;		//begin from the currently invalid audf
	int tmp_audf_down = audf;
	double tmp_freq_up = 0;
	double tmp_freq_down = 0;
	double PITCH = 0;

	if (distortion != 0x40 && distortion != 0xC0) { tmp_audf_up++; tmp_audf_down--; }	//anything not distortion 4 or C, simpliest delta method

	else if (distortion == 0x40)
	{
		if (IS_SMOOTH_DIST_4)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + modoffset) % 3 != 0 || (tmp_audf_up + modoffset) % 5 == 0 || (tmp_audf_up + modoffset) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 != 0 || (tmp_audf_down + modoffset) % 5 == 0 || (tmp_audf_down + modoffset) % 31 == 0) tmp_audf_down--;
			}
		}
		else if (IS_BUZZY_DIST_4)
		{
			for (int o = 0; o < 6; o++) 
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 == 0 || (tmp_audf_up + modoffset) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 == 0 || (tmp_audf_down + modoffset) % 31 == 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely 
	}

	else if (distortion == 0xC0)
	{
		if (CLOCK_15)
		{
			for (int o = 0; o < 3; o++)	//MOD5 must be avoided!
			{
				if ((tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_BUZZY_DIST_C)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + modoffset) % 3 != 0 || (tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 != 0 || (tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_GRITTY_DIST_C)	//verify neither MOD3 or MOD5 is used
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (IS_UNSTABLE_DIST_C)	//verify MOD5 integrity
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + modoffset) % 3 == 0 || (tmp_audf_up + modoffset) % 5 != 0) tmp_audf_up++;
				if ((tmp_audf_down + modoffset) % 3 == 0 || (tmp_audf_down + modoffset) % 5 != 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely
	}

	PITCH = get_pitch(tmp_audf_up, coarse_divisor, divisor, modoffset);
	tmp_freq_up = freq - PITCH;	//first delta, up
	PITCH = get_pitch(tmp_audf_down, coarse_divisor, divisor, modoffset);
	tmp_freq_down = PITCH - freq;	//second delta, down
	PITCH = tmp_freq_down - tmp_freq_up;

	if (PITCH > 0) audf = tmp_audf_up; //positive, meaning delta up is closer than delta down
	else audf = tmp_audf_down; //negative, meaning delta down is closer than delta up
	return audf;
}

//code originally written for POKEY Frequencies Calculator, changes have been done to adapt it for RMT 1.31+
void CTuning::macro_table_gen(int distortion, int note_offset, bool IS_15KHZ, bool IS_179MHZ, bool IS_16BIT)
{
	//generate the array
	for (int i = 0; i < 64; i++)
	{
		int note = i + note_offset;
		double freq = GetTruePitch(g_basetuning, g_temperament, g_basenote, note);
		generate_table(i, freq, distortion, IS_15KHZ, IS_179MHZ, IS_16BIT);
	}
}

double CTuning::GetTruePitch(double tuning, int temperament, int basenote, int semitone)
{
	int notesnum = 12;	//unless specified otherwise
	int note = (semitone + basenote) % notesnum;	//current note
	double ratio = 0;	//will be used for calculations
	double octave = 2;	//an octave is usually the frequency of a note multiplied by 2
	double multi = 1; //octave multiplyer for the ratio

	//Equal temperament is generated using the 12th root of 2
	if (!temperament)
	{
		ratio = pow(2.0, 1.0 / 12.0);
		return (tuning / 64) * pow(ratio, semitone + basenote);
	}

	//Custom Ratio tuning is used if the temperament value is higher than the number of presets
	if (temperament >= 29)
	{
		//ratio = GetCustomRatio(note);
		//octave = g_OCTAVE;
		//multi = pow(octave, trunc((semitone + basenote) / notesnum));
		return (tuning / 64) * (pow(g_OCTAVE, trunc((semitone + basenote) / notesnum)) * GetCustomRatio(note));
	}

	switch (temperament)
	{
	case 1:		//Thomas Young 1799's Well Temperament no.1
		octave = YOUNG1[notesnum];
		ratio = YOUNG1[note];
		break;

	case 2:		//Thomas Young 1799's Well Temperament no.2
		octave = YOUNG2[notesnum];
		ratio = YOUNG2[note];
		break;

	case 3:		//Thomas Young 1807's Well Temperament
		octave = YOUNG3[notesnum];
		ratio = YOUNG3[note];
		break;

	case 4:		//Andreas Werckmeister's temperament III (the most famous one, 1681)
		octave = WERCK3[notesnum];
		ratio = WERCK3[note];
		break;

	case 5:		//Tempérament Égal à Quintes Justes 
		octave = QUINTE[notesnum];
		ratio = QUINTE[note];
		break;

	case 6:		//d'Alembert and Rousseau tempérament ordinaire (1752/1767)
		octave = TEMPORD[notesnum];
		ratio = TEMPORD[note];
		break;

	case 7:		//Aron - Neidhardt equal beating well temperament
		octave = ARONIED[notesnum];
		ratio = ARONIED[note];
		break;

	case 8:		//Atom Schisma Scale
		octave = ATOMSCH[notesnum];
		ratio = ATOMSCH[note];
		break;

	case 9:		//12 - tET approximation with minimal order 17 beats
		octave = APPRX12[notesnum];
		ratio = APPRX12[note];
		break;

	case 10:	//Paul Bailey's modern well temperament (2002)
		octave = BAILEY1[notesnum];
		ratio = BAILEY1[note];
		break;

	case 11:	//John Barnes' temperament (1977) made after analysis of Wohltemperierte Klavier, 1/6 P
		octave = BARNES1[notesnum];
		ratio = BARNES1[note];
		break;

	case 12:	//Bethisy temperament ordinaire, see Pierre - Yves Asselin : Musique et temperament
		octave = BETHISY[notesnum];
		ratio = BETHISY[note];
		break;

	case 13:	//Big Gulp
		octave = BIGGULP[notesnum];
		ratio = BIGGULP[note];
		break;

	case 14:	//12 - tone scale by Bohlen generated from the 4:7 : 10 triad, Acustica 39 / 2, 1978
		octave = BOHLEN12[notesnum];
		ratio = BOHLEN12[note];
		break;

	case 15:	//This scale may also be called the "Wedding Cake"
		octave = WEDDING[notesnum];
		ratio = WEDDING[note];
		break;

	case 16:	//Upside - Down Wedding Cake(divorce cake)
		octave = DIVORCE[notesnum];
		ratio = DIVORCE[note];
		break;

	case 17:	//12 - tone Pythagorean scale
		octave = PYTHAG1[notesnum];
		ratio = PYTHAG1[note];
		break;

	case 18:	//Robert Schneider, scale of log(4) ..log(16), 1 / 1 = 264Hz
		octave = LOGSCALE[notesnum];
		ratio = LOGSCALE[note];
		break;

	case 19:	//Zarlino temperament extraordinaire, 1024 - tET mapping
		octave = ZARLINO1[notesnum];
		ratio = ZARLINO1[note];
		break;

	case 20:	//Fokker's 7-limit 12-tone just scale
		octave = FOKKER7[notesnum];
		ratio = FOKKER7[note];
		break;

	case 21:	//Bach temperament, a'=400 Hz
		octave = BACH400[notesnum];
		ratio = BACH400[note];
		break;

	case 22:	//Vallotti& Young scale(Vallotti version) also known as Tartini - Vallotti(1754)
		octave = VALYOUNG[notesnum];
		ratio = VALYOUNG[note];
		break;

	case 23:	//Vallotti - Young and Werckmeister III, 10 cents 5 - limit lesfip scale
		octave = VALYOWER[notesnum];
		ratio = VALYOWER[note];
		break;

	case 24:	//Optimally consonant major pentatonic, John deLaubenfels(2001)
		notesnum = 5;
		octave = PENTAOPT[notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = PENTAOPT[note];
		break;

	case 25:	//Ancient Greek Aeolic, also tritriadic scale of the 54:64 : 81 triad
		notesnum = 7;
		octave = AEOLIC[notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = AEOLIC[note];
		break;

	case 26:	//African Bapare xylophone(idiophone; loose log)
		notesnum = 10;
		octave = XYLO1[notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = XYLO1[note];
		break;

	case 27:	//African Yaswa xylophones(idiophone; calbash resonators with membrane)
		notesnum = 10;
		octave = XYLO2[notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = XYLO2[note];
		break;

	case 28:	//19-EDO generated using Scale Workshop
		notesnum = 19;
		octave = NINTENDO[notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = NINTENDO[note];
		break;

	default:	//custom, ratio used for each note => NOTE_L / NOTE_R, must be treated as doubles!!!
		return 0;
	}

	multi = pow(octave, trunc((semitone + basenote) / notesnum));
	return (tuning / 64) * (multi * ratio);
}

//TODO: turn this shit into an array that could be loaded from the function above...
double CTuning::GetCustomRatio(int note)
{
	switch (note)
	{
	case 0:
		return g_UNISON;

	case 1:
		return g_MIN_2ND;

	case 2:
		return g_MAJ_2ND;

	case 3:
		return g_MIN_3RD;

	case 4:
		return g_MAJ_3RD;

	case 5:
		return g_PERF_4TH;

	case 6:
		return g_TRITONE;

	case 7:
		return g_PERF_5TH;

	case 8:
		return g_MIN_6TH;

	case 9:
		return g_MAJ_6TH;

	case 10:
		return g_MIN_7TH;

	case 11:
		return g_MAJ_7TH;
	}
	return 0;
}

void CTuning::init_tuning()
{
	//reset all the tables so no leftover will stay in memory...
	memset(tab_64khz_2, 0, 64);
	memset(tab_179mhz_2, 0, 64);
	memset(tab_16bit_2, 0, 128);
	memset(tab_64khz_4_smooth, 0, 64);
	memset(tab_179mhz_4_smooth, 0, 64);
	memset(tab_16bit_4_smooth, 0, 128);
	memset(tab_64khz_4_buzzy, 0, 64);
	memset(tab_179mhz_4_buzzy, 0, 64);
	memset(tab_16bit_4_buzzy, 0, 128);
	memset(tab_64khz_a_pure, 0, 64);
	memset(tab_179mhz_a_pure, 0, 64);
	memset(tab_16bit_a_pure, 0, 128);
	memset(tab_64khz_c_buzzy, 0, 64);
	memset(tab_179mhz_c_buzzy, 0, 64);
	memset(tab_16bit_c_buzzy, 0, 128);
	memset(tab_64khz_c_gritty, 0, 64);
	memset(tab_179mhz_c_gritty, 0, 64);
	memset(tab_16bit_c_gritty, 0, 128);
	memset(tab_15khz_a_pure, 0, 64);
	memset(tab_15khz_c_buzzy, 0, 64);

	//if base tuning is null, make sure to reset it, else the program could crash!
	if (!g_basetuning)
	{
		g_basetuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
		g_basenote = 3;	//3 = A-
		g_temperament = 0;	//no temperament 
		MessageBox(g_hwnd, "An invalid tuning configuration has been detected!\n\nTuning has been reset to default parameters.", "Tuning error", MB_ICONERROR); 
	}

	//calculate the custom ratio used for each semitone
	g_UNISON = (double)g_UNISON_L / (double)g_UNISON_R;
	g_MIN_2ND = (double)g_MIN_2ND_L / (double)g_MIN_2ND_R;
	g_MAJ_2ND = (double)g_MAJ_2ND_L / (double)g_MAJ_2ND_R;
	g_MIN_3RD = (double)g_MIN_3RD_L / (double)g_MIN_3RD_R;
	g_MAJ_3RD = (double)g_MAJ_3RD_L / (double)g_MAJ_3RD_R;
	g_PERF_4TH = (double)g_PERF_4TH_L / (double)g_PERF_4TH_R;
	g_TRITONE = (double)g_TRITONE_L / (double)g_TRITONE_R;
	g_PERF_5TH = (double)g_PERF_5TH_L / (double)g_PERF_5TH_R;
	g_MIN_6TH = (double)g_MIN_6TH_L / (double)g_MIN_6TH_R;
	g_MAJ_6TH = (double)g_MAJ_6TH_L / (double)g_MAJ_6TH_R;
	g_MIN_7TH = (double)g_MIN_7TH_L / (double)g_MIN_7TH_R;
	g_MAJ_7TH = (double)g_MAJ_7TH_L / (double)g_MAJ_7TH_R;
	g_OCTAVE = (double)g_OCTAVE_L / (double)g_OCTAVE_R;
	
	//All the tables that could be calculated will be generated inside this entire block
	for (int d = 0x00; d < 0xE0; d += 0x20)
	{
		if (d == 0x00 || d == 0x60 || d == 0x80) continue;	//no good use yet
		
		int note_offset[4] = { 0 };
		int dist_2_offset[4] = { 12, 0, 48, 24 };
		int dist_4_smooth_offset[4] = { 12, 0, 24, 24 };
		int dist_4_buzzy_offset[4] = { 12, 0, 12, 24 };
		int dist_a_offset[4] = { 48, 24, 108, 24 };
		int dist_c_buzzy_offset[4] = { 24, 12, 84, 24 };
		int dist_c_gritty_offset[4] = { 12, 0, 72, 24 };
		int dist_c_unstable_offset[4] = { 36, 0, 96, 24 };

		bool IS_15KHZ = 0;
		bool IS_179MHZ = 0;
		bool IS_16BIT = 0;
		int dist_counter = 0;	//a shitty hack but who will call the police really?

		if (d == 0x40)
		{
			IS_SMOOTH_DIST_4 = 1;	//iteration 1: Smooth 
			IS_BUZZY_DIST_4 = 0;
			IS_UNSTABLE_DIST_4_1 = 0;
			IS_UNSTABLE_DIST_4_2 = 0;
		}
		if (d == 0xC0)
		{
			IS_BUZZY_DIST_C = 1;	//iteration 1: Buzzy
			IS_GRITTY_DIST_C = 0;
			IS_UNSTABLE_DIST_C = 0;
		}

repeat_dist:
		for (int c = 0; c < 4; c++)
		{
			if (d == 0x20) note_offset[c] = dist_2_offset[c];
			else if (d == 0x40) 
			{
				if (IS_SMOOTH_DIST_4) note_offset[c] = dist_4_smooth_offset[c];
				else if (IS_BUZZY_DIST_4) note_offset[c] = dist_4_buzzy_offset[c];
			}
			else if (d == 0xA0) note_offset[c] = dist_a_offset[c];
			else if (d == 0xC0)
			{
				if (IS_BUZZY_DIST_C) note_offset[c] = dist_c_buzzy_offset[c];
				else if (IS_GRITTY_DIST_C) note_offset[c] = dist_c_gritty_offset[c];
				else if (IS_UNSTABLE_DIST_C) note_offset[c] = dist_c_unstable_offset[c];
			}
			if (!note_offset[c]) continue;	//if no offset, no table should be created
			if (c == 0) { IS_15KHZ = 0; IS_179MHZ = 0; IS_16BIT = 0; }
			if (c == 1) { IS_15KHZ = 1; IS_179MHZ = 0; IS_16BIT = 0; }
			if (c == 2) { IS_15KHZ = 0; IS_179MHZ = 1; IS_16BIT = 0; }
			if (c == 3) { IS_15KHZ = 0; IS_179MHZ = 0; IS_16BIT = 1; }
			macro_table_gen(d, note_offset[c], IS_15KHZ, IS_179MHZ, IS_16BIT);
		}
		if (d == 0x40)
		{
			dist_counter++;	//the number of times the counter was used in the Distortion 4 case
			if (dist_counter == 1)
			{
				IS_SMOOTH_DIST_4 = 0;
				IS_BUZZY_DIST_4 = 1;		//iteration 2: Buzzy
				goto repeat_dist;
			}
			else IS_BUZZY_DIST_4 = 0;	//done all Distortion 4 modes
		} 
		if (d == 0xC0)
		{
			dist_counter++;	//the number of times the counter was used in the Distortion C case
			if (dist_counter == 1)
			{
				IS_BUZZY_DIST_C = 0;
				IS_GRITTY_DIST_C = 1;		//iteration 2: Gritty
				goto repeat_dist;
			}
			else if (dist_counter == 2)
			{
				IS_GRITTY_DIST_C = 0;
				IS_UNSTABLE_DIST_C = 1;		//iteration 3: Unstable
				goto repeat_dist;
			}
			else IS_UNSTABLE_DIST_C = 0;	//done all Distortion C modes
		}
	}
	for (int i = 0; i < 64; i++) //8-bit tables 
	{
		g_atarimem[RMT_FRQTABLES + 0x000 + i] = tab_64khz_2[i];
		g_atarimem[RMT_FRQTABLES + 0x040 + i] = tab_179mhz_2[i];
		g_atarimem[RMT_FRQTABLES + 0x200 + i] = tab_64khz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x240 + i] = tab_179mhz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x400 + i] = tab_64khz_c_gritty[i];
		g_atarimem[RMT_FRQTABLES + 0x440 + i] = tab_179mhz_c_gritty[i];
		g_atarimem[RMT_FRQTABLES + 0x580 + i] = tab_15khz_a_pure[i];
		g_atarimem[RMT_FRQTABLES + 0x5C0 + i] = tab_15khz_c_buzzy[i];
	}
	for (int i = 0; i < 52; i++) //8-bit tables (fill only the bytes that actually have valid pitches for each combinations) 
	{
		g_atarimem[RMT_FRQTABLES + 0x140 + i] = tab_179mhz_4_buzzy[i + 12];
		g_atarimem[RMT_FRQTABLES + 0x300 + i] = tab_64khz_c_gritty[i + 12];
		g_atarimem[RMT_FRQTABLES + 0x340 + i] = tab_179mhz_c_gritty[i + 12];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion 4 1.79mhz Smooths)
	{
		if (tab_179mhz_4_smooth[i] == 0xFF) continue;	//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x140 + i] = tab_179mhz_4_smooth[i];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion C 64khz Buzzies)
	{
		if (tab_64khz_c_buzzy[i] == 0xFF) continue;		//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x300 + i] = tab_64khz_c_buzzy[i];
	}
	for (int i = 0; i < 64; i++) //8-bit tables (Distortion C 1.79mhz Buzzies)
	{
		if (tab_179mhz_c_buzzy[i] == 0xFF) continue;	//no useful pitch
		g_atarimem[RMT_FRQTABLES + 0x340 + i] = tab_179mhz_c_buzzy[i];
	}
	for (int i = 0; i < 64; i++) //16-bit tables LSB
	{
		g_atarimem[RMT_FRQTABLES + 0x080 + (i * 2)] = tab_16bit_2[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x180 + (i * 2)] = tab_16bit_4_smooth[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x280 + (i * 2)] = tab_16bit_a_pure[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x380 + (i * 2)] = tab_16bit_c_buzzy[i * 2] & 0x00FF;
		g_atarimem[RMT_FRQTABLES + 0x480 + (i * 2)] = tab_16bit_c_gritty[i * 2] & 0x00FF;
	}
	for (int i = 0; i < 64; i++) //16-bit tables MSB
	{
		g_atarimem[RMT_FRQTABLES + 0x080 + (i * 2) + 1] = tab_16bit_2[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x180 + (i * 2) + 1] = tab_16bit_4_smooth[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x280 + (i * 2) + 1] = tab_16bit_a_pure[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x380 + (i * 2) + 1] = tab_16bit_c_buzzy[i * 2] >> 8;
		g_atarimem[RMT_FRQTABLES + 0x480 + (i * 2) + 1] = tab_16bit_c_gritty[i * 2] >> 8;
	}
}
