#include "xplayer_param_parser.h"
#include <stack>

template <typename T>
static T get_val(const std::map<std::string, std::string> & kvs, const std::string & key, T default_val)
{
    auto found = kvs.find(key);
    if (kvs.end() == found)
        return default_val;

    const auto & val = (*found).second;
    if constexpr (std::is_same_v<T, bool>)
    {
        if ("0" == val || "no" == val || "false" == val)
            return false;
        if ("1" == val || "yes" == val || "true" == val)
            return true;
        return default_val;
    }
    else if constexpr (std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>)
    {
        try
        {
            return static_cast<T>(std::stoll(val));
        }
        catch (...)
        {
            return default_val;
        }
    }
    else if constexpr (std::is_same_v<T, uint16_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>)
    {
        try
        {
            return static_cast<T>(std::stoull(val));
        }
        catch (...)
        {
            return default_val;
        }
    }
    else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
    {
        try 
        { 
            return static_cast<T>(std::stod(val));
        }
        catch (...)
        {
            return default_val;
        }
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        return val;
    }

    return default_val;
}

bool CXPlayerParser::parse(const std::string & params, char pair_delim, char kv_delim, int flags)
{
    size_t i = 0, n = params.size(), pos = 0;
    std::stack<char> bracket;

    parse(flags);

    for (i = 0; i < n; i++)
    {
        for (const auto & tmp : _chars)
        {
            if (tmp.second == params[i] && !bracket.empty() && bracket.top() == tmp.first)
            {
                bracket.pop();
                break;
            }

            if (tmp.first == params[i])
            {
                bracket.push(params[i]);
                break;
            }
        }

        if (pair_delim == params[i] && !bracket.empty())
            continue;

        if (pair_delim == params[i] || i == n - 1)
        {
            auto bytes = i - pos;
            if (i == n - 1)
                bytes++;

            auto tmp = params.substr(pos, bytes);
            std::pair<std::string, std::string> kv;
            if (split(tmp, kv_delim, kv))
                _kvs.emplace(kv.first, kv.second);
            pos = i + 1;
        }
    }

    return true;
}

bool CXPlayerParser::exist(const std::string & key)
{
    auto found = _kvs.find(key);
    return _kvs.end() != found;
}

std::map<std::string, std::string> CXPlayerParser::get()
{
    return _kvs;
}

bool CXPlayerParser::get(const std::string & key, bool default_val)
{
    return get_val(_kvs, key, default_val);
}

int16_t CXPlayerParser::get(const std::string & key, int16_t default_val)
{
    return get_val(_kvs, key, default_val);
}

int32_t CXPlayerParser::get(const std::string & key, int32_t default_val)
{
    return get_val(_kvs, key, default_val);
}

int64_t CXPlayerParser::get(const std::string & key, int64_t default_val)
{
    return get_val(_kvs, key, default_val);
}

uint16_t CXPlayerParser::get(const std::string & key, uint16_t default_val)
{
    return get_val(_kvs, key, default_val);
}

uint32_t CXPlayerParser::get(const std::string & key, uint32_t default_val)
{
    return get_val(_kvs, key, default_val);
}

uint64_t CXPlayerParser::get(const std::string & key, uint64_t default_val)
{
    return get_val(_kvs, key, default_val);
}

float CXPlayerParser::get(const std::string & key, float default_val)
{
    return get_val(_kvs, key, default_val);
}

double CXPlayerParser::get(const std::string & key, double default_val)
{
    return get_val(_kvs, key, default_val);
}

std::string CXPlayerParser::get(const std::string & key, std::string default_val)
{
    return get_val(_kvs, key, default_val);
}

void CXPlayerParser::parse(int flags)
{
    if (XPLAYER_PARSER_PARENS & flags)
        _chars.emplace_back('(', ')');
    if (XPLAYER_PARSER_BRACKETS & flags)
        _chars.emplace_back('[', ']');
    if (XPLAYER_PARSER_BRACES & flags)
        _chars.emplace_back('{', '}');
    if (XPLAYER_PARSER_ANGLES & flags)
        _chars.emplace_back('<', '>');
    if (XPLAYER_PARSER_SINGLE_QUOTES & flags)
        _chars.emplace_back('\'', '\'');
    if (XPLAYER_PARSER_DOUBLE_QUOTES & flags)
        _chars.emplace_back('\"', '\"');
}

bool CXPlayerParser::split(const std::string & str, char delim, std::pair<std::string, std::string> & kv)
{
    if (str.empty())
        return false;

    std::string key, val;
    auto pos = str.find_first_of(delim);
    if (std::string::npos == pos)
    {
        key = str;
        val.clear();
    }
    else
    {
        key = str.substr(0, pos);
        val = str.substr(pos + 1);
    }

    if (key.empty())
        return false;

    if (val.empty())
    {
        kv.first = key;
        kv.second = val;
        return true;
    }

    const auto & l = val.front();
    const auto & r = val.back();
    auto found = std::find_if(_chars.cbegin(), _chars.cend(), [l, r](const std::pair<char, char> & tmp)
    {
        if (tmp.first == l && tmp.second == r)
            return true;
        return false;
    });
    if (_chars.end() != found)
    {
        val = val.substr(1, val.length() - 2);
    }

    kv.first = key;
    kv.second = val;

    return true;
}
