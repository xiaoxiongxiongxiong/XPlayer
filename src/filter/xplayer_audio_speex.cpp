#include "xplayer_audio_speex.h"

#include <vector>
#include "soundtouch/SoundTouch.h"
#include "xplayer_utils.h"

bool CXPlayerAudioSpeex::create(int channels, int sample_rate, int samples)
{
    _ctx = std::make_shared<soundtouch::SoundTouch>();
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Create SoundTouch instance failed");
        return false;
    }

    _ctx->setSampleRate(sample_rate);
    _ctx->setChannels(channels);
    _ctx->setPitch(1.0);

    _channels = channels;
    _sample_rate = sample_rate;
    _samples = samples;

    return true;
}

void CXPlayerAudioSpeex::destroy()
{
    if (nullptr != _ctx)
    {
        _ctx.reset();
        _ctx = nullptr;
    }
}

void CXPlayerAudioSpeex::update(int channels, int sample_rate, int samples)
{
    if (nullptr == _ctx)
        return;

    _ctx->setSampleRate(sample_rate);
    _ctx->setChannels(channels);

    _channels = channels;
    _sample_rate = sample_rate;
    _samples = samples;
}

void CXPlayerAudioSpeex::setSpeed(double speed)
{
    if (nullptr != _ctx)
    {
        _ctx->setTempo(speed);
    }
}

bool CXPlayerAudioSpeex::send(const uint8_t * data, int len)
{
    const auto samples = len / sizeof(int16_t);
    std::vector<float> buff(samples);
    s162flt(data, len, buff.data());

    _ctx->putSamples(buff.data(), samples / _channels);

    return true;
}

int CXPlayerAudioSpeex::recv(uint8_t * data, int len)
{
    const auto total_samples = len / static_cast<int>(sizeof(int16_t));
    std::vector<float> buff(total_samples);
    uint samples = _ctx->receiveSamples(buff.data(), total_samples / _channels);
    if (samples < 1u)
        return 0;

    len = samples * _channels * static_cast<int>(sizeof(int16_t));
    flt2s16(buff.data(), samples * _channels, data);

    return len;
}

void CXPlayerAudioSpeex::flush()
{
    if (nullptr != _ctx)
    {
        _ctx->flush();
    }
}

void CXPlayerAudioSpeex::clear()
{
    if (nullptr == _ctx)
        return;

    _ctx->flush();

    uint samples = 0u;
    do 
    {
        samples = _ctx->receiveSamples(_samples);
    } while (samples > 0u);
}

const char * CXPlayerAudioSpeex::err() const
{
    return _err.c_str();
}
