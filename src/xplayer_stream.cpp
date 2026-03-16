#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "decoder/xplayer_decoder.h"
#include "rescaler/xplayer_video_rescaler.h"
#include "rescaler/xplayer_audio_resampler.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "renderer/xplayer_audio_render_sdl.h"

CXPlayerStream::CXPlayerStream(int index) :
    _index(index)
{
    _pkt_dts = AV_NOPTS_VALUE;
}

bool CXPlayerStream::init(const AVCodecParameters * codecpar, const AVRational & timebase)
{
    if (nullptr == codecpar)
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    if (AVMEDIA_TYPE_AUDIO == codecpar->codec_type)
    {
        _max_pkts = static_cast<int>(ceil(static_cast<double>(codecpar->sample_rate) / static_cast<double>(codecpar->frame_size)));
    }
    else if (AVMEDIA_TYPE_VIDEO == codecpar->codec_type)
    {
        _max_pkts = static_cast<int>(ceil(av_q2d(codecpar->framerate)));
    }
    else
    {
        xpu_format_string(_err, "Unsupported codec type: %d", codecpar->codec_type);
        return false;
    }


    _codecpar = avcodec_parameters_alloc();
    if (nullptr == _codecpar)
    {
        xpu_format_string(_err, "avcodec_parameters_alloc failed");
        return false;
    }

    int ret = avcodec_parameters_copy(_codecpar, codecpar);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        avcodec_parameters_free(&_codecpar);
        return false;
    }

    _timebase = timebase;

    return true;
}

void CXPlayerStream::uninit()
{
    if (nullptr == _codecpar)
        return;

    avcodec_parameters_free(&_codecpar);
    destroyDecoder();
    destroyConvertor();
    destroyRenderer();
}

bool CXPlayerStream::pushPacket(const AVPacket & pkt)
{
    if (_index != pkt.stream_index)
    {
        xpu_format_string(_err, "Stream index mismatch");
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts && AV_NOPTS_VALUE != _pkt_dts && pkt.dts < _pkt_dts)
    {
        xpu_format_string(_err, "Recv non-increasing timecode");
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts)
        _pkt_dts = pkt.dts;

    _pkts.push(pkt);

    return true;
}

bool CXPlayerStream::popPacket(AVPacket & pkt)
{
    if (_pkts.empty())
        return false;

    return _pkts.pop(pkt);
}

void CXPlayerStream::clearPackets()
{
    _pkts.clear();
    _pkt_dts = AV_NOPTS_VALUE;
}

bool CXPlayerStream::isCacheFull()
{
    return _max_pkts <= _pkts.size();
}

bool CXPlayerStream::setup(const void * wnd, int width, int height)
{
    if (!createDecoder())
        return false;

    if (!createConvertor(width, height))
        goto err;

    if (!createRenderer(wnd, width, height))
        goto err;

    return true;

err:
    destroyDecoder();
    destroyConvertor();
    destroyRenderer();

    return false;
}

int64_t CXPlayerStream::timestamp(int64_t timecode)
{
    if (AV_NOPTS_VALUE == timecode)
        return AV_NOPTS_VALUE;

    return timecode * _timebase.num * 1000 / _timebase.den;
}

int64_t CXPlayerStream::frameDuration()
{
    return static_cast<int64_t>(1000.0 / av_q2d(_codecpar->framerate));
}

const char * CXPlayerStream::err() const
{
    return _err.c_str();
}

bool CXPlayerStream::createDecoder()
{
    _decoder = std::make_shared<CXPlayerDecoder>();
    if (nullptr == _decoder)
    {
        xpu_format_string(_err, "Create decoder failed");
        return false;
    }

    if (!_decoder->create(_codecpar))
    {
        xpu_format_string(_err, "%s", _decoder->err());
        _decoder.reset();
        return false;
    }

    return true;
}

void CXPlayerStream::destroyDecoder()
{
    if (nullptr == _decoder)
        return;

    _decoder->destroy();
    _decoder.reset();
    _decoder = nullptr;
}

bool CXPlayerStream::createConvertor(int width, int height)
{
    if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
    {
        _video_rescaler = std::make_shared<CXPlayerVideoRescaler>();
        if (!_video_rescaler)
        {
            xpu_format_string(_err, "Create CXPlayerVideoRescaler instance failed");
            return false;
        }

        CXPlayerVideoInfo src(static_cast<AVPixelFormat>(_codecpar->format), _codecpar->width, _codecpar->height);
        CXPlayerVideoInfo dst(AV_PIX_FMT_YUV420P, width, height);

        if (!_video_rescaler->create(src, dst))
        {
            _err = _video_rescaler->err();
            _video_rescaler.reset();
            _video_rescaler = nullptr;
            return false;
        }
    }
    else
    {
        _audio_resampler = std::make_shared<CXPlayerAudioResampler>();
        if (!_audio_resampler)
        {
            xpu_format_string(_err, "Create CXPlayerAudioResampler instance failed");
            return false;
        }

        CXPlayerAudioInfo src(_codecpar->ch_layout, static_cast<AVSampleFormat>(_codecpar->format), _codecpar->sample_rate);
        AVChannelLayout dst_layout{};
        av_channel_layout_default(&dst_layout, 2);
        CXPlayerAudioInfo dst(dst_layout, AV_SAMPLE_FMT_S16, _codecpar->sample_rate);
        
        if (!_audio_resampler->create(src, dst, _codecpar->frame_size))
        {
            _err = _audio_resampler->err();
            _audio_resampler.reset();
            _audio_resampler = nullptr;
            return false;
        }
    }

    return true;
}

void CXPlayerStream::destroyConvertor()
{
    if (_audio_resampler)
    {
        _audio_resampler->destroy();
        _audio_resampler.reset();
        _audio_resampler = nullptr;
    }

    if (_video_rescaler)
    {
        _video_rescaler->destroy();
        _video_rescaler.reset();
        _video_rescaler = nullptr;
    }
}

bool CXPlayerStream::createRenderer(const void * wnd, int width, int height)
{
    if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
    {
        _video_renderer = std::make_shared<CXPlayerVideoRenderSDL>();
        if (nullptr == _video_renderer)
        {
            xpu_format_string(_err, "Create CXPlayerVideoRenderSDL instance failed");
            return false;
        }

        if (!_video_renderer->create(wnd, width, height))
        {
            _err = _video_renderer->err();
            _video_renderer.reset();
            _video_renderer = nullptr;
            return false;
        }
    }
    else if (AVMEDIA_TYPE_AUDIO == _codecpar->codec_type)
    {
        _audio_renderer = std::make_shared<CXPlayerAudioRender>();
        if (nullptr == _audio_renderer)
        {
            xpu_format_string(_err, "Create CXPlayerAudioRender instance failed");
            return false;
        }

        if (!_audio_renderer->create(_codecpar->sample_rate, 2, _codecpar->frame_size, 128))
        {
            _err = _audio_renderer->err();
            _audio_renderer.reset();
            _audio_renderer = nullptr;
            return false;
        }
    }

    return true;
}

void CXPlayerStream::destroyRenderer()
{
    if (_video_renderer)
    {
        _video_renderer->destroy();
        _video_renderer.reset();
        _video_renderer = nullptr;
    }

    if (_audio_renderer)
    {
        _audio_renderer->destroy();
        _audio_renderer.reset();
        _audio_renderer = nullptr;
    }
}
