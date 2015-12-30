// PLATFORM INDEPENDENT SHARED RUNTIME LIBRARY IMPLEMENTATION
// for interface, see library.h

#include "socklib/library.h"
#ifdef _WIN32 // DLL loading

sc::Library::Library(std::string file) {
    this->lib = NULL;
    this->Load(file.c_str());
}

bool sc::Library::Load(std::string file) {
    if(this->lib != NULL)
        this->Unload();
    this->lib = LoadLibrary(file.c_str());
    return this->IsLoaded();
}


FUNCPTR sc::Library::GetSymbol(std::string sym) {
    if(this->lib != NULL)
        return GetProcAddress(this->lib, sym.c_str());
    else
        return NULL;
}

bool sc::Library::IsLoaded() {
    return this->lib != NULL;
}

void sc::Library::Unload() {
    if(this->lib != NULL) {
        FreeLibrary(this->lib);
        this->lib = NULL;
    }
}

sc::Library::~Library() {
    this->Unload();
}

#else // Shared Library (ELF or whatever) loading

sc::Library::Library(std::string file) {
    this->lib = NULL;
    this->Load(file);
}

bool sc::Library::Load(std::string file) {
    if(this->lib != NULL)
        this->Unload();
    this->lib = dlopen(file.c_str(), RTLD_LAZY);
    return this->IsLoaded();
}


FUNCPTR sc::Library::GetSymbol(std::string sym) {
    if(this->lib != NULL)
        return dlsym(this->lib, sym.c_str());
    else
        return NULL;
}

bool sc::Library::IsLoaded() {
    return this->lib != NULL;
}

void sc::Library::Unload() {
    if(this->lib != NULL) {
        dlclose(this->lib);
        this->lib = NULL;
    }
}

sc::Library::~Library() {
    this->Unload();
}

#endif