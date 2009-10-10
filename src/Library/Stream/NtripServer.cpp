#include "NtripServer.h"
#include "Parse.h"


NtripServer::NtripServer(Socket::Address addr, char *user, char *passwd, char *mount)
: Socket(addr)
{
    Printf("SOURCE %s/%s\r\n", passwd, mount);
    Printf("Source-Agent NTRIP 1.0 Precision-gps.org\r\n");
    Printf("\r\n");

    ErrCode = ParseHeader();
}



bool NtripServer::ParseHeader()
{
    char line[256];
    bool errcode = ~OK;
    for (;;) {
        ReadLine(line, sizeof(line));

        // parse the first token in the line
        Parse p(line);
        p.Next(" "); 

        // Look for "ICY 200". Good news. 
        if (p == "ICY") {
           p.Next(" ");
           if (p == "200")
              errcode = OK;
        }

        // Look for "ERROR - ...."  Bad news
        else if (p == "ERROR") {
            char msg[256];
            p.Next("");
            p.GetToken(msg, sizeof(msg));
            errcode = Error(msg);
        }

        // Blank line is end of header
        else if (p == "")
            break;
    }

    return errcode;
}



NtripServer::~NtripServer()
{
}
