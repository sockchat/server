#include "socklib/ini.h"

sc::INI::INI(std::string file, bool readonly) {
    this->readonly = readonly;
    this->map = sc::imap<INIValueTable>();
    LoadFile(file);
}

sc::INI::INI(std::string file, std::map<std::string, std::map<std::string, ValueType>> verify, bool readonly) {
    this->readonly = readonly;
    this->map = sc::imap<INIValueTable>();
    LoadFile(file, verify);
}

sc::INI::INI(std::map<std::string, std::map<std::string, std::string>> map, bool readonly) {
    this->readonly = readonly;
    this->map = sc::imap<INIValueTable>();
    for(auto i = map.begin(); i != map.end(); ++i)
        this->map[i->first] = INIValueTable(i->second);
}

void sc::INI::LoadFile(std::string file) {
    //this->readonly = readonly;
    this->map = sc::imap<INIValueTable>();
    LoadFile(file, std::map<std::string, std::map<std::string, ValueType>>());
}

void sc::INI::LoadFile(std::string file,
                       std::map<std::string, std::map<std::string, ValueType>> verify) {
    std::ifstream fp;
    fp.open(file);
    if(!fp.is_open()) 
        throw sc::exception(
            sc::str::join({"Could not open INI configuration file ", file, " for reading."}, "")
        );

    std::string line; std::string section = ""; uint64_t lineNum = 1;
    while(std::getline(fp, line)) {
        line = sc::str::trim(line);
        if(line != "") {
            if(line[0] != ';') {
                if(line[0] == '[') {
                    if(line[line.length() - 1] == ']') {
                        std::string tmp = sc::str::trim(line.substr(1, line.length() - 2));
                        if(tmp != "") {
                            if(this->map.count(tmp) == 0)
                                this->map[tmp] = INIValueTable();
                             else throw LoadError(&fp, file, lineNum,
                                sc::str::join({"Duplicate section '", tmp, "'."}, ""));
                            section = tmp;
                        } else throw LoadError(&fp, file, lineNum, "Section header cannot be empty.");
                    } else throw LoadError(&fp, file, lineNum, "Section header must end with ].");
                } else {
                    auto parts = sc::str::split(line, '=', 2);
                    if(parts.size() == 2) {
                        if(section != "") {
                            parts[0] = sc::str::trim(parts[0]);
                            parts[1] = sc::str::trim(parts[1]);

                            if(parts[0] != "") {
                                if(!this->map[section].contains(parts[0]))
                                    this->map[section].set(parts[0], parts[1]);
                                else throw LoadError(&fp, file, lineNum,
                                    sc::str::join({"Duplicate key '", parts[0], "' in section '", section, "'."}, ""));
                            } else throw LoadError(&fp, file, lineNum, "Key cannot be empty.");
                        } else throw LoadError(&fp, file, lineNum, "Key-value pair before first section.");
                    } else throw LoadError(&fp, file, lineNum, "Line is not a key-value pair.");
                }
            }
        }
        ++lineNum;
    }
    fp.close();

    for(auto i = this->map.begin(); i != this->map.end(); ++i)
        i->second = INIValueTable(i->second, this->readonly);

    for(auto i = verify.begin(); i != verify.end(); ++i) {
        if(this->map.count(i->first) > 0) {
            for(auto j = i->second.begin(); j != i->second.end(); ++j) {
                if(this->map[i->first].contains(j->first)) {
                    std::string type = "string";
                    int64_t a = 0; double b = 0; bool c = false;
                    try {
                        switch(j->second) {
                        case ValueType::INTEGER:
                            type = "integer";
                            a = this->map[i->first][j->first];
                            break;
                        case ValueType::DOUBLE:
                            type = "double";
                            b = this->map[i->first][j->first];
                            break;
                        case ValueType::BOOLEAN:
                            type = "boolean";
                            c = this->map[i->first][j->first];
                            break;
                        }
                    } catch(...) {
                        throw LoadError(NULL, file, 0, sc::str::join({"Value of key '", j->first, "' in section '", i->first, "' could not be casted to ", type, "."}, ""));
                    }
                } else throw LoadError(NULL, file, 0, sc::str::join({"Section '", i->first, "' contains no key '", j->first, "'."}, ""));
            }
        } else throw LoadError(NULL, file, 0, sc::str::join({"Section '", i->first, "' not found."}, ""));
    }
}

void sc::INI::SaveToFile(std::string file) {
    std::ofstream fp;
    fp.open(file, std::ofstream::out | std::ofstream::trunc);
    if(!fp.is_open())
        throw sc::exception(
            sc::str::join({"Could not open INI configuration file ", file, " for writing."}, "")
        );

    for(auto i = this->map.begin(); i != this->map.end(); ++i) {
        fp << "[" << i->first << "]" << std::endl;
        auto tmp = i->second.get();
        for(auto j = tmp.begin(); j != tmp.end(); ++j)
            fp << j->first << " = " << j->second << std::endl;
        fp << std::endl;
    }
    fp.close();
}

sc::INI::INIValueTable& sc::INI::operator[] (std::string section) {
    if(this->map.count(section) > 0 || !this->readonly)
        return this->map[section];
    else throw sc::exception(sc::str::join({"Could not find section '", section, "'."}, ""));
}

bool sc::INI::IsReadOnly() {
    return this->readonly;
}

bool sc::INI::INIValueTable::IsReadOnly() {
    return this->readonly;
}

void sc::INI::Add(std::vector<std::string> sections) {
    if(this->readonly) return;
    for(auto i = sections.begin(); i != sections.end(); ++i) {
        if(this->map.count(*i) == 0 && *i != "")
            this->map[*i] = INIValueTable();
    }
}

void sc::INI::Add(std::map<std::string, std::map<std::string, std::string>> map) {
    if(this->readonly) return;
    for(auto i = map.begin(); i != map.end(); ++i) {
        if(i->first == "") continue;
        if(this->map.count(i->first) == 0)
            this->map[i->first] = INIValueTable(i->second);
        else {
            for(auto j = i->second.begin(); j != i->second.end(); ++j)
                this->map[i->first].set(j->first, j->second);
        }
    }
}

void sc::INI::Remove(std::vector<std::string> sections) {
    if(this->readonly) return;
    for(auto i = sections.begin(); i != sections.end(); ++i)
        this->map.erase(*i);
}

void sc::INI::Remove(std::map<std::string, std::vector<std::string>> map) {
    if(this->readonly) return;
    for(auto i = map.begin(); i != map.end(); ++i) {
        if(this->map.count(i->first) > 0) {
            for(auto j = i->second.begin(); j != i->second.end(); ++j)
                this->map[i->first].unset(*j);
        }
    }
}

void sc::INI::INIValueTable::set(std::string key, std::string value) {
    if(key != "" && !this->readonly)
        this->map[key] = value;
}

std::map<std::string, std::string> sc::INI::INIValueTable::get() {
    std::map<std::string, std::string> ret;
    for(auto i = this->map.begin(); i != map.end(); ++i)
        ret[i->first] = i->second;
    return ret;
}

void sc::INI::INIValueTable::unset(std::string key) {
    if(key != "" && !this->readonly)
        this->map.erase(key);
}

bool sc::INI::INIValueTable::contains(std::string key) const {
    return this->map.count(key) > 0;
}

sc::INI::INIValueTable::cast_proxy sc::INI::INIValueTable::operator[] (std::string key) {
    if(this->map.count(key) > 0)
        return cast_proxy(this->map[key]);
    else throw sc::exception(sc::str::join({"Could not find key '", key, "'."}, ""));
}