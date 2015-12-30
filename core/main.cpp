#include "stdcc.h"
#include "socklib/socket.h"
#include <iostream>

extern "C" LIBPUB void initMod() {
    auto req = sc::HTTPRequest::Get("http://aroltd.com/");
    std::cout << req.content;
}