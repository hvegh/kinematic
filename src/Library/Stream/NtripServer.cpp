#include "NtripServer.h"
#include "Parse.h"


NtripServer::NtripServer(Socket::Address addr, char *mount, char *user, char *passwd)
: Socket(addr)
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
           p.Next(" ");
           if (p != "200") {
               p.GetToken(msg,sizeof(msg));
               return Error("ParseHeader: Bad code - ICY %s\n", msg);
           }
        }

        // Look for "ERROR - ...."  Bad news
        else if (p == "ERROR") {
            p.Next("");
            p.GetToken(msg, sizeof(msg));
            return Error("Ntrip Caster says: ERROR %s\n", msg);
        }

        else if (p == "")
            break;
    }

    return OK;
}



NtripServer::~NtripServer()
{
}
