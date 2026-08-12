#include "xplayer_config.h"
#include <type_traits>
#include <fstream>

#include "nlohmann/json.hpp"
#include "xplayer_utils.h"

struct xplayer_config_t
{
    nlohmann::ordered_json ctx;
};

static inline void to_json(nlohmann::ordered_json & body, const xplayer_color_t & c)
{
    body = std::to_string(c.red) + "," + std::to_string(c.green) + "," + std::to_string(c.blue) + "," + std::to_string(c.alpha);
}

static inline void from_json(const nlohmann::ordered_json & body, xplayer_color_t & c)
{
    auto str = body.get<std::string>();
    c = xplayer_color_t(str.c_str());
}

CXPlayerConfig::CXPlayerConfig() = default;
CXPlayerConfig::~CXPlayerConfig() = default;

bool CXPlayerConfig::load(const std::string & path)
{
    if (!_path.empty())
        return true;

    if (nullptr == _ctx)
    {
        _ctx = std::make_unique<xplayer_config_t>();
        if (nullptr == _ctx)
        {
            xpu_format_string(_err, "make_unique failed");
            return false;
        }
    }

    _path = path;

    std::ifstream fp(path, std::ios::in | std::ios::binary);
    if (fp.is_open())
    {
        try
        {
            fp >> _ctx->ctx;
        }
        catch (...)
        {
            xpu_format_string(_err, "Parse error");
            _ctx->ctx = nlohmann::ordered_json::object();
            init_defaults();
            return false;
        }
    }
    else
    {
        xpu_format_string(_err, "File not found, using defaults");
        _ctx->ctx = nlohmann::ordered_json::object();
        init_defaults();
        return false;
    }

    return true;
}

void CXPlayerConfig::unload()
{
    if (_path.empty())
        return;

    if (nullptr == _ctx)
    {
        _ctx = std::make_unique<xplayer_config_t>();
        if (nullptr == _ctx)
        {
            xpu_format_string(_err, "make_unique failed");
            return;
        }
    }

    try
    {
        std::ofstream fp(_path, std::ios::out | std::ios::binary);
        fp << _ctx->ctx.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::replace);
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
        std::string str = _ctx->ctx.at(ptr).get<std::string>();
        return xplayer_color_t(str.c_str());
    }

    if (!_ctx->ctx.contains(ptr))
        return Type(xplayer_config_trait_t<Tag>::val);
    return _ctx->ctx.at(ptr).get<Type>();
}

template <typename Tag>
void CXPlayerConfig::set(typename xplayer_config_trait_t<Tag>::type val)
{
    auto ptr = nlohmann::ordered_json::json_pointer(xplayer_config_trait_t<Tag>::path);

    if constexpr (std::is_same_v<typename xplayer_config_trait_t<Tag>::type, xplayer_font_color_t>)
    {
        std::string str = std::to_string(val.red) + "," +
            std::to_string(val.green) + "," +
            std::to_string(val.blue) + "," + 
            std::to_string(val.alpha);
        _ctx->ctx[ptr] = str;
    }
    else
        _ctx->ctx[ptr] = val;
}


const char * CXPlayerConfig::err() const
{
    return _err.c_str();
}

template <typename Tag>
void CXPlayerConfig::apply_default()
{
    auto ptr = nlohmann::ordered_json::json_pointer(xplayer_config_trait_t<Tag>::path);
    if (_ctx->ctx.contains(ptr))
        return;

    using Type = typename xplayer_config_trait_t<Tag>::type;
    if constexpr (std::is_same_v<Type, xplayer_font_color_t>)
        _ctx->ctx[ptr] = Type(xplayer_config_trait_t<Tag>::val);
    else
        _ctx->ctx[ptr] = xplayer_config_trait_t<Tag>::val;
}

void CXPlayerConfig::init_defaults()
{
    apply_default<xplayer_record_flag_t>();
    apply_default<xplayer_common_detail_t>();
    apply_default<xplayer_common_cache_t>();
    apply_default<xplayer_speed_realtime_t>();
    apply_default<xplayer_speed_custom_t>();
    apply_default<xplayer_speed_id_t>();
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

template float CXPlayerConfig::get<xplayer_speed_realtime_t>() const;
template void CXPlayerConfig::set<xplayer_speed_realtime_t>(float);

template float CXPlayerConfig::get<xplayer_speed_custom_t>() const;
template void CXPlayerConfig::set<xplayer_speed_custom_t>(float);

template int CXPlayerConfig::get<xplayer_speed_id_t>() const;
template void CXPlayerConfig::set<xplayer_speed_id_t>(int);

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
template void CXPlayerConfig::apply_default<xplayer_common_detail_t>();
template void CXPlayerConfig::apply_default<xplayer_common_cache_t>();
template void CXPlayerConfig::apply_default<xplayer_speed_realtime_t>();
template void CXPlayerConfig::apply_default<xplayer_speed_custom_t>();
template void CXPlayerConfig::apply_default<xplayer_speed_id_t>();
template void CXPlayerConfig::apply_default<xplayer_audio_volume_t>();
template void CXPlayerConfig::apply_default<xplayer_audio_device_t>();
template void CXPlayerConfig::apply_default<xplayer_video_renderer_t>();
template void CXPlayerConfig::apply_default<xplayer_video_decoder_t>();
template void CXPlayerConfig::apply_default<xplayer_font_size_t>();
template void CXPlayerConfig::apply_default<xplayer_font_path_t>();
template void CXPlayerConfig::apply_default<xplayer_font_color_t>();

