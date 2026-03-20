#include "xplayer_video_rescaler.h"
#include "utils/xplayer_utils.h"

CXPlayerVideoInfo::CXPlayerVideoInfo(enum AVPixelFormat fmt, int width, int height):
    _fmt(fmt),
    _width(width),
    _height(height)
{
}

bool CXPlayerVideoInfo::operator==(const CXPlayerVideoInfo & other) const
{
    if (_fmt != other._fmt)
        return false;

    if (_width != other._width)
        return false;

    if (_height != other._height)
        return false;

    return true;
}


bool CXPlayerVideoRescaler::create(const CXPlayerVideoInfo & src, const CXPlayerVideoInfo & dst)
{
    if (_rescaler)
    {
        xpu_format_string(_err, "Already opened rescaler");
        return false;
    }

    // 判断像素格式是否支持
    if (!sws_isSupportedInput(src._fmt))
    {
        xpu_format_string(_err, "Input pixel:%d format is not supported", src._fmt);
        return false;
    }
    if (!sws_isSupportedOutput(dst._fmt))
    {
        xpu_format_string(_err, "Output pixel:%d format is not supported.", dst._fmt);
        return false;
    }

    // 完全相同，不需要转换
    if (dst == src)
    {
        xpu_format_string(_err, "No need swscale!");
        _need_rescale = false;
        return true;
    }

    _rescaler = sws_getContext(src._width, src._height, src._fmt, dst._width, dst._height, dst._fmt, 0, nullptr, nullptr, nullptr);
    if (nullptr == _rescaler)
    {
        xpu_format_string(_err, "sws_alloc_context failed!");
        return false;
    }

    int ret = av_image_alloc(_out_data, _out_linesize, dst._width, dst._height, dst._fmt, 64);
    if (ret <= 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "av_image_alloc failed, err:%s", buff);
        sws_freeContext(_rescaler);
        _rescaler = nullptr;
        return false;
    }

    _src = src;
    _dst = dst;

    _need_rescale = true;

    return true;
}

void CXPlayerVideoRescaler::destroy()
{
    if (nullptr != _rescaler)
    {
        sws_freeContext(_rescaler);
        _rescaler = nullptr;
    }

    if (nullptr != _out_data)
    {
        av_freep(&_out_data[0]);
        memset(_out_data, 0, sizeof(_out_data));
        memset(_out_linesize, 0, sizeof(_out_linesize));
    }

    _need_rescale = true;
}

bool CXPlayerVideoRescaler::updateParameters(const CXPlayerVideoInfo & dst, const bool & flag)
{
    if (flag && _src == dst)
        return true;

    if (!flag && _dst == dst)
        return true;

    if (flag)
    {
        _rescaler = sws_getCachedContext(_rescaler,
                                         dst._width, dst._height, dst._fmt,
                                         _dst._width, _dst._height, _dst._fmt,
                                         0, nullptr, nullptr, nullptr);
    }
    else
    {
        _rescaler = sws_getCachedContext(_rescaler,
                                         _src._width, _src._height, _src._fmt,
                                         dst._width, dst._height, dst._fmt,
                                         0, nullptr, nullptr, nullptr);
    }
    if (nullptr == _rescaler)
    {
        xpu_format_string(_err, "Cannot initialize the conversion context");
        return false;
    }

    if (!flag)
    {
        if (nullptr != _out_data)
        {
            av_freep(&_out_data[0]);
            memset(_out_data, 0, sizeof(_out_data));
            memset(_out_linesize, 0, sizeof(_out_linesize));
        }

        int ret = av_image_alloc(_out_data, _out_linesize, dst._width, dst._height, dst._fmt, 64);
        if (ret <= 0)
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            xpu_format_string(_err, "av_image_alloc failed, err:%s", buff);
            return false;
        }
        _dst = dst;
    }
    else
        _src = dst;

    return true;
}

bool CXPlayerVideoRescaler::rescale(const AVFrame * in_frm, AVFrame * out_frm)
{
    if (nullptr == in_frm || nullptr == in_frm->data[0] || 0 >= in_frm->linesize[0] || nullptr == out_frm)
    {
        xpu_format_string(_err, "Input param is invalid");
        return false;
    }

    if (in_frm->width != _src._width || in_frm->height != _src._height || in_frm->format != static_cast<int>(_src._fmt))
    {
        CXPlayerVideoInfo pvi(static_cast<AVPixelFormat>(in_frm->format), in_frm->width, in_frm->height);
        if (!updateParameters(pvi))
        {
            xpu_format_string(_err, "Input changed");
            return false;
        }
        _need_rescale = pvi == _dst;
    }

    if (!_need_rescale)
    {
        memcpy(out_frm->data, in_frm->data, sizeof(in_frm->data[0]) * AV_NUM_DATA_POINTERS);
        memcpy(out_frm->linesize, in_frm->linesize, sizeof(in_frm->linesize[0]) * AV_NUM_DATA_POINTERS);
        return true;
    }

    if (nullptr == _rescaler)
    {
        xpu_format_string(_err, "rescaler is not open!");
        return false;
    }

    int ret = sws_scale(_rescaler, in_frm->data, in_frm->linesize, 0, _src._height, _out_data, _out_linesize);
    if (ret <= 0)
    {
        xpu_format_string(_err, "sws_scale failed!");
        return false;
    }

    copyFrame(out_frm, in_frm);

    return true;
}

const char * CXPlayerVideoRescaler::err() const
{
    return _err.c_str();
}

void CXPlayerVideoRescaler::copyFrame(AVFrame * dst_frm, const AVFrame * src_frm)
{
    if (nullptr == src_frm || nullptr == dst_frm)
    {
        xpu_format_string(_err, "Input param is nullptr!");
        return;
    }

    // 拷贝数据
    memcpy(dst_frm->data, _out_data, sizeof(_out_data[0]) * AV_NUM_DATA_POINTERS);
    memcpy(dst_frm->linesize, _out_linesize, sizeof(_out_linesize[0]) * AV_NUM_DATA_POINTERS);

    // 拷贝参数
    dst_frm->format = static_cast<int>(_dst._fmt);
    dst_frm->pts = dst_frm->pts;
    dst_frm->pkt_dts = src_frm->pkt_dts;
    dst_frm->duration = src_frm->duration;
    dst_frm->width = _dst._width;
    dst_frm->height = _dst._height;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->color_primaries = src_frm->color_primaries;
    dst_frm->color_trc = src_frm->color_trc;
    dst_frm->color_range = src_frm->color_range;
    dst_frm->colorspace = src_frm->colorspace;
}
