// POKEY Frequencies Calculator
// by VinsCool
// Based on the code originally used for the Pitch Calculations in Raster Music Tracker 1.31+
// Backported to RMT with additional improvements

#include "tuning.h"
#include "global.h"

/*
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
*/


//Parts of this code was rewritten for POKEY Frequencies Calculator, then backported to RMT 1.31+
double CTuning::generate_freq(int audc, int audf, int audctl, int channel)
{
	//variables for pitch calculation, divisors must never be 0!
	double divisor = 1;	
	int coarse_divisor = 1;
	int cycle = 1;

	//register variables 
	int distortion = audc & 0xf0;
	//int skctl = 0;	//not yet implemented in calculations
	//bool TWO_TONE = (skctl == 0x8B) ? 1 : 0;
	bool CLOCK_15 = audctl & 0x01;
	bool HPF_CH24 = audctl & 0x02;
	bool HPF_CH13 = audctl & 0x04;
	bool JOIN_34 = audctl & 0x08;
	bool JOIN_12 = audctl & 0x10;
	bool CH3_179 = audctl & 0x20;
	bool CH1_179 = audctl & 0x40;
	bool POLY9 = audctl & 0x80;

	//combined modes for some special output...
	bool JOIN_16BIT = ((JOIN_12 && CH1_179 && (channel == 1 || channel == 5)) || (JOIN_34 && CH3_179 && (channel == 3 || channel == 7))) ? 1 : 0;
	bool CLOCK_179 = ((CH1_179 && (channel == 0 || channel == 4)) || (CH3_179 && (channel == 2 || channel == 6))) ? 1 : 0;
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

	//Many combinations depend entirely on the Modulo of POKEY frequencies to generate different tones
	//If a known value provide unstable results, it may be avoided on purpose 
	bool MOD3 = ((audf + cycle) % 3 == 0);
	bool MOD5 = ((audf + cycle) % 5 == 0);
	bool MOD7 = ((audf + cycle) % 7 == 0);
	bool MOD15 = ((audf + cycle) % 15 == 0);
	bool MOD31 = ((audf + cycle) % 31 == 0);
	bool MOD73 = ((audf + cycle) % 73 == 0);

	switch (distortion)
	{
	case 0x00:
		if (POLY9)
		{
			divisor = 255.5;	//Metallic Buzzy
			if (MOD7 || (!CLOCK_15 && !CLOCK_179 && !JOIN_16BIT)) divisor = 36.5;	//seems to only sound "uniform" in 64kHz mode for some reason 
			if (MOD31 || MOD73) return 0;	//MOD31 and MOD73 values are invalid 
		}
		break;

	case 0x20:
	case 0x60:	//Duplicate of Distortion 2
		divisor = 31;
		if (MOD31) return 0;
		break;

	case 0x40:
		divisor = 232.5;		//Buzzy tones, neither MOD3 or MOD5 or MOD31
		if (MOD3 || CLOCK_15) divisor = 77.5;	//Smooth tones, MOD3 but not MOD5 or MOD31
		if (MOD5) divisor = 46.5;	//Unstable tones #1, MOD5 but not MOD3 or MOD31
		if (MOD31) divisor = (MOD3 || MOD5) ? 2.5 : 7.5;	//Unstables Tones #2 and #3, MOD31, with MOD3 or MOD5 
		if (MOD15 || (MOD5 && CLOCK_15)) return 0;	//Both MOD3 and MOD5 at once are invalid 
		break;

	case 0x80:
		if (POLY9)
		{
			divisor = 255.5;	//Metallic Buzzy
			if (MOD7 || (!CLOCK_15 && !CLOCK_179 && !JOIN_16BIT)) divisor = 36.5;	//seems to only sound "uniform" in 64kHz mode for some reason 
			if (MOD73) return 0;	//MOD73 values are invalid
		}
		break;

	case 0xC0:
		divisor = 7.5;	//Gritty tones, neither MOD3 or MOD5
		if (MOD3 || CLOCK_15) divisor = 2.5;	//Buzzy tones, MOD3 but not MOD5
		if (MOD5) divisor = 1.5;	//Unstable Buzzy tones, MOD5 but not MOD3
		if (MOD15 || (MOD5 && CLOCK_15)) return 0;	//Both MOD3 and MOD5 at once are invalid 
		break;		
	}
	return get_pitch(audf, coarse_divisor, divisor, cycle);
}

//this code was originally added in POKEY Frequencies Calculator, and adapted for RMT 1.31+
void CTuning::generate_table(unsigned char* table, int length, int semitone, int timbre, int audctl)
{
	//variables for pitch calculation, divisors must never be 0!
	double divisor = 1;
	int coarse_divisor = 1;
	int cycle = 1;

	//register variables 
	//int distortion = timbre & 0xF0;
	//int skctl = 0;	//not yet implemented in calculations
	//bool TWO_TONE = (skctl == 0x8B) ? 1 : 0;
	bool CLOCK_15 = audctl & 0x01;
	bool HPF_CH24 = audctl & 0x02;
	bool HPF_CH13 = audctl & 0x04;
	bool JOIN_34 = audctl & 0x08;
	bool JOIN_12 = audctl & 0x10;
	bool CH3_179 = audctl & 0x20;
	bool CH1_179 = audctl & 0x40;
	bool POLY9 = audctl & 0x80;

	//combined modes for some special output... 
	//the channel number doesn't actually matter for creating tables, so the parameter is omitted
	bool JOIN_16BIT = ((JOIN_12 && CH1_179) || (JOIN_34 && CH3_179)) ? 1 : 0;
	bool CLOCK_179 = (CH1_179 || CH3_179) ? 1 : 0;
	if (JOIN_16BIT || CLOCK_179) CLOCK_15 = 0;	//override, these 2 take priority over 15khz mode if they are enabled at the same time

	//TODO: apply Two-Tone timer offset into calculations when channel 1+2 are linked in 1.79mhz mode
	//This would help generating tables using patterns discovered by synthpopalooza
	if (JOIN_16BIT) cycle = 7;
	else if (CLOCK_179) cycle = 4;
	else coarse_divisor = (CLOCK_15) ? 114 : 28;

	//Many combinations depend entirely on the Modulo of POKEY frequencies to generate different tones
	//If a known value provide unstable results, it may be avoided on purpose 
	bool MOD3 = 0;
	bool MOD5 = 0;
	bool MOD7 = 0;
	bool MOD15 = 0;
	bool MOD31 = 0;
	bool MOD73 = 0;

	//Use the modulo flags to make sure the correct timbre will be output
	switch (timbre)
	{
	case TIMBRE_PINK_NOISE:
		break;

	case TIMBRE_BROWNIAN_NOISE:
		divisor = 36.5;	//Brownian noise, not MOD31 and not MOD73
		break;

	case TIMBRE_FUZZY_NOISE:
		divisor = 255.5;	//Fuzzy noise, not MOD7, not MOD31 and not MOD73
		break;

	case TIMBRE_BELL:
		divisor = 31;	//Bell tones, not MOD31
		break;

	case TIMBRE_BUZZY_4:
		divisor = 232.5;	//Buzzy tones, neither MOD3 or MOD5 or MOD31
		break;

	case TIMBRE_SMOOTH_4:
		divisor = 77.5;	//Smooth tones, MOD3 but not MOD5 or MOD31
		break;

	case TIMBRE_WHITE_NOISE:
		break;

	case TIMBRE_METALLIC_NOISE:
		divisor = 36.5;	//Metallic noise, not MOD73
		break;

	case TIMBRE_BUZZY_NOISE:
		divisor = 255.5;	//Buzzy noise, not MOD7 and not MOD73
		break;

	case TIMBRE_PURE:
		break;

	case TIMBRE_GRITTY_C:
		divisor = 7.5;	//Gritty tones, neither MOD3 or MOD5
		break;

	case TIMBRE_BUZZY_C:
		divisor = 2.5;	//Buzzy tones, MOD3 but not MOD5
		break;

	case TIMBRE_UNSTABLE_C:
		divisor = 1.5;	//Unstable Buzzy tones, MOD5 but not MOD3
		break;

	default:
		//Distortion A is assumed if no valid parameter is supplied 
		break;

	}

	//generate the table using all the initialised parameters 
	for (int i = 0; i < length; i++)
	{
		//get the current semitone 
		int note = i + semitone;

		//calculate the reference pitch using the semitone as an offset
		double pitch = GetTruePitch(g_basetuning, g_temperament, g_basenote, note);

		//get the nearest POKEY frequency using the reference pitch
		int audf = get_audf(pitch, coarse_divisor, divisor, cycle);

		//TODO: insert whatever delta method that could be suitable here... 
		MOD3 = ((audf + cycle) % 3 == 0);
		MOD5 = ((audf + cycle) % 5 == 0);
		MOD7 = ((audf + cycle) % 7 == 0);
		MOD15 = ((audf + cycle) % 15 == 0);
		MOD31 = ((audf + cycle) % 31 == 0);
		MOD73 = ((audf + cycle) % 73 == 0);

		switch (timbre)
		{
		case TIMBRE_BELL:
			if (MOD31) audf = delta_audf(pitch, audf, coarse_divisor, divisor, cycle, timbre);
			break;

		case TIMBRE_GRITTY_C:
			if (MOD3 || MOD5) audf = delta_audf(pitch, audf, coarse_divisor, divisor, cycle, timbre);
			break;

		case TIMBRE_BUZZY_C:
			if (!(MOD3 || CLOCK_15) || MOD5) audf = delta_audf(pitch, audf, coarse_divisor, divisor, cycle, timbre);
			if (!JOIN_16BIT && audf > 0xFF)
			{	//use the gritty timbre on the lower range instead
				audf = get_audf(pitch, coarse_divisor, 7.5, cycle);
				MOD3 = ((audf + cycle) % 3 == 0);
				MOD5 = ((audf + cycle) % 5 == 0);
				if (MOD3 || MOD5) audf = delta_audf(pitch, audf, coarse_divisor, 7.5, cycle, TIMBRE_GRITTY_C); 
			}
			break;
		}

		if (audf < 0) audf = 0;
		if (!JOIN_16BIT && audf > 0xFF) audf = 0xFF;
		if (JOIN_16BIT && audf > 0xFFFF) audf = 0xFFFF;

		//write the POKEY frequency to the table
		if (JOIN_16BIT)
		{	//in 16-bit tables, 2 bytes have to be written contiguously
			table[i * 2] = audf & 0x0FF;	//LSB
			table[i * 2 + 1] = audf >> 8;	//MSB
			continue;
		}
		table[i] = audf; 
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

//TODO: much better than that
//code originally written for POKEY Frequencies Calculator
int CTuning::delta_audf(double pitch, int audf, int coarse_divisor, double divisor, int cycle, int timbre)
{
	int distortion = timbre & 0xF0;

	int tmp_audf_up = audf;		//begin from the currently invalid audf
	int tmp_audf_down = audf;
	double tmp_freq_up = 0;
	double tmp_freq_down = 0;
	double PITCH = 0;

	if (distortion != 0x40 && distortion != 0xC0) { tmp_audf_up++; tmp_audf_down--; }	//anything not distortion 4 or C, simpliest delta method

	else if (distortion == 0x40)
	{
		if (timbre == TIMBRE_SMOOTH_4)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + cycle) % 3 != 0 || (tmp_audf_up + cycle) % 5 == 0 || (tmp_audf_up + cycle) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 3 != 0 || (tmp_audf_down + cycle) % 5 == 0 || (tmp_audf_down + cycle) % 31 == 0) tmp_audf_down--;
			}
		}
		else if (timbre == TIMBRE_BUZZY_4)
		{
			for (int o = 0; o < 6; o++) 
			{
				if ((tmp_audf_up + cycle) % 3 == 0 || (tmp_audf_up + cycle) % 5 == 0 || (tmp_audf_up + cycle) % 31 == 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 3 == 0 || (tmp_audf_down + cycle) % 5 == 0 || (tmp_audf_down + cycle) % 31 == 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely 
	}

	else if (distortion == 0xC0)
	{
		//if (CLOCK_15)
		if (coarse_divisor == 114)	//15kHz mode
		{
			for (int o = 0; o < 3; o++)	//MOD5 must be avoided!
			{
				if ((tmp_audf_up + cycle) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (timbre == TIMBRE_BUZZY_C)	//verify MOD3 integrity
		{
			for (int o = 0; o < 6; o++)
			{
				if ((tmp_audf_up + cycle) % 3 != 0 || (tmp_audf_up + cycle) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 3 != 0 || (tmp_audf_down + cycle) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (timbre == TIMBRE_GRITTY_C)	//verify neither MOD3 or MOD5 is used
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + cycle) % 3 == 0 || (tmp_audf_up + cycle) % 5 == 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 3 == 0 || (tmp_audf_down + cycle) % 5 == 0) tmp_audf_down--;
			}
		}
		else if (timbre == TIMBRE_UNSTABLE_C)	//verify MOD5 integrity
		{
			for (int o = 0; o < 6; o++)	//get the closest compromise up and down first
			{
				if ((tmp_audf_up + cycle) % 3 == 0 || (tmp_audf_up + cycle) % 5 != 0) tmp_audf_up++;
				if ((tmp_audf_down + cycle) % 3 == 0 || (tmp_audf_down + cycle) % 5 != 0) tmp_audf_down--;
			}
		}
		else return 0;	//invalid parameter most likely
	}

	PITCH = get_pitch(tmp_audf_up, coarse_divisor, divisor, cycle);
	tmp_freq_up = pitch - PITCH;	//first delta, up
	PITCH = get_pitch(tmp_audf_down, coarse_divisor, divisor, cycle);
	tmp_freq_down = PITCH - pitch;	//second delta, down
	PITCH = tmp_freq_down - tmp_freq_up;

	if (PITCH > 0) audf = tmp_audf_up; //positive, meaning delta up is closer than delta down
	else audf = tmp_audf_down; //negative, meaning delta down is closer than delta up
	return audf;
}
//

/*
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
*/


//TODO: optimise further, the method in place for loading temperaments is terrible
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

	default:	//Custom ratio, anything can go!
		octave = CUSTOM[notesnum];
		ratio = CUSTOM[note];
		break;
	}

	multi = pow(octave, trunc((semitone + basenote) / notesnum));
	return (tuning / 64) * (multi * ratio);
}


void CTuning::init_tuning()
{
	//if base tuning is null, make sure to reset it, else the program could crash!
	if (!g_basetuning)
	{
		g_basetuning = (g_ntsc) ? 444.895778867913 : 440.83751645933;
		g_basenote = 3;	//3 = A-
		g_temperament = 0;	//no temperament 
		MessageBox(g_hwnd, "An invalid tuning configuration has been detected!\n\nTuning has been reset to default parameters.", "Tuning error", MB_ICONERROR); 
	}

	//calculate the custom ratio used for each semitone
	CUSTOM[0] = (double)g_UNISON_L / (double)g_UNISON_R;
	CUSTOM[1] = (double)g_MIN_2ND_L / (double)g_MIN_2ND_R;
	CUSTOM[2] = (double)g_MAJ_2ND_L / (double)g_MAJ_2ND_R;
	CUSTOM[3] = (double)g_MIN_3RD_L / (double)g_MIN_3RD_R;
	CUSTOM[4] = (double)g_MAJ_3RD_L / (double)g_MAJ_3RD_R;
	CUSTOM[5] = (double)g_PERF_4TH_L / (double)g_PERF_4TH_R;
	CUSTOM[6] = (double)g_TRITONE_L / (double)g_TRITONE_R;
	CUSTOM[7] = (double)g_PERF_5TH_L / (double)g_PERF_5TH_R;
	CUSTOM[8] = (double)g_MIN_6TH_L / (double)g_MIN_6TH_R;
	CUSTOM[9] = (double)g_MAJ_6TH_L / (double)g_MAJ_6TH_R;
	CUSTOM[10] = (double)g_MIN_7TH_L / (double)g_MIN_7TH_R;
	CUSTOM[11] = (double)g_MAJ_7TH_L / (double)g_MAJ_7TH_R;
	CUSTOM[12] = (double)g_OCTAVE_L / (double)g_OCTAVE_R;

	TTuning distortion_a{ 48, 24, 108, 24 };
	generate_table(g_atarimem + RMT_FRQTABLES + 0x200, 64, distortion_a.table_64khz, TIMBRE_PURE, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x240, 64, distortion_a.table_179mhz, TIMBRE_PURE, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x280, 64, distortion_a.table_16bit, TIMBRE_PURE, 0x50);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x580, 64, distortion_a.table_15khz, TIMBRE_PURE, 0x01);

	TTuning distortion_c{ 24, 12, 84, 24 };
	generate_table(g_atarimem + RMT_FRQTABLES + 0x300, 64, distortion_c.table_64khz, TIMBRE_BUZZY_C, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x340, 64, distortion_c.table_179mhz, TIMBRE_BUZZY_C, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x380, 64, distortion_c.table_16bit, TIMBRE_BUZZY_C, 0x50);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x5C0, 64, distortion_c.table_15khz, TIMBRE_BUZZY_C, 0x01);

	TTuning distortion_2{ 12, 0, 48, 24 };
	generate_table(g_atarimem + RMT_FRQTABLES + 0x000, 64, distortion_2.table_64khz, TIMBRE_BELL, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x040, 64, distortion_2.table_179mhz, TIMBRE_BELL, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x080, 64, distortion_2.table_16bit, TIMBRE_BELL, 0x50);

/*
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
*/	
	
/*
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
*/

}
