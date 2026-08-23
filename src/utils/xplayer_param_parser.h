#ifndef __XPLAYER_PARAM_PARSER_H__
#define __XPLAYER_PARAM_PARSER_H__

#include <string>
#include <vector>
#include <map>

#define XPLAYER_PARSER_PARENS        0b00000001 // ()
#define XPLAYER_PARSER_BRACKETS      0b00000010 // []
#define XPLAYER_PARSER_BRACES        0b00000100 // {}
#define XPLAYER_PARSER_ANGLES        0b00001000 // <>
#define XPLAYER_PARSER_SINGLE_QUOTES 0b00010000 // ''
#define XPLAYER_PARSER_DOUBLE_QUOTES 0b00100000 // ""
#define XPLAYER_PARSER_ALL           0b00111111 // all

class CXPlayerParser
{
public:
    CXPlayerParser() = default;
	~CXPlayerParser() = default;

    // 初始化
    bool parse(const std::string & params, char pair_delim, char kv_delim, int flags);

    // 是否存在
    bool exist(const std::string & key);

    // 获取所有的键值对
    std::map<std::string, std::string> get();

    // 获取数值
    bool get(const std::string & key, bool default_val);
    int16_t get(const std::string & key, int16_t default_val);
    int32_t get(const std::string & key, int32_t default_val);
    int64_t get(const std::string & key, int64_t default_val);
    uint16_t get(const std::string & key, uint16_t default_val);
    uint32_t get(const std::string & key, uint32_t default_val);
    uint64_t get(const std::string & key, uint64_t default_val);
    float get(const std::string & key, float default_val);
    double get(const std::string & key, double default_val);
    std::string get(const std::string & key, std::string default_val);

private:
    // 解析标记
    void parse(int flags);

    // 分割kv
    bool split(const std::string & str, char delim, std::pair<std::string, std::string> & kv);

private:
    // kv键值对
    std::map<std::string, std::string> _kvs;

    // 包含特殊字符
    std::vector<std::pair<char, char>> _chars;
};

#endif
