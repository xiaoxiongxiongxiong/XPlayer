#include "xplayer_audio_render_sdl.h"
#include <vector>

#include "SDL2/SDL_audio.h"
#include "SDL2/SDL_timer.h"

#include "utils/xplayer_utils.h"

bool CXPlayerAudioRender::create(int sample_rate, int channels, int frame_size, int vol)
{
    SDL_AudioSpec spec = {};
    spec.freq = sample_rate;
    spec.format = AUDIO_S16SYS;
    spec.channels = channels;
    spec.samples = frame_size;
    _dev_id = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (0 == _dev_id)
    {
        xpu_format_string(_err, "Failed to open audio: %s", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(_dev_id, 0);

    return true;
}

void CXPlayerAudioRender::destroy()
{
    if (_dev_id > 0u)
    {
        SDL_CloseAudioDevice(_dev_id);
        _dev_id = 0u;
    }
}

void CXPlayerAudioRender::setVolume(int vol)
{
    _volume.store(vol);
}

void CXPlayerAudioRender::mute(bool flag)
{
    if (_dev_id > 0u)
    {
        SDL_PauseAudioDevice(_dev_id, flag);
    }
}

bool CXPlayerAudioRender::renderer(uint8_t * data, int len)
{
    std::vector<uint8_t> cache(len, 0);
    SDL_MixAudioFormat(cache.data(), data, AUDIO_S16SYS, len, _volume.load());

    if (-1 == SDL_QueueAudio(_dev_id, cache.data(), static_cast<Uint32>(len)))
    {
        xpu_format_string(_err, "Failed to queue audio: %s", SDL_GetError());
        return false;
    }

    auto bytes = SDL_GetQueuedAudioSize(_dev_id);
    while (0u != bytes)
    {
        SDL_Delay(1);
        bytes = SDL_GetQueuedAudioSize(_dev_id);
    }

    return true;
}

const char * CXPlayerAudioRender::err() const
{
    return _err.c_str();
}
