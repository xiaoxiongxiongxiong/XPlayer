#include "xplayer_source.h"

extern "C" {
#include "libavformat/avformat.h"
}

#include "utils/xplayer_utils.h"
#include "demuxer/xplayer_demuxer.h"
#include "decoder/xplayer_decoder.h"
#include "rescaler/xplayer_audio_resampler.h"
#include "rescaler/xplayer_video_rescaler.h"
#include "renderer/xplayer_audio_render_sdl.h"
#include "renderer/xplayer_video_render_sdl.h"
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

    if (!createStreams())
    {
        delete _ctx;
        _ctx = nullptr;
        return false;
    }

    try 
    {
        _is_running.store(true);
        _thr = std::thread{ &CXPlayerSource::readPacketsThr,this };
        if (-1 != _audio_stream_index)
            _audio_thr = std::thread{ &CXPlayerSource::audioPlayThr,this };
        if (-1 != _video_stream_index)
            _video_thr = std::thread{ &CXPlayerSource::videoPlayThr,this };
    }
    catch (const std::exception & e)
    {
        xpu_format_string(_err, "%s", e.what());
        close();
        return false;
    }

    _state.store(XPLAYER_STATE_READY);

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

    _audio_cond.notify_one();
    if (_audio_thr.joinable())
    {
        _audio_thr.join();
        _audio_stream_index.store(-1);
    }

    _video_cond.notify_one();
    if (_video_thr.joinable())
    {
        _video_thr.join();
        _video_stream_index.store(-1);
    }

    destroyStreams();

    _ctx->close();
    delete _ctx;
    _ctx = nullptr;

    _state.store(XPLAYER_STATE_NONE);
    _dst_pos_ms.store(-1);
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
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Not opened yet");
        return false;
    }

    for (const auto & si : _streams)
    {
        if (!si.second->setup(wnd, width, height))
        {
            _err = si.second->err();
            return false;
        }
    }

    _cond.notify_one();
    _state.store(XPLAYER_STATE_PLAYING);

    return true;
}

void CXPlayerSource::resize(int width, int height)
{
    _wnd_width.store(width);
    _wnd_height.store(height);
    _wnd_changed.store(true);
}

bool CXPlayerSource::pause(bool flag)
{
    if (XPLAYER_STATE_PLAYING != _state.load() && XPLAYER_STATE_PAUSE != _state.load())
    {
        xpu_format_string(_err, "Unsupported operation");
        return false;
    }

    _state.store(flag ? XPLAYER_STATE_PAUSE : XPLAYER_STATE_PLAYING);

    return true;
}

bool CXPlayerSource::seek(const int64_t pos)
{
    _is_skip.store(true);
    _dst_pos_ms.store(pos);

    return true;
}

int64_t CXPlayerSource::progress()
{
    if (_is_skip.load() || !_ctx || _ctx->duration() <= 0)
        return -1LL;
    return _audio_clock.load();
}

void CXPlayerSource::setVolume(int volume)
{
    if (!_is_running.load())
        return;

    if (-1 == _audio_stream_index.load())
        return;

    if (nullptr != _streams[_audio_stream_index.load()]->_audio_renderer)
        _streams[_audio_stream_index.load()]->_audio_renderer->setVolume(volume);
}

XPLAYER_STATE CXPlayerSource::state() const
{
    return _state.load();
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

        if (!si->init(stream->codecpar, stream->time_base))
        {
            xpu_format_string(_err, "%s", si->err());
            destroyStreams();
            return false;
        }

        if (AVMEDIA_TYPE_AUDIO == codec_type && -1 == _audio_stream_index)
        {
            _audio_play_over.store(false);
            _audio_stream_index.store(i);
        }
        else if (AVMEDIA_TYPE_VIDEO == codec_type && -1 == _video_stream_index)
        {
            _video_play_over.store(false);
            _video_stream_index.store(i);
        }

        _streams.emplace(i, si);
    }

    return true;
}

void CXPlayerSource::destroyStreams()
{
    for (auto & si : _streams)
    {
        si.second->uninit();
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
        if (_is_skip.load())
        {
            int stream_index = -1;
            if (-1 != _video_stream_index.load())
            {
                _video_skip_over.store(false);
                stream_index = _video_stream_index.load();
            }
            if (-1 != _audio_stream_index.load())
            {
                _audio_skip_over.store(false);
                if (-1 == stream_index)
                    stream_index = _audio_stream_index.load();
            }

            if (0 != _ctx->seek(stream_index, _dst_pos_ms.load()))
            {
                xpu_format_string(_err, "%s", _ctx->err());
                _state.store(XPLAYER_STATE_ERROR);
                break;
            }

            while (!_audio_skip_over || !_video_skip_over)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            _audio_clock.store(-1LL);
            _is_skip.store(false);
        }

        while (_is_running.load() && XPLAYER_STATE_PAUSE == _state.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        bool over = false;
        AVPacket pkt = {};
        if (!_ctx->readPacket(pkt, over))
        {
            xpu_format_string(_err, "%s", _ctx->err());
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        if (over)
        {
            _is_over.store(true);
            break;
        }

        auto & stream = _streams[pkt.stream_index];
        while (_is_running.load() && !_is_skip.load() && stream->isCacheFull())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (_audio_stream_index.load() == pkt.stream_index)
        {
            stream->pushPacket(pkt);
            _audio_cond.notify_one();
        }
        else if (_video_stream_index.load() == pkt.stream_index)
        {
            stream->pushPacket(pkt);
            _video_cond.notify_one();
        }
        else
        {
            av_packet_unref(&pkt);
            continue;
        }
    }
}

void CXPlayerSource::audioPlayThr()
{
    std::unique_lock<std::mutex> lck(_audio_mtx);
    _audio_cond.wait(lck);
    bool flush_flag = false;
    bool mute_flag = false;
    while (_is_running.load())
    {
        while (_is_running.load() && XPLAYER_STATE_PLAYING != _state.load())
        {
            if (XPLAYER_STATE_PAUSE == _state.load() & !mute_flag)
            {
                _streams[_audio_stream_index.load()]->_audio_renderer->mute(true);
                mute_flag = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        const auto & stream = _streams[_audio_stream_index.load()];

        if (_is_skip.load())
        {
            if (!_audio_skip_over)
            {
                stream->_audio_renderer->mute(true);
                stream->clearPackets();
                stream->_decoder->clear();
                _audio_skip_over.store(true);
            }
            mute_flag = true;
            continue;
        }

        bool succ = false;
        AVPacket pkt{};
        if (!stream->popPacket(pkt))
        {
            if (_is_over && !flush_flag)
            {
                flush_flag = true;
                succ = stream->_decoder->send(nullptr);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
        }
        else
        {
            succ = stream->_decoder->send(&pkt);
            av_packet_unref(&pkt);
        }

        if (!succ)
        {
            _err = stream->_decoder->err();
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        AVFrame frm{};
        bool got = false;
        bool over = false;
        while (_is_running.load() && stream->_decoder->recv(frm, got, over) && got)
        {
            if (_dst_pos_ms > 0 && (stream->timestamp(frm.pts) < _dst_pos_ms.load() || AV_NOPTS_VALUE == frm.pts))
                continue;

            if (AV_NOPTS_VALUE != frm.pts)
                _audio_clock.store(stream->timestamp(frm.pts));
            uint8_t * data = nullptr;
            int len = 0;
            if (!stream->_audio_resampler->rescale(&frm, &data, &len))
            {
                _err = stream->_audio_resampler->err();
                stream->_audio_renderer->mute(true);
                _state.store(XPLAYER_STATE_ERROR);
                break;
            }

            if (mute_flag)
            {
                stream->_audio_renderer->mute(false);
                mute_flag = false;
            }

            stream->_audio_renderer->renderer(data, len);
            while (_is_running.load() && !stream->_audio_renderer->finished())
                std::this_thread::sleep_for(std::chrono::microseconds(20));

            got = false;
        }

        _audio_play_over.store(over);
        if (_audio_play_over && _video_play_over)
        {
            _state.store(XPLAYER_STATE_OVER);
            stream->_audio_renderer->mute(true);
        }
    }
}

void CXPlayerSource::videoPlayThr()
{
    std::unique_lock lck(_video_mtx);
    _video_cond.wait(lck);
    bool flush_flag = false;

    while (_is_running.load())
    {
        while (_is_running.load() && XPLAYER_STATE_PLAYING != _state.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        const auto & stream = _streams[_video_stream_index.load()];

        if (_is_skip.load())
        {
            if (!_video_skip_over)
            {
                stream->clearPackets();
                stream->_decoder->clear();
                _video_skip_over.store(true);
            }
            continue;
        }

        bool succ = false;
        AVPacket pkt{};
        if (!stream->popPacket(pkt))
        {
            if (_is_over && !flush_flag)
            {
                flush_flag = true;
                succ = stream->_decoder->send(nullptr);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
        }
        else
        {
            succ = stream->_decoder->send(&pkt);
            av_packet_unref(&pkt);
        }

        if (!succ)
        {
            _err = stream->_decoder->err();
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        AVFrame frm{};
        bool got = false;
        bool over = false;
        while (_is_running.load() && stream->_decoder->recv(frm, got, over) && got)
        {
            if (_dst_pos_ms > 0 && (stream->timestamp(frm.pts) < _dst_pos_ms.load() || AV_NOPTS_VALUE == frm.pts))
                continue;

            if (_wnd_changed)
            {
                //CXPlayerVideoInfo dst(AV_PIX_FMT_YUV420P, _wnd_width.load(), _wnd_height.load());
                //stream->_video_rescaler->updateParameters(dst);
                stream->_video_renderer->resizeWindow(_wnd_width.load(), _wnd_height.load());
                _wnd_changed.store(false);
            }

            //AVFrame out_frm{};
            //if (!stream->_video_rescaler->rescale(&frm, &out_frm))
            //{
            //    _err = stream->_video_rescaler->err();
            //    _state.store(XPLAYER_STATE_ERROR);
            //    break;
            //}

            int64_t delay_ms = 0;
            if (-1 != _audio_stream_index.load())
            {
                auto tmp = stream->timestamp(frm.pts);
                delay_ms = tmp - _audio_clock.load();
            }
            else
            {
                if (frm.duration > 0)
                    delay_ms = stream->timestamp(frm.duration);
                else
                    delay_ms = stream->frameDuration();
            }
            if (delay_ms > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

            stream->_video_renderer->renderer(frm.data, frm.linesize);
            got = false;
        }

        _video_play_over.store(over);
        if (_audio_play_over && _video_play_over)
            _state.store(XPLAYER_STATE_OVER);
    }
}

