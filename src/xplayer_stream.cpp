#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "decoder/xplayer_decoder.h"
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
    _active.store(flag);
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

bool CXPlayerStream::send(bool & over)
{
    if (_pkts.empty())
    {
        if (_demux_over.load())
        {
            over = true;
            return true;
        }
        return false;
    }

    return true;
}

bool CXPlayerStream::recv(AVFrame & frm, bool & got, bool & over)
{
    //if (_pkts.empty())
    //{
    //    if (_demux_over.load())
    //        break;
    //    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    //    continue;
    //}

    //AVPacket pkt = {};
    //if (!_pkts.pop(pkt))
    //    continue;

    //if (XPLAYER_DECODE_IDLE == _state.load())
    //{
    //    av_packet_unref(&pkt);
    //    should_reopen_decoder = true;
    //    continue;
    //}

    //if (!XPLAYER_DECODE_RUNNING == _state.load())
    //    continue;

    //if (should_reopen_decoder && !reopenDecoder())
    //    break;
    //should_reopen_decoder = false;

    //bool succ = _decoder->send(&pkt);
    //if (!succ)
    //{
    //    xpu_format_string(_err, "%s", _decoder->err());
    //    _state.store(XPLAYER_DECODE_FAIL);
    //    break;
    //}

    //do
    //{
    //    AVFrame frm = {};
    //    bool got = false;
    //    bool over = false;
    //    succ = _decoder->recv(frm, got, over);
    //    if (!succ)
    //    {
    //        xpu_format_string(_err, "%s", _decoder->err());
    //        _state.store(XPLAYER_DECODE_FAIL);
    //        break;
    //    }

    //    if (!got)
    //        break;

    //    if (over)
    //    {
    //        _demux_over.store(true);
    //        _state.store(XPLAYER_DECODE_SUCC);
    //        break;
    //    }

    //    // TODO 渲染
    //    if (AVMEDIA_TYPE_AUDIO == _codecpar->codec_type)
    //    {

    //    }
    //    else if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
    //    {

    //    }
    //    av_frame_unref(&frm);
    //} while (true);

    //if (XPLAYER_DECODE_FAIL == _state.load() || XPLAYER_DECODE_SUCC == _state.load())
    //    break;

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
    _decoder = std::make_shared<CXPlayerDecoder>();
    if (nullptr == _decoder)
    {
        xpu_format_string(_err, "Create decoder failed");
        return false;
    }

    if (!_decoder->create(_codecpar))
    {
        xpu_format_string(_err, "%s", _decoder->err());
        _decoder.reset();
        return false;
    }

    return true;
}

void CXPlayerStream::destroyDecoder()
{
    if (nullptr == _decoder)
        return;

    _decoder->destroy();
    _decoder.reset();
}

bool CXPlayerStream::reopenDecoder()
{
    if (nullptr == _decoder)
        return false;
    return _decoder->reopen();
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

        bool succ = _decoder->send(&pkt);
        if (!succ)
        {
            xpu_format_string(_err, "%s", _decoder->err());
            _state.store(XPLAYER_DECODE_FAIL);
            break;
        }

        do
        {
            AVFrame frm = {};
            bool got = false;
            bool over = false;
            succ = _decoder->recv(frm, got, over);
            if (!succ)
            {
                xpu_format_string(_err, "%s", _decoder->err());
                _state.store(XPLAYER_DECODE_FAIL);
                break;
            }

            if (!got)
                break;

            if (over)
            {
                _demux_over.store(true);
                _state.store(XPLAYER_DECODE_SUCC);
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
