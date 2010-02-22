
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


#include "RawRtcm3.h"
#include "EphemerisXmit.h"

RawRtcm3::RawRtcm3(Stream& in)
: In(in)
{
	strcpy(Description, "Rtcm104File");
	ErrCode = In.GetError();
	for (int s=0; s<MaxSats; s++) {
		PreviousPhase[s] = 0;
                PhaseAdjust[s] = 0;
	}
}


bool RawRtcm3::NextEpoch()
{
	// repeat until we get an observation record or an error
        bool errcode;
        int msgid;
	do {

		// Read a frame
                Block blk;
		if (In.GetBlock(blk) != OK) return Error();

                // Process according to the type of block
                Bits b(blk);
                msgid = b.GetBits(12);
                int StationId = b.GetBits(12);

                if (msgid == 1002)        errcode = ProcessObservations(b);
                else if (msgid == 1005)   errcode = ProcessStationRef(b);
                else                      errcode = OK;

	} until (msgid == 1002 || errcode != OK);

	return errcode;
}

bool RawRtcm3::ProcessObservations(Bits& b)
{

    static const double MaxDelta = 0x7ffff;
    static const double BigDelta = MaxDelta - 800*L1WaveLength/.0005;

    // Assume no observations until shown otherwise
    for (int s=0; s<MaxSats; s++)
        obs[s].Valid = false;

    // Get the header info from the record
    int32 Tow = b.GetBits(20);
    int Synch = b.GetBits(1);
    int NrSats = b.GetBits(5);
    int Smoothing = b.GetBits(1);
    int Interval = b.GetBits(3);
    debug("RawRtcm3::ProcessObservations \n");
    debug(" Tow=%d Synch=%d NrSats=%d Smoothing=%d Interval=%d\n",
            Tow,   Synch,   NrSats,   Smoothing,   Interval);


    // Do for each measurement
    debug("   Svid Code   iPR  iDelta  Modulus  Locktime Snr\n");
    for (int i=0; i<NrSats; i++) {

        // Extract the measurement's fields
        int Svid = b.GetBits(6);
        int Code = b.GetBits(1);
        int32 iPR  = b.GetBits(24);
        int32 iDelta = b.GetSignedBits(20);
        int32 Modulus = b.GetBits(8);
        int32 LockTime = b.GetBits(7);
        int32 Snr = b.GetBits(8);
        debug("   %2d %d %8d %8d %3d %3d %3d\n",
               Svid,Code,iPR,iDelta,Modulus,LockTime,Snr);

        // Figure out which satellite
        int s = SvidToSat(Svid);
        RawObservation& o = obs[s];

        double PseudoRange = Modulus*(C/1000) + iPR*.02;
        double PhaseRange = PseudoRange + iDelta*.0005 
                              + PhaseAdjust[s]*L1WaveLength;

        // How do we determine if phase was adjusted by 1500 cycles?
        //  If we PhaseRange shrank, but it is at higher boundary
        double Doppler = PhaseRange - PreviousPhaseRange[s];
        if (Doppler < 0 && iDelta > BigDelta)       
           {PhaseAdjust[s] += 1500; PhaseRange += 1500*L1WaveLength;}
       else if (Doppler > 0 && iDelta < -BigDelta)  PhaseAdjust[s] += 1500;
           {PhaseAdjust[s] -= 1500; PhaseRange -= 1500*L1WaveLength;}

        o.PR = PseudoRange;
        o.Phase = PhaseRange / L1WaveLength;
        o.SNR = LevelToSnr(Snr);
        o.Slip = (LockTime < PreviousLockTime[s] || PreviousPhaseRange[s] == 0);
        o.Doppler = PhaseRange - PreviousPhaseRange[s];
        o.Valid = true;

        if (iDelta == 0x80000) 
            PhaseRange = o.Doppler = o.Phase = 0;
 
        if (PreviousPhaseRange[s] == 0)
            o.Doppler = 0;

        PreviousPhaseRange[s] = PhaseRange;
        PreviousLockTime[s] = LockTime;
        if (o.Slip)
            PhaseAdjust[s] = 0;

     }
    
    return OK;
}

bool RawRtcm3::ProcessAntennaRef(Bits& b)
{
    return OK;
}


RawRtcm3::~RawRtcm3()
{
}
