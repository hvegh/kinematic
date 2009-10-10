#ifndef Socket_Included
#define Socket_Included

#include "Util.h"
#include "Stream.h"
#include <sys/socket.h>



class Socket: public Stream {
public:
    class Address {
    public:
        Address(uint32 ipaddr, uint32 port);
        Address(char* hostname, uint32 port);
       
        bool GetError() {return ErrCode;}
        sockaddr InAddr;
    protected:
        bool ErrCode;
        bool Init(uint32 ipaddr, uint32 port);
    };

    Socket();
    Socket(Address& addr);
    virtual ~Socket();
    
    bool SetTimeout(uint32 msec);
    virtual bool Connect(Address& addr);
    virtual bool Close();

    bool Read(byte*, size_t size, size_t& actual);
    bool Write(const byte*, size_t size);
    bool ReadOnly() {return false;}

protected:
    int fd;
    bool Init();
 
};




#endif 
