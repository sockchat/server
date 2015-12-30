// PLATFORM INDEPENDENT TCP SOCKET INTERFACE
// For interface, see socket.h

#include "socklib/socket.h"
#ifdef _WIN32 // winsock implementation

sc::Socket::Socket() {
    this->blocking = true;
    this->type = ESOCKTYPE::UNINIT;
}

bool sc::Socket::Init(short port) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    struct addrinfo *result;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result) != 0)
        return false;

    this->sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(this->sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    if(bind(this->sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(this->sock);
        return false;
    }

    freeaddrinfo(result);
    if(listen(this->sock, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(this->sock);
        return false;
    }

    this->type = ESOCKTYPE::SERVER;
    return true;
}

bool sc::Socket::Init(std::string addr, uint16_t port) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    struct addrinfo hints, *results, *ptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if(getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &results) != 0) return false;

    for(ptr = results; ptr != NULL; ptr = ptr->ai_next) {
        if((this->sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == INVALID_SOCKET) {
            freeaddrinfo(results);
            return false;
        }
        if(connect(this->sock, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR) break;
        closesocket(this->sock);
        this->sock = INVALID_SOCKET;
    }

    freeaddrinfo(results);
    if(this->sock == INVALID_SOCKET) return false;

    this->type = ESOCKTYPE::CLIENT;
    return true;
}

bool sc::Socket::Init(HSOCKET sock, HADDR addr, int addrlen) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    this->sock = sock;
    this->addr = addr;
    this->addrlen = addrlen;

    this->type = ESOCKTYPE::SERVERSPAWN;
    return true;
}

sc::ip sc::Socket::GetIPAddress() {
    if(this->type != ESOCKTYPE::SERVERSPAWN) return sc::ip("0.0.0.0");
    char buffer[128];
    inet_ntop(this->addr.sin_family, (PVOID)&this->addr.sin_addr, buffer, 128);
    return sc::ip(buffer);
}

void sc::Socket::SetBlocking(bool block) {
    if(this->type == ESOCKTYPE::UNINIT) return;
    u_long blocking = block ? 0 : 1;
    ioctlsocket(this->sock, FIONBIO, &blocking);
    this->blocking = block;
}

bool sc::Socket::GetBlocking() {
    return this->blocking;
}

void sc::Socket::SetTimeout(int seconds, int msecs) {
    seconds = (seconds * 1000) + msecs;
    setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&seconds, sizeof(int));
    setsockopt(this->sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&seconds, sizeof(int));
}

int sc::Socket::Accept(Socket &conn) {
    if(this->type != ESOCKTYPE::SERVER) return -1;

    HSOCKET newsock; SOCKADDR_IN newaddr = {0}; int newlen = sizeof(newaddr);
    newsock = accept(this->sock, (struct sockaddr *)&newaddr, &newlen);

	//Couldn't this if statemnt run if the socket connected properly but the previous attempt to 
	//use accept had resulted in a WSAEWOULDBLOCK error?
    if(WSAGetLastError() == WSAEWOULDBLOCK)
        return 1;
    else if(newsock == INVALID_SOCKET) {
        Socket::Close();
        return -1;
    }

    conn = Socket();
    conn.Init(newsock, newaddr, newlen);
    return 0;
}

int sc::Socket::Recv(std::string &str, uint32_t length) {
    if(this->type == ESOCKTYPE::UNINIT || this->type == ESOCKTYPE::SERVER) return -1;

    length = length > SOCK_BUFLEN ? SOCK_BUFLEN : length;
    int get = recv(this->sock, this->recvbuf, length, 0);
    if(WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAETIMEDOUT)
        return 1;
    else if(get <= 0) {
        Socket::Close();
        return -1;
    }

    str = std::string(this->recvbuf, get);
    return 0;
}

int sc::Socket::Send(std::string str) {
    if(this->type == ESOCKTYPE::UNINIT || this->type == ESOCKTYPE::SERVER) return -1;

    int sent = send(this->sock, str.c_str(), str.length(), 0);
    if(sent == SOCKET_ERROR) {
        Socket::Close();
        return -1;
    }

    return 0;
}

int sc::Socket::GetLastError() {
    return WSAGetLastError();
}

void sc::Socket::Close() {
    shutdown(this->sock, SD_BOTH);
    closesocket(this->sock);
    this->type = ESOCKTYPE::UNINIT;
}

sc::Socket::~Socket() {
    //delete[] this->recvbuf;
}

#else // posix (berkeley) socket implementation

sc::Socket::Socket() {
    this->blocking = true;
    this->type = ESOCKTYPE::UNINIT;
}

bool sc::Socket::Init(short port) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    struct addrinfo *result;
    struct addrinfo hints;
    bzero((char*)&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &result) != 0)
        return false;

    this->sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(this->sock < 0) {
        freeaddrinfo(result);
        return false;
    }

    if(bind(this->sock, result->ai_addr, (int)result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        close(this->sock);
        return false;
    }

    freeaddrinfo(result);
    if(listen(this->sock, SOMAXCONN) < 0) {
        close(this->sock);
        return false;
    }

    this->type = ESOCKTYPE::SERVER;
    return true;
}

bool sc::Socket::Init(std::string addr, uint16_t port) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    struct addrinfo hints, *results, *ptr;
    bzero((char*)&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if(getaddrinfo(addr.c_str(), std::to_string(port).c_str(), &hints, &results) != 0) return false;

    for(ptr = results; ptr != NULL; ptr = ptr->ai_next) {
        if((this->sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
            freeaddrinfo(results);
            return false;
        }
        if(connect(this->sock, ptr->ai_addr, (int)ptr->ai_addrlen) == 0) break;
        close(this->sock);
        this->sock = -1;
    }

    freeaddrinfo(results);
    if(this->sock < 0) return false;

    this->type = ESOCKTYPE::CLIENT;
    return true;
}

bool sc::Socket::Init(HSOCKET sock, HADDR addr, int addrlen) {
    if(this->type != ESOCKTYPE::UNINIT) return false;

    this->sock = sock;
    this->addr = addr;
    this->addrlen = addrlen;

    this->type = ESOCKTYPE::SERVERSPAWN;
    return true;
}

sc::ip sc::Socket::GetIPAddress() {
    if(this->type != ESOCKTYPE::SERVERSPAWN) return "0.0.0.0";
    char buffer[128];
    inet_ntop(this->addr.sin_family, (void*)&this->addr.sin_addr, buffer, 128);
    return sc::ip(buffer);
}

void sc::Socket::SetBlocking(bool block) {
    if(this->type == ESOCKTYPE::UNINIT) return;
    int flags = fcntl(this->sock, F_GETFL, 0);
    flags = block ? flags & ~O_NONBLOCK : flags | O_NONBLOCK;
    fcntl(this->sock, F_SETFL, flags);
    this->blocking = block;
}

bool sc::Socket::GetBlocking() {
    if(this->type == ESOCKTYPE::UNINIT) return false;
    int flags = fcntl(this->sock, F_GETFL, 0);
    return (flags & O_NONBLOCK) == 0;
}

void sc::Socket::SetTimeout(int seconds, int msecs) {
    if(this->type == ESOCKTYPE::UNINIT) return;
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = msecs * 1000;

    setsockopt(this->sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(this->sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
}

int sc::Socket::Accept(Socket &conn) {
    if(this->type != ESOCKTYPE::SERVER) return -1;

    HSOCKET newsock; struct sockaddr_in newaddr = {0}; unsigned int newlen = sizeof(newaddr);
    newsock = accept(this->sock, (struct sockaddr *)&newaddr, &newlen);

    if(newsock < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        return 1;
    } else if(newsock < 0) {
        Socket::Close();
        return -1;
    }

    conn = Socket();
    conn.Init(newsock, newaddr, newlen);
    return 0;
}

int sc::Socket::Recv(std::string &str, uint32_t length) {
    if(this->type == ESOCKTYPE::UNINIT || this->type == ESOCKTYPE::SERVER) return -1;

    length = length > SOCK_BUFLEN ? SOCK_BUFLEN : length;
    int get = read(this->sock, this->recvbuf, length);
    if(get < 0 && (errno == EWOULDBLOCK || errno == EAGAIN))
        return 1;
    else if(get <= 0) {
        Socket::Close();
        return -1;
    }

    str = std::string(this->recvbuf, get);
    return 0;
}

int sc::Socket::Send(std::string str) {
    if(this->type == ESOCKTYPE::UNINIT || this->type == ESOCKTYPE::SERVER) return -1;

    int sent = write(this->sock, str.c_str(), str.length());
    if(sent < 0) {
        Socket::Close();
        return -1;
    }

    return 0;
}

int sc::Socket::GetLastError() {
    return errno;
}

void sc::Socket::Close() {
    shutdown(this->sock, SHUT_RDWR);
    close(this->sock);
    this->type = ESOCKTYPE::UNINIT;
}

sc::Socket::~Socket() {
    //delete[] this->recvbuf;
}

#endif

/*
    PARLA IMPONI

    L'ARDITE MIE SCHIERE
    SORGAN TUTTE ALLE TROMBE GUERRIERE
    E WODAN CHE A GLORIA V'APPELLA
    MOVIAM TOSTO

    SIA GLORIA A WODAN SIA GLORIA A WODAN
    SIA GLORIA A WODAN SIA GLORIA A WODAN
    ALLO SQUILLO CHE AL SANGUE NE INVITA
    PRONTI OGNORA I TUOI FIDI SARAN
    SIA GLORIA A WODAN SIA GLORIA A WODAN
    GLORIA GLORIA A WODAN
*/