// PLATFORM INDEPENDENT TCP SOCKET AND GENERAL WEB SOCKET INTERFACE
// For implementation, see socket.cpp

#ifndef SOCKETH
#define SOCKETH

#define SOCK_BUFLEN 2048
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#pragma comment (lib, "ws2_32.lib")
#define HSOCKET SOCKET
#define HADDR SOCKADDR_IN
#else
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#define HSOCKET int
#define HADDR struct sockaddr_in
#endif

#include <map>
#include <string>
#include <random>
#include <time.h>
#include <iomanip>
#include <thread>
#include "stdcc.h"
#include "utils.h"
#include "sha1.h"
#include "base64.h"
#include "msg.h"

namespace sc {
    class LIBPUB Socket {
    public:
        enum LIBPUB ESOCKTYPE { SERVER, SERVERSPAWN, CLIENT, UNINIT }; //What is a serverspawn?

        Socket();

        bool Init(short port);
        bool Init(std::string addr, uint16_t port);
        bool Init(HSOCKET sock, HADDR addr, int addrlen);
        
        sc::ip GetIPAddress();

        void SetBlocking(bool block);
        bool GetBlocking();

        void SetTimeout(int seconds, int msecs = 0);

        // all of the following return -1 on error, 0 on success, and 1 if the nonblocking process would block
        virtual int Accept(Socket &conn);
        virtual int Recv(std::string &str, uint32_t size = SOCK_BUFLEN);
        virtual int Send(std::string str);

        virtual void Close();

        int GetLastError();

        virtual ~Socket();
    protected:
        HSOCKET sock;
        HADDR addr;
        int addrlen;
        bool blocking;
        char recvbuf[SOCK_BUFLEN];
        ESOCKTYPE type;
    };

    class LIBPUB HTTPRequest { //Why do you put this in socket.h instead of a http.h?
    private:
        struct URL {
            std::string protocol;
            std::string target;
            std::string resource;
            uint16_t port;
        };

        static uint16_t GetPortFromProtocol(std::string protocol);
        static URL DecipherURL(std::string url);
        static std::string URIEscapeCharacter(uint32_t c);
    public:
        class LIBPUB Response {
        private:
            void Error(int status = -1);
        public:
            enum ESTATUSCODE { OK = 200, FORBIDDEN = 403, NOTFOUND = 404, INTERR = 500 };
            int status;
            sc::imap<> headers;
            std::string content;

            Response();
            Response(std::string raw, std::string action = "HTTP");
            Response(int status, sc::imap<> headers, std::string content);

            bool IsValid();
        };

        static std::string EncodeURI(std::string uri, bool spaceIsPlus = false);
        static std::string EncodeURIComponent(std::string comp, bool spaceIsPlus = false);
        static std::string EncodeURIComponentStrict(std::string comp, bool spaceIsPlus = false);

        static Response Raw(std::string action, std::string url, sc::imap<> headers = sc::imap<>(), std::string body = "");

        static Response Get(std::string url, sc::imap<> data = sc::imap<>(), sc::imap<> headers = sc::imap<>());

        static Response Post(std::string url, sc::imap<> data, sc::imap<> headers = sc::imap<>());
    };

    class LIBPUB RawSocket : public Socket {
        std::string buffer = "";
    public:
        RawSocket(Socket sock);

        int Recv(std::string &str, uint32_t size = SOCK_BUFLEN);
    };

    class LIBPUB WebSocket : public Socket {
        bool handshaked;
        sc::HTTPRequest::Response header;
        std::string fragment;
        std::string buffer;

        LIBPRIV std::string CalculateConnectionHash(std::string in);
    public:
        WebSocket();
        WebSocket(Socket sock);

        bool Handshake(std::string data = "");

        int Recv(std::string &str, uint32_t size = SOCK_BUFLEN);
        int Send(std::string str);

        void Close();

        ~WebSocket();

        class LIBPUB Frame {
        public:
            union LIBPUB MaskData {
                uint32_t block;
                uint8_t bytes[4];
            };

            enum LIBPUB Opcode { CONTINUATION = 0x0, TEXT = 0x1, BINARY = 0x2, CLOSE = 0x8, PING = 0x9, PONG = 0xA };

            Frame();
            Frame(std::string data, bool mask = false, int type = Opcode::BINARY, bool fin = true, uint8_t rsv = 0x0);
            Frame(std::string data, uint8_t maskdata[4], bool mask = false, int type = Opcode::BINARY, bool fin = true, uint8_t rsv = 0x0);
            
            void SetOpcode(int opcode);
            int GetOpcode();
            
            void SetData(std::string data);
            std::string GetData();
            
            void SetMasked(bool mask);
            bool IsMasked();

            bool IsLegal();

            void GenerateMask();
            void SetMask(uint8_t mask[4]);
            void SetMask(uint32_t mask);
            MaskData GetMask();

            void SetFin(bool fin);
            bool IsFin();
            
            std::string Get();

            static Frame ErrorFrame();
            static Frame FromRaw(std::string raw);
            static std::vector<Frame> GenerateFrameset(std::vector<std::string> data, bool mask = false, int opcode = Opcode::TEXT);
        private:
            int opcode;
            std::string data;
            bool mask;
            MaskData maskdata;
            uint8_t rsv;
            bool fin;
            bool legal = true;
        };
    };
}

#endif