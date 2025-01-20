#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "renderer/xplayer_video_render_sdl.h"
#include "renderer/xplayer_audio_render_sdl.h"

CXPlayerStream::CXPlayerStream(int index) :
    _index(index)
{
    _latest_pkt_dts = AV_NOPTS_VALUE;
}

bool CXPlayerStream::create(const AVCodecParameters * codecpar)
{
    if (nullptr == codecpar)
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    _state.store(XPLAYER_DECODE_PREPARE);

    _codecpar = avcodec_parameters_alloc();
    if (nullptr == _codecpar)
    {
        xpu_format_string(_err, "avcodec_parameters_alloc failed");
        return false;
    }

    int ret = avcodec_parameters_copy(_codecpar, codecpar);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        avcodec_parameters_free(&_codecpar);
        return false;
    }

    if (!createDecoder())
    {
        avcodec_parameters_free(&_codecpar);
        return false;
    }

    _state.store(XPLAYER_DECODE_READY);

    try
    {
        _running.store(true);
        _thr = std::thread{ &CXPlayerStream::decodeThr, this };
    }
    catch (const std::exception & e)
    {
        xpu_format_string(_err, "%s", e.what());
        destroy();
        return false;
    }
    catch (...)
    {
        xpu_format_string(_err, "unknown exception");
        destroy();
        return false;
    }

    return true;
}

void CXPlayerStream::destroy()
{
    if (nullptr == _codecpar)
        return;

    _running.store(false);
    if (_thr.joinable())
        _thr.join();

    while (!_pkts.empty())
    {
        AVPacket pkt = {};
        _pkts.pop(pkt);
        av_packet_unref(&pkt);
    }

    avcodec_parameters_free(&_codecpar);
    destroyDecoder();

    if (XPLAYER_DECODE_SUCC != _state.load() && XPLAYER_DECODE_FAIL != _state.load())
        _state.store(XPLAYER_DECODE_SUCC);
}

void CXPlayerStream::enable(const bool flag)
{
    if (flag)
        _state.store(XPLAYER_DECODE_RUNNING);
    else
        _state.store(XPLAYER_DECODE_IDLE);
}

void CXPlayerStream::flush()
{
    _reset.store(true);
}

bool CXPlayerStream::push(const AVPacket & pkt, bool over)
{
    if (over)
    {
        _demux_over.store(over);
        return true;
    }

    if (_index != pkt.stream_index)
    {
        xpu_format_string(_err, "Invalid stream index '%d'", pkt.stream_index);
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts && AV_NOPTS_VALUE != _latest_pkt_dts && _latest_pkt_dts >= pkt.dts)
    {
        xpu_format_string(_err, "Invalid dts: %" PRId64 ", less than last dts %" PRId64 ", stream index is %d",
                          pkt.dts, _latest_pkt_dts, pkt.stream_index);
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts)
        _latest_pkt_dts = pkt.dts;

    while (_running.load() && _pkts.size() > 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    _pkts.push(pkt);

    return true;
}

XPLAYER_DECODE_STATE CXPlayerStream::state() const
{
    return _state.load();
}

const char * CXPlayerStream::err() const
{
    return _err.c_str();
}

bool CXPlayerStream::createDecoder()
{
    if (nullptr == _codecpar)
    {
        xpu_format_string(_err, "_codecpar is nullptr");
        return false;
    }

    auto * codec = avcodec_find_decoder(_codecpar->codec_id);
    if (nullptr == codec)
    {
        xpu_format_string(_err, "Find codec by id '%d' failed", _codecpar->codec_id);
        return false;
    }

    _codec = avcodec_alloc_context3(codec);
    if (nullptr == _codec)
    {
        xpu_format_string(_err, "avcodec_alloc_context3 failed");
        return false;
    }

    int ret = avcodec_parameters_to_context(_codec, _codecpar);
    if (ret < 0)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        destroyDecoder();
        return false;
    }

    ret = avcodec_open2(_codec, codec, nullptr);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        destroyDecoder();
        return false;
    }

    return true;
}

void CXPlayerStream::destroyDecoder()
{
    if (nullptr == _codec)
        return;

    avcodec_close(_codec);
    avcodec_free_context(&_codec);
}

bool CXPlayerStream::reopenDecoder()
{
    destroyDecoder();
    return createDecoder();
}

bool CXPlayerStream::createRender(const void * wnd, int width, int height)
{
    return true;
}

void CXPlayerStream::destroyRender()
{
}

void CXPlayerStream::reset()
{
    if (!_reset.load())
        return;

    while (!_pkts.empty())
    {
        AVPacket pkt = {};
        _pkts.pop(pkt);
        av_packet_unref(&pkt);
    }

    reopenDecoder();
    _reset.store(false);
}

void CXPlayerStream::decodeThr()
{
    bool should_reopen_decoder = false;
    _state.store(XPLAYER_DECODE_IDLE);

    while (_running.load())
    {
        reset();

        if (_pkts.empty())
        {
            if (_demux_over.load())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        AVPacket pkt = {};
        if (!_pkts.pop(pkt))
            continue;

        if (XPLAYER_DECODE_IDLE == _state.load())
        {
            av_packet_unref(&pkt);
            should_reopen_decoder = true;
            continue;
        }

        if (!XPLAYER_DECODE_RUNNING == _state.load())
            continue;

        if (should_reopen_decoder && !reopenDecoder())
            break;
        should_reopen_decoder = false;

        int ret = avcodec_send_packet(_codec, &pkt);
        av_packet_unref(&pkt);
        if (0 != ret)
        {
            if (AVERROR_EOF == ret)
            {
                _demux_over.store(true);
                _state.store(XPLAYER_DECODE_SUCC);
            }
            else
            {
                char buff[AV_ERROR_MAX_STRING_SIZE] = {};
                av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
                xpu_format_string(_err, "%s", buff);
                _state.store(XPLAYER_DECODE_FAIL);
            }
            break;
        }

        do 
        {
            AVFrame frm = {};
            ret = avcodec_receive_frame(_codec, &frm);
            if (0 != ret)
            {
                if (AVERROR_EOF == ret)
                {
                    _demux_over.store(true);
                    _state.store(XPLAYER_DECODE_SUCC);
                }
                else if (AVERROR(EAGAIN) == ret)
                    break;
                else
                {
                    char buff[AV_ERROR_MAX_STRING_SIZE] = {};
                    av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
                    xpu_format_string(_err, "%s", buff);
                    _state.store(XPLAYER_DECODE_FAIL);
                }
                break;
            }

            // TODO 渲染
            if (AVMEDIA_TYPE_AUDIO == _codecpar->codec_type)
            {

            }
            else if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
            {

            }
            av_frame_unref(&frm);
        } while (true);

        if (XPLAYER_DECODE_FAIL == _state.load() || XPLAYER_DECODE_SUCC == _state.load())
            break;
    }
}
