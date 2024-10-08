#include "xplayer_utils.h"

#include <cstring>
#include <cstdarg>

#define XPLAYER_BUFF_MAX_LEN 256

std::string xpu_format_string(std::string & msg, const char * fmt, ...)
{
    char * cache = nullptr;
    char buff[XPLAYER_BUFF_MAX_LEN] = { 0 };

    int bytes = 0;
    va_list vl;

    va_start(vl, fmt);
    bytes = vsnprintf(buff, XPLAYER_BUFF_MAX_LEN, fmt, vl);
    va_end(vl);

    if (bytes < XPLAYER_BUFF_MAX_LEN)
    {
        msg.assign(buff, static_cast<size_t>(bytes));
        return msg;
    }

    cache = new(std::nothrow) char[static_cast<size_t>(bytes) + 1]{};
    if (nullptr == cache)
    {
        msg.assign(buff, XPLAYER_BUFF_MAX_LEN);
        return msg;
    }

    va_start(vl, fmt);
    vsnprintf(cache, static_cast<size_t>(bytes), fmt, vl);
    va_end(vl);

    msg.assign(cache, static_cast<size_t>(bytes));
    delete[] cache;

    return msg;
}
