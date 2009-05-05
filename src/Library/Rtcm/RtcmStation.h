#ifndef RTCMSTATION_INCLUDED
#define RTCMSTATION_INCLUDED
// Part of Kinematic, a utility for GPS positioning
//
// Copyright (C) 2006  John Morris    www.precision-gps.org
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


#include "util.h"
#include "Frame.h"
#include "RtcmOut.h"
#include "RawReceiver.h"


class RtcmStation
{
protected:
	RawReceiver& Gps;
	RtcmOut Out;
	int StationId;
	int Health;
	int SequenceNr;
	bool ErrCode;
	bool FirstEpoch;

	// The station has a fixed position with no wandering.
    //   If the value isn't know precisely, at least choose
	//   an approximate and consistent value. 
	Position StationPos;

	// When to output the next rtcm frames
	Time TimeTagTime;
	Time AntennaRefTime;
	Time EphemerisTime[MaxSats];

	// Some information to keep track of whether we are locked to each satellite
    uint32 CumulativeLossOfLock[MaxSats];
    bool PreviouslyValid[MaxSats];
	double PhaseAdjust[MaxSats];

public:	
	RtcmStation(Stream& out, RawReceiver& gps, int id=0, int health=0);
    bool OutputEpoch();
	bool GetError() {return ErrCode;}
	virtual ~RtcmStation(void);

private:
	bool OutputTimeTag(Time& NextTime);
    bool OutputAntennaRef(Time& NextTime);
	bool OutputAntennaTypeDef(Time& NextTime);
    bool OutputEphemeris(int sat, Time& NextTime);
    bool OutputCarrierPhase(Time& NextTime);
    bool OutputPseudorange(Time& NextTime);

	void Header(Frame& f, Time time, int type);
	void SetField(int value, int word, int bit, int size);
	bool WriteFrame();
};

#endif // RTCMSTATION_INCLUDED

