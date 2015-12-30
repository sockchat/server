#include "socklib/socket.h"

sc::RawSocket::RawSocket(Socket sock) : sc::Socket(sock) {}

int sc::RawSocket::Recv(std::string &str, uint32_t size) {
    std::string in; int status; sc::Message tmp;
    while(!(tmp = sc::Message(this->buffer)).IsLegal()) {
        if((status = Socket::Recv(in)) != 0) return status;
        this->buffer += in;
    }

    this->buffer = this->buffer.substr(tmp.Get().length());
    str = tmp.Get();
    return 0;
}