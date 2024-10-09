#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "utils/xplayer_utils.h"

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

}

void CXPlayerStream::enable(const bool flag)
{
}

bool CXPlayerStream::push(const AVPacket & pkt, bool over)
{
    if (over)
    {
        _is_over.store(over);
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

    while (_running && _pkts.size() > 200)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    _pkts.push(pkt);

    return true;
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

void CXPlayerStream::decodeThr()
{

}
