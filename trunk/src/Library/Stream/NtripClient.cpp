#include "NtripClient.h"
#include "Parse.h"


NtripClient::NtripClient(Socket::Address addr, char *mount, char *user, char *passwd)
: Socket(addr)
{
    debug("NtripClient: user=%s passwd=%s  mount=%s\n", user, passwd, mount);
    if (ErrCode != OK) return;
    ErrCode = Error();

    if (Printf("GET /%s HTTP/1.0\r\n", mount) != OK) return;
    if (Printf("User-Agent NTRIP 1.0 Precision-gps.org\r\n") != OK) return;

    if (!IsEmpty(user) || !IsEmpty(passwd)) {
        char buf[256];
        Encode(buf, user, passwd);
        if (printf("Authorization: Basic %s\r\n", buf) != OK) return;
    }
    if (Printf("\r\n") != OK) return;

    if (ParseHeader() != OK) return;

    ErrCode = OK;
}


bool NtripClient::ParseHeader()
{
    char msg[256];
    char line[256];
    forever {

        if (ReadLine(line, sizeof(line)) != OK)
            return Error("Can't read Ntrip header from Caster\n");

        // parse the first token in the line
        Parse p(line);
        p.Next(" /"); 

        // Look for "ICY 200 OK". Good news. 
        if (p == "ICY") {
           if (p.Next(" ") != "200" || p.Next(" ") != "OK")
               return Error("ParseHeader: Bad code - %s\n", line);
        }

        // Mount point not available
        else if (p == "SOURCETABLE") {

            // Skip the rest of the header
            if (p.Next(" ") != "200" || p.Next(" ") != "OK")
                return Error("Caster sent funny header: %s\n", line);

            return Error("Mountpoint is not available\n");
        }

        // User not authorized
        else if (p == "HTTP") {
            if (p.Next(" ") != "1.0" || p != "1.1")
                return Error("HTTP version not recognized %s\n, line");
            if (p.Next(" ") != "401" || p.Next(" ") != "Unauthorized")
                return Error("HTTP error not recognized - %s\n", line);
            return Error("User not authorized to access mountpoint\n");
        }

        // End of header.  Successful!
        else if (p == "")
            break;
    }

    return OK;
}



NtripClient::~NtripClient()
{
}
