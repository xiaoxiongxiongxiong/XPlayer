#include "xplayer_filter_bsf.h"
#include "xplayer_utils.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/bsf.h"
}

int CXPlayerFilterBsf::create(AVCodecParameters * par)
{
    if (nullptr == par)
    {
        xpu_format_string(m_strError, "Input param is nullptr");
        return -1;
    }

    const char * bsf_name = nullptr;
    if (AV_CODEC_ID_H264 == par->codec_id)
        bsf_name = "h264_mp4toannexb";
    else if (AV_CODEC_ID_HEVC == par->codec_id)
        bsf_name = "hevc_mp4toannexb";
    else
    {
        xpu_format_string(m_strError, "Unsupported codec '%s'", avcodec_get_name(par->codec_id));
        return 0;
    }

    auto * filter = av_bsf_get_by_name(bsf_name);
    if (nullptr == filter)
    {
        xpu_format_string(m_strError, "av_bsf_get_by_name '%s' failed", bsf_name);
        return -1;
    }

    int ret = av_bsf_alloc(filter, &m_ptrCtx);
    if (0 != ret)
    {
        goto err;
    }

    ret = avcodec_parameters_copy(m_ptrCtx->par_in, par);
    if (ret < 0)
    {
        av_bsf_free(&m_ptrCtx);
        goto err;
    }

    ret = av_bsf_init(m_ptrCtx);
    if (ret < 0)
    {
        av_bsf_free(&m_ptrCtx);
        goto err;
    }

    ret = avcodec_parameters_copy(par, m_ptrCtx->par_out);
    if (ret < 0)
    {
        av_bsf_free(&m_ptrCtx);
        goto err;
    }

    return 1;

err:
    char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
    xpu_format_string(m_strError, "%s", buff);
    return -1;
}

void CXPlayerFilterBsf::destroy()
{
    if (nullptr != m_ptrCtx)
    {
        av_bsf_free(&m_ptrCtx);
    }
}

bool CXPlayerFilterBsf::send(AVPacket & pkt)
{
    if (nullptr == m_ptrCtx)
    {
        xpu_format_string(m_strError, "Not initialized yet");
        return false;
    }

    int ret = av_bsf_send_packet(m_ptrCtx, &pkt);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(m_strError, "%s", buff);
        return false;
    }

    return true;
}

bool CXPlayerFilterBsf::recv(AVPacket & pkt, bool & got)
{
    got = false;

    if (nullptr == m_ptrCtx)
    {
        xpu_format_string(m_strError, "Not initialized yet");
        return false;
    }

    int ret = av_bsf_receive_packet(m_ptrCtx, &pkt);
    if (0 != ret)
    {
        if (AVERROR(EAGAIN) == ret || AVERROR_EOF == ret)
            return true;

        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(m_strError, "%s", buff);
        return false;
    }

    got = true;

    return true;
}

const char * CXPlayerFilterBsf::err() const
{
    return m_strError.c_str();
}
