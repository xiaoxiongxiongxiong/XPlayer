#include "xplayer_config.h"
#include <type_traits>
#include "jansson.h"
#include "xplayer_utils.h"

template <typename T> 
static bool xplayer_parse_object(json_t * jso, const char * key, T & t, std::string & err)
{
    json_t * obj = json_object_get(jso, key);
    if (nullptr == obj)
    {
        xpu_format_string(err, "Lack key word '%s'", key);
        return false;
    }

    if constexpr (std::is_same_v<T, std::string>)
    {
        auto * val = json_string_value(obj);
        t = val ? val : std::string{};
    }
    else if constexpr (std::is_integral_v<T>)
    {
        auto val = json_integer_value(obj);
        t = static_cast<T>(val);
    }

    return true;
}

template <typename T>
static bool xplayer_object_set(json_t * jso, const char * key, const T & t, std::string & err)
{
    json_t * obj = nullptr;
    if constexpr (std::is_same_v<T, std::string>)
    {
        obj = json_string(t.c_str());
    }
    else if constexpr (std::is_integral_v<T>)
    {
        obj = json_integer(static_cast<json_int_t>(t));
    }

    if (nullptr == obj)
    {
        xpu_format_string(err, "build object for '%s' failed", key);
        return false;
    }

    json_object_set_new(jso, key, obj);

    return true;
}

bool CXPlayerConfig::loadConfig(const std::string & path)
{
    json_error_t jso_err = {};
    _path = path;
    json_t * jso_root = json_load_file(path.c_str(), 0, &jso_err);
    if (nullptr == jso_root)
    {
        xpu_format_string(_err, "json_loads %s failed, pos: %d, source: %s",
                          path.c_str(), jso_err.position, jso_err.source);
        return false;
    }

    xplayer_parse_object(jso_root, "volume", _vol, _err);
    xplayer_parse_object(jso_root, "record_flag", _record_flag, _err);
    xplayer_parse_object(jso_root, "font_path", _font_path, _err);
    xplayer_parse_object(jso_root, "font_size", _font_size, _err);

    json_decref(jso_root);

    return true;
}

void CXPlayerConfig::unloadConfig()
{
    if (_path.empty())
        return;

    auto * jso_root = json_object();
    if (nullptr == jso_root)
    {
        xpu_format_string(_err, "json_object failed");
        return;
    }

    xplayer_object_set(jso_root, "volume", _vol, _err);
    xplayer_object_set(jso_root, "record_flag", _record_flag, _err);
    xplayer_object_set(jso_root, "font_path", _font_path, _err);
    xplayer_object_set(jso_root, "font_size", _font_size, _err);

    json_dump_file(jso_root, _path.c_str(), JSON_INDENT(4) | JSON_ENSURE_ASCII);
    json_decref(jso_root);
}

void CXPlayerConfig::setVolume(int vol)
{
    _vol = vol;
}

int CXPlayerConfig::getVolume()
{
    return _vol;
}

void CXPlayerConfig::setRecordVisible(bool flag)
{
    _record_flag = flag;
}

bool CXPlayerConfig::getRecordVisible()
{
    return _record_flag;
}

void CXPlayerConfig::setFontPath(const std::string & path)
{
    _font_path = path;
}

const std::string & CXPlayerConfig::getFontPath()
{
    return _font_path;
}

void CXPlayerConfig::setFontSize(int size)
{
    _font_size = size;
}

int CXPlayerConfig::getFontSize()
{
    return _font_size;
}

const char * CXPlayerConfig::err() const
{
    return _err.c_str();
}

