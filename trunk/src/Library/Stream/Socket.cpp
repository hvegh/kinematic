#include "Socket.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>




Socket::Socket()
{
    ErrCode = Init();
}


Socket::Socket(Address& addr)
{
    ErrCode = Init() || Connect(addr);
}



bool Socket::Init()
/////////////////////////////////////////////////////////////////
// Init is the real constructor for a socket. 
//   common code for the various constructors.
///////////////////////////////////////////////////////////////////
{
    fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1) return SysError("Can't create a new tcp socket\n");

    // Default to a 10 second timeout
    return SetTimeout(10000);
}

bool Socket::Connect(Address& addr)
{
    // Connect to the address
    if (::connect(fd, &addr.InAddr, sizeof(addr.InAddr)) == -1)
        return SysError("Cannot connect to IP addr %d.%d.%d.%d port %d\n",
              addr.InAddr.sa_data[2], addr.InAddr.sa_data[3],
              addr.InAddr.sa_data[4], addr.InAddr.sa_data[5],
              (addr.InAddr.sa_data[0]<<8) | addr.InAddr.sa_data[1]);

    // Set socket options.
    int temp = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &temp, sizeof(temp)) == -1)
        return SysError("Can't set keepalive socket option\n");
    static const struct linger l = {1, 5};
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) == -1)
        return SysError("Can't set socket linger option\n");

    return OK;
}

bool Socket::Read(byte* buf, size_t size, size_t& actual)
{
    debug(3, "Socket::Read size=%d\n", size); 

    actual = ::read(fd, buf, size);
    if (actual == -1) return SysError("Reading from socket\n");

    debug_buf(3, buf, actual);
    return OK;
}


bool Socket::Write(const byte* buf, size_t size)
{
    debug(3, "Socket::Write: size=%d\n", size); 
    debug_buf(3, buf, size);

    // We don't want to receive a signal, so use "send" instead of "write"
    if (::send(fd, buf, size, MSG_NOSIGNAL) != size) 
        return SysError("Can't write to socket\n");
    return OK;
}



bool Socket::Close()
{
    if (::close(fd) == -1) 
        ErrCode = SysError("Can't close socket. fd=%d\n", fd);
    fd = -1;
    return OK;
}

Socket::~Socket()
{
    Close();
}


bool Socket::SetTimeout(uint32 msec)
{
    struct timeval to = {msec/1000, (msec%1000)*1000};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) == -1 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to)) == -1)
        return SysError("Can't set socket timeout\n");
    // TODO: should we set linger?
    return OK;
}


Socket::Address::Address(uint32 ipaddr, uint32 port)
{
    ErrCode = Init(ipaddr, port);
}




bool Socket::Address::Init(uint32 ipaddr, uint32 port)
{
    // Cast it to a ipv4 socket address
    struct sockaddr_in &ip = *(sockaddr_in*)&InAddr;

    // Actually must be big endian. 
    InAddr.sa_family = AF_INET;
    InAddr.sa_data[0] = port>>8;
    InAddr.sa_data[1] = port;
    InAddr.sa_data[2] = ipaddr>>24;
    InAddr.sa_data[3] = ipaddr>>16;
    InAddr.sa_data[4] = ipaddr>>8;
    InAddr.sa_data[5] = ipaddr;
    for (int i=6; i<sizeof(InAddr.sa_data); i++)
        InAddr.sa_data[i] = 0;

    return OK;
}
