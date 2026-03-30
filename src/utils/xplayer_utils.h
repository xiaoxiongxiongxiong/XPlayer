#ifndef __XPLAYER_UTILS_H__
#define __XPLAYER_UTILS_H__

#include <string>

// 格式化字符串
std::string xpu_format_string(std::string & msg, const char * fmt, ...);

// s16转为float
void s162flt(const uint8_t * data, int len, float * flt);

// float转为s16
void flt2s16(const float * flt, int len, uint8_t * data);

#endif
