#include "xplayer_decoder.h"

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "xplayer_decoder_software.h"
#include "xplayer_decoder_hardware.h"

void ICXPlayerDecoder::destroy()
{
    if (nullptr == _ctx)
        return;

    avcodec_close(_ctx);
    avcodec_free_context(&_ctx);
}

bool ICXPlayerDecoder::clear()
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Decoder not open yet");
        return false;
    }

    avcodec_flush_buffers(_ctx);

    return true;
}

bool ICXPlayerDecoder::send(const AVPacket * pkt)
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

bool ICXPlayerDecoder::recv(AVFrame & frm, bool & got, bool & over)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Decoder not open yet");
        return false;
    }

    int ret = avcodec_receive_frame(_ctx, &frm);
    if (0 != ret)
    {
        got = false;
        if (AVERROR(EAGAIN) == ret)
        {
            return true;
        }
        if (AVERROR_EOF == ret)
        {
            over = true;
            return true;
        }
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        return false;
    }

    got = true;

    return true;
}

ICXPlayerDecoder * CXPlayerDecoderFactory::create(XPLAYER_DECODER_TYPE type, const AVCodecParameters * codec_par)
{
    ICXPlayerDecoder * decoder = nullptr;
    if (XPLAYER_DECODER_SOFTWARE == type)
    {
        auto * ctx = new (std::nothrow) CXPlayerDecoderSoftware;
        if (nullptr == ctx)
            return nullptr;

        if (!ctx->create(codec_par))
        {
            delete ctx;
            return nullptr;
        }

        decoder = dynamic_cast<ICXPlayerDecoder *>(ctx);
    }
    else if (XPLAYER_DECODER_HARDWARE == type)
    {
        auto * ctx = new (std::nothrow) CXPlayerDecoderHardware;
        if (nullptr == ctx)
            return nullptr;

        if (!ctx->create(codec_par))
        {
            delete ctx;
            return nullptr;
        }

        decoder = dynamic_cast<ICXPlayerDecoder *>(ctx);
    }
    else
        return nullptr;

    return decoder;
}

void CXPlayerDecoderFactory::destroy(ICXPlayerDecoder * decoder)
{
    if (nullptr == decoder)
        return;

    auto type = decoder->getType();
    if (XPLAYER_DECODER_SOFTWARE == type)
    {
        auto * ctx = dynamic_cast<CXPlayerDecoderSoftware *>(decoder);
        ctx->destroy();
        delete ctx;
    }
    else if (XPLAYER_DECODER_HARDWARE == type)
    {
        auto * ctx = dynamic_cast<CXPlayerDecoderHardware *>(decoder);
        ctx->destroy();
        delete ctx;
    }
}
