#include "xplayer_audio_speex.h"

#include <vector>
#include "soundtouch/SoundTouch.h"
#include "xplayer_utils.h"

bool CXPlayerAudioSpeex::create(int channels, int sample_rate, int samples)
{
    _ctx = std::make_unique<soundtouch::SoundTouch>();
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

    _ctx->putSamples(buff.data(), samples);

    return true;
}

bool CXPlayerAudioSpeex::recv(uint8_t * data, int & len)
{
    auto totol_samples = _samples * _channels;
    if (len / sizeof(int16_t) < totol_samples)
        totol_samples = len / sizeof(int16_t);

    std::vector<float> buff(totol_samples);
    uint samples = st.receiveSamples(buff.data(), totol_samples);
    if (samples <= 0u)
    {
        len = 0;
        return true;
    }

    len = samples * _channels * static_cast<int>(sizeof(int16_t));
    flt2s16(buff.data(), samples * _channels, data);

    return true;
}

void CXPlayerAudioSpeex::flush()
{
    if (nullptr != _ctx)
    {
        _ctx->flush();
    }
}

const char * CXPlayerAudioSpeex::err() const
{
    return _err.c_str();
}
