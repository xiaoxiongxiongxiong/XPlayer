#include "xplayer_config.h"
#include <type_traits>
#include <fstream>
#include <iostream>
#include <sstream>

#include "xplayer_utils.h"

xplayer_color_t::xplayer_color_t(const char * str)
{
    char comma;
    std::istringstream iss(str);
    if (!(iss >> red >> comma >> green >> comma >> blue))
    {
        red = 255;
        green = 0;
        blue = 0;
    }
}

bool CXPlayerConfig::load(const std::string & path)
{
    if (!_path.empty())
        return true;

    _path = path;

    std::ifstream fp(path, std::ios::in | std::ios::binary);
    if (fp.is_open())
    {
        try
        {
            fp >> _ctx;
        }
        catch (...)
        {
            xpu_format_string(_err, "Parse error");
            _ctx = nlohmann::ordered_json::object();
            init_defaults();
            return false;
        }
    }
    else
    {
        xpu_format_string(_err, "File not found, using defaults");
        _ctx = nlohmann::ordered_json::object();
        init_defaults();
        return false;
    }

    return true;
}

void CXPlayerConfig::unload()
{
    if (_path.empty())
        return;

    //init_defaults();

    try
    {
        std::ofstream fp(_path, std::ios::out | std::ios::binary);
        fp << _ctx.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
    }
    catch (const std::exception & e)
    {
        xpu_format_string(_err, "%s", e.what());
    }
}

template <typename Tag>
typename xplayer_config_trait_t<Tag>::type CXPlayerConfig::get() const
{
    using Type = typename xplayer_config_trait_t<Tag>::type;
    auto ptr = nlohmann::ordered_json::json_pointer(xplayer_config_trait_t<Tag>::path);

    if constexpr (std::is_same_v<typename xplayer_config_trait_t<Tag>::type, xplayer_font_color_t>)
    {
        std::string str = _ctx.at(ptr).get<std::string>();
        return xplayer_color_t(str.c_str());
    }

    if (!_ctx.contains(ptr))
        return Type(xplayer_config_trait_t<Tag>::val);
    return _ctx.at(ptr).get<Type>();
}

template <typename Tag>
void CXPlayerConfig::set(typename xplayer_config_trait_t<Tag>::type val)
{
    auto ptr = nlohmann::ordered_json::json_pointer(xplayer_config_trait_t<Tag>::path);

    if constexpr (std::is_same_v<typename xplayer_config_trait_t<Tag>::type, xplayer_font_color_t>)
    {
        std::string str = std::to_string(val.red) + "," +
            std::to_string(val.green) + "," +
            std::to_string(val.blue);
        _ctx[ptr] = str;
    }
    else
        _ctx[ptr] = val;
}


const char * CXPlayerConfig::err() const
{
    return _err.c_str();
}

template <typename Tag>
void CXPlayerConfig::apply_default()
{
    auto ptr = nlohmann::ordered_json::json_pointer(xplayer_config_trait_t<Tag>::path);
    if (_ctx.contains(ptr))
        return;

    using Type = typename xplayer_config_trait_t<Tag>::type;
    if constexpr (std::is_same_v<Type, xplayer_font_color_t>)
        _ctx[ptr] = Type(xplayer_config_trait_t<Tag>::val);
    else
        _ctx[ptr] = xplayer_config_trait_t<Tag>::val;
}

void CXPlayerConfig::init_defaults()
{
    apply_default<xplayer_record_flag_t>();
    apply_default<xplayer_common_speed_t>();
    apply_default<xplayer_common_detail_t>();
    apply_default<xplayer_common_cache_t>();
    apply_default<xplayer_audio_volume_t>();
    apply_default<xplayer_audio_device_t>();
    apply_default<xplayer_video_renderer_t>();
    apply_default<xplayer_video_decoder_t>();
    apply_default<xplayer_font_size_t>();
    apply_default<xplayer_font_path_t>();
    apply_default<xplayer_font_color_t>();
}

template bool CXPlayerConfig::get<xplayer_record_flag_t>() const;
template void CXPlayerConfig::set<xplayer_record_flag_t>(bool);

template float CXPlayerConfig::get<xplayer_common_speed_t>() const;
template void CXPlayerConfig::set<xplayer_common_speed_t>(float);

template bool CXPlayerConfig::get<xplayer_common_detail_t>() const;
template void CXPlayerConfig::set<xplayer_common_detail_t>(bool);

template int CXPlayerConfig::get<xplayer_common_cache_t>() const;
template void CXPlayerConfig::set<xplayer_common_cache_t>(int);

template int CXPlayerConfig::get<xplayer_audio_volume_t>() const;
template void CXPlayerConfig::set<xplayer_audio_volume_t>(int);

template std::string CXPlayerConfig::get<xplayer_audio_device_t>() const;
template void CXPlayerConfig::set<xplayer_audio_device_t>(std::string);

template std::string CXPlayerConfig::get<xplayer_video_renderer_t>() const;
template void CXPlayerConfig::set<xplayer_video_renderer_t>(std::string);

template std::string CXPlayerConfig::get<xplayer_video_decoder_t>() const;
template void CXPlayerConfig::set<xplayer_video_decoder_t>(std::string);

template int CXPlayerConfig::get<xplayer_font_size_t>() const;
template void CXPlayerConfig::set<xplayer_font_size_t>(int);

template std::string CXPlayerConfig::get<xplayer_font_path_t>() const;
template void CXPlayerConfig::set<xplayer_font_path_t>(std::string);

template xplayer_color_t CXPlayerConfig::get<xplayer_font_color_t>() const;
template void CXPlayerConfig::set<xplayer_font_color_t>(xplayer_color_t);

template void CXPlayerConfig::apply_default<xplayer_record_flag_t>();
template void CXPlayerConfig::apply_default<xplayer_common_speed_t>();
template void CXPlayerConfig::apply_default<xplayer_common_detail_t>();
template void CXPlayerConfig::apply_default<xplayer_common_cache_t>();
template void CXPlayerConfig::apply_default<xplayer_audio_volume_t>();
template void CXPlayerConfig::apply_default<xplayer_audio_device_t>();
template void CXPlayerConfig::apply_default<xplayer_video_renderer_t>();
template void CXPlayerConfig::apply_default<xplayer_video_decoder_t>();
template void CXPlayerConfig::apply_default<xplayer_font_size_t>();
template void CXPlayerConfig::apply_default<xplayer_font_path_t>();
template void CXPlayerConfig::apply_default<xplayer_font_color_t>();

