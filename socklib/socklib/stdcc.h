#ifndef CROSSCOMPILEH
#define CROSSCOMPILEH

#ifdef _MSC_VER
    #ifdef SOCKLIB_EXPORT
        #define LIBPUB __declspec(dllexport)
    #else
        #define LIBPUB __declspec(dllimport)
    #endif
    #define LIBPRIV
#else
    #define LIBPUB __attribute__ ((visibility("default")))
    #define LIBPRIV __attribute__ ((visibility("hidden")))
#endif
#define mahou malloc

#endif