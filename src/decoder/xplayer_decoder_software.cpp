#include "xplayer_decoder_software.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}
#include "utils/xplayer_utils.h"

bool CXPlayerDecoderSoftware::create(const AVCodecParameters * codec_par)
{
    if (nullptr == codec_par)
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    auto * codec = avcodec_find_decoder(codec_par->codec_id);
    if (nullptr == codec)
    {
        xpu_format_string(_err, "Find decoder by id '%d' failed", codec_par->codec_id);
        return false;
    }

    _ctx = avcodec_alloc_context3(codec);
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "avcodec_alloc_context3 failed for decoder %d", codec_par->codec_id);
        return false;
    }

    int ret = avcodec_parameters_to_context(_ctx, codec_par);
    if (ret < 0)
    {
        xpu_format_string(_err, "Copy parameters to decoder '%d' failed", codec_par->codec_id);
        avcodec_free_context(&_ctx);
        return false;
    }

    ret = avcodec_open2(_ctx, codec, nullptr);
    if (0 != ret)
    {
        xpu_format_string(_err, "Open decoder '%d' failed", codec_par->codec_id);
        avcodec_free_context(&_ctx);
        return false;
    }

    return true;
}

void CXPlayerDecoderSoftware::destroy()
{
    ICXPlayerDecoder::destroy();
}

bool CXPlayerDecoderSoftware::clear()
{
    return ICXPlayerDecoder::clear();
}

bool CXPlayerDecoderSoftware::send(const AVPacket * pkt)
{
    return ICXPlayerDecoder::send(pkt);
}

bool CXPlayerDecoderSoftware::recv(AVFrame & frm, bool & got, bool & over)
{
    return ICXPlayerDecoder::recv(frm, got, over);
}

XPLAYER_DECODER_TYPE CXPlayerDecoderSoftware::getType() const
{
    return XPLAYER_DECODER_SOFTWARE;
}

const char * CXPlayerDecoderSoftware::err() const
{
    return _err.c_str();
}
