#ifndef UTILSH
#define UTILSH

#include <exception>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <algorithm>
#include <chrono>
#include <time.h>
#include "stdcc.h"
#include "utf8.h"

#undef htonb
#undef HTONB
#undef htonub
#undef HTONUB
#undef ntohb
#undef NTOHB
#undef ntohub
#undef NTOHUB

#undef htons
#undef HTONS
#undef htonus
#undef HTONUS
#undef ntohs
#undef NTOHS
#undef ntohus
#undef NTOHUS

#undef htonl
#undef HTONL
#undef htonul
#undef HTONUL
#undef ntohl
#undef NTOHL
#undef ntohul
#undef NTOHUL

#undef htonll
#undef HTONLL
#undef htonull
#undef HTONULL
#undef ntohll
#undef NTOHLL
#undef ntohull
#undef NTOHULL

#undef tostr
#undef TOSTR

#define TOSTR(X) std::to_string(X)

#define HTONB (X) sc::net::htonv<int8_t>(X)
#define HTONUB(X) sc::net::htonv<uint8_t>(X)
#define NTOHB (X) sc::net::ntohv<int8_t>(X)
#define NTOHUB(X) sc::net::ntohv<uint8_t>(X)

#define HTONS (X) sc::net::htonv<int16_t>(X)
#define HTONUS(X) sc::net::htonv<uint16_t>(X)
#define NTOHS (X) sc::net::ntohv<int16_t>(X)
#define NTOHUS(X) sc::net::ntohv<uint16_t>(X)

#define HTONL (X) sc::net::htonv<int32_t>(X)
#define HTONUL(X) sc::net::htonv<uint32_t>(X)
#define NTOHL (X) sc::net::ntohv<int32_t>(X)
#define NTOHUL(X) sc::net::ntohv<uint32_t>(X)

#define HTONLL (X) sc::net::htonv<int64_t>(X)
#define HTONULL(X) sc::net::htonv<uint64_t>(X)
#define NTOHLL (X) sc::net::ntohv<int64_t>(X)
#define NTOHULL(X) sc::net::ntohv<uint64_t>(X)

namespace sc {
    class LIBPUB str {
        LIBPRIV static short getCharSize(uint32_t);
    public:
        typedef int(*transformFunc)(int);

        static uint32_t at(std::string str, int loc);
        static std::string transformBytes(std::string str, transformFunc func);
        static std::string tolower(std::string str);
        static std::string toupper(std::string str);
        static int length(std::string str);
        static bool valid(std::string str);
        static std::string fix(std::string str);
        static std::string substr(std::string str, int start, int end = -1);
        static std::string substring(std::string str, int start, int length = -1);
        static std::vector<std::string> split(std::string str, char delim, int count = -1);
        static std::vector<std::string> split(std::string str, std::string delim, int count = -1);
        static std::string join(std::vector<std::string> arr, std::string delim, int count = -1);
        static std::string trim(std::string str);
        static std::string ftrim(std::string str);
        static std::string btrim(std::string str);
        static bool starts(std::string str, std::string test);
        static bool ends(std::string str, std::string test);

        template<typename T>
        static bool cast(std::string str, T &ret, T(*castFunc)(std::string)) {
            try {
                ret = castFunc(str);
                return true;
            } catch(...) {
                return false;
            }
        }

        template<typename T>
        static bool cast(std::string str, T &ret, std::function<T(std::string)> castFunc) {
            try {
                ret = castFunc(str);
                return true;
            } catch(...) {
                return false;
            }
        }

        template<typename T = int32_t>
        static bool toInt(std::string str, T &ret, bool isSigned = false) {
            return cast<T>(str, ret, [&](std::string strin)->T {
                return static_cast<T>(isSigned ? std::stoll(strin) : std::stoull(strin));
            });
        }

        template<typename T = double>
        static bool toDouble(std::string str, T &ret) {
            return cast<T>(str, ret, [](std::string strin)->T {
                return static_cast<T>(std::stold(strin));
            });
        }
    };

    class LIBPUB net {
    public:
        template<typename T = uint32_t>
        static std::string htonv(T in) {
            size_t byteCount = sizeof(T);
            std::string ret(byteCount, '\0');

            for(int i = 0; i < byteCount; ++i) {
                ret[byteCount - 1 - i] =
                    (in & (0xFF << 8 * i)) >> 8 * i;
            }

            return ret;
        } 

        template<typename T = uint32_t>
        static T ntohv(std::string in) {
            size_t byteCount = in.length();
            if(byteCount > sizeof(T))
                byteCount = sizeof(T);

            T ret = 0;
            for(int i = 0; i < byteCount; ++i)
                ret = ret | ((in[byteCount - 1 - i] & 0xFF) << 8*i);

            return ret;
        }

        static std::string packTime();
        static std::string packTime(std::chrono::time_point<std::chrono::system_clock> t);
        static std::string packErrorTime();
        
        static std::chrono::time_point<std::chrono::system_clock> unpackTime(std::string sockstamp);
    };

    class LIBPUB exception : public std::exception {
        std::string details;
    public:
        exception() : std::exception() {
            this->details = "generic exception";
        }

        exception(std::exception &e) : std::exception(e) {
            this->details = "generic exception";
        }

        exception(std::string details) {
            this->details = details;
        }

        virtual const char* what() const throw() {
            return this->details.c_str();
        }
    };
    
    class LIBPUB ip {
        // first pair item is the octet pair as seen in an IPv6 address
        // second pair item determines what nibble(s) of this octet pair are to be treated as a wildcard
        //  basic bitmask, four most significant bits are unused while the four least significant bits
        //  are set according to what nibble is to be treated as a wildcard
        //  ex. ::F F F F
        //        0 1 1 0 (bitmask)
        //        4 3 2 1 (bit position where 1 is LSB)
        //      would treat the middle two nibbles as wildcards; so this ip would match, for example,
        //      ::F00F or ::FAAF or ::FEAF etc.
        typedef std::pair<uint16_t, uint8_t> addrpart;
        addrpart parts[8] = {};
        
        static addrpart ParsePart(std::string part);
    public:
        // note that all strings taken as arguments will accept both IPv4 and IPv6 addresses
        // an IPv4 address will be stored internally as an IPv6 address in this format:
        //      127.126.0.255
        //   -> ::FFFF:7F7E:FF (which fully qualified is 0000:0000:0000:0000:0000:FFFF:7F7E:00FF)
        //          ^  ^ ^ ^
        //          |-- - - ----- UNIQUE PREFIX VALUE (decimal 65535)
        //             |- - ----- FIRST OCTET AS HEXADECIMAL (127 -> 7F)
        //               |- ----- SECOND OCTET AS HEXADECIMAL (126 -> 7E)
        //                 |----- THIRD OCTET IS HIDDEN SINCE FOURTH IS LOWER (0.255 -> 00FF -(equiv. to)> FF)
        
        static bool IsFormatCorrect(std::string ip, std::string *details = NULL);
        
        ip();
        ip(std::string ip);
        ip(addrpart parts[8]);
        
        bool IsIPv4() const;
        std::string str(bool forceIPv6 = false) const;
        
        bool operator== (const sc::ip &other) const; // weak equality comparison (all addrparts match or conform to wildcards)
        bool operator!= (const sc::ip &other) const; // weak inequality comparison (any addrpart does not match nor conform to wildcards)
        
        // the following operators were randomly picked, don't try to make sense out of them
        bool operator & (const sc::ip &other) const; // strict equality comparison (are all addrparts EXACTLY the same, including wildcards?)
        bool operator ^ (const sc::ip &other) const; // strict inequality comparison (is any addrpart DIFFERENT, including wildcards?)
    };

    struct compimap {
        bool operator() (const std::string &lhs, const std::string &rhs) const { //What does this even do?????  How do you call it???????
            return sc::str::tolower(lhs) < sc::str::tolower(rhs);
        }
    };

    template<typename T = std::string> using imap = std::map<std::string, T, compimap>;
}

#endif