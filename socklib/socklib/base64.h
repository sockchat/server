#ifndef BASE64H
#define BASE64H
#include "stdcc.h"
#include <string>

LIBPUB std::string base64_encode(unsigned char const*, unsigned int len);
LIBPUB std::string base64_decode(std::string const& s);

#endif