#ifndef __XPLAYER_CONFIG_H__
#define __XPLAYER_CONFIG_H__

#include <cstdbool>
#include <string>
#include <memory>

#include "xplayer_definitions.h"

struct xplayer_config_t;

struct xplayer_record_flag_t {};

struct xplayer_common_detail_t {};

// 播放倍速
struct xplayer_speed_realtime_t {}; // 实时倍速
struct xplayer_speed_custom_t {};   // 自定义倍速
struct xplayer_speed_id_t {};

struct xplayer_cache_duration_t {};
struct xplayer_cache_frame_t {};

struct xplayer_audio_volume_t {};
struct xplayer_audio_device_t {};

struct xplayer_video_renderer_t {};
struct xplayer_video_decoder_t {};

struct xplayer_font_size_t {};
struct xplayer_font_path_t {};
struct xplayer_font_color_t {};

template <typename Tag>
struct xplayer_config_trait_t;

#include "xplayer_config.inl"

class CXPlayerConfig final
{
public:
    CXPlayerConfig(const CXPlayerConfig &) = delete;
    CXPlayerConfig & operator=(const CXPlayerConfig &) = delete;

    CXPlayerConfig(CXPlayerConfig &&) = delete;
    CXPlayerConfig & operator= (const CXPlayerConfig &&) = delete;

    static CXPlayerConfig & uniqueInstance()
    {
        static CXPlayerConfig instance;
        return instance;
    }

    // 加载配置文件
    bool load(const std::string & path);
    // 卸载配置文件
    void unload();

    // 获取值
    template <typename Tag>
    typename xplayer_config_trait_t<Tag>::type get() const;

    // 设置值
    template <typename Tag>
    void set(typename xplayer_config_trait_t<Tag>::type val);

    // 错误信息
    const char * err() const;

private:
    template <typename Tag>
    void apply_default(); // 只有声明，没有实现

    void init_defaults(); // 普通成员函数，只有声明

private:
    CXPlayerConfig();
    ~CXPlayerConfig();

    std::unique_ptr<xplayer_config_t> _ctx = nullptr;

    // 配置文件路径
    std::string _path;

    // 错误信息
    std::string _err;
};

#endif
