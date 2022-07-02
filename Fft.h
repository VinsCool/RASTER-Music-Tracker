#include "stdafx.h"

#if !defined FFT_H
#define FFT_H
//------------------------------------
//  fft.h
//  Fast Fourier Transform
//  (c) Reliable Software, 1996
//------------------------------------

#include "complex.h"
//#include "wassert.h"

class SampleIter
{
public:
	int Count() { return 4;};
	void Advance();
	double GetSample() {return 2;};
};

class Fft
{
public:
    Fft (int Points, long sampleRate);
    ~Fft ();
    int     Points () const { return _Points; }
    void    Transform ();
//    void    CopyIn (SampleIter& iter);
	void CopySample(int i);

    double  GetIntensity (int i) const
    { 
//        Assert (i < _Points);
        return _X[i].Mod()/_sqrtPoints; 
    }

    int     GetFrequency (int point) const
    {
        // return frequency in Hz of a given point
//        Assert (point < _Points);
        long x =_sampleRate * point;
        return x / _Points;
    }

    int     HzToPoint (int freq) const 
    { 
        return (long)_Points * freq / _sampleRate; 
    }

    int     MaxFreq() const { return _sampleRate; }

    int     Tape (int i) const
    {
//        Assert (i < _Points);
        return (int) _aTape[i];
    }

//private:

    void PutAt ( int i, double val )
    {
        _X [_aBitRev[i]] = Complex (val);
    }

	int GetNotes(int& n1, int& n2, int& n3);

    int			_Points;
    long		_sampleRate;
    int			_logPoints;
    double		_sqrtPoints;
    int		   *_aBitRev;       // bit reverse vector
    Complex	   *_X;             // in-place fft array
    Complex	  **_W;             // exponentials
    double     *_aTape;         // recording tape
};

#endif
