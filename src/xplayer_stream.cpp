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

bool CXPlayerStream::create(const AVCodecParameters * codecpar)
{
    if (nullptr == codecpar)
    {
        xpu_format_string(_err, "Invalid params");
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

    if (!createDecoder())
    {
        avcodec_parameters_free(&_codecpar);
        return false;
    }

    return true;
}

void CXPlayerStream::destroy()
{
    if (nullptr == _codecpar)
        return;

    _running.store(false);

    avcodec_parameters_free(&_codecpar);
    destroyDecoder();
}

bool CXPlayerStream::pushPacket(const AVPacket & pkt)
{
    if (_index != pkt.stream_index)
        return false;

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

bool CXPlayerStream::prepare(const void * wnd, int width, int height)
{
    if (!createDecoder())
        return false;

    if (!createConvertor())
    {
        destroyDecoder();
        return false;
    }

    if (!createRenderer(wnd, width, height))
    {
        destroyDecoder();
        destroyConvertor();
        return false;
    }

    return true;
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
}

bool CXPlayerStream::createConvertor()
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
        CXPlayerVideoInfo dst(AV_PIX_FMT_YUV420P, _codecpar->width, _codecpar->height);

        if (!_video_rescaler->create(src, src))
        {
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
        
        if (!_audio_resampler->create(src, src, _codecpar->frame_size))
        {
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
}

bool CXPlayerStream::createRenderer(const void * wnd, int width, int height)
{

    return true;
}

void CXPlayerStream::destroyRenderer()
{
}
