#ifndef CCLIBH
#define CCLIBH

#ifdef _WIN32
#include <windows.h>
#define LIBHANDLE HINSTANCE
#define FUNCPTR FARPROC
#else
#include <stdlib.h>
#include <dlfcn.h>
#define LIBHANDLE void*
#define FUNCPTR void*
#endif

#include <string>
#include "stdcc.h"

namespace sc {
    class LIBPUB Library {
        LIBHANDLE lib;
    public:
        Library(std::string file);
        bool Load(std::string file);
        bool IsLoaded();
        FUNCPTR GetSymbol(std::string sym);
        void Unload();
        ~Library();
    };
}

#endif