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

#include "NtripServer.h"
#include "Rtcm3Station.h"
#include "Rs232.h"
#include "RawAC12.h"
#include <stdio.h>

bool Configure(int argc, const char** argv);
void DisplayHelp();
bool GpsSession(const char *PortName, const char *CasterName, int Port, const char* Mount);
void Display(RawReceiver& gps);

// Globals which are set up by "configure"
const char *CasterName;
int Port;
const char *Mount;
const char *SerialName;
Rtcm3Station::Attributes Station;
extern int DebugLevel;


int main(int argc, const char** argv)
{
    // Get configured according to arguments
    if (Configure(argc, argv) != OK) {
        DisplayHelp();
        return ShowErrors();
    }

    // Repeat forever
    for (;;) {

        // Start a GPS session
        printf("Starting Session\n");
        GpsSession(SerialName, CasterName, Port, Mount);

        ShowErrors();
        ClearError();

        // Sleep a bit before restarting the session
        printf("Session Failed -- Restart in 15 seconds\n");
        Sleep(15000);
    }
    
    return 0;
}




bool GpsSession(const char *SerialName, const char *Castername, int Port, const char *Mount)
{
    // Initialize the gps receiver
    Rs232   in(SerialName);
    RawAC12 gps(in);
    if (gps.GetError() != OK)
        return Error("Unable to init the AC12  on port %s\n", SerialName);

    // Create the RTCM output file
    //OutputFile out(RtcmName);
    Socket::Address  addr( (127<<24)|1, Port);
    NtripServer out(addr, "mnt", "", "password");
    Rtcm3Station rtcm(out, gps, Station);
    if (rtcm.GetError() != OK) 
        return Error();

    // Repeat forever
    for (;;) {
        // Read next epoch of data
        if (gps.NextEpoch() != OK) return Error("Can't get gps data\n");

        Display(gps);

        // Write it out as RTCM
        if (rtcm.OutputEpoch() != OK) return Error("Can't send RTCM to caster\n");
    }

    // Done
    return OK;
}



void Display(RawReceiver& gps)
{
    // Display the satellites being tracked
    int32 day, month, year, hour, min, sec, nsec;
    TimeToDate(gps.GpsTime, year, month, day); 
    TimeToTod(gps.GpsTime, hour, min, sec, nsec);
    printf("%2d/%02d/%04d %02d:%02d:%02d  ", month,day,year,hour,min,sec);
    for (int s=0; s<MaxSats; s++) {
        if (gps.obs[s].Valid)
            if (gps[s].Valid(gps.GpsTime)) printf("*%d ",SatToSvid(s));
            else                            printf("%d ", SatToSvid(s));
    }
    
    printf("\07\n");  // Ring the bell so we know things are alive
}


bool Configure(int argc, const char** argv)
{
	// Set the defaults
	Station.ARP = Position(0,0,0);
	SerialName = NULL;
        Port = 0;
        Mount = NULL;
        CasterName = NULL;

	// Process each option
	int i;
	const char* val;
	for (i=1; i<argc; i++) {
		if      (Match(argv[i], "caster=", CasterName))      ;
                else if (Match(argv[i], "port=", val)) Port=atoi(val);
                else if (Match(argv[i], "mount=", Mount)) ;
                else if (Match(argv[i], "serial=", SerialName))      ;
		else if (Match(argv[i], "x=", val))  Station.ARP.x = atof(val);
		else if (Match(argv[i], "y=", val))  Station.ARP.y = atof(val);
		else if (Match(argv[i], "z=", val))  Station.ARP.z = atof(val);
		else if (Match(argv[i], "debug=", val)) DebugLevel = atoi(val);
		else    return Error("Didn't recognize option %s\n", argv[i]);
	}
	

        if (SerialName == NULL  || Mount == NULL || Port == 0)
            return Error("Must specify SerialName, PortName and mount\n");

	return OK;
}


void DisplayHelp()
{
	printf("\n");
	printf("NtripAc12 caster=CasterName port=PortNr mount=MountPoint serial=SerialPort x=xxx y=yyyy z=zzz\n");
	printf("   Acquires rtcm data from an AC12 GPS receiver.\n");
	printf("\n");
	printf("   SerialPort  - the name of the Rs-232 port to talk to the receiver\n");
	printf("               eg. /dev/ttyUSB0\n");
        printf("   x, y, z  are ECEF station coordinates\n");
	printf("\n");


}

