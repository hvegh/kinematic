#ifndef NtripServer_included
#define NtripServer_included

#include "Util.h"
#include "Socket.h"

class NtripClient: public Socket
{
public:

    NtripClient(Socket::Address addr, char *mount, char *user, char *password);
    ~NtripClient();

protected:
    bool ParseHeader();
};


#endif
