// POKEY Frequencies Calculator
// by VinsCool
// Based on the code originally used for the Pitch Calculations in Raster Music Tracker 1.31+
// Backported to RMT with additional improvements
//
// TODO: further cleanup, better documentation, better structure, fix any bug I may have missed so far

#include "tuning.h"
#include "global.h"


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

		case TIMBRE_BUZZY_4:
			if (MOD3 || MOD5 || MOD31) audf = delta_audf(pitch, audf, coarse_divisor, divisor, cycle, timbre);
			break;

		case TIMBRE_SMOOTH_4:
			if (!(MOD3 || CLOCK_15) || MOD5) audf = delta_audf(pitch, audf, coarse_divisor, divisor, cycle, timbre);
			if (!JOIN_16BIT && audf > 0xFF)
			{	//use the buzzy timbre on the lower range instead
				audf = get_audf(pitch, coarse_divisor, 232.5, cycle);
				MOD3 = ((audf + cycle) % 3 == 0);
				MOD5 = ((audf + cycle) % 5 == 0);
				if (MOD3 || MOD5) audf = delta_audf(pitch, audf, coarse_divisor, 232.5, cycle, TIMBRE_BUZZY_4);
			}
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


//TODO: optimise further 
double CTuning::GetTruePitch(double tuning, int temperament, int basenote, int semitone)
{
	int notesnum = 12;	//unless specified otherwise
	int note = (semitone + basenote) % notesnum;	//current note
	double ratio = 0;	//will be used for calculations
	double octave = 2;	//an octave is usually the frequency of a note multiplied by 2
	double multi = 1; //octave multiplyer for the ratio

	//Equal temperament is generated using the 12th root of 2
	if (temperament == NO_TEMPERAMENT)
	{
		ratio = pow(2.0, 1.0 / 12.0);
		return (tuning / 64) * pow(ratio, semitone + basenote);
	}
	if (temperament >= TUNING_CUSTOM)	//custom temperament will be used using ratio
	{
		octave = CUSTOM[notesnum];
		ratio = CUSTOM[note];
	}
	else	//any temperament preset will be used
	{
		for (int i = 0; i < PRESETS_LENGTH; i++)
		{
			if (temperament_preset[temperament][i]) continue;
			notesnum = i - 1;
			break;
		}
		octave = temperament_preset[temperament][notesnum];
		note = (semitone + basenote) % notesnum;
		ratio = temperament_preset[temperament][note];
	}
	multi = pow(octave, trunc((semitone + basenote) / notesnum));
	return (tuning / 64) * (multi * ratio);
}

void CTuning::init_tuning()
{
	//if base tuning is null, make sure to reset it, else the program could crash!
	if (!g_basetuning)
	{
		g_Song.ResetTuningVariables();	//TODO(?): move this function here instead
		MessageBox(g_hwnd, "An invalid tuning configuration has been detected!\n\nTuning has been reset to default parameters.", "Tuning error", MB_ICONERROR); 
		return;	//without initialisation, the function must be called at an ulterior time
	}

	g_notesperoctave = 12;

	if (g_temperament > NO_TEMPERAMENT && g_temperament < TUNING_CUSTOM)
	{
		for (int i = 0; i < PRESETS_LENGTH; i++)
		{
			if (temperament_preset[g_temperament][i]) continue;
			g_notesperoctave = i - 1;
			break;
		}
	}

	//calculate the custom ratio used for each semitone
	//TODO: restructure this to something much better, and flexible
	//these individual variables are just too uncomfortable to use that way 
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

	//Generate all lookup tables used by the RMT driver for tuning purposes
	//TODO: optimise this procedure, even if right now this is much better than what it used to be

	//Distortion 2, at 0xB000
	generate_table(g_atarimem + RMT_FRQTABLES + 0x000, 64, dist_2_bell.table_64khz * g_notesperoctave, TIMBRE_BELL, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x040, 64, dist_2_bell.table_179mhz * g_notesperoctave, TIMBRE_BELL, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x080, 64, dist_2_bell.table_16bit * g_notesperoctave, TIMBRE_BELL, 0x50);
	//no 15kHz table...

	//Distortion 4 (Smooth), at 0xB100
	generate_table(g_atarimem + RMT_FRQTABLES + 0x100, 64, dist_4_smooth.table_64khz * g_notesperoctave, TIMBRE_SMOOTH_4, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x140, 64, dist_4_smooth.table_179mhz * g_notesperoctave, TIMBRE_SMOOTH_4, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x180, 64, dist_4_smooth.table_16bit * g_notesperoctave, TIMBRE_SMOOTH_4, 0x50);
	//no 15kHz table...

	//Distortion A (Pure), at 0xB200
	generate_table(g_atarimem + RMT_FRQTABLES + 0x200, 64, dist_a_pure.table_64khz * g_notesperoctave, TIMBRE_PURE, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x240, 64, dist_a_pure.table_179mhz * g_notesperoctave, TIMBRE_PURE, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x280, 64, dist_a_pure.table_16bit * g_notesperoctave, TIMBRE_PURE, 0x50);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x580, 64, dist_a_pure.table_15khz * g_notesperoctave, TIMBRE_PURE, 0x01);

	//Distortion C (Buzzy), at 0xB300
	generate_table(g_atarimem + RMT_FRQTABLES + 0x300, 64, dist_c_buzzy.table_64khz * g_notesperoctave, TIMBRE_BUZZY_C, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x340, 64, dist_c_buzzy.table_179mhz * g_notesperoctave, TIMBRE_BUZZY_C, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x380, 64, dist_c_buzzy.table_16bit * g_notesperoctave, TIMBRE_BUZZY_C, 0x50);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x5C0, 64, dist_c_buzzy.table_15khz * g_notesperoctave, TIMBRE_BUZZY_C, 0x01);

	//Distortion C (Buzzy), at 0xB300
	generate_table(g_atarimem + RMT_FRQTABLES + 0x400, 64, dist_c_gritty.table_64khz * g_notesperoctave, TIMBRE_GRITTY_C, 0x00);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x440, 64, dist_c_gritty.table_179mhz * g_notesperoctave, TIMBRE_GRITTY_C, 0x40);
	generate_table(g_atarimem + RMT_FRQTABLES + 0x480, 64, dist_c_gritty.table_16bit * g_notesperoctave, TIMBRE_GRITTY_C, 0x50);
	//no 15kHz table...
}
