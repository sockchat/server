#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <locale>
#include <time.h>
#include "socklib/socket.h"
#include "socklib/library.h"
#include "socklib/utils.h"
#include "socklib/ini.h"
#include "socklib/context.h"
#include "cthread.h"

sc::Socket sock = sc::Socket();

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
BOOL ctrlHandler(DWORD ctrlno) {
    sock.Close();
    WSACleanup();
    exit(0);
}
#else
#include <signal.h>
void sigHandler(int signo) {
    sock.Close();
    exit(0);
}
#endif

int main(int argc, char* argv[]) {
    //int c = 0;
    
    //std::thread t(shit, 2, 3, std::ref(c));
    //std::cout << c << std::endl;
    
    /*sc::Library lib("modules/libtestmod.dylib");
    
    typedef void(*modfunc)();
    modfunc f = (modfunc)lib.GetSymbol("initMod");
    f();*/

    //std::string hash = sha1::hash("x3JJHMbDL1EzLkh9GBhXDw==258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    //std::string str = sc::str::tolower("x3JJHMbDL1 € EzLkh9GBh¢XDw==");

    //auto ef = sc::str::split("fu\nck", '\n');

    //std::cout << calculateConnectionHash(str);

    //std::cout << base64_decode("HSmrc0sMlYUkAGmm5OPpG2HaGWk=");

#ifdef _WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)&ctrlHandler, TRUE);
    WSADATA wdata;
    if(WSAStartup(MAKEWORD(2, 2), &wdata) != 0)
        return false;
#else
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGABRT, sigHandler);
#endif

    std::string wowo = sc::net::packTime();
	auto owow = sc::net::unpackTime(wowo);
	struct tm utc;
	time_t f = std::chrono::system_clock::to_time_t(owow);
	gmtime_s(&utc, &f);

    sc::Context::Init();
    
    sc::Socket client;
    if(!sock.Init(sc::Context::Config()["socket"]["port"])) {
        std::cout << "Could not open socket on port " << (int)sc::Context::Config()["socket"]["port"] << "! Error: " << sock.GetLastError() << std::endl;
        return -1;
    }
    sock.SetBlocking(true);

    int status;
    Threads::Init();
    while(true) {
        if((status = sock.Accept(client)) == 0)
            Threads::AddConnection(new sc::Socket(client));
        else if(status == -1) break;
    }

    sock.Close();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}