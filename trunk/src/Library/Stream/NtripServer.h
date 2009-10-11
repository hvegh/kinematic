#ifndef NtripServer_included
#define NtripServer_included

#include "Util.h"
#include "Socket.h"

class NtripServer: public Socket
{
public:

    NtripServer(Socket::Address addr, char *mount, char *user, char *password);
    ~NtripServer();

protected:
    bool ParseHeader();
};


#endif
