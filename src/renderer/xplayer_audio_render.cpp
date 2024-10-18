#include "xplayer_audio_render.h"

#include "SDL2/SDL_audio.h"

#include "utils/xplayer_utils.h"

struct _audio_info_t
{
    int volume;
    int len;
    int pos;         // 播放位置
    uint8_t * data;
};

static void audio_render_cb(void * udata, uint8_t * data, int len)
{
    auto * audio_info = reinterpret_cast<audio_info_t *>(udata);
    if (nullptr == audio_info || 0 == audio_info->len)
    {
        return;
    }

    SDL_memset(data, 0, len);
    int tmp_len = len > audio_info->len ? audio_info->len : len;

    SDL_MixAudioFormat(data, audio_info->data + audio_info->pos, AUDIO_S16, tmp_len, audio_info->volume);
    audio_info->pos += tmp_len;
    audio_info->len -= tmp_len;
}

bool CXPlayerAudioRender::create(int sample_rate, int channels, int frame_size, int vol)
{
    _cache = static_cast<audio_info_t *>(calloc(1, sizeof(audio_info_t)));
    if (nullptr == _cache)
    {
        xpu_format_string(_err, "No enough memory");
        return false;
    }
    _cache.load()->volume = vol;

    SDL_AudioSpec audio_player = {};
    audio_player.freq = sample_rate;
    audio_player.format = AUDIO_S16SYS;
    audio_player.channels = channels;
    audio_player.silence = 0;
    audio_player.samples = frame_size;
    audio_player.callback = audio_render_cb;
    audio_player.userdata = _cache;
    if (SDL_OpenAudio(&audio_player, nullptr) < 0)
    {
        xpu_format_string(_err, "SDL_OpenAudio failed!");
        free(_cache);
        _cache = nullptr;
        return false;
    }
    SDL_PauseAudio(0);

    return true;
}

void CXPlayerAudioRender::destroy()
{
    SDL_CloseAudio();

    if (nullptr != _cache)
    {
        free(_cache.load());
        _cache = nullptr;
    }
}

void CXPlayerAudioRender::setVolume(int vol)
{
    if (nullptr != _cache)
        return;
    _cache.load()->volume = vol;
}

void CXPlayerAudioRender::mute(bool flag)
{
    SDL_PauseAudio(flag);
}

void CXPlayerAudioRender::renderer(uint8_t * data, int len)
{
    if (nullptr != _cache)
    {
        _cache.load()->data = data;
        _cache.load()->len = len;
        _cache.load()->pos = 0;
    }
}

bool CXPlayerAudioRender::finished()
{
    if (nullptr != _cache)
        return _cache.load()->len <= 0;
    return true;
}
