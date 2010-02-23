#include "NtripServer.h"
#include "Parse.h"


NtripServer::NtripServer(const char* host, const char* port, const char *mount, 
                         const char *user, const char *passwd)
: Socket(host, port)
{
    debug("NtripServer: user=%s passwd=%s  mount=%s\n", user, passwd, mount);
    ErrCode = ErrCode 
           || Printf("SOURCE %s/%s\r\n", passwd, mount) 
           || Printf("Source-Agent NTRIP 1.0 Precision-gps.org\r\n")
           || Printf("\r\n") 
           || ParseHeader();
}


bool NtripServer::ParseHeader()
{
    char msg[256];
    char line[256];
    forever {

        if (ReadLine(line, sizeof(line)) != OK)
            return Error("Can't read Ntrip header from Caster\n");

        // parse the first token in the line
        Parse p(line);
        p.Next(" "); 

        // Look for "ICY 200". Good news. 
        if (p == "ICY") {
           if (p.Next(" "), p != "200") 
               return Error("ParseHeader: Bad code - %s\n", line);
        }

        // Look for "ERROR - ...."  Bad news
        else if (p == "ERROR") 
            return Error("Ntrip Caster says: %s\n", line);

        else if (p == "")
            break;
    }

    return OK;
}



NtripServer::~NtripServer()
{
}
