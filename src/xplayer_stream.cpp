#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"
#include "decoder/xplayer_decoder.h"

CXPlayerStream::CXPlayerStream(int index) :
    _index(index)
{
    _pkt_dts = AV_NOPTS_VALUE;
}

CXPlayerStream::~CXPlayerStream()
{
    _index = -1;
}

bool CXPlayerStream::init(const AVCodecParameters * codecpar, const AVRational & timebase)
{
    if (nullptr == codecpar)
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    if (AVMEDIA_TYPE_AUDIO == codecpar->codec_type)
    {
        _max_pkts = static_cast<int>(ceil(static_cast<double>(codecpar->sample_rate) / static_cast<double>(codecpar->frame_size)));
    }
    else if (AVMEDIA_TYPE_VIDEO == codecpar->codec_type)
    {
        _max_pkts = static_cast<int>(ceil(av_q2d(codecpar->framerate)));
    }
    else
    {
        xpu_format_string(_err, "Unsupported codec type: %d", codecpar->codec_type);
        return false;
    }


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

    _timebase = timebase;

    return true;
}

void CXPlayerStream::uninit()
{
    if (nullptr == _codecpar)
        return;

    _running.store(false);
    if (_thr.joinable())
    {
        _thr.join();
    }

    reset();
    avcodec_parameters_free(&_codecpar);
    destroyDecoder();
}

bool CXPlayerStream::send(const AVPacket & pkt, const bool & over)
{
    if (over)
    {
        _demux_over.store(true);
        return true;
    }

    if (_index != pkt.stream_index)
    {
        xpu_format_string(_err, "Stream index mismatch");
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts && AV_NOPTS_VALUE != _pkt_dts && pkt.dts < _pkt_dts)
    {
        xpu_format_string(_err, "Recv non-increasing timecode");
        return false;
    }

    if (AV_NOPTS_VALUE != pkt.dts)
    {
        _pkt_dts = pkt.dts;
    }

    _pkts.push(pkt);
    _demux_over.store(over);

    return true;
}

bool CXPlayerStream::recv(AVFrame & frm, bool & got, bool & over)
{
    AVFrame * tmp = nullptr;
    if (!_frms.pop(tmp))
    {
        got = false;
        over = _demux_over.load();
        return true;
    }

    av_frame_ref(&frm, tmp);
    av_frame_free(&tmp);
    got = true;
    over = false;

    return true;
}

void CXPlayerStream::clear()
{
    _need_reset.store(true);
    while (_running.load() && _need_reset.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(20));
    }
}

bool CXPlayerStream::isFull()
{
    return _max_pkts <= _pkts.size();
}

bool CXPlayerStream::prepare()
{
    if (!createDecoder())
        return false;

    try
    {
        _running.store(true);
        _thr = std::thread{ &CXPlayerStream::decodeThr, this };
    }
    catch (const std::exception & e)
    {
        xpu_format_string(_err, "%s", e.what());
        return false;
    }

    return true;
}

int64_t CXPlayerStream::timestamp(int64_t timecode)
{
    if (AV_NOPTS_VALUE == timecode)
        return AV_NOPTS_VALUE;

    return timecode * _timebase.num * 1000 / _timebase.den;
}

int64_t CXPlayerStream::frameDuration()
{
    return static_cast<int64_t>(1000.0 / av_q2d(_codecpar->framerate));
}

const char * CXPlayerStream::err() const
{
    return _err.c_str();
}

bool CXPlayerStream::createDecoder()
{
    _decoder = std::make_unique<CXPlayerDecoder>();
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
    _decoder = nullptr;
}

void CXPlayerStream::decodeThr()
{
    while (_running.load())
    {
        if (_need_reset.load())
        {
            reset();
            _need_reset.store(false);
        }

        // 无包且未结束
        if (_pkts.empty() && !_demux_over.load())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            continue;
        }

        AVPacket pkt = {};
        bool succ = true;
        if (_pkts.pop(pkt))
        {
            succ = _decoder->send(&pkt);
            av_packet_unref(&pkt);
        }
        else if (!_flushed.load())
        {
            succ = _decoder->send(nullptr);
            _flushed.store(true);
        }

        if (!succ)
        {
            _err = _decoder->err();
            _decode_error.store(true);
            break;
        }

        bool got = false;
        bool over = false;
        do 
        {
            auto * frm = av_frame_alloc();
            if (nullptr == frm)
            {
                xpu_format_string(_err, "av_frame_alloc failed");
                _decode_error.store(true);
                break;
            }

            succ = _decoder->recv(*frm, got, over);
            if (got)
                _frms.push(frm);
            else
                av_frame_free(&frm);

            if (!succ)
            {
                _err = _decoder->err();
                _decode_error.store(true);
                break;
            }

            if (over)
            {
                _decode_over.store(true);
                break;
            }
        } while (got);

        while (_running.load() && !_need_reset.load() && _frms.size() > _max_frms)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(20));
        }
    }
}

void CXPlayerStream::reset()
{
    AVPacket pkt = {};
    while (_pkts.pop(pkt))
    {
        av_packet_unref(&pkt);
    }
    
    AVFrame * frm = nullptr;
    while (_frms.pop(frm))
    {
        av_frame_free(&frm);
    }

    _pkts.clear();
    _frms.clear();
    _pkt_dts = AV_NOPTS_VALUE;
    _decoder->clear();
}
