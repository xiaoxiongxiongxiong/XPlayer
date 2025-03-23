#include "xplayer_decoder.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}
#include "utils/xplayer_utils.h"

bool CXPlayerDecoder::create(const AVCodecParameters * codec_par, const bool flag)
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

    if (!flag)
        return true;

    _codec_par = avcodec_parameters_alloc();
    if (nullptr == _codec_par)
    {
        xpu_format_string(_err, "avcodec_parameters_alloc failed.");
        return true;
    }

    ret = avcodec_parameters_copy(_codec_par, codec_par);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "avcodec_parameters_copy failed, err: %s", buff);
        _codec_par = nullptr;
    }

    return true;
}

void CXPlayerDecoder::destroy(const bool flag)
{
    if (nullptr == _ctx)
        return;

    avcodec_close(_ctx);
    avcodec_free_context(&_ctx);

    if (nullptr != _codec_par && flag)
    {
        avcodec_parameters_free(&_codec_par);
        _codec_par = nullptr;
    }
}

bool CXPlayerDecoder::reopen()
{
    destroy(true);
    return create(_codec_par, false);
}

bool CXPlayerDecoder::send(const AVPacket * pkt)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Decoder not open yet");
        return false;
    }

    int ret = avcodec_send_packet(_ctx, pkt);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        return false;
    }

    return true;
}

bool CXPlayerDecoder::recv(AVFrame & frm, bool & got, bool & over)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Decoder not open yet");
        return false;
    }

    int ret = avcodec_receive_frame(_ctx, &frm);
    if (0 != ret)
    {
        if (AVERROR(EAGAIN) == ret)
        {
            got = false;
            return true;
        }
        if (AVERROR_EOF == ret)
        {
            over = true;
            got = false;
            return true;
        }
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        return false;
    }

    return true;
}

const char * CXPlayerDecoder::err() const
{
    return _err.c_str();
}
