#ifndef RAWRTCM_INCLUDED
#define RAWRTCM_INCLUDED
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


#include "RtcmIn.h"
#include "RawReceiver.h"

class RawRtcm: public RawReceiver
{
protected:
	RtcmIn In;

	// To keep track of slips and overflows between epochs
	int32 CarrierLossCount[MaxSats];
	int64 PreviousPhase[MaxSats];
	int SequenceNr;

	// For keeping time
	int Week;
	int HourOfWeek;
	double PreviousReceiverTime;

	// For processing an epoch
	bool MoreToCome;
	double EpochTow;
	bool HasPhase[MaxSats];

public:
	RawRtcm(Stream& in);
	virtual bool NextEpoch();
	virtual ~RawRtcm(void);

private:
	bool ProcessTimeTag(Frame& f);
	bool ProcessAntennaRef(Frame& f);
	bool ProcessAntennaTypeDef(Frame& f);
	bool ProcessPseudorange(Frame& f);
	bool ProcessCarrierPhase(Frame& f);
	bool ProcessEphemeris(Frame& f);
	void Header(Frame& f, Time& t, int& type);
	bool GetMeasurementTime(Frame& f);
};

#endif // RAWRTCM_INCLUDED

