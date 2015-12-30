#ifndef SCMESSAGEH
#define SCMESSAGEH    

#include <string>
#include <vector>
#include <list>
#include <stdint.h>
#include "stdcc.h"
#include "utils.h"

#define P_PING     0
#define P_JOIN     1
#define P_MSG      2
#define P_SETTINGS 3
#define P_RSV      4
#define P_CHCHAN   5
#define P_CTX_DEL  6
#define P_CTX_DATA 7
#define P_CTX_CLR  8
#define P_BAKA     9

namespace sc {
    class LIBPUB Message {
        bool legal = true;
        
        size_t headerSize = 0, bodySize = 0;
        std::string raw = "";

        uint16_t id = 0;
        std::vector<std::string> parts = std::vector<std::string>();
    public:        
        Message() { this->legal = false; };
        Message(std::string raw);
        Message(uint16_t id, std::vector<std::string> parts);

        bool Init(std::string raw);
        bool Init(uint16_t id, std::vector<std::string> parts);

        bool IsLegal();
        uint16_t GetID();
        std::vector<std::string> GetParts();
        std::string operator[] (uint16_t i);
        std::string Get();

        size_t Size();
        size_t HeaderSize();
        size_t BodySize();

        size_t Count();
    };

    class LIBPUB Backlog {
    public:
        typedef std::list<sc::Message> Logs;

        Backlog() { this->length = 10; }
        Backlog(size_t logLength) { this->length = logLength; };
        void SetLength(size_t length) { this->length = length; };
        Logs Get() const { return this->logs; };
        void Push(sc::Message msg);
    private:
        Logs logs = Logs();
        size_t length;
    };
}

#endif