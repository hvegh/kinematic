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

#ifndef RTCMIN_INCLUDED
#define RTCMIN_INCLUDED
#include "Stream.h"
#include "Frame.h"

class RtcmIn  // Later, make this CommRtcm
{
protected:
	Stream& In;
	uint64 RawWord;
	int BitShift;
	bool ErrCode;

public:
	RtcmIn(Stream& in);
	bool ReadFrame(Frame& f, bool& slip);
	bool GetError() {return ErrCode;}
	virtual ~RtcmIn(void);
private:
	bool ReadWord(uint32& word, bool& slip);
	bool Synchronize(uint32& word);
};

#endif // RTCMIN_INCLUDED

