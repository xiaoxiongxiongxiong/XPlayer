#include "xplayer_source.h"

extern "C" {
#include "libavformat/avformat.h"
}

#include "utils/xplayer_utils.h"
#include "demuxer/xplayer_demuxer.h"
#include "xplayer_stream.h"

bool CXPlayerSource::open(const std::string & url, const std::string & params)
{
    _ctx = new(std::nothrow) CXPlayerDemuxImpl();
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "no enough memory");
        return false;
    }

    if (!_ctx->open(url, params))
    {
        xpu_format_string(_err, "%s", _ctx->err());
        delete _ctx;
        _ctx = nullptr;
        return false;
    }

    return true;
}

void CXPlayerSource::close()
{
    if (nullptr == _ctx)
        return;

    _is_running.store(false);
    _cond.notify_one();
    if (_thr.joinable())
        _thr.join();

    _ctx->close();
    delete _ctx;
    _ctx = nullptr;
}

int64_t CXPlayerSource::duration() const
{
    if (nullptr == _ctx)
        return 0;

    return _ctx->duration();
}

void CXPlayerSource::getStreamsInfo(std::vector<int> & ais, std::vector<int> & vis)
{
    ais.clear();
    vis.clear();
    if (nullptr == _ctx)
        return;

    const auto cnt = _ctx->getStreamsCount();
    for (int i = 0; i < cnt; i++)
    {
        const auto * stream = _ctx->getStreamInfo(i);
        if (AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type)
            ais.emplace_back(i);
        else if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type)
            vis.emplace_back(i);
    }
}

bool CXPlayerSource::play(const void * wnd, int width, int height)
{


    return true;
}

bool CXPlayerSource::pause()
{
    return true;
}

bool CXPlayerSource::seek(const int64_t pos)
{
    _dst_pos_ms.store(pos);
    return true;
}

int64_t CXPlayerSource::progress()
{
    return _cur_pos_ms.load();
}

void CXPlayerSource::setVolume(int volume)
{
}

const char * CXPlayerSource::err() const
{
    return _err.c_str();
}

bool CXPlayerSource::createStreams()
{
    const auto cnt = _ctx->getStreamsCount();
    for (int i = 0; i < cnt; ++i)
    {
        const auto * stream = _ctx->getStreamInfo(i);
        if (nullptr == stream)
            continue;

        const auto codec_type = stream->codecpar->codec_type;
        if (AVMEDIA_TYPE_AUDIO != codec_type && AVMEDIA_TYPE_VIDEO != codec_type)
            continue;

        auto si = std::make_shared<CXPlayerStream>(i);
        if (!si)
        {
            xpu_format_string(_err, "no enough memory");
            destroyStreams();
            return false;
        }

        if (!si->create(stream->codecpar))
        {
            xpu_format_string(_err, "%s", si->err());
            destroyStreams();
            return false;
        }

        _streams.emplace(i, si);
    }

    return true;
}

void CXPlayerSource::destroyStreams()
{
    for (auto & si : _streams)
    {
        si.second->destroy();
        si.second.reset();
    }
    _streams.clear();
}

void CXPlayerSource::readPacketsThr()
{
    std::unique_lock<std::mutex> lck(_mtx);
    _cond.wait(lck);

    while (_is_running.load())
    {
        AVPacket pkt = {};
        int ret = _ctx->readPacket(&pkt);
        if (0 != ret)
        {
            if (1 == ret)
            {
                for (auto & si : _streams)
                    si.second->push(pkt, true);
            }
            else
                xpu_format_string(_err, "%s", _ctx->err());
            break;
        }

        auto found = _streams.find(pkt.stream_index);
        if (_streams.end() == found || !(*found).second->push(pkt))
            av_packet_unref(&pkt);
    }
}
