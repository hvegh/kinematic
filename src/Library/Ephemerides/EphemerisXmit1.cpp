
// EphemerisXmit1 implements the ephemeris as transmitted by the satellites
//    Part of Kinematic, a utility for GPS positioning
//
// Copyright (C) 2005  John Morris    kinetic@precision-gps.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.

//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


#include "EphemerisXmit1.h"

static const double RelativisticConstant = -4.442807633e-10;

EphemerisXmit1::EphemerisXmit1(int Sat, const char* description)
:Ephemeris(Sat, description)
{
	  iode = -1;
      MinTime = 0;
      MaxTime = -1;
      acc = INFINITY;
}

EphemerisXmit1::~EphemerisXmit1(void)
{
}

bool EphemerisXmit1::Valid(Time t)
{
	if (MaxTime > 0 && (t < MinTime || t > MaxTime))
		debug("EphemerisXmit1 - No longer valid\n s=%d  t=%.0f Min=%.0f Max=%.0f\n",
		   SatIndex, S(t), S(MinTime), S(MaxTime) );
    return (t >= MinTime && t <= MaxTime);
}


double EphemerisXmit1::Accuracy(Time t)
{
    if (!Valid(t)) return INFINITY;
    else           return acc;
}


bool EphemerisXmit1::SatPos(Time XmitTime, Position& XmitPos, double& Adjust)
{
	if (!Valid(XmitTime))
		return Error("Ephemeris is not valid\n");

	// Calculate the satellite position, as described in ICD200.
	
	// Semi-major axis
	double A = sqrt_A * sqrt_A;

	// Computed mean motion (rad/sec)
	double n_0 = sqrt(mu)/(A*sqrt_A);

	// time from ephemeris reference epoch
	double t = S(XmitTime - t_oe);

	// corrected mean motion
	double n = n_0 + delta_n;

	// mean anomaly
	double M = M_0 + n*t;

	// Calculate E, Ecentric anomaly, using fix point of Kepler's equation
	double E = M;
	for (int i=0; i<20; i++) 
		E = M + e*sin(E);

	// True anomaly
	double S_nu = sqrt(1-e*e) * sin(E) / (1 - e * cos(E));
	double C_nu = (cos(E) - e) / (1 - e *cos(E));
	double nu = atan2(S_nu, C_nu);
	debug("  S_nu=%.6f  C_nu=%.6f  nu=%.6f\n", S_nu, C_nu, nu);

	// argument of latitude
	double phi = nu + omega;

	// Second harmonic perterbations (latitude, radius, inclination)
	double du = C_uc*cos(2*phi) + C_us*sin(2*phi);
	double dr = C_rc*cos(2*phi) + C_rs*sin(2*phi);
	double di = C_ic*cos(2*phi) + C_is*sin(2*phi);
	debug("  du=%.6f  dr=%.6f  di=%.6f\n", du, dr, di);

	// Corrected latitude, radius, inclination
	double u = phi + du;
	double r = A * (1 - e*cos(E)) + dr;
	double i = i_0 + IDOT*t + di;
	debug("  u=%.6f  r=%.6f  i=%.6f\n", u, r, i);

	// Positions in orbital plane
	double Xdash = r * cos(u);
	double Ydash = r * sin(u);
	debug("  Xdash = %.3f  Ydash=%.3f\n", Xdash, Ydash); 

	// Corrected longitude of ascending node, including transit time
	double Omega_C = Omega_0 - GpsTow(t_oe)*OmegaEDot 
		           + (OmegaDot - OmegaEDot)*t;
	debug("Omega_c=%g  Omega_0=%g  OmegaDot=%g  OmegaEDot=%g  t=%g Tow=%g\n",
		Omega_C, Omega_0, OmegaDot, OmegaEDot, t, GpsTow(t));

	debug("  ToW*OmegaEDot=%.3f  (O-Oe)*t=%.3f\n", GpsTow(t_oe)*OmegaEDot, (OmegaDot-OmegaEDot)*t);
	
	// Earth fixed coordinates
	XmitPos.x = Xdash*cos(Omega_C) - Ydash*cos(i)*sin(Omega_C);
	XmitPos.y = Xdash*sin(Omega_C) + Ydash*cos(i)*cos(Omega_C);
	XmitPos.z = Ydash*sin(i);

	// Clock Adjustment
	double tc = S(XmitTime - t_oc);
    double AdjustClock = (  (a_f2 * tc) + a_f1 ) * tc + a_f0;
	double AdjustRelativity = RelativisticConstant * e * sqrt_A * sin(E);
	Adjust = AdjustClock + AdjustRelativity - t_gd;
	debug("Adjustclock=%g  AdjustRelativity=%g t_gd=%g Adjust=%g\n",
		AdjustClock,AdjustRelativity,t_gd, Adjust);
	
	return OK;
}



bool EphemerisXmit1::AddFrame(NavFrame& f)
{
	f.Display("AddFrame");

	// Make sure the 3 subframes are the same version
	int NewIode = f.GetField(2,61,8);
	int NewIode2 = f.GetField(3,271,8);
	int NewIode3 = f.GetField(1,211,8);
	if (NewIode != NewIode2 || NewIode != NewIode3)
		return Error("Inconsistent Ephemeris for sat=%d iode=(%d,%d,%d)\n",
		                SatIndex, NewIode,NewIode2,NewIode3);

	// If this is the same ephemeris we already have, then done
	if (NewIode == iode)
		return OK;

	// Make note we have a new ephemeris
	debug("New xmit ephemeris  old=%d  new=%d\n", iode, NewIode);
	iode = NewIode;

	// Read the clock data, subframe 1
	int32 iodc = (f.GetField(1,83,2)<<8) + iode;
	int32 WN = f.GetField(1,61,10);
	acc = SvaccToAcc(f.GetField(1,73,4));
	Health = f.GetField(1,77,6);
	t_gd = f.GetSigned(1,197,8) / p2(31);
	double tow_oc = f.GetField(1,219,16) * p2(4);
	t_oc = ConvertGpsTime(WN, tow_oc);
	a_f2 = f.GetSigned(1,241,8) / p2(55);
	a_f1 = f.GetSigned(1,249,16) / p2(43);
	a_f0 = f.GetSigned(1,271,22) / p2(31);
	debug("a_f0=%.9f  bits=0x%06x  signed=0x%08x  p2(31)=%f\n",
		a_f0, f.GetField(1,271,22), f.GetSigned(1,271,22), p2(31));

	// Read the ephemeris, subframe 2
	C_rs = f.GetSigned(2,69,16) / p2(5);
	delta_n = f.GetSigned(2,91,16) * PI / p2(43);
	M_0 = (int32)((f.GetField(2,107,8)<<24) + f.GetField(2,121,24))
		* PI / p2(31);
	C_uc = f.GetSigned(2,151,16) / p2(29);
	debug("C_uc=%.6f  bits=0x%x   p2(29)=%.0f\n",C_uc,f.GetSigned(2,141,16),p2(29));
	e = ((f.GetField(2,167,8)<<24) + f.GetField(2,181,24)) / p2(33);
	C_us = f.GetSigned(2,211,16) / p2(29);
	sqrt_A = ((f.GetField(2,227,8)<<24) + f.GetField(2,241,24)) / p2(19);
	double tow_oe = f.GetField(2,271,16) * p2(4);
	t_oe = ConvertGpsTime(WN, tow_oe);
	int32 FitIntervalFlag = f.GetField(2,286,1);
	int32 AODO = f.GetField(2,287,5);

	// Read the ephemeris, subframe 3
	C_ic = f.GetSigned(3,61,16) / p2(29);
	Omega_0 = (int32)((f.GetField(3,77,8)<<24) + f.GetField(3,91,24)) 
		       * PI / p2(31);
	C_is = f.GetSigned(3,121,16) / p2(29);
	i_0 = (int32)((f.GetField(3,137,8)<<24)+ f.GetField(3,151,24)) 
		       * PI / p2(31);
	C_rc = f.GetSigned(3,181,16) / p2(5);
	omega = (int32)((f.GetField(3,197,8)<<24) + f.GetField(3,211,24)) 
		       * PI / p2(31);
	OmegaDot = f.GetSigned(3,241,24) * PI / p2(43);
	IDOT = f.GetSigned(3,279,14) * PI / p2(43);

	MinTime = t_oe - 2*NsecPerHour;
	MaxTime = t_oe + 2*NsecPerHour;

	Display("Ephemeris from Nav Frame");
	return OK;
}


bool::EphemerisXmit1::AddFrame(Frame& f)
{
	// If we already have this set of values, then done
	if (iode == f.GetField(4, 1, 8))
		return OK;

	// Extract the fields from the RTCM Ephemeris frame
    int WN = f.GetField(  3,  1, 10);
	IDOT =   f.GetSigned( 3, 11, 24) / p2(43);
	iode =   f.GetField(  4,  1,  8);
	double tow_oc = f.GetField( 4, 9, 24) * p2(4);
	t_oc = ConvertGpsTime(WN, tow_oc);
	a_f1 =   f.GetSigned( 5,  1, 16) / p2(43);
	a_f2 =   f.GetSigned( 5, 17, 24) / p2(55);
	C_rs =   f.GetSigned( 6,  1, 16) / p2(5);
	delta_n = (int32)( (f.GetField(6, 17,  24)<<8) |
		              f.GetField(7,  1,   8) )     / p2(43) * PI;
	C_uc =   f.GetSigned( 7,  9, 24) / p2(29);  
	e    =   f.Get32(8) / p2(33);
	C_us =   f.GetSigned(9,  9, 24) / p2(29);
	sqrt_A = ( (f.GetField(10, 1, 24)<<8) | f.GetField(11, 1, 8) ) / p2(19);
	double tow_oe = f.GetField(11,  9, 24) * p2(4);
	t_oe = ConvertGpsTime(WN, tow_oe);
	Omega_0 = f.Get32(12) / p2(31) * PI;
	C_ic =    f.GetSigned(13,  9, 24) / p2(29);
	i_0  =    f.Get32(14) / p2(31) * PI;
	C_is =    f.GetSigned(15,  9, 24) / p2(31);
	omega =   f.Get32(16) / p2(31) * PI;
	C_rc =    f.GetSigned(17,  9, 24) / p2(5);
	OmegaDot= f.GetSigned(18,  1, 24) / p2(43) * PI;
	debug("AddFrame: OmegaDot=%g  bits=0x%08x\n", OmegaDot, f.GetSigned(18,1,24));
	M_0     = f.Get32(19) / p2(32) * PI;
	iodc    = (f.GetField(20,  9, 18)<<8) + iode;
	a_f0    = (int32)( (f.GetSigned(20, 19, 24)<<16)
		              | f.GetField(21,  1, 16) ) / p2(31);
	int PrnId   = f.GetField( 21, 17, 21);
	if (PrnId == 0) PrnId = 32;
	if (SvidToSat(PrnId) != SatIndex)
		return Error("RTCM had mismatched satellite svid\n");
	t_gd    = f.GetSigned(22,  1,  8) / p2(31);
	L2Code  = f.GetField( 22,  9, 10);
	acc     = SvaccToAcc( f.GetField(22, 11, 14));
	Health  = f.GetField( 22, 14, 20);
	L2Pcode = f.GetField( 22, 21, 21);

	// Keep track of times the ephemeris is valid  // TODO 
	MinTime = t_oe - 2*NsecPerHour;
	MaxTime = t_oe + 2*NsecPerHour;

	Display("Added RTCM frame\n");
	return OK;
}

bool EphemerisXmit1::GetFrame(Frame& f)
{
	// Clear the frame
	f.Init(22);

	// Insert the fields, as given in the standard
	f.PutField( 3,  1, 10, GpsWeek(t_oe));
	f.PutField( 3, 11, 24, IDOT*p2(43));
	f.PutField( 4,  1,  8, iode); 
	f.PutField( 4,  9, 24, GpsTow(t_oc)/p2(4));
	f.PutField( 5,  1, 16, a_f1*p2(43));
	f.PutField( 5, 17, 24, a_f2*p2(55));
	f.PutField( 6,  1, 16, C_rs*p2(5));
	int32 i_delta_n = delta_n*p2(43)/PI;  // signed?
	f.PutField( 6, 17, 24, i_delta_n>>8);
	f.PutField( 7,  1,  8, i_delta_n);
	f.PutField( 7,  9, 24, C_uc*p2(29));
	f.Put32(    8,         e*p2(33));
	f.PutField( 9,  9, 24, C_us*p2(29));
	f.Put32(   10,         sqrt_A*p2(19));
	f.PutField(11,  9, 24, GpsTow(t_oe)/p2(4));
	f.Put32(12,            Omega_0*p2(31)/PI);
	f.PutField(13,  9, 24, C_ic*p2(29));
	f.Put32   (14,         i_0*p2(31)/PI);
	f.PutField(15,  9, 24, C_is*p2(31));
	f.Put32   (16,         omega*p2(31)/PI);
	f.PutField(17,  9, 24, C_rc*p2(5));
	f.PutField(18,  1, 24, OmegaDot*p2(43)/PI);
	f.Put32   (19,         M_0*p2(32)/PI);
	f.PutField(20,  9, 18, iodc>>8);
	int32 i_a_f0 = a_f0*p2(31);
	f.PutField(20, 19, 24, i_a_f0>>16);
	f.PutField(21,  1, 16, i_a_f0);
	f.PutField(21, 17, 21, SatToSvid(SatIndex));
	f.PutField(21, 22, 24, 0x3);   // FILL
	f.PutField(22,  1,  8, t_gd*p2(31));
	f.PutField(22,  9, 10, L2Code);
	f.PutField(22, 11, 14, AccToSvacc(acc));
	f.PutField(22, 14, 20, Health);
	f.PutField(22, 21, 21, L2Pcode);
	f.PutField(22, 22, 24, 0x3);  // FILL

	Display("About to write RTCM ephemeris");

	return OK;
}


void EphemerisXmit1::Display(const char* str)
{
	debug("Broadcast Ephemeris[%d] %s  %s  at 0x%p\n", 
		SatIndex, Description, str, this);
	if (iode == -1) {
		debug("    Not Initialized\n");
		return;
	}

	debug("   Mintime=%.0f  Maxtime=%.0f  Health=%d Accuracy=%.3f\n", 
		      S(MinTime), S(MaxTime), Health, acc);
	debug("  iode=%d  t_oc=%.0f   t_oe=%.0f\n", iode, S(t_oc), S(t_oe));
	debug("  a_f2=%g  a_f1=%g  a_f0=%g\n", a_f2, a_f1, a_f0);

	debug("  omega=%g Omega_0=%g OmegaDot=%g  IDOT=%g\n",
		     omega,     Omega_0,    OmegaDot,      IDOT);

	debug("  C_rs=%g  C_rc=%g  C_is=%g  C_ic=%g\n", C_rs, C_rc, C_is, C_ic);
	debug("  C_us=%g  C_uc=%g\n", C_us, C_uc);

	debug("  sqrt_A=%g  e=%g  M_0=%g  delta_n=%g\n", sqrt_A, e, M_0, delta_n);
	debug("  i_0=%g\n", i_0);
}



int EphemerisXmit1::AccToSvacc(double acc)
{
	int i;
    for (i=0; i<15; i++)
		if (AccuracyIndex[i] > acc)
			break;
	return i;
}

double EphemerisXmit1::SvaccToAcc(int svacc)
{
	return AccuracyIndex[svacc];
}

