#include "xplayer_source.h"

#include <filesystem>

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"
}

#include "xplayer_utils.h"
#include "xplayer_demuxer.h"
#include "xplayer_audio_render_sdl.h"
#include "xplayer_video_renderer.h"
#include "xplayer_stream.h"

CXPlayerSource::CXPlayerSource() = default;
CXPlayerSource::~CXPlayerSource() = default;

void CXPlayerSource::setFontPath(const std::string & path)
{
    _font_path = path;
}

void CXPlayerSource::setFontSize(int size)
{
    _font_size = size;
}

void CXPlayerSource::setFontColor(int red, int green, int blue, int alpha)
{

}

bool CXPlayerSource::open(const std::string & url, const std::string & params)
{
    _ctx = std::make_unique<CXPlayerDemuxImpl>();
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "no enough memory");
        return false;
    }

    if (!_ctx->open(url, params))
    {
        xpu_format_string(_err, "%s", _ctx->err());
        _ctx.reset();
        _ctx = nullptr;
        return false;
    }

    if (!createStreams())
    {
        _ctx.reset();
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

    std::filesystem::path tmp(url);
    _name = tmp.filename().string();

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

    uninitAudioRenderer();
    uninitVideoRenderer();

    _ctx->close();
    _ctx.reset();
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

bool CXPlayerSource::play(const void * wnd, int width, int height, const std::string & audio_device)
{
    if (nullptr == _ctx)
    {
        xpu_format_string(_err, "Not opened yet");
        return false;
    }

    if (!initAudioRenderer(audio_device))
        return false;

    if (!initVideoRenderer(wnd, width, height))
        return false;

    std::vector<XPLAYER_PIXEL_FORMAT_TYPE> formats;
    _video_renderer->supportedPixelFormat(formats);

    for (const auto & si : _streams)
    {
        if (!si.second->prepare(formats))
        {
            _err = si.second->err();
            return false;
        }
    }

    _wnd_width.store(width);
    _wnd_height.store(height);

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
    if (XPLAYER_STATE_PLAYING != _state.load())
    {
        xpu_format_string(_err, "Not playing");
        return false;
    }

    _is_skip.store(true);
    _dst_pos_ms.store(pos);

    return true;
}

int64_t CXPlayerSource::progress()
{
    if (_is_skip.load() || !_ctx || _ctx->duration() <= 0)
        return -1LL;
    return _cur_pos_ms.load();
}

bool CXPlayerSource::selectStream(int index, bool is_video)
{
    if (is_video)
        _video_stream_index.store(index > -1 ? index : -1);
    else
        _audio_stream_index.store(index > -1 ? index : -1);

    return true;
}

void CXPlayerSource::selectVideoRenderer(XPLAYER_VIDEO_RENDERER_TYPE type)
{
    _video_renderer_type.store(type);
}

void CXPlayerSource::setVolume(int volume)
{
    _volume.store(volume);
    if (nullptr != _audio_renderer)
    {
        _audio_renderer->setVolume(volume);
    }
}

void CXPlayerSource::setSpeed(const float speed)
{
    const float epsilon = 1e-6f;
    const auto diff = _speed.load() - speed;
    if (std::abs(diff) > epsilon)
    {
        _speed.store(speed);
        _speed_changed.store(true);
    }
}

void CXPlayerSource::showDetail(bool flag)
{
    _show.store(flag);
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

bool CXPlayerSource::initAudioRenderer(const std::string & device)
{
    if (nullptr != _audio_renderer)
        return true;

    _audio_renderer = std::make_unique<CXPlayerAudioRender>();
    if (nullptr == _audio_renderer)
    {
        xpu_format_string(_err, "Create CXPlayerAudioRender instance failed");
        return false;
    }

    if (-1 == _audio_stream_index.load())
        return true;

    auto * avs = _ctx->getStreamInfo(_audio_stream_index.load());
    auto * codecpar = avs ? avs->codecpar : nullptr;
    if (nullptr == codecpar)
    {
        xpu_format_string(_err, "Get audio stream info failed");
        _audio_renderer.reset();
        _audio_renderer = nullptr;
        return false;
    }

    if (!_audio_renderer->create(codecpar->sample_rate, codecpar->channels, codecpar->frame_size, _volume.load(), device))
    {
        _err = _audio_renderer->err();
        return false;
    }

    return true;
}

void CXPlayerSource::uninitAudioRenderer()
{
    if (nullptr != _audio_renderer)
    {
        _audio_renderer->destroy();
        _audio_renderer.reset();
        _audio_renderer = nullptr;
    }
}

bool CXPlayerSource::initVideoRenderer(const void * wnd, int width, int height)
{
    if (nullptr != _video_renderer)
        return true;

    _video_renderer = CXPlayerVideoRendererFactory::create(_video_renderer_type.load(), wnd);
    if (nullptr == _video_renderer)
    {
        xpu_format_string(_err, "Create CXPlayerVideoRenderSDL instance failed");
        return false;
    }

    if (!_video_renderer->create(wnd, width, height, _font_path, _font_size))
    {
        _err = _video_renderer->err();
        CXPlayerVideoRendererFactory::destroy(_video_renderer);
        return false;
    }

    return true;
}

void CXPlayerSource::uninitVideoRenderer()
{
    if (_video_renderer)
    {
        _video_renderer->destroy();
        CXPlayerVideoRendererFactory::destroy(_video_renderer);
    }
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
            for (auto & stream : _streams)
                stream.second->send(pkt, true);
            break;
        }

        auto & stream = _streams[pkt.stream_index];
        while (_is_running.load() && !_is_skip.load() && stream->isFull())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        bool succ = false;
        if (_audio_stream_index.load() == pkt.stream_index)
        {
            succ = stream->send(pkt);
            _audio_cond.notify_one();
        }
        else if (_video_stream_index.load() == pkt.stream_index)
        {
            succ = stream->send(pkt);
            _video_cond.notify_one();
        }

        if (!succ)
            av_packet_unref(&pkt);
    }
}

void CXPlayerSource::audioPlayThr()
{
    std::unique_lock<std::mutex> lck(_audio_mtx);
    _audio_cond.wait(lck);
    int stream_index = _audio_stream_index.load();
    bool mute_flag = false;
    std::vector<uint8_t> buff(8192);

    while (_is_running.load())
    {
        while (_is_running.load() && XPLAYER_STATE_PLAYING != _state.load())
        {
            if (XPLAYER_STATE_PAUSE == _state.load() && !mute_flag)
            {
                _audio_renderer->mute(true);
                mute_flag = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (_audio_stream_index.load() < 0)
        {
            if (!mute_flag)
            {
                _audio_renderer->mute(true);
                _streams[stream_index]->clear();
                _audio_clock.store(-1LL);
                stream_index = _audio_stream_index.load();
                mute_flag = true;
            }
            continue;
        }

        changeStream(stream_index, _audio_stream_index.load());

        const auto & stream = _streams[stream_index];

        if (_is_skip.load())
        {
            if (!_audio_skip_over)
            {
                _cur_pos_ms.store(-1LL);
                _audio_renderer->mute(true);
                _streams[stream_index]->clear();
                mute_flag = true;
                _audio_skip_over.store(true);
            }
            continue;
        }

        if (_speed_changed.load())
        {
            stream->setSpeed(_speed.load());
            _speed_changed.store(false);
        }

        AVFrame frm{};
        bool got = false;
        bool over = false;
        if (!stream->recv(frm, got, over))
        {
            _err = stream->err();
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        if (over)
        {
            _audio_play_over.store(over);
            if (_audio_play_over && _video_play_over)
            {
                _state.store(XPLAYER_STATE_OVER);
            }
            break;
        }

        if (!got || (_dst_pos_ms > 0 && (stream->timestamp(frm.pts) < _dst_pos_ms.load() || AV_NOPTS_VALUE == frm.pts)))
        {
            av_frame_unref(&frm);
            continue;
        }

        if (AV_NOPTS_VALUE != frm.pts)
        {
            _audio_clock.store(stream->timestamp(frm.pts));
            _cur_pos_ms.store(_audio_clock.load());
        }

        if (mute_flag)
        {
            _audio_renderer->mute(false);
            mute_flag = false;
        }

        _audio_renderer->renderer(frm.data[0], frm.linesize[0]);
        av_frame_unref(&frm);
    }

    if (_audio_renderer)
        _audio_renderer->mute(true);
}

void CXPlayerSource::videoPlayThr()
{
    std::unique_lock lck(_video_mtx);
    _video_cond.wait(lck);
    bool flag = false;
    int stream_index = _video_stream_index.load();

    while (_is_running.load())
    {
        while (_is_running.load() && XPLAYER_STATE_PLAYING != _state.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        if (_video_stream_index.load() < 0)
        {
            if (!flag)
            {
                _streams[stream_index]->clear();
                _video_renderer->clear();
                stream_index = _video_stream_index.load();
                flag = true;
            }
            continue;
        }

        changeStream(stream_index, _video_stream_index.load());

        const auto & stream = _streams[stream_index];

        if (_is_skip.load())
        {
            if (!_video_skip_over)
            {
                _dst_pos_ms;
                _cur_pos_ms.store(-1LL);
                _cur_frames.store(-1LL);
                stream->clear();
                _video_skip_over.store(true);
            }
            continue;
        }

        AVFrame frm{};
        bool got = false;
        bool over = false;
        if (!stream->recv(frm, got, over))
        {
            _err = stream->err();
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        if (over)
        {
            _video_play_over.store(over);
            if (_audio_play_over && _video_play_over)
            {
                _state.store(XPLAYER_STATE_OVER);
            }
            break;
        }

        if (!got || (_dst_pos_ms > 0 && (stream->timestamp(frm.pts) < _dst_pos_ms.load() || AV_NOPTS_VALUE == frm.pts)))
        {
            av_frame_unref(&frm);
            continue;
        }

        if (_wnd_changed)
        {
            _video_renderer->resize(_wnd_width.load(), _wnd_height.load());
            _wnd_changed.store(false);
        }

        int64_t delay_ms = 0;
        if (-1 != _audio_stream_index.load() && _audio_clock.load() > 0)
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
            delay_ms = static_cast<int64_t>(std::round(static_cast<double>(delay_ms) / _speed.load()));
            _cur_pos_ms.store(stream->timestamp(frm.pts));
        }

        if (delay_ms > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        auto str = formatDetailString();
        _video_renderer->renderer(frm.width, frm.height, stream->getPixelFormat(), frm.data, frm.linesize, str);
        _cur_frames++;
        flag = false;
        av_frame_unref(&frm);
    }

    if (_video_renderer)
        _video_renderer->clear();
    _last_ts = 0;
    _cur_frames.store(0);
    _last_frames.store(0);
}

void CXPlayerSource::changeStream(int & src, const int & dst)
{
    if (src == dst)
        return;

    if (src > -1)
    {
        _streams[src]->clear();
    }

    if (dst > -1)
    {
        _streams[dst]->setSpeed(_speed.load());
    }

    src = dst;
}

double CXPlayerSource::calcFrameRate()
{
    const auto cur_ts = xpu_time_ms();
    if (0 == _last_ts)
    {
        _last_ts = cur_ts;
        _last_frames.store(_cur_frames.load());
        return _cur_fps.load();
    }

    const auto ms = cur_ts - _last_ts;
    if (ms >= 1000)
    {
        auto frames = _cur_frames.load() - _last_frames.load();
        _cur_fps.store(static_cast<double>(frames) / static_cast<double>(ms) * 1000.0);
        _last_frames.store(_cur_frames.load());
        _last_ts = cur_ts;
    }

    return _cur_fps.load();
}

std::string CXPlayerSource::formatDetailString()
{
    std::string str;

    if (_cur_frames.load() < 0 && -1 != _video_stream_index.load())
    {
        const auto * stream = _ctx->getStreamInfo(_video_stream_index.load());
        const auto * codecpar = stream->codecpar;

        auto duration_ms = static_cast<double>(stream->duration) * av_q2d(stream->time_base) * 1000.0;
        auto tmp = static_cast<double>(_dst_pos_ms.load()) / duration_ms * static_cast<double>(stream->nb_frames);
        _cur_frames.store(static_cast<int64_t>(std::round(tmp)));
        _last_frames.store(_cur_frames.load());
        _last_ts = 0;
    }

    auto fps = calcFrameRate();
    if (!_show.load() || nullptr == _ctx)
    {
        return str;
    }

    xpu_format_string(
        str,
        "%s\n"
        "%s / %s",
        _name.c_str(),
        xpu_time2str(_cur_pos_ms).c_str(),
        xpu_time2str(_ctx->duration()).c_str()
    );

    if (-1 != _video_stream_index.load())
    {
        const auto * stream = _ctx->getStreamInfo(_video_stream_index.load());
        const auto * codecpar = stream->codecpar;

        std::string video_info;
        xpu_format_string(
            video_info,
            ", %" PRId64 " / %" PRId64 "\nVideo: %s, %d * %d, %s, %.2f ( %.2f )",
            _cur_frames.load(), stream->nb_frames,
            avcodec_get_name(codecpar->codec_id),
            codecpar->width, codecpar->height,
            av_get_pix_fmt_name(static_cast<AVPixelFormat>(codecpar->format)),
            av_q2d(codecpar->framerate), fps
        );
        str.append(video_info);
    }

    if (-1 != _audio_stream_index.load())
    {
        const auto * stream = _ctx->getStreamInfo(_audio_stream_index.load());
        const auto * codecpar = stream->codecpar;
        std::string audio_info;
        xpu_format_string(
            audio_info,
            "\nAudio: %s, %d Hz, %d channels, %s",
            avcodec_get_name(codecpar->codec_id),
            codecpar->sample_rate,
            codecpar->channels,
            av_get_sample_fmt_name(static_cast<AVSampleFormat>(codecpar->format))
        );
        str.append(audio_info);
    }

    return str;
}
