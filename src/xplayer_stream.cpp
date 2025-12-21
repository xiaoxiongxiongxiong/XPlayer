#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "decoder/xplayer_decoder.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "renderer/xplayer_audio_render_sdl.h"

CXPlayerStream::CXPlayerStream(int index) :
    _index(index)
{
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

bool CXPlayerStream::prepare(const void * wnd, int width, int height)
{
    return true;
}

// 获取解码器
const std::shared_ptr<CXPlayerDecoder> & CXPlayerStream::decoder()
{
    return nullptr;
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

bool CXPlayerStream::reopenDecoder()
{
    if (nullptr == _decoder)
        return false;
    return _decoder->reopen();
}

bool CXPlayerStream::createRender(const void * wnd, int width, int height)
{

    return true;
}

void CXPlayerStream::destroyRender()
{
}

void CXPlayerStream::reset()
{
    if (!_reset.load())
        return;

    reopenDecoder();
    _reset.store(false);
}
