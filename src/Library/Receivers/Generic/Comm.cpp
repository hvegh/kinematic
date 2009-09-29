
// Comm - an abstract class for storing and communicating binary records
//   Part of the Kinetic GPS utilities
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
//////////////////////////////////////////////////////////////////////////////

#include "Comm.h"
#include <stdarg.h>
#include <ctype.h>


Comm::Comm(Stream &s)
: com(s)
{
	ErrCode = com.GetError();
}

// This is a TEMPORARY hack to support Garmin usb
DummyStream dummystream;
Comm::Comm()
: com(dummystream)
{
	ErrCode = Error();
}

bool Comm::PutBlock(int Id, ...)
{
	Block block(Id);
	LittleEndian b(block);

	va_list args;
	va_start(args, Id);

	int Data;
	while( (Data=va_arg(args,int)) >= 0)
		b.Put(Data);

	va_end(args);
	return PutBlock(block);
}

bool Comm::PutString(int id, const char* str)
{
	Block blk(id);
	LittleEndian b(blk);
	while (*str != '\0')
		b.Put(*str++);
	return PutBlock(blk);
}

bool Comm::AwaitBlock(int32 id, Block& b)
{
	debug("Comm::AwaitBlock - Id=%d(0x%x)\n", id, id);
	do {
		if (GetBlock(b) != OK) return Error();
		debug("Waiting for %d.  Got %d\n", id, b.Id);
	} while (b.Id != id);
	
	return OK;
}

bool Comm::AwaitBlock(int32 Id)
{
	Block b;
	return AwaitBlock(Id, b);
}




void Block::Display(int level, const char* s)
{
	if (level > DebugLevel) return;
	debug(level, "%s - Id=%d(0x%x)  Length=%d\n", s, Id, Id, Length);
      for (int j=0; j < Length; j += 16) {
          for (int i=j; i<j+16; i++)
              if (i < Length)   debug(level, "%02x ", Data[i]);
              else              debug(level, "   ");
          debug(level, "    ");
          for (int i=j; i<j+16; i++)
              if (i < Length)   debug(level, "%c", isprint(Data[i])?Data[i]:'.');
              else              debug(level, " ");
          debug(level, "\n");
      }
}



void NmeaBlock::GetField(char *buf, int size)
{
    int i;
    for (i=0; i<size-1; i++) {
        buf[i] = (char)Get();
        if (buf[i] == ',' || buf[i] == '\0') break;
    }

    buf[i] = '\0';
}

void NmeaBlock::PutField(char *buf)
{
}

int NmeaBlock::GetInt()
{
    char buf[sizeof(NmeaBlock)];
    GetField(buf, sizeof(buf));
    return atoi(buf);
}

double NmeaBlock::GetFloat()
{
    char buf[sizeof(NmeaBlock)];
    GetField(buf, sizeof(buf));
    return atof(buf);
}

void NmeaBlock::PutInt(int value)
{
    Error("NmeaBlock::PutInt not implemented\n");
}

void NmeaBlock::PutFloat(double value)
{
}


Comm::~Comm()
{
}

