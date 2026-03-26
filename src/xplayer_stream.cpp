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
        _pkt_dts = pkt.dts;

    _pkts.push(pkt);
    _demux_over.store(over);

    return true;
}

bool CXPlayerStream::recv(AVFrame & frm, bool & got, bool & over)
{
    got = false;
    over = false;

    if (_pkts.empty() && !_demux_over.load())
    {
        return true;
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
        return false;
    }

    succ = _decoder->recv(frm, got, over);
    if (!succ)
    {
        _err = _decoder->err();
        return false;
    }

    return true;
}

void CXPlayerStream::clear()
{
    _pkts.clear();
    _pkt_dts = AV_NOPTS_VALUE;
    if (nullptr != _decoder)
    {
        _decoder->clear();
    }
}

bool CXPlayerStream::isFull()
{
    return _max_pkts <= _pkts.size();
}

bool CXPlayerStream::prepare()
{
    return createDecoder();
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
    _decoder = nullptr;
}
