#include "xplayer_audio_volume.h"

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
}

#include "xplayer_utils.h"

bool CXPlayerAudioVolume::create(int sample_rate, int format, uint64_t channel_layout, float val, const bool & flag)
{
    _graph = avfilter_graph_alloc();
    if (nullptr == _graph)
    {
        xpu_format_string(_err, "avfilter_graph_alloc failed");
        return false;
    }

    const auto * abuffer = avfilter_get_by_name("abuffer");
    if (nullptr == abuffer)
    {
        xpu_format_string(_err, "avfilter_get_by_name 'abuffer' failed");
        return false;
    }

    char args[256] = {};
    snprintf(args, sizeof(args),
             "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx",
             sample_rate, sample_rate, av_get_sample_fmt_name(static_cast<AVSampleFormat>(format)), channel_layout);
    int ret = avfilter_graph_create_filter(&_ctx, abuffer, "in", args, nullptr, _graph);
    if (ret < 0)
        goto fail;

    const auto * abuffer_sink = avfilter_get_by_name("abuffersink");
    ret = avfilter_graph_create_filter(&_sink_ctx, abuffer_sink, "out", nullptr, nullptr, _graph);
    if (ret < 0)
        goto fail;

    const auto * filter = avfilter_get_by_name("volume");
    char vol[16] = {};
    if (flag)
        snprintf(vol, sizeof(vol), "volume=%.2f", val);
    else
        snprintf(vol, sizeof(vol), "volume=%.2fdB", val);

    AVFilterContext * vol_ctx = nullptr;
    ret = avfilter_graph_create_filter(&vol_ctx, filter, "vol", vol, nullptr, _graph);
    if (ret < 0)
        goto fail;

    ret = avfilter_link(_ctx, 0, vol_ctx, 0);
    if (ret < 0)
        goto fail;

    ret = avfilter_link(vol_ctx, 0, _sink_ctx, 0);
    if (ret < 0)
        goto fail;

    ret = avfilter_graph_config(_graph, nullptr);
    if (ret < 0)
        goto fail;

    return true;

fail:
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
    }

    if (nullptr != _graph)
    {
        avfilter_graph_free(&_graph);
    }

    return false;
}

void CXPlayerAudioVolume::destroy()
{
    if (nullptr != _graph)
    {
        avfilter_graph_free(&_graph);
    }
}

bool CXPlayerAudioVolume::send(AVFrame & frm)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Not init yet");
        return false;
    }

    int ret = av_buffersrc_add_frame_flags(_ctx, &frm, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        return false;
    }

    return true;
}

bool CXPlayerAudioVolume::recv(AVFrame & frm, bool & got, bool & over)
{
    if (nullptr == _sink_ctx)
    {
        xpu_format_string(_err, "Not init yet");
        return false;
    }

    int ret = av_buffersink_get_frame(_sink_ctx, &frm);
    if (ret < 0)
    {
        if (AVERROR(EAGAIN) == ret)
        {
            got = false;
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

const char * CXPlayerAudioVolume::err() const
{
    return _err.c_str();
}
