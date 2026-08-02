#pragma once

#define XPLAYER_CONFIG_TRAIT(Tag, Type, Path, Val) \
         template<>                                \
         struct xplayer_config_trait_t<Tag> {      \
            using type = Type;                     \
            static constexpr const char* path = Path; \
            static constexpr auto val = Val;       \
         };

XPLAYER_CONFIG_TRAIT(xplayer_record_flag_t, bool, "/record_flag", false)
XPLAYER_CONFIG_TRAIT(xplayer_common_speed_t, float, "/common/speed", 1.0f)
XPLAYER_CONFIG_TRAIT(xplayer_common_detail_t, bool, "/common/detail", false)
XPLAYER_CONFIG_TRAIT(xplayer_common_cache_t, int, "/common/cache", 1)
XPLAYER_CONFIG_TRAIT(xplayer_audio_volume_t, int, "/audio/volume", 64)
XPLAYER_CONFIG_TRAIT(xplayer_audio_device_t, std::string, "/audio/device", "default")
XPLAYER_CONFIG_TRAIT(xplayer_video_renderer_t, std::string, "/video/renderer", "opengl")
XPLAYER_CONFIG_TRAIT(xplayer_video_decoder_t, std::string, "/video/decoder", "software")
XPLAYER_CONFIG_TRAIT(xplayer_font_size_t, int, "/font/size", 24)
XPLAYER_CONFIG_TRAIT(xplayer_font_path_t, std::string, "/font/path", "")
XPLAYER_CONFIG_TRAIT(xplayer_font_color_t, xplayer_color_t, "/font/color", "255,0,255")