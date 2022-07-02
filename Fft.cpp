#include "stdafx.h"
//------------------------------------
//  fft.cpp
//  The implementation of the 
//  Fast Fourier Transform algorithm
//  (c) Reliable Software, 1996
//------------------------------------
#include "fft.h"
//#include "recorder.h"

// log (1) = 0, log(2) = 1, log(3) = 2, log(4) = 2 ...

#define PI (2.0 * asin(1.0))

// Points must be a power of 2

Fft::Fft (int Points, long sampleRate)
: _Points (Points), _sampleRate (sampleRate)
{
    _aTape = new double [_Points];

    for (int i = 0; i < _Points; i++)
        _aTape[i] = 0;

    _sqrtPoints = sqrt((double)_Points);
    // calculate binary log
    _logPoints = 0;
    Points--;
    while (Points != 0)
    {
        Points >>= 1;
        _logPoints++;
    }

    _aBitRev = new int [_Points];
    _X = new Complex[_Points];
    _W = new Complex* [_logPoints+1];
    // Precompute complex exponentials
    int _2_l = 2;
    for (int l = 1; l <= _logPoints; l++)
    {
        _W[l] = new Complex [_Points];

        for ( int i = 0; i < _Points; i++ )
        {
            double re =  cos (2. * PI * i / _2_l);
            double im = -sin (2. * PI * i / _2_l);
            _W[l][i] = Complex (re, im);
        }
        _2_l *= 2;
    }

    // set up bit reverse mapping
    int rev = 0;
    int halfPoints = _Points/2;
    for (i = 0; i < _Points - 1; i++)
    {
        _aBitRev[i] = rev;
        int mask = halfPoints;
        // add 1 backwards
        while (rev >= mask)
        {
            rev -= mask; // turn off this bit
            mask >>= 1;
        }
        rev += mask;
    }
    _aBitRev [_Points-1] = _Points-1;



	for (i = 0; i < _Points; i++)
		PutAt (i, _aTape[i]);
}

Fft::~Fft()
{
    delete []_aTape;
    delete []_aBitRev;
    for (int l = 1; l <= _logPoints; l++)
    {
        delete []_W[l];
    }
    delete []_W;
    delete []_X;
}

void Fft::CopySample(int i)
{
	double freqHz=i;
    for (i = 0; i < _Points; i++)
    {
//		_aTape[i] = 1600 * sin (2 * PI * freqHz * i / _sampleRate);
		_aTape[i] = 1600 * sin (2 * PI * freqHz * i / _sampleRate);
//		_aTape[i] += 1600 * sin (2 * PI * freqHz/2 * i / _sampleRate);
		_aTape[i] += (rand()%1600) * sin (2 * PI * freqHz * i / _sampleRate);
	}

	for (i = 0; i < _Points; i++)
		PutAt (i, _aTape[i]);
}


/*
void Fft::CopyIn (SampleIter& iter)
{
    int cSample = iter.Count();
    if (cSample > _Points)
        return;

    // make space for cSample samples at the end of tape
    // shifting previous samples towards the beginning
    memmove (_aTape, &_aTape[cSample], 
              (_Points - cSample) * sizeof(double));
    // copy samples from iterator to tail end of tape
    int iTail  = _Points - cSample;
    for (int i = 0; i < cSample; i++, iter.Advance())
    {
        _aTape [i + iTail] = (double) iter.GetSample();
    }
    // Initialize the FFT buffer
    for (i = 0; i < _Points; i++)
        PutAt (i, _aTape[i]);
}

//
//               0   1   2   3   4   5   6   7
//  level   1
//  step    1                                     0
//  increm  2                                   W 
//  j = 0        <--->   <--->   <--->   <--->   1
//  level   2
//  step    2
//  increm  4                                     0
//  j = 0        <------->       <------->      W      1
//  j = 1            <------->       <------->   2   W
//  level   3                                         2
//  step    4
//  increm  8                                     0
//  j = 0        <--------------->              W      1
//  j = 1            <--------------->           3   W      2
//  j = 2                <--------------->            3   W      3
//  j = 3                    <--------------->             3   W
//                                                              3
//
*/

void Fft::Transform ()
{
    // step = 2 ^ (level-1)
    // increm = 2 ^ level;
    int step = 1;
    for (int level = 1; level <= _logPoints; level++)
    {
        int increm = step * 2;
        for (int j = 0; j < step; j++)
        {
            // U = exp ( - 2 PI j / 2 ^ level )
            Complex U = _W [level][j];
            for (int i = j; i < _Points; i += increm)
            {
                // butterfly
                Complex T = U;
                T *= _X [i+step];
                _X [i+step] = _X[i];
                _X [i+step] -= T;
                _X [i] += T;
            }
        }
        step *= 2;
    }
}

int NoteByFrq(double frq)
{
#define NMULTIPLE	1.05946309435929526456182529494634
#define NOTE_C1	65.4063913251496586694624983808262
#define N	0.0152890257318857189642022813860599

	double f1=NOTE_C1;
	double f2;
	for(int i=1;i<8*12;i++)
	{
		f2=f1*NMULTIPLE;
		if (frq>=f1 && frq<f2)
		{
			if ( frq < ((f2-f1)/2)) 
				return i-1;
			else
				return i;
		}
		f1=f2;
	}
	return -1;
}

int Fft::GetNotes(int& n1, int& n2, int& n3)
{
	n1=n2=n3=-1;

	int index=0;
	int index2=0;
	int index3=0;
	double max=0;
	double max2=0;
	double max3=0;
	double freq=0;
	int i;
	
	for(i=0;i<_Points/2;i++)
	{
		if(GetIntensity(i)>max)
		{
			max=GetIntensity(i);
			index=i;
		}
	}
	if (!index) return 0;
	freq=GetFrequency(index);
	n1 = NoteByFrq(freq);
	if (n1<0) return 0;
	
	//hleda druhou a treti
	//max=GetIntensity(index-1);
	//if (GetIntensity(index+1)>max) max=GetIntensity(index+1);
	//max2=max;

	max2=max/2;
	max3=max2;
	index2=0;
	index3=0;
	for(i=0;i<_Points/2;i++)
	{
		if (i==index) continue;		//vynecha tu uz zdetekovanou
		if(GetIntensity(i)>max3)
		{
			if (GetIntensity(i)>max2)
			{
				max3=max2;
				index3=index2;
				max2=GetIntensity(i);
				index2=i;
			}
			else
			{
				max3=GetIntensity(i);
				index3=i;
			}
		}
	}
	if (!index2) return 1;
	freq=GetFrequency(index2);
	n2 = NoteByFrq(freq);
	if (n2<0) return 1;

	if (!index3) return 2;
	freq=GetFrequency(index3);
	n3 = NoteByFrq(freq);
	if (n3<0) return 2;

	return 3;
}
