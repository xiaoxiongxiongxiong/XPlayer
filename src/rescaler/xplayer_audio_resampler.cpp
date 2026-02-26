#include "xplayer_audio_resampler.h"
#include "utils/xplayer_utils.h"

CXPlayerAudioInfo::CXPlayerAudioInfo(AVChannelLayout layout, enum AVSampleFormat fmt, int sample_rate):
    _layout(layout),
    _fmt(fmt),
    _sample_rate(sample_rate)
{
}

bool CXPlayerAudioInfo::operator==(const CXPlayerAudioInfo & other) const
{
    if (0 != av_channel_layout_compare(&_layout, &other._layout))
        return false;

    if (_fmt != other._fmt)
        return false;

    if (_sample_rate != other._sample_rate)
        return false;

    return true;
}


bool CXPlayerAudioResampler::create(const CXPlayerAudioInfo & src, const CXPlayerAudioInfo & dst, int frame_size)
{
    if (src._fmt <= AV_SAMPLE_FMT_NONE || src._fmt >= AV_SAMPLE_FMT_NB)
    {
        xpu_format_string(_err, "Invalid input format: %d!", src._fmt);
        return false;
    }

    if (dst._fmt <= AV_SAMPLE_FMT_NONE || dst._fmt >= AV_SAMPLE_FMT_NB)
    {
        xpu_format_string(_err, "Invalid output format: %d!", dst._fmt);
        return false;
    }

    if (dst._layout.nb_channels <= 0)
    {
        xpu_format_string(_err, "Output channels %d is invalid.", dst._layout.nb_channels);
        return false;
    }

    _in._fmt = src._fmt;
    _out._fmt = dst._fmt;

    if (0 == av_channel_layout_compare(&dst._layout, &src._layout) &&
        dst._fmt == src._fmt && dst._sample_rate == src._sample_rate)
    {
        _need_rescale = false;
        return true;
    }

    int ret = swr_alloc_set_opts2(&_swr_ctx, &dst._layout, _out._fmt, dst._sample_rate,
                                  &src._layout, _in._fmt, src._sample_rate, 0, nullptr);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "swr_init failed, err:%s", buff);
        return false;
    }

    ret = swr_init(_swr_ctx);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "swr_init failed, err:%s", buff);
        swr_free(&_swr_ctx);
        _swr_ctx = nullptr;
        return false;
    }

    _in = src;
    _out = dst;
    _in_nb_samples = frame_size;

    return true;
}

void CXPlayerAudioResampler::destroy()
{
    if (nullptr != _swr_ctx)
    {
        swr_close(_swr_ctx);
        swr_free(&_swr_ctx);
        _swr_ctx = nullptr;
    }

    if (nullptr != _out_data[0])
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, AV_NUM_DATA_POINTERS);
        memset(_out_size, 0, AV_NUM_DATA_POINTERS);
    }
}

bool CXPlayerAudioResampler::rescale(const AVFrame * in_frm, uint8_t ** out_data, int * out_len)
{
    if (nullptr == in_frm)
    {
        xpu_format_string(_err, "Input param is nullptr!");
        return false;
    }

    if (!_need_rescale)
    {
        *out_data = in_frm->data[0];
        *out_len = av_samples_get_buffer_size(_out_size, in_frm->ch_layout.nb_channels, in_frm->nb_samples, _in._fmt, 0);
        return true;
    }

    if (nullptr == _swr_ctx)
    {
        xpu_format_string(_err, "rescaler is not open!");
        return false;
    }

    int in_samples_per_channel = 0;
    int out_samples_per_channel = 0;
    const auto out_channels = _out._layout.nb_channels;

    in_samples_per_channel = in_frm->linesize[0] / av_get_bytes_per_sample(_in._fmt);
    if (!av_sample_fmt_is_planar(_in._fmt))
        in_samples_per_channel /= in_frm->ch_layout.nb_channels;
    out_samples_per_channel = swr_get_out_samples(_swr_ctx, in_samples_per_channel);
    if (!resizeCache(out_samples_per_channel))
    {
        xpu_format_string(_err, "resizeCache failed.");
        return false;
    }

    int ret = swr_convert(_swr_ctx, _out_data, out_samples_per_channel, (const uint8_t **)in_frm->data, _in_nb_samples);
    if (ret <= 0)
    {
        xpu_format_string(_err, "swr_convert failed!");
        return false;
    }

    int buf_size = av_samples_get_buffer_size(_out_size, out_channels, ret, _out._fmt, 0);
    if (buf_size < 0)
    {
        xpu_format_string(_err, "Could not get sample buffer size!");
        return false;
    }
    *out_data = _out_data[0];
    *out_len = buf_size;

    return true;
}

const char * CXPlayerAudioResampler::err() const
{
    return _err.c_str();
}

bool CXPlayerAudioResampler::resizeCache(const int nb_samples)
{
    if (nb_samples <= _out_max_nb_samples)
        return true;

    if (nullptr != _out_data[0])
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, AV_NUM_DATA_POINTERS);
        memset(_out_size, 0, AV_NUM_DATA_POINTERS);
    }

    const auto out_channels = _out._layout.nb_channels;
    int ret = av_samples_alloc(_out_data, _out_size, out_channels, nb_samples, _out._fmt, 1);
    if (ret < 0)
    {
        xpu_format_string(_err, "av_samples_alloc failed.");
        return false;
    }

    _out_max_nb_samples = nb_samples;

    return true;
}
