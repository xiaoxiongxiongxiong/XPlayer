#ifndef __XPLAYER_AUDIO_RENDER_H__
#define __XPLAYER_AUDIO_RENDER_H__

#include <cstdbool>
#include <cstdint>
#include <atomic>
#include <string>

typedef struct _audio_info_t audio_info_t;

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
    void renderer(uint8_t * data, int len);

    // 是否完毕
    bool finished();

private:
    std::atomic<audio_info_t *> _cache = { nullptr };

    // 错误信息
    std::string _err;
};

#endif
