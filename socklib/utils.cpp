#include "socklib/utils.h"

uint32_t sc::str::at(std::string str, int loc) {
    const char *front = str.c_str(), *back = front + str.length(), *ptr = front;
    uint32_t ret = 0;
    for(; loc >= 0; loc--) {
        if(ptr == back) break;
        ret = utf8::next(ptr, back);
    }
    return loc == 0 ? ret : 0;
}

std::string sc::str::transformBytes(std::string str, sc::str::transformFunc func) {
    std::string ret = str;
    const char *ptr = str.c_str(), *lptr = ptr;
    const char *start = ptr, *end = ptr + str.length();
    while(ptr != end) {
        uint32_t c = utf8::next(ptr, end);
        if(sc::str::getCharSize(c) == 1) {
            ret[lptr - start] = func((char)c);
        }
        lptr = ptr;
    }
    return ret;
}

std::string sc::str::tolower(std::string str) {
    return sc::str::transformBytes(str, ::tolower);
}

std::string sc::str::toupper(std::string str) {
    return sc::str::transformBytes(str, ::toupper);
}

int sc::str::length(std::string str) {
    return utf8::distance(str.begin(), str.end());
}

bool sc::str::valid(std::string str) {
    return utf8::is_valid(str.begin(), str.end());
}

std::string sc::str::fix(std::string str) {
    utf8::replace_invalid(str.begin(), str.end(), std::back_inserter(str));
    return str;
}

std::string sc::str::substr(std::string str, int start, int end) {
    if(start > end) return "";
    return sc::str::substring(str, start, end == -1 ? -1 : end - start);
}

std::string sc::str::substring(std::string str, int start, int length) {
    if(length == 0) return "";
    else {
        const char *front = str.c_str(), *back = front + str.length();
        const char *pstart = front, *pend;

        while(start != 0 && pstart != back) {
            utf8::next(pstart, back);
            start--;
        }
        if(pstart == back) return "";

        if(length > 0) {
            pend = pstart;
            while(length != 0 && pend != back) {
                utf8::next(pend, back);
                length--;
            }
        } else pend = back;
        return str.substr(pstart - front, pend - pstart);
    }
}

std::vector<std::string> sc::str::split(std::string str, char delim, int count) {
    count--;
    std::stringstream ss(str);
    auto ret = std::vector<std::string>();
    std::string buffer;
    while(count != 0) {
        if(!std::getline(ss, buffer, delim)) break;
        ret.push_back(buffer);
        count--;
    }
    if(std::getline(ss, buffer)) ret.push_back(buffer);
    return ret;
}

std::vector<std::string> sc::str::split(std::string str, std::string delim, int count) {
	count--;
    auto ret = std::vector<std::string>();
    std::size_t pos = 0, lastpos = 0;
    while((pos = str.find(delim, pos)) != std::string::npos && count != 0) {
        if(pos - lastpos > 0)
            ret.push_back(str.substr(lastpos, pos - lastpos));
        else
            ret.push_back("");

        pos += delim.length();
        lastpos = pos;
        count--;
    }
    ret.push_back(str.substr(lastpos, std::string::npos));
    return ret;
}

std::string sc::str::join(std::vector<std::string> arr, std::string delim, int count) {
    std::string ret;
    for(int i = 0; i < arr.size(); i++) {
        if(count == 0) break;
        ret += (i == 0 ? "" : delim) + arr[i];
        count--;
    }
    return ret;
}

std::string sc::str::trim(std::string str) {
    return sc::str::btrim(sc::str::ftrim(str));
}

std::string sc::str::ftrim(std::string str) {
    if(str.length() == 0) {
        return "";
    } else {
        const char *front = str.c_str();
        const char *ptr = front;

        do {
            uint32_t c = utf8::next(ptr, front + str.length());
            char ch = (char)c;
            if(sc::str::getCharSize(c) > 1 || (!isspace(ch) && !iscntrl(ch))) {
                utf8::prior(ptr, front);
                break;
            }
        } while(ptr != front + str.length()); //Any reason why you don't make a back pointer?

        return ptr == front + str.length() ? "" : str.substr(ptr-front);
    } 
}

std::string sc::str::btrim(std::string str) {
    if(str.length() == 0) {
        return "";
    } else {
        const char *back = str.c_str() + str.length();
        const char *ptr = back;

        do {
            uint32_t c = utf8::prior(ptr, str.c_str());
            char ch = (char)c;
            if(sc::str::getCharSize(c) > 1 || (!isspace(ch) && !iscntrl(ch))) {
                utf8::next(ptr, back);
                break;
            }
        } while(ptr != str.c_str()); //Any reason why you don't make a front pointer?

        return ptr == str.c_str() ? "" : str.substr(0, ptr - str.c_str());
    }
}

bool sc::str::starts(std::string str, std::string test) {
    if(test.length() > str.length()) return false;
    return str.substr(0, test.length()) == test;
}

bool sc::str::ends(std::string str, std::string test) {
    if(test.length() > str.length()) return false;
    return str.substr(str.length() - test.length()) == test;
}

short sc::str::getCharSize(uint32_t c) {
    return c <= 0xFF        ? 1 :
          (c <= 0xFFFF      ? 2 :
          (c <= 0xFFFFFF    ? 3 :
          (c <= 0xFFFFFFFF  ? 4 : 5)));
}

std::string sc::net::packTime() {
    return packTime(std::chrono::system_clock::now());
}

std::string sc::net::packTime(std::chrono::time_point<std::chrono::system_clock> t) {
    time_t tmp = std::chrono::system_clock::to_time_t(t);
    struct tm utc;

#ifdef _WIN32
    gmtime_s(&utc, &tmp);
#else
    gmtime_r(&tmp, &utc);
#endif
    
    int year = utc.tm_year - 115;
    if(year > 2047) year = 2047;
    if(year < -2048) year = -2048;

    std::stringstream ss;
    ss << (char)((year & 0xFF0) >> 4) << (char)(((year & 0xF) << 4) | utc.tm_mon) << (char)(utc.tm_mday - 1)
       << (char)utc.tm_hour << (char)utc.tm_min << (char)utc.tm_sec;
    
    return ss.str();
}

bool sc::ip::IsFormatCorrect(std::string ip, std::string *details) {
    try {
        auto test = sc::ip(ip);
        return true;
    } catch(sc::exception e) {
        *details = e.what();
        return false;
    }
}

sc::ip::ip() {}

sc::ip::ip(addrpart parts[8]) {
    for(int i = 0; i < 8; i++)
        this->parts[i] = parts[i];
}

sc::ip::addrpart sc::ip::ParsePart(std::string part) {
    addrpart ret;
    part = sc::str::trim(part);
    
    if(part == "*")
        ret = std::make_pair(0, 0xF);
    else {
        if(part.length() <= 4) {
            ret.second = 0;
            
            while(part.length() < 4)
                part = std::string("0") + part;
            
            size_t pos = 0;
            while((pos = part.find("*", pos)) != std::string::npos) {
                part.replace(pos, 1, "F");
                ret.second |= 1 << (3-pos);
            }
            
            try {
                ret.first = std::stoi(part, NULL, 16);
            } catch(...) {
                throw sc::exception("IPv6 address part contained (a) character(s) that was not a hex char (0-9, A-F) or wildcard (*).");
            }
        } else throw sc::exception("IPv6 address part contained more than four ASCII characters.");
    }
    
    return ret;
}

sc::ip::ip(std::string ip) {
    ip = sc::str::trim(ip);
    std::vector<std::string> parts;
    if((parts = sc::str::split(ip, ':')).size() > 1) {
        if(ip == "::") {
            for(int i = 0; i < 8; i++)
                this->parts[i] = std::make_pair(0, 0);
        } else {
            if(ip.find(".") != std::string::npos) throw sc::exception("Address specified is not a valid IPv6 or IPv4 address.");
            
            auto doubleColonSplit = sc::str::split(ip, "::");
            if(doubleColonSplit.size() == 1) {
                if(parts.size() == 8) {
                    for(int i = 0; i < 8; i++)
                        this->parts[i] = ParsePart(parts[i]);
                } else throw sc::exception("IPv6 address did not contain eight colon-seperated segments.");
            } else if(doubleColonSplit.size() == 2) {
                doubleColonSplit[0] = sc::str::trim(doubleColonSplit[0]);
                doubleColonSplit[1] = sc::str::trim(doubleColonSplit[1]);

                if(doubleColonSplit[0].length() > 0) {
                    if(doubleColonSplit[0][0] == ':' ||
                       doubleColonSplit[0][doubleColonSplit[0].length() - 1] == ':')
                       throw sc::exception("IPv6 address contains invalid sequence of colon delimeters.");
                }

                if(doubleColonSplit[1].length() > 0) {
                    if(doubleColonSplit[1][0] == ':' ||
                       doubleColonSplit[1][doubleColonSplit[1].length() - 1] == ':')
                       throw sc::exception("IPv6 address contains invalid sequence of colon delimeters.");
                }
                
                if(doubleColonSplit[0] == "") {
                    parts = sc::str::split(doubleColonSplit[1], ':');
                    if(parts.size() > 8) throw sc::exception("IPv6 address contained more than eight colon-seperated segments.");
                    
                    for(int i = 0; i < 8; i++)
                        this->parts[7 - i] = i < parts.size() ? ParsePart(parts[parts.size() - (i+1)]) : std::make_pair((uint16_t)0, (uint8_t)0);
                } else if (doubleColonSplit[1] == "") {
                    parts = sc::str::split(doubleColonSplit[0], ':');
                    if(parts.size() > 8)
                        throw sc::exception("IPv6 address contained more than eight colon-seperated segments.");
                    
                    for(int i = 0; i < 8; i++)
                        this->parts[i] = i < parts.size() ? ParsePart(parts[i]) : std::make_pair((uint16_t)0, (uint8_t)0);
                } else {
                    auto first = sc::str::split(doubleColonSplit[0], ':');
                    auto second = sc::str::split(doubleColonSplit[1], ':');
                    
                    if(first.size() + second.size() > 8)
                        throw sc::exception("IPv6 address contained more than eight colon-seperated segments.");
                    
                    for(int i = 0; i < 8; i++) {
                        if(i < first.size())
                            this->parts[i] = ParsePart(first[i]);
                        else if (i < (8 - second.size()))
                            this->parts[i] = std::make_pair(0, 0);
                        else
                            this->parts[i] = ParsePart(second[i - (8 - second.size())]);
                    }
                }
            } else throw sc::exception("IPv6 address contains more than one double-colon identifier.");
        }
    } else if((parts = sc::str::split(ip, '.')).size() == 4) {
        parts.resize(4);
        uint16_t ipBytes[4];
        for(int i = 0; i < 4; ++i) {
            try {
                parts[i] = sc::str::trim(parts[i]);
                
                if(parts[i] != "*") {
                    int test = std::stoi(parts[i]);
                    if(test <= 255 && test >= 0) {
                        ipBytes[i] = test;
                    } else throw sc::exception(sc::str::join({"Address was out of range at ASCII encoded byte ", TOSTR(i+1)}, ""));
                } else ipBytes[i] = 0xFFFF;
            } catch(...) {
                throw sc::exception(sc::str::join({"Address was malformed at ASCII encoded byte ", TOSTR(i+1)}, ""));
            }
            
            for(int i = 0; i < 5; i++)
                this->parts[i] = std::make_pair(0, 0);
            
            this->parts[5] = std::make_pair(0xFFFF, 0);
            this->parts[6] = std::make_pair(((uint8_t)ipBytes[0] << 8) | (uint8_t)ipBytes[1], (ipBytes[0] == 0xFFFF ? 0xC : 0) | (ipBytes[1] == 0xFFFF ? 0x3 : 0));
            this->parts[7] = std::make_pair(((uint8_t)ipBytes[2] << 8) | (uint8_t)ipBytes[3], (ipBytes[2] == 0xFFFF ? 0xC : 0) | (ipBytes[3] == 0xFFFF ? 0x3 : 0));
        }
    } else throw sc::exception("Address specified is not a valid IPv6 or IPv4 address.");
}

bool sc::ip::IsIPv4() const {
    bool ret = true;
    
    for(int i = 0; i < 5; i++)
        ret = ret && (parts[i].first == 0 && parts[i].second == 0);
    ret = ret && (parts[5].first == 0xFFFF && parts[5].second == 0);
    
    return ret;
}

std::string sc::ip::str(bool forceIPv6) const {
    if(IsIPv4() && !forceIPv6) {
        std::stringstream ss;

        ss  << ((parts[6].second & 0xC == 0xC) ? "*" : TOSTR((parts[6].first & 0xFF00) >> 8))   << "."
            << ((parts[6].second & 0x3 == 0x3) ? "*" : TOSTR( parts[6].first & 0x00FF))         << "."
            << ((parts[7].second & 0xC == 0xC) ? "*" : TOSTR((parts[7].first & 0xFF00) >> 8))   << "."
            << ((parts[7].second & 0x3 == 0x3) ? "*" : TOSTR( parts[7].first & 0x00FF));

        return ss.str();
    } else {
        auto tmp = std::vector<std::string>();

        std::pair<uint8_t, uint8_t> largestGap = { 0, 0 };
        std::pair<uint8_t, uint8_t> currentGap = { 0, 0 };
        bool inGap = false;

        for(int i = 0; i < 8; i++) {
            if(parts[i].first == 0) {
                tmp.push_back("0");

                if(inGap)
                    ++currentGap.second;
                else {
                    inGap = true;
                    currentGap = {i, 1};
                }

                continue;
            }

            if(inGap) {
                inGap = false;
                if(currentGap.second > largestGap.second)
                    largestGap = currentGap;
            }
            
            if(parts[i].second == 0xF)
                tmp.push_back("*");
            else {
                std::stringstream ss;
                ss << std::hex << parts[i].first;
                std::string holder = ss.str();

                for(int j = 0; j < 4; j++) {
                    auto view = 0x1 << j;
                    if((parts[i].second & (0x1 << j)) >> j == 1)
                        if(j < holder.size())
                            holder[holder.size() - j - 1] = '*';
                }
                
                /*
                bool nonZeroYet = false;

                for(int j = 3; j >= 0; j--) {
                    uint8_t part = (parts[i].first & (0xF << (4*j))) >> (4*j);

                    nonZeroYet = nonZeroYet || part != 0;
                    if(nonZeroYet) {
                        if(parts[i].second & (0x1 << j) == 0x1 << j)
                            ss << "*";
                        else
                            ss << std::hex << part;
                    }
                }
                */
                
                tmp.push_back(holder);
            }
        }

        if(currentGap.second > largestGap.second)
            largestGap = currentGap;

        if(largestGap.second > 1) {
            tmp[largestGap.first] = largestGap.first == 0 || largestGap.first + largestGap.second == 8 ? ":" : "";
            tmp.erase(tmp.begin() + largestGap.first + 1, tmp.begin() + largestGap.first + largestGap.second);
        }

        return sc::str::join(tmp, ":");
    }
}

bool sc::ip::operator== (const sc::ip &other) const {
    bool ret = true;
    for(int i = 0; i < 8; i++) {
        if(this->parts[i].second == 0 && other.parts[i].second == 0)
            ret = ret && this->parts[i].first == other.parts[i].first;
        else {
            for(int j = 0; j < 4; j++) {
                if((this->parts[i].second & (1 << j)) == 0 &&
                   (other.parts[i].second & (1 << j)) == 0) {
                    ret = ret && (
                        (this->parts[i].first & (0xF << (4 * j))) ==
                        (other.parts[i].first & (0xF << (4 * j)))
                    );
                }
            }
        }
    }

    return ret;
}

bool sc::ip::operator!= (const sc::ip &other) const {
    return !(*this == other);
}