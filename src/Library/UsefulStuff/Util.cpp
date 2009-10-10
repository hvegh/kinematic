// util contains an assortment of utility routines
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
#include "util.h"
#include <math.h>

#ifdef Windows
#include <windows.h>
#include <winioctl.h>

#else

#include <strings.h>
#include <errno.h>
#include <unistd.h>

const char * GetLastError()
{
    return strerror(errno);
}

void Sleep(int msec)
{
    usleep(msec*1000);
}


#endif


int SvidToSat(int svid)
{
	if (svid >= 1 && svid <=32)
		return svid;
	else if (svid >= 120 && svid < 152)
		return svid - 87;
	else
		return -1;
}

int SatToSvid(int s)
{
	if (s >= 1 && s <= 32)
		return s;
	else if (s >= 33 && s < 65)
		return s+87;
	else
		return -1;
}


// Signal to noise levels as used in Rinex files

double LevelToSnr(int level)
{
	double snr;
	if (level <= 0)     snr = 0;
	else                snr = level*6 + 3;
	return snr;
}

int SnrToLevel(double snr)
{
	int level;
	if (snr == 0)       level = 0;
	else if (snr < 12)  level = 1;
	else if (snr >= 54) level = 9;
	else                level = (int)(snr/6);
	return level;
}










// Some very quick+dirty random numbers for testing. Replace!
static int dummy = (srand(0), 0);  // start with the same seed every time.
double Uniform()
{
	double value = rand() / (double)RAND_MAX;
	debug(5, "Uniform = %.3f\n", value);
	return value;
}

double Normal()
{
	static int count = 16;
	double sum = 0;
	for (int i=0; i<count; i++)
		sum += Uniform();
	double value = (sum - count/2.0);
	debug(4, "Normal %.3f\n", value);
	return value;
}

double Normal(double mean, double dev)
{
	return Normal()*dev + mean;
}


#include "stdio.h"

extern int DebugLevel;

#ifdef DEBUG
void vdebug(int level, const char* fmt, va_list args);
void debug_write(const char* buffer, size_t len);

void debug_buf(int level, byte* buf, size_t size)
{   
	if (level > DebugLevel) return;
	if (size > 256) size=256;
	size_t i;
	for (i=0; i<size; i++) {
		debug(" %02x", buf[i]);
		if (i%16 == 15) debug("\n");
	}
	if (i%16 != 0) debug("\n");
}



void debug(const char* fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
    vdebug(1, fmt, arglist);
	va_end(arglist);
}

void debug(int level, const char* fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
    vdebug(level, fmt, arglist);
	va_end(arglist);
}

void vdebug(int level, const char* fmt, va_list args)
{
	if (level > DebugLevel) return;

	char buffer[256]; buffer[255] = '\0';
	vsnprintf(buffer, 255, fmt, args);
	debug_write(buffer, strlen(buffer));
}


// Has to be low level to avoid accidental recursion.
//   but we want buffering for performance.
static FILE *DebugFile = NULL;

void debug_write(const char* buf, size_t len)
{
	if (DebugFile == NULL) {
		DebugFile = fopen("debug.txt", "wc");
		if (DebugFile == NULL) DebugFile = stderr;
	}

	fwrite(buf, len, 1, DebugFile);
    return;
}


#endif




/////////////////////////////////////////////////////////////
// Error handling
//
// Error() adds the message to an internal buffer.
// ClearError() clears the error messages
// ShowError()  displays the error messages
//
////////////////////////////////////////////////////////////////


int ErrCount = 0;
static const int ErrMax = 15;
static const int ErrMaxStr = 256;
char   ErrSlot[ErrMax][ErrMaxStr];  // Make this thread specific later


bool SysError(const char* fmt, ...)
{
	Error("System error %d\n", GetLastError());

	va_list arglist;
	va_start(arglist, fmt);
	Verror(fmt, arglist);
	va_end(arglist);
    return true;
}




bool Error(const char *fmt, ...)
{
	va_list arglist;
	va_start(arglist, fmt);
	Verror(fmt, arglist);
	va_end(arglist);
	return true;
}

bool Verror(const char *fmt, va_list arglist)
{
	debug("ERROR: "); vdebug(1, fmt, arglist);

	// Format into the slot
	ErrSlot[ErrCount][ErrMaxStr-1] = '\0';
	vsnprintf(ErrSlot[ErrCount], ErrMaxStr-1, fmt, arglist);

	// if we have more room in the error list, then allocate a slot
	if (ErrCount < ErrMax)
	    ErrCount++;

	return true;
}


void ClearError()
{
	debug("CLEAR errors\n");
	ErrCount = 0;
}


int ShowErrors()
{
	for (int i=0; i<ErrCount; i++)
		printf("%5d: %s", i, ErrSlot[i]);

/***********************************
	if (ErrCount > 0) {
		printf("\nHit ""enter"" to continue.\n");
		while (getchar() != '\n')
			;
	}
*******************************/

        if (ErrCount > 0) return 1;
        else              return 0;

}


