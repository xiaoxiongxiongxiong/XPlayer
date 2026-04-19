#ifndef __XPLAYER_AUDIO_RENDER_SDL_H__
#define __XPLAYER_AUDIO_RENDER_SDL_H__

#include <cstdbool>
#include <cstdint>
#include <atomic>
#include <string>

class CXPlayerAudioRender
{
public:
    CXPlayerAudioRender() = default;
    ~CXPlayerAudioRender() = default;

    // 创建
    bool create(int sample_rate, int channels, int frame_size, int vol);
    // 销毁
    void destroy();

    // 设置音量
    void setVolume(int vol);

    // 是否静音
    void mute(bool flag);

    // 渲染
    bool renderer(uint8_t * data, int len);

    // 错误信息
    const char * err() const;

private:
    // audio device id
    uint32_t _dev_id = 0u;
    // 音量
    std::atomic_int _volume = { 64 };

    // 错误信息
    std::string _err;
};

#endif
