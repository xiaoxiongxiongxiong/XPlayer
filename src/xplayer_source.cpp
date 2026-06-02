#include "xplayer_source.h"

#include <filesystem>

extern "C" {
#include "libavformat/avformat.h"
}

#include "xplayer_utils.h"
#include "xplayer_demuxer.h"
#include "xplayer_audio_resampler.h"
#include "xplayer_video_rescaler.h"
#include "xplayer_audio_speex.h"
#include "xplayer_audio_render_sdl.h"
#include "xplayer_video_renderer.h"
#include "xplayer_stream.h"

CXPlayerSource::~CXPlayerSource()
{
    uninitConvertor();

    if (nullptr != _audio_renderer)
    {
        _audio_renderer.reset();
        _audio_renderer = nullptr;
    }
}

void CXPlayerSource::setFontPath(const std::string & path)
{
    _font_path = path;
}

void CXPlayerSource::setFontSize(int size)
{
    _font_size = size;
}

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

    uninitFilter();
    uninitRenderer();

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
        if (!si.second->prepare())
        {
            _err = si.second->err();
            return false;
        }
    }

    _wnd_width.store(width);
    _wnd_height.store(height);

    if (!initConvertor())
        return false;

    if (!initFilter())
        return false;

    if (!initRenderer(wnd, width, height))
        return false;

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

void CXPlayerSource::setSpeed(XPLAYER_SPEED_MODE speed)
{
    if (_speed_mode.load() != speed)
    {
        _speed_mode.store(speed);
        processSpeed(speed);
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

XPLAYER_PIXEL_FORMAT_TYPE CXPlayerSource::getPixelFormat(int format)
{
    XPLAYER_PIXEL_FORMAT_TYPE type = XPLAYER_PIXEL_FORMAT_NONE;

    switch (format)
    {
    case AV_PIX_FMT_YUV420P:
        type = XPLAYER_PIXEL_FORMAT_YUV420P;
        break;
    case AV_PIX_FMT_YUYV422:
        type = XPLAYER_PIXEL_FORMAT_YUY2;
        break;
    case AV_PIX_FMT_UYVY422:
        type = XPLAYER_PIXEL_FORMAT_UYVY;
        break;
    case AV_PIX_FMT_YVYU422:
        type = XPLAYER_PIXEL_FORMAT_YVYU;
        break;
    case AV_PIX_FMT_YUV420P10:
        type = XPLAYER_PIXEL_FORMAT_YUV420P10;
        break;
    case AV_PIX_FMT_NV12:
        type = XPLAYER_PIXEL_FORMAT_NV12;
        break;
    case AV_PIX_FMT_NV21:
        type = XPLAYER_PIXEL_FORMAT_NV21;
        break;
    case AV_PIX_FMT_P010:
        type = XPLAYER_PIXEL_FORMAT_P010;
        break;
    default:
        break;
    }

    return type;
}

bool CXPlayerSource::initRescaler(int format, int width, int height)
{
    if (nullptr != _video_rescaler)
        return true;

    _video_rescaler = std::make_shared<CXPlayerVideoRescaler>();
    if (nullptr == _video_rescaler)
    {
        xpu_format_string(_err, "Create CXPlayerVideoRescaler instance failed");
        return false;
    }

    auto pix_fmt = static_cast<AVPixelFormat>(format);
    auto tmp = getPixelFormat(format);
    if (!_video_renderer->supportedPixelFormat(tmp))
    {
        pix_fmt = AV_PIX_FMT_YUV420P;
        tmp = XPLAYER_PIXEL_FORMAT_YUV420P;
    }

    CXPlayerVideoInfo src(static_cast<AVPixelFormat>(format), width, height);
    CXPlayerVideoInfo dst(pix_fmt, width, height);
    if (!_video_rescaler->create(src, dst))
    {
        _err = _video_rescaler->err();
        _video_rescaler.reset();
        _video_rescaler = nullptr;
        return false;
    }

    _pixel_format.store(tmp);

    return true;
}

void CXPlayerSource::uninitRescaler()
{
    if (nullptr == _video_rescaler)
        return;

    _video_rescaler->destroy();
    _video_rescaler.reset();
    _video_rescaler = nullptr;
}

bool CXPlayerSource::initConvertor()
{
    auto * avs = _ctx->getStreamInfo(_audio_stream_index.load());
    auto * codecpar = avs ? avs->codecpar : nullptr;
    if (nullptr == _audio_renderer)
    {
        _audio_resampler = std::make_shared<CXPlayerAudioResampler>();
        if (nullptr == _audio_resampler)
        {
            xpu_format_string(_err, "Create CXPlayerAudioResampler instance failed");
            return false;
        }
    }

    if (codecpar)
    {
        _audio_resampler->destroy();
        CXPlayerAudioInfo src(codecpar->ch_layout, static_cast<AVSampleFormat>(codecpar->format), codecpar->sample_rate);
        AVChannelLayout dst_layout{};
        av_channel_layout_default(&dst_layout, 2);
        CXPlayerAudioInfo dst(dst_layout, AV_SAMPLE_FMT_S16, codecpar->sample_rate);
        if (!_audio_resampler->create(src, dst, codecpar->frame_size))
        {
            _err = _audio_resampler->err();
            return false;
        }
    }

    return true;
}

void CXPlayerSource::uninitConvertor()
{
    if (_audio_resampler)
    {
        _audio_resampler->destroy();
        _audio_resampler.reset();
        _audio_resampler = nullptr;
    }


}

bool CXPlayerSource::initFilter()
{
    if (_audio_stream_index.load() < 0)
        return true;

    auto * stream = _ctx->getStreamInfo(_audio_stream_index.load());
    if (nullptr == stream)
    {
        xpu_format_string(_err, "Get stream info by index '%d' failed", _audio_stream_index.load());
        return false;
    }

    auto * codec_par = stream->codecpar;

    _audio_speex = std::make_shared<CXPlayerAudioSpeex>();
    if (nullptr == _audio_speex)
    {
        xpu_format_string(_err, "Create CXPlayerAudioSpeex instance failed");
        return false;
    }

    if (!_audio_speex->create(codec_par->channels, codec_par->sample_rate, codec_par->frame_size))
    {
        _err = _audio_speex->err();
        _audio_speex.reset();
        _audio_speex = nullptr;
        return false;
    }

    _audio_speex->setSpeed(_speed.load());

    return true;
}

void CXPlayerSource::uninitFilter()
{
    if (nullptr != _audio_speex)
    {
        _audio_speex->destroy();
        _audio_speex.reset();
        _audio_speex = nullptr;
    }
}

bool CXPlayerSource::initRenderer(const void * wnd, int width, int height)
{
    auto * avs = _ctx->getStreamInfo(_audio_stream_index.load());
    auto * codecpar = avs ? avs->codecpar : nullptr;
    if (nullptr == _audio_renderer)
    {
        _audio_renderer = std::make_shared<CXPlayerAudioRender>();
        if (nullptr == _audio_renderer)
        {
            xpu_format_string(_err, "Create CXPlayerAudioRender instance failed");
            return false;
        }
    }

    if (nullptr != codecpar && 
        !_audio_renderer->create(codecpar->sample_rate, codecpar->channels, codecpar->frame_size, _volume.load()))
    {
        _err = _audio_renderer->err();
        return false;
    }

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

void CXPlayerSource::uninitRenderer()
{
    if (_video_renderer)
    {
        _video_renderer->destroy();
        CXPlayerVideoRendererFactory::destroy(_video_renderer);
    }

    if (_audio_renderer)
    {
        _audio_renderer->destroy();
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

        if (_audio_stream_index.load() == pkt.stream_index)
        {
            stream->send(pkt);
            _audio_cond.notify_one();
        }
        else if (_video_stream_index.load() == pkt.stream_index)
        {
            stream->send(pkt);
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
                audioClear(stream_index);
                _audio_clock.store(-1LL);
                mute_flag = true;
            }
            continue;
        }

        if (stream_index != _audio_stream_index.load())
        {
            processAudioStream(stream_index);
        }

        stream_index = _audio_stream_index.load();
        const auto & stream = _streams[stream_index];

        if (_is_skip.load())
        {
            if (!_audio_skip_over)
            {
                _cur_pos_ms.store(-1LL);
                audioClear(stream_index);
                mute_flag = true;
                _audio_skip_over.store(true);
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
            audioMultiSpeedRenderer(buff, 4096, true);
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

        uint8_t * data = nullptr;
        int len = 0;
        if (!_audio_resampler->resampler(&frm, &data, &len))
        {
            _err = _audio_resampler->err();
            av_frame_unref(&frm);
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }
        av_frame_unref(&frm);

        if (mute_flag)
        {
            _audio_renderer->mute(false);
            mute_flag = false;
        }

        if (_speed_changed.load())
        {
            _audio_speex->setSpeed(_speed.load());
            _speed_changed.store(false);
        }

        if (XPLAYER_SPEED_NORMAL == _speed_mode.load())
        {
            _audio_renderer->renderer(data, len);
            continue;
        }

        _audio_speex->send(data, len);
        audioMultiSpeedRenderer(buff, len);
    }

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
                flag = true;
            }
            continue;
        }

        stream_index = _video_stream_index.load();
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

        if (!initRescaler(frm.format, frm.width, frm.height))
        {
            av_frame_unref(&frm);
            _state.store(XPLAYER_STATE_ERROR);
            break;
        }

        uint8_t * data[AV_NUM_DATA_POINTERS]{};
        int linesize[AV_NUM_DATA_POINTERS]{};

        if (!_video_rescaler->rescale(&frm, data, linesize))
        {
            _err = _video_rescaler->err();
            av_frame_unref(&frm);
            _state.store(XPLAYER_STATE_ERROR);
            break;
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
        _video_renderer->renderer(frm.width, frm.height, _pixel_format.load(), data, linesize, str);
        _cur_frames++;
        flag = false;
        av_frame_unref(&frm);
    }

    _video_renderer->clear();
    _last_ts = 0;
    _cur_frames.store(0);
    _last_frames.store(0);
}

void CXPlayerSource::audioMultiSpeedRenderer(std::vector<std::uint8_t> & buff, int bytes, const bool & over)
{
    if (XPLAYER_SPEED_NORMAL == _speed_mode.load())
        return;

    if (over)
        _audio_speex->flush();

    std::vector<uint8_t> cache(bytes);
    int len = _audio_speex->recv(cache.data(), bytes);
    while (len > 0)
    {
        buff.insert(buff.end(), cache.begin(), cache.begin() + len);
        if (buff.size() >= bytes)
        {
            _audio_renderer->renderer(buff.data(), bytes);
            buff.erase(buff.begin(), buff.begin() + bytes);
        }
        len = _audio_speex->recv(cache.data(), bytes);
    }
    
    if (over && !buff.empty())
    {
        _audio_renderer->renderer(buff.data(), buff.size());
        buff.clear();
    }
}

void CXPlayerSource::audioClear(int stream_index)
{
    _audio_renderer->mute(true);
    _streams[stream_index]->clear();
    _audio_speex->clear();
}

void CXPlayerSource::processSpeed(XPLAYER_SPEED_MODE mode)
{
    switch (mode)
    {
    case XPLAYER_SPEED_NORMAL:
        _speed.store(1.0);
        break;
    case XPLAYER_SPEED_ONE_QUATER:
        _speed.store(0.25);
        break;
    case XPLAYER_SPEED_ONE_HALF:
        _speed.store(0.5);
        break;
    case XPLAYER_SPEED_DOUBLE:
        _speed.store(2.0);
        break;
    case XPLAYER_SPEED_QUADRUPLE:
        _speed.store(4.0);
        break;
    default:
        _speed.store(1.0);
        break;
    }
}

bool CXPlayerSource::processAudioStream(int stream_index)
{
    _streams[stream_index]->clear();

    const auto * stream = _ctx->getStreamInfo(_audio_stream_index.load());
    if (nullptr == stream)
    {
        xpu_format_string(_err, "Find audio stream by index '%d' failed", _audio_stream_index.load());
        return false;
    }

    const auto * codecpar = stream->codecpar;
    _audio_speex->clear();
    _audio_speex->update(codecpar->channels, codecpar->sample_rate, codecpar->frame_size);

    return true;
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
            av_q2d(codecpar->framerate), calcFrameRate()
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

