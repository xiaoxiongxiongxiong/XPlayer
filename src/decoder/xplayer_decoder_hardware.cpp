#include "xplayer_decoder_hardware.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/pixdesc.h"
#include "libavutil/hwcontext.h"
}

#include "xplayer_utils.h"

static enum AVPixelFormat get_hw_format(AVCodecContext * ctx,
                                        const enum AVPixelFormat * pix_fmts)
{
    const enum AVPixelFormat * p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_D3D11)
            return *p;
    }

    return AV_PIX_FMT_NONE;
}

bool CXPlayerDecoderHardware::create(const AVCodecParameters * codec_par)
{
    auto type = av_hwdevice_find_type_by_name("d3d11va");
    if (AV_HWDEVICE_TYPE_NONE == type)
    {
        xpu_format_string(_err, "av_hwdevice_find_type_by_name 'd3d11va' failed");
        return false;
    }

    enum AVPixelFormat pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;

    auto * codec = avcodec_find_decoder(codec_par->codec_id);
    for (int i = 0; ; i++)
    {
        const AVCodecHWConfig * config = avcodec_get_hw_config(codec, i);
        if (nullptr == config)
        {
            xpu_format_string(_err, "avcodec_get_hw_config failed");
            return false;
        }

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type)
        {
            pix_fmt = config->pix_fmt;
            break;
        }
    }

    _ctx = avcodec_alloc_context3(codec);
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "avcodec_alloc_context3 failed");
        return false;
    }

    int ret = avcodec_parameters_to_context(_ctx, codec_par);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        avcodec_free_context(&_ctx);
        return false;
    }

    _ctx->get_format = get_hw_format;

    ret = av_hwdevice_ctx_create(&_hw_ctx, type, nullptr, nullptr, 0);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        avcodec_free_context(&_ctx);
        return false;
    }
    _ctx->hw_device_ctx = av_buffer_ref(_hw_ctx);

    ret = avcodec_open2(_ctx, codec, nullptr);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        avcodec_free_context(&_ctx);
        return false;
    }

    return true;
}

void CXPlayerDecoderHardware::destroy()
{
    ICXPlayerDecoder::destroy();
    av_buffer_unref(&_hw_ctx);
}

bool CXPlayerDecoderHardware::clear()
{
    return ICXPlayerDecoder::clear();
}

bool CXPlayerDecoderHardware::send(const AVPacket * pkt)
{
    return ICXPlayerDecoder::send(pkt);
}

bool CXPlayerDecoderHardware::recv(AVFrame & frm, bool & got, bool & over)
{
    AVFrame tmp = {};
    bool ret = ICXPlayerDecoder::recv(tmp, got, over);
    if (!got)
        return ret;

    if (AV_PIX_FMT_D3D11 == tmp.format)
    {
        ret = av_hwframe_transfer_data(&frm, &tmp, 0);
        frm.pts = tmp.pts;
        frm.pkt_dts = tmp.pkt_dts;
        frm.pkt_duration = tmp.pkt_duration;
        av_frame_unref(&tmp);
        if (0 != ret)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = {};
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            xpu_format_string(_err, "%s", buff);
            got = false;
            return false;
        }
    }

    return true;
}

XPLAYER_DECODER_TYPE CXPlayerDecoderHardware::getType() const
{
    return XPLAYER_DECODER_HARDWARE;
}

const char * CXPlayerDecoderHardware::err() const
{
    return _err.c_str();
}
