#ifndef __XPLAYER_UTILS_H__
#define __XPLAYER_UTILS_H__

#include <string>
#include <vector>

#include "xplayer_definitions.h"

// 格式化字符串
std::string xpu_format_string(std::string & msg, const char * fmt, ...);

// 文件转字符串
bool xpu_file2str(const std::string & path, std::vector<char> & buff, std::string & err);

// s16转为float
void s162flt(const uint8_t * data, int len, float * flt);

// float转为s16
void flt2s16(const float * flt, int len, uint8_t * data);

// 时间转字符串
std::string xpu_time2str(int64_t ms);

// 获取时间
int64_t xpu_time_ms();

// 像素格式转换
XPLAYER_PIXEL_FORMAT_TYPE xpu_f2x(int format);
int xpu_x2f(XPLAYER_PIXEL_FORMAT_TYPE format);

#endif
