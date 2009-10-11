
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


#include "CommRtcm3.h"
#include "Crc.h"

static const byte preamble = 0xd3;


CommRtcm3::CommRtcm3(Stream& com)
: Comm(com)
{
}

bool CommRtcm3::PutBlock(Block& blk)
{
    byte LenHi = (blk.Length>>8) & 0x3;
    byte LenLo = blk.Length;

    // Send the header and body
    if (com.Write(preamble) != OK
     || com.Write(LenHi) != OK
     || com.Write(LenLo) != OK
     || com.Write(blk.Data, blk.Length) != OK) return Error();

    // Send the crc
    Crc24 crc;
    crc.Add(preamble); crc.Add(LenHi); crc.Add(LenLo);
    crc.Add(blk.Data, blk.Length);
    if (com.Write(crc.AsBytes(), 3) != OK) return Error();

    return OK;
}


bool CommRtcm3::GetBlock(Block& blk)
{
}


CommRtcm3::~CommRtcm3()
{
}

