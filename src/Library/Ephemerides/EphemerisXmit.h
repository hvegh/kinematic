#ifndef EPHEMERISXMIT_INCLUDED
#define EPHEMERISXMIT_INCLUDED

#include "Util.h"
#include "Ephemeris.h"
#include "NavFrame.h"
#include "Frame.h"

static const double AccuracyIndex[16] = {2.4, 3.4, 4.85, 6.85, 9.65, 
	  13.65, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144, INFINITY};


struct EphemerisXmit: public Ephemeris
{
    virtual bool Valid(Time t);
    virtual double Accuracy(Time t);
	virtual bool SatPos(Time t, Position& XmitPos, double& Adjust);

	EphemerisXmit(int Sat, const char* description = "Xmit Ephemeris");
	virtual ~EphemerisXmit();

	bool AddFrame(NavFrame& frame);
	bool AddFrame(Frame& frame);
	bool GetFrame(Frame& frame);

	// Convert accuracy index to meters (and back)
	int AccToSvacc(double acc);
	double SvaccToAcc(int svacc);
	virtual void Display(const char* str = "");

//protected:

    // Validity
    Time MinTime;
    Time MaxTime;

	// Clock adjustment
	Time t_oc;                // reference time of clock
	double a_f0, a_f1, a_f2;  // polynomial equation

	// Orbit parameters. All angles are radians.
	Time t_oe;       // Reference time of ephemeris
	double M_0;      // Mean Anomaly at Reference Time
	double delta_n;  // Mean motion difference from computed value
	double e;        // Eccentricity
	double sqrt_A;   // Square Root of the Semi-Major Axis
    double Omega_0;  // Longitude of Ascending node at weekly epoch.
	double i_0;      // Inclination angle at reference time
	double omega;     // Argument of Perigee
	double OmegaDot;  // Rate of right ascension
	double IDOT;      // Rate inclination angle
	double C_uc;      // Amplitude of Cosine harmonic - Latitude
	double C_us;      // Amplitude of Sine harminic - Latitude
	double C_rc;      // Amplitude of Cosine harmonic - radius
	double C_rs;      // Amplitude of Sine harmonic - radius
	double C_ic;      // Amplitude of Cosine harmonic - inclination
	double C_is;      // Amplitude of Sine harmonic - inclination

	// L2 information
	int L2Pcode;
	int L2Code;
	double t_gd;      // L2 adjustment(seconds) ????

	// Status
	int PrnId;
    int Health;
	int iode;
	int iodc;
	double acc;       // accuracy in meters
};


#endif // EPHEMERISXMIT_INCLUDED

