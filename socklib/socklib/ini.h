#ifndef INILOADERH
#define INILOADERH

#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include "utils.h"

namespace sc {
    class LIBPUB INI {
    private:
        sc::exception LoadError(std::ifstream *fp, std::string file, uint64_t line, std::string message) {
            if(fp != NULL)
                fp->close();

            if(line != 0) {
                return sc::exception(
                        sc::str::join({"Error in ", file, " on line ", std::to_string(line), ": ", message}, "")
                    );
            } else {
                return sc::exception(sc::str::join({"Error in ", file, ": ", message}, ""));
            }
        }
    public:
        class LIBPUB INIValueTable {
            sc::imap<> map;
            bool readonly = false;
        public:
            struct LIBPUB cast_proxy {
                std::string str;
                cast_proxy(std::string const &str) : str(str) {}

                /*operator std::string() const {
                    return this->str;
                }*/

                operator bool() const {
                    if(this->str == "true" || this->str == "1")
                        return true;
                    else if(this->str == "false" || this->str == "0")
                        return false;
                    else
                        throw std::bad_cast();
                }

                template<typename T> T cast() const {
                    std::stringstream str;
                    str << this->str;

                    T retval;
                    if(!(str >> retval))
                        throw std::bad_cast();
                    else return retval;
                }

                template<
                    typename T
                    , typename Decayed = typename std::decay<T>::type
                    , typename = typename std::enable_if<
                        !std::is_same<
                            const char*
                            , Decayed
                        >::value
                        && !std::is_same<
                            std::allocator<char>
                            , Decayed
                        >::value
                        && !std::is_same<
                            std::initializer_list<char>
                            , Decayed
                        >::value
                    >::type
                > operator T() {
                    return (*this).cast<T>();
                }

                template<typename T> bool operator== (const T &comp) const {
                    return this->cast<T>() == comp;
                }

                template<typename T> bool operator!= (const T &comp) const {
                    return this->cast<T>() != comp;
                }

                template<typename T> bool operator> (const T &comp) const {
                    return this->cast<T>() > comp;
                }

                template<typename T> bool operator>= (const T &comp) const {
                    return this->cast<T>() >= comp;
                }

                template<typename T> bool operator< (const T &comp) const {
                    return this->cast<T>() < comp;
                }

                template<typename T> bool operator<= (const T &comp) const {
                    return this->cast<T>() <= comp;
                }
            };

            INIValueTable(bool readonly = false) { 
                this->readonly = readonly; 
                this->map = sc::imap<>();
            }

            INIValueTable(INIValueTable table, bool readonly): INIValueTable(table.get(), readonly) {}

            INIValueTable(std::map<std::string, std::string> map, bool readonly = false) {
                this->readonly = readonly;
                this->map = sc::imap<>();
                for(auto i = map.begin(); i != map.end(); ++i)
                    this->map[i->first] = i->second;
            }

            cast_proxy operator[] (std::string key);
            bool contains(std::string key) const;
            void set(std::string key, std::string value);
            void unset(std::string key);
            std::map<std::string, std::string> get();
            bool IsReadOnly();
        };

        enum LIBPUB ValueType { STRING, INTEGER, DOUBLE, BOOLEAN };

        INI() {
            this->map = sc::imap<INIValueTable>();
        }

        INI(std::string file, bool readonly = true);
        INI(std::string file, std::map<std::string, std::map<std::string, ValueType>> verify, bool readonly = true);
        INI(std::map<std::string, std::map<std::string, std::string>> map, bool readonly = false);

        void Add(std::vector<std::string> sections);
        void Add(std::map<std::string, std::map<std::string, std::string>> map);
        
        void Remove(std::vector<std::string> sections);
        void Remove(std::map<std::string, std::vector<std::string>> map);

        bool IsReadOnly();

        INIValueTable& operator[] (std::string section);

        void SaveToFile(std::string file);
    private:
        void LoadFile(std::string file);
        void LoadFile(std::string file,
            std::map<std::string, std::map<std::string, ValueType>> verify);

        bool readonly = false;
        sc::imap<INIValueTable> map;
    };
}

#endif