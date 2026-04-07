#include "xplayer_utils.h"

#include <cstring>
#include <cstdarg>
#include <chrono>

#define XPLAYER_BUFF_MAX_LEN 256

static int64_t ctv_time_default_base();
static int64_t g_time_base_ms = ctv_time_default_base();

int64_t ctv_time_default_base()
{
    auto x = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    auto y = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    return x.count() - y.count();
}

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

void s162flt(const uint8_t * data, int len, float * flt)
{
    auto samples = len / sizeof(int16_t);
    for (int i = 0; i < samples; i++)
    {
        // 获取低字节和高字节
        auto low = data[i * 2];
        auto high = data[i * 2 + 1];

        // 组合成 int16_t (小端序: low + (high << 8))
        // 注意：这里强转为 int16_t 会自动处理符号位 (负数)
        int16_t tmp = static_cast<int16_t>(low | (high << 8));

        // 归一化到 [-1.0, 1.0]
        // 除以 32768.0f 而不是 32767.0f 是为了防止最大值溢出到 >1.0
        flt[i] = tmp / 32768.0f;
    }
}

void flt2s16(const float * flt, int len, uint8_t * data)
{
    for (int i = 0; i < len; ++i)
    {
        // 截断防止溢出
        float val = flt[i];
        if (val > 1.0f) val = 1.0f;
        if (val < -1.0f) val = -1.0f;

        // 放大并取整
        int32_t temp = static_cast<int32_t>(val * 32767.0f);
        int16_t sample = static_cast<int16_t>(temp);

        // 拆分为小端序字节
        data[i * 2] = static_cast<uint8_t>(sample & 0xFF);       // 低字节
        data[i * 2 + 1] = static_cast<uint8_t>((sample >> 8) & 0xFF); // 高字节
    }
}

std::string xpu_time2str(int64_t ms)
{
    std::string str;

    auto hour = ms / 1000 / 3600;
    auto minute = ms / 1000 % 3600 / 60;
    auto sec = ms / 1000 % 3600 % 60;
    auto msec = ms % 1000;

    xpu_format_string(str, "%02d:%02d:%02d.%03d", hour, minute, sec, msec);

    return str;
}

int64_t xpu_time_ms()
{
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    return g_time_base_ms + ms.count();
}
