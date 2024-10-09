#include "xplayer_demuxer.h"
#include "xplayer_stream.h"

extern "C" {
#include "libavformat/avformat.h"
}

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

    av_dump_format(_ctx, 0, url.c_str(), 0);

    return true;
}

void CXPlayerDemuxImpl::close()
{
}

int64_t CXPlayerDemuxImpl::duration() const
{
    return _duration_ms;
}

const char * CXPlayerDemuxImpl::err() const
{
    return _err.c_str();
}

bool CXPlayerDemuxImpl::createStreams()
{
    const auto cnt = static_cast<int>(_ctx->nb_streams);
    for (int i = 0; i < cnt; ++i)
    {
        const auto * codec_par = _ctx->streams[i]->codecpar;
        if (AVMEDIA_TYPE_VIDEO != codec_par->codec_type && AVMEDIA_TYPE_AUDIO != codec_par->codec_type)
            continue;

        auto si = std::make_shared<CXPlayerStream>(i);
        if (!si)
        {
            xpu_format_string(_err, "Create stream %d failed", i);
            return false;
        }

        si->create(codec_par);
    }

    return true;
}

void CXPlayerDemuxImpl::destroyStreams()
{
}
