
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


#include "Frame.h"


inline uint32 ExtractBits(uint32 word, int32 BitNr, int32 NrBits)
///////////////////////////////////////////////////////////////
// ExtractBits extracts a bit field where bit 0 is MSB
//////////////////////////////////////////////////////////////
{
	return (word<<BitNr)>>(32-NrBits);
}

inline int32 ExtractSigned(uint32 word, int bitnr, int nrbits)
/////////////////////////////////////////////////////////////////
// ExtractSigned extracts a signed bit field where bit 0 is MSB
////////////////////////////////////////////////////////////////////
{
	return ((int32)word<<bitnr)>>(32-nrbits);
}

inline uint32 InsertBits(uint32 value, uint32 word, int bitnr, int nrbits)
{
	uint32 mask = (~0u << (32-nrbits)) >> bitnr;
	value =       (value << (32-nrbits)) >> bitnr;
	debug("InsertBits value=0x%x mask=0x%08x word=0x%08x bitnr=%d nrbits=%d\n",
		value, mask, word, bitnr, nrbits);
	return (word & ~mask) | value;	
}



inline uint32 Parity(uint32 w)
/////////////////////////////////////////////////////////////////
// Parity calculates the parity (0 or 1) of a word
/////////////////////////////////////////////////////////////////
{
	w ^= w>>16;
	w ^= w>>8;
	w ^= w>>4;
	w ^= w>>2;
	w ^= w>>1;
	return (w&1);
}


uint32 CalculateParity(uint32 word, uint32 PrevD29, uint32 PrevD30)
/////////////////////////////////////////////////////////////////
// CalculateParity calculates the 6 bit parity for a word
/////////////////////////////////////////////////////////////////
{   
	// Extract the 24 data bits, complementing if necessary
	//  (can adjust the bit masks and avoid the extract - later)
	uint32 data;
	if (PrevD30) data = ExtractBits(~word, 2, 24);
	else         data = ExtractBits( word, 2, 24);

	// compute the desired parity bits
	uint32 D25 = PrevD29 ^ Parity(data & 0xec7cd2);
	uint32 D26 = PrevD30 ^ Parity(data & 0x763e69);
	uint32 D27 = PrevD29 ^ Parity(data & 0xbb1f34);
	uint32 D28 = PrevD30 ^ Parity(data & 0x5d8f9a);
	uint32 D29 = PrevD30 ^ Parity(data & 0xaec7cd);
	uint32 D30 = PrevD29 ^ Parity(data & 0x2dea27);
	uint32 parity = (D25<<5) + (D26<<4) + (D27<<3) + (D28<<2) + (D29<<1) + D30;

	return parity;
}


uint32 AddParity(uint32 data, uint32 PrevD29, uint32 PrevD30)
{
	uint32 parity = CalculateParity(data, PrevD29, PrevD30);
	uint32 word = (PrevD30)? ~data: data;
	word = (word&0x3fffffc0) | parity;
	//debug("AddParity:  data=%08x  PrevD29=%d  PrevD30=%d  word=%08x\n", 
	//	data,PrevD29,PrevD30,word);
	return word;
}

bool CheckParity(uint32 word)
{
	return CheckParity(word, (word>>31)&1, (word>>30)&1);
}

bool CheckParity(uint32 word, uint32 PrevD29, uint32 PrevD30)
{
	uint32 parity = word & 0x3f;
	uint32 data = (PrevD30)? ~word: word;
	data = data & 0x3fffffc0;
	bool ret = (parity != CalculateParity(data, PrevD29, PrevD30));
	//debug("CheckParity: data=%08x  PrevD29=%d  PrevD30=%d  word=%08x  %s\n",
	//		            data,     PrevD29,     PrevD30,   word,   (ret)?"BAD":"");
    return ret;
}

uint32 RemoveParity(uint32 word)
{
	return RemoveParity(word, (word>>31)&1, (word>>30)&1);
}

uint32 RemoveParity(uint32 word, uint32 PrevD29, uint32 PrevD30)
{
	uint32 data;
	if (PrevD30) data = (~word) & 0x3fffffc0;
	else         data =    word & 0x3fffffc0;

	return data;
}





Frame::Frame(int nrwords)
{
	Init(nrwords);
}

void Frame::Init(int nrwords)
{
	NrWords = nrwords;
	for (int i=0; i<NrWords; i++)
		Data[i] = 0;
}

uint32 Frame::GetField(int wordnr, int first, int last)
//////////////////////////////////////////////////////////////////
// GetField extracts an unsigned data field from a frame
///////////////////////////////////////////////////////////////////
{
	// Note: bit 1, word 1 is the most significant bit of the frame.
	return ExtractBits(Data[wordnr-1], first+1, last-first+1);
}


int32 Frame::GetSigned(int wordnr, int first, int last)
{
	return ExtractSigned(Data[wordnr-1], first+1, last-first+1);
}

void Frame::AddWord(uint32 value)
{
	NrWords++;
	PutWord(NrWords, value);
}

void Frame::PutWord(int wordnr, uint32 value)
{
	Data[wordnr-1] = value;
}

void Frame::PutField(int wordnr, int first, int last, uint32 value)
{
	Data[wordnr-1] = InsertBits(value, Data[wordnr-1], first+1, last-first+1);
}

void Frame::Put32(int wordnr, uint32 value)
{
	PutField(wordnr, 1, 24, value>>8);
	PutField(wordnr+1, 1, 8, value);
}

int32 Frame::Get32(int wordnr)
{
	return (int) ( (GetField(wordnr, 1, 24)<<8) | GetField(wordnr+1, 1, 8) );
}


void Frame::Display(const char *str)
{
	debug("Frame NrWords=%d - %s", NrWords, str);
	for (int i=0; i<NrWords; i++) {
		if ( (i%8) == 0 )
			debug("\n    ");
		debug("%08x ", Data[i]);
	}
	debug("\n");
}

