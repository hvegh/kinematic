
//    Part of Kinematic, a utility for GPS positioning
//
// Copyright (C) 2005  John Morris    www.precision-gps.org
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


#include "Rtcm3Station.h"
#include "Util.h"



Rtcm3Station::Rtcm3Station(Stream& com, RawReceiver& gps, Attributes& attr)
: Gps(gps), comm(com), Station(attr)
{
	ErrCode = comm.GetError();

	// Setup times so we output all records at the beginning
	StationRefTime = -1;
	AntennaRefTime = -1;
        AuxiliaryTime = -1;

        // Keep count of the number of each record type
        StationRefCount = 0;
        AntennaRefCount = 0;
        AuxiliaryCount = 0;
        ObservationsCount = 0;

	// We need to keep track of slips
	for (int s=0; s<MaxSats; s++) {
             TrackingTime[s] = 0;
             PreviouslyValid[s] = false;
             PhaseAdjust[s] = 0;
        }
		
}


bool Rtcm3Station::OutputEpoch()
{
        // Use the gps position if we haven't set it already
        if (Station.ARP == Position(0,0,0))
            Station.ARP = Gps.Pos;

	// if the time is right, output the various records
	if (StationRefTime <= Gps.GpsTime)
	   if (OutputStationRef(StationRefTime) != OK) return Error();
	if (AntennaRefTime <= Gps.GpsTime)
            if (OutputAntennaRef(AntennaRefTime) != OK) return Error();
        if (AuxiliaryTime <= Gps.GpsTime) 
            if (OutputAuxiliary(AuxiliaryTime) != OK) return Error();

	// Send the measurements for this epoch
	if (OutputObservations() != OK) return Error();

	return OK;
}

// MaxDelta is the largest phase-pseudo range which can be stored
static const double MaxDelta = 0x3ffff;

// ExtremeDelta means the phase has gone crazy and "slipped"
static const double ExtremeDelta = MaxDelta + 700*L1WaveLength/.02;

bool Rtcm3Station::OutputObservations()
{

    // Find how many valid satellites
    int nrsats = 0;
    for (int s=0; s<MaxSats; s++)
        if (Gps.obs[s].Valid)
            nrsats++;

    // Create the RTCM Observation header
    Block blk;
    Bits b(blk);
    b.PutBits(1002, 12);         // Message Number 1002
    b.PutBits(Station.Id, 12);   // Station Id
    b.PutBits(GpsTow(Gps.GpsTime), 20); // TOW TODO: Round? 20bits?
    b.PutBits(0, 1);             // Synchronous GNSS flag
    b.PutBits(nrsats, 5);        // No. of GPS Satellites Processed
    b.PutBits(0, 1);             // Smoothing (none)
    b.PutBits(0, 3);             // Smoothing interval (none)

    // Do for each valid satellite observation
    for (int s=0; s<MaxSats; s++) {
        RawObservation& o = Gps.obs[s];
        if (!o.Valid) {PreviouslyValid[s] = false;  continue;}

        // Calculate PR value
        uint32 Modulus = floor(o.PR / (C/1000));
        uint32 iPR = round((o.PR - Modulus*(C/1000)) / .02);
        double pseudorange = Modulus*(C/1000) + iPR*.02;

        // Calculate phaserange value
        int32 oldadjust = PhaseAdjust[s];
        double phaserange = (o.Phase-PhaseAdjust[s]) * L1WaveLength;
        double iDelta = round( (phaserange-pseudorange)/0.0005  );

        // Case: there was a slip. Create a new phase adjustment
        if (o.Slip || !PreviouslyValid[s] || abs(iDelta) > ExtremeDelta) {
            PhaseAdjust[s] = round(o.Phase - o.PR/L1WaveLength);
            TrackingTime[s] = 0;
        } 
    
        // Case: phase has drifted too high to store in field. adjust
        else if (iDelta > MaxDelta) 
            PhaseAdjust[s] += 1500;

        // Case: phase has drifted too low to store in field. adjust
        else if (iDelta < -MaxDelta) 
             PhaseAdjust[s] -= 1500;

        // Recompute phase range if adjustments were made
        if (PhaseAdjust[s] != oldadjust) {
            phaserange = (o.Phase-PhaseAdjust[s]) * L1WaveLength;
            iDelta = round(  (phaserange-pseudorange)/.0005  );
        }

        // If there was no phase measurement, then throw away all of the above
        if (o.Phase == 0) iDelta = 0x40000;

        // Add the satellite's data to the observation record
        b.PutBits(SatToSvid(s), 6);     // GPS Satellite ID
        b.PutBits(0, 1);               // GPS L1 Code Indicater (C/A)
        b.PutBits(iPR, 24);             // GPS L1 Pseudorange
        b.PutBits((uint32)iDelta, 20);   // GPS L1 PhaseRange - L1 Pseudorange
        b.PutBits(Modulus, 8);         // GPS L1 Pseudorange Modulus Ambiguity
        b.PutBits(LockTime(TrackingTime[s]), 7);  // GPS L1 Lock time indicator;
        b.PutBits(SnrToLevel(o.SNR), 8); // GPS L1 CNR
    
        TrackingTime[s]++;
        PreviouslyValid[s] = o.Valid;
    }

    // Output the observations record
    return comm.PutBlock(blk);
    }




bool Rtcm3Station::OutputStationRef(Time& time)
{
    Block  blk;
    Bits   b(blk);

    // Assemble the Station Reference record
    b.PutBits(1005, 12);  
    b.PutBits(Station.Id, 12);
    b.PutBits(0,6);
    b.PutBits(1,1);   // GPS measurements
    b.PutBits(0, 3);
    b.PutBits(Station.ARP.x/.0005, 38);
    b.PutBits(0,2);
    b.PutBits(Station.ARP.y/.0005, 38);
    b.PutBits(0,2);
    b.PutBits(Station.ARP.z/.0005, 38);

    // Reschedule in a minute
    time = Gps.GpsTime + 60*NsecPerSec;

    // Output it
    return  comm.PutBlock(blk);
}





bool Rtcm3Station::OutputAntennaRef(Time& time)
{
    return OK;
}


bool Rtcm3Station::OutputAuxiliary(Time& time)
{
    return OK;
}


uint32 Rtcm3Station::LockTime(uint32 i)
////////////////////////////////////////////////////////////
// Locktime converts Lock Time Indicator according table 3.4-2 in RTCM 3.0
////////////////////////////////////////////////////////////////////////
{
    if (i < 24)   return i;
    if (i < 72)   return i*2 - 24;
    if (i < 168)  return i*4 - 120;
    if (i < 360)  return i*8 - 408;
    if (i < 937)  return i*16 - 1176;
    return 127;  // TODO
}


Rtcm3Station::~Rtcm3Station(void)
{
}

