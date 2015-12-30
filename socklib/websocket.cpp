// WEBSOCKET INTERFACE
// For interface, see socket.h

#include "socklib/socket.h"

sc::WebSocket::WebSocket() : sc::Socket() {
    mahou(NULL);
    this->handshaked = false;
    this->fragment = "";
}

sc::WebSocket::WebSocket(sc::Socket sock) : sc::Socket(sock) {
    this->handshaked = false;
    this->fragment = "";
}

bool sc::WebSocket::Handshake(std::string data) {
    if(this->handshaked) return true;

    int status;
    bool tmp = this->GetBlocking();
    time_t start = time(NULL);
    this->buffer = "";
    this->SetBlocking(false);
    while(!(this->header = HTTPRequest::Response(data, "GET")).IsValid() && difftime(time(NULL), start) <= 30) {
        if(this->header.status == -2) return false;
        if((status = Socket::Recv(this->buffer)) == -1) return false;
        else if(status == 0)
            data += this->buffer;
    }
    this->SetBlocking(tmp);
    if(!this->header.IsValid()) return false;

    this->buffer = this->header.content;
    if(this->header.headers.count("host") > 0 && this->header.headers.count("upgrade") > 0 &&
        this->header.headers.count("connection") > 0 && this->header.headers.count("sec-websocket-key") > 0 &&
        this->header.headers.count("sec-websocket-version") > 0) {

        if(sc::str::tolower(this->header.headers["upgrade"]) == "websocket" &&
            sc::str::tolower(this->header.headers["connection"]).find("upgrade") != std::string::npos &&
            (this->header.headers["sec-websocket-version"] == "13")) {

            std::string shake =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: ";
            shake += this->CalculateConnectionHash(this->header.headers["sec-websocket-key"]) + "\r\n\r\n";

            Socket::Send(shake);
        } else return false;
    } else return false;

    this->handshaked = true;
    return true;
}

int sc::WebSocket::Send(std::string str) {
    return Socket::Send(Frame(str, this->type == ESOCKTYPE::CLIENT).Get());
}

int sc::WebSocket::Recv(std::string &str, uint32_t size) {
    if(!this->handshaked) return -1;

    std::string buffer;
    int status; bool fin = false;
    while(!fin) {
        sc::WebSocket::Frame frame;
        while(!(frame = sc::WebSocket::Frame::FromRaw(this->buffer)).IsLegal()) {
            if((status = Socket::Recv(buffer, size)) != 0) {
                auto a = WSAGetLastError();
                return status;
            }
            this->buffer += buffer;
        }
        this->buffer = /*this->buffer.length() == frame.Get().length() ? "" :*/ this->buffer.substr(frame.Get().length());

        switch(frame.GetOpcode()) {
        case Frame::TEXT:
        case Frame::BINARY:
            this->fragment = "";
        case Frame::CONTINUATION:
            fragment += frame.GetData();
            fin = frame.IsFin();
            break;
        case Frame::PING:
            Socket::Send(Frame(frame.GetData(), false, Frame::PONG).Get());
            break;
        case Frame::PONG:
            break;
        case Frame::CLOSE:
        default:
            this->Close();
            return -1;
        }
    }

    str = this->fragment;
    return 0;
}

void sc::WebSocket::Close() {
    Socket::Send(Frame("", false, Frame::CLOSE).Get());
    Socket::Close();
}

std::string sc::WebSocket::CalculateConnectionHash(std::string in) {
    in += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char hash[20];
    sha1::calc(in.c_str(), in.length(), hash);
    return std::string(base64_encode(hash, 20));
}

///////////////////////////////////////
//                                   //
//        WEBSOCKET FRAME CODE       //
//                                   //
///////////////////////////////////////

sc::WebSocket::Frame::Frame() {
    this->maskdata.block = rand() % 0xFFFFFFFF;
}

sc::WebSocket::Frame::Frame(std::string data, bool mask, int opcode, bool fin, uint8_t rsv) {
    this->data = data;
    this->mask = mask;
    this->opcode = opcode;
    this->fin = fin;
    this->rsv = rsv;
    
    this->maskdata.block = rand() % 0xFFFFFFFF;
}

sc::WebSocket::Frame::Frame(std::string data, uint8_t maskdata[4], bool mask, int opcode, bool fin, uint8_t rsv)
    : Frame(data, mask, opcode, fin, rsv) {
    for(int i = 0; i < 4; i++)
        this->maskdata.bytes[i] = maskdata[i];
}

void sc::WebSocket::Frame::SetOpcode(int opcode) {
    this->opcode = opcode;
}

int sc::WebSocket::Frame::GetOpcode() {
    return this->opcode;
}

void sc::WebSocket::Frame::SetData(std::string data) {
    this->data = data;
}

std::string sc::WebSocket::Frame::GetData() {
    return this->data;
}

void sc::WebSocket::Frame::SetMasked(bool mask) {
    this->mask = mask;
}

bool sc::WebSocket::Frame::IsMasked() {
    return this->mask;
}

bool sc::WebSocket::Frame::IsLegal() {
    return this->legal;
}

void sc::WebSocket::Frame::GenerateMask() {
    this->maskdata.block = rand() % 0xFFFFFFFF;
}

void sc::WebSocket::Frame::SetMask(uint8_t mask[4]) {
    for(int i = 0; i < 4; i++)
        this->maskdata.bytes[i] = mask[i];
}

void sc::WebSocket::Frame::SetMask(uint32_t mask) {
    this->maskdata.block = mask;
}

sc::WebSocket::Frame::MaskData sc::WebSocket::Frame::GetMask() {
    return this->maskdata;
}

void sc::WebSocket::Frame::SetFin(bool fin) {
    this->fin = fin;
}

bool sc::WebSocket::Frame::IsFin() {
    return this->fin;
}

std::string sc::WebSocket::Frame::Get() {
    std::string ret = "12";
    ret[0] = ((this->fin ? 1 : 0) << 7) | (this->rsv << 4) | this->opcode;
    ret[1] = (this->mask ? 1 : 0) << 7;
    if(this->data.length() < 126)
        ret[1] = ret[1] | this->data.length();
    else {
        ret[1] = ret[1] | (this->data.length() <= 0xFFFF ? 126 : 127);
        if(this->data.length() <= 0xFFFF) {
            ret += std::string("12", 2);
            ret[2] = (this->data.length() & 0xFF00) >> 8;
            ret[3] = this->data.length() & 0xFF;
        } else {
            ret += std::string("12345678", 8);
            for(int i = 0; i < 8; i++)
                ret[2 + (7 - i)] = (this->data.length() & (0xFF << (8 * i))) >> (8 * i);
        }
    }
    if(this->mask) ret += std::string((char*)this->maskdata.bytes, 4);
    if(!this->mask)
        ret += this->data;
    else {
        std::string tmp = this->data;
        for(uint64_t i = 0; i < tmp.length(); i++)
            tmp[i] = tmp[i] ^ this->maskdata.bytes[i % 4];
        ret += tmp;
    }
    return ret;
}

sc::WebSocket::Frame sc::WebSocket::Frame::ErrorFrame() {
    Frame f = Frame();
    f.legal = false;
    return f;
}

sc::WebSocket::Frame sc::WebSocket::Frame::FromRaw(std::string raw) {
    Frame f = Frame();
    if(raw.length() >= 2) {
        f.fin = (raw[0] & 0x80) != 0;
        f.rsv = raw[0] & 0x70;
        f.opcode = raw[0] & 0x0F;
        f.mask = (raw[1] & 0x80) != 0;

        uint64_t size = raw[1] & 0x7F;
        int nextByte = 2;
        if(size == 126) {
            if(raw.length() > 4) {
                size = ((uint8_t)raw[2] << 8) | (uint8_t)raw[3];
            } else return ErrorFrame();
            nextByte = 4;
        } else if(size == 127) {
            if(raw.length() > 10) {
                size = 0;
                for(int i = 0; i < 8; i++)
                    size = ((uint8_t)raw[2 + i] << 8 * (7 - i)) | size;
                nextByte = 10;
            } else return ErrorFrame();
        }

        if(f.mask) {
            if(raw.length() >= nextByte + 4) {
                for(int i = 0; i < 4; i++)
                    f.maskdata.bytes[i] = raw[nextByte + i];
            } else return ErrorFrame();
            nextByte += 4;
        } else f.GenerateMask();

        if(raw.length() >= size + nextByte) {
            if(f.mask) {
                for(int i = 0; i < size; i++)
                    raw[i + nextByte] = raw[i + nextByte] ^ f.maskdata.bytes[i % 4];
            }
            f.data = raw.substr(nextByte, size);
        } else return ErrorFrame();
    } else return ErrorFrame();
    return f;
}

std::vector<sc::WebSocket::Frame> sc::WebSocket::Frame::GenerateFrameset(std::vector<std::string> data, bool mask, int opcode) {
    std::vector<Frame> ret = std::vector<Frame>();
    for(int i = 0; i < data.size(); i++)
        ret.push_back(Frame(data[i], mask, opcode, i + 1 == data.size()));
    return ret;
}

sc::WebSocket::~WebSocket() {

}