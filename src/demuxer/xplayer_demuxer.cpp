#include "xplayer_demuxer.h"
#include "xplayer_stream.h"

extern "C" {
#include "libavformat/avformat.h"
}
#include "utils/xplayer_utils.h"

bool CXPlayerDemuxImpl::open(const std::string & url, const std::string & specs)
{
    AVDictionary * dict = nullptr;

    int ret = avformat_open_input(&_ctx, url.c_str(), nullptr, &dict);
    if (nullptr != dict)
        av_dict_free(&dict);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        _err.assign(buff);
        return false;
    }

    ret = avformat_find_stream_info(_ctx, nullptr);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        _err.assign(buff);
        avformat_close_input(&_ctx);
        return false;
    }

    const auto timebase = av_make_q(1, AV_TIME_BASE);
    _duration_ms = static_cast<int64_t>(static_cast<double>(_ctx->duration) * av_q2d(timebase) * 1000.0);

    return true;
}

void CXPlayerDemuxImpl::close()
{
    if (nullptr != _ctx)
    {
        avformat_close_input(&_ctx);
        _ctx = nullptr;
    }
}

int64_t CXPlayerDemuxImpl::duration() const
{
    return _duration_ms;
}

int CXPlayerDemuxImpl::getStreamsCount()
{
    if (nullptr == _ctx)
        return 0;

    return static_cast<int>(_ctx->nb_streams);
}

// 获取流信息
AVStream * CXPlayerDemuxImpl::getStreamInfo(const int index)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "not open yet");
        return nullptr;
    }

    if (index < 0 || index >= static_cast<int>(_ctx->nb_streams))
    {
        xpu_format_string(_err, "invalid stream index: %d", index);
        return nullptr;
    }

    return _ctx->streams[index];
}

int CXPlayerDemuxImpl::seek(int stream_index, int64_t timestamp)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "not open yet");
        return -1;
    }

    if (stream_index < 0 || stream_index >= static_cast<int>(_ctx->nb_streams))
    {
        xpu_format_string(_err, "invalid stream index: %d", stream_index);
        return -1;
    }

    const auto time_base = _ctx->streams[stream_index]->time_base;
    const auto ts = static_cast<int64_t>(static_cast<double>(timestamp) * av_q2d(time_base));
    int ret = av_seek_frame(_ctx, stream_index, ts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
    }

    return ret;
}

int CXPlayerDemuxImpl::readPacket(AVPacket * pkt)
{
    int ret = -1;
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "not open yet");
        return ret;
    }

    if (nullptr == pkt)
    {
        xpu_format_string(_err, "invalid param");
        return ret;
    }

    ret = av_read_frame(_ctx, pkt);
    if (0 != ret)
    {
        if (AVERROR_EOF == ret)
            ret = 1;
        else
        {
            char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
            av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
            xpu_format_string(_err, "%s", buff);
        }
    }

    return ret;
}

const char * CXPlayerDemuxImpl::err() const
{
    return _err.c_str();
}
