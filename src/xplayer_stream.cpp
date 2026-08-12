#include "xplayer_stream.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "xplayer_utils.h"
#include "xplayer_config.h"
#include "xplayer_filter_bsf.h"
#include "xplayer_decoder.h"
#include "xplayer_video_rescaler.h"
#include "xplayer_audio_resampler.h"
#include "xplayer_audio_speex.h"

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

    const auto cache_duration = CXPlayerConfig::uniqueInstance().get<xplayer_cache_duration_t>();
    const auto radio = static_cast<float>(cache_duration) / 1000.0f;

    if (AVMEDIA_TYPE_AUDIO == codecpar->codec_type)
    {
        auto pkts = static_cast<float>(codecpar->sample_rate) / static_cast<float>(codecpar->frame_size);
        _max_pkts = static_cast<int>(ceil(radio * pkts));
    }
    else if (AVMEDIA_TYPE_VIDEO == codecpar->codec_type)
    {
        auto pkts = static_cast<float>(av_q2d(codecpar->framerate));
        _max_pkts = static_cast<int>(ceil(radio * pkts));
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
    _max_frms = CXPlayerConfig::uniqueInstance().get<xplayer_cache_frame_t>();

    return true;
}

void CXPlayerStream::uninit()
{
    if (nullptr == _codecpar)
        return;

    _running.store(false);
    if (_thr.joinable())
        _thr.join();

    reset();
    avcodec_parameters_free(&_codecpar);
    destroyDecoder();
    destroyFilter();
    uninitAudioSpeex();
    uninitResampler();
    uninitRescaler();
}

void CXPlayerStream::setSpeed(double speed)
{
    if (_speex)
        _speex->setSpeed(speed);
    _speed.store(speed);
}

bool CXPlayerStream::send(AVPacket & pkt, const bool & over)
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

    _demux_over.store(over);

    if (AV_NOPTS_VALUE != pkt.dts)
    {
        _pkt_dts = pkt.dts;
    }

    if (!_bsf)
    {
        _pkts.push(pkt);
        return true;
    }

    bool succ = _bsf->send(pkt);
    av_packet_unref(&pkt);
    if (!succ)
    {
        _err = _bsf->err();
        return false;
    }

    bool got = false;
    do 
    {
        AVPacket tmp = {};
        if (!_bsf->recv(tmp, got))
        {
            _err = _bsf->err();
            break;
        }
        if (got)
        {
            _pkts.push(tmp);
        }
    } while (got);

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

bool CXPlayerStream::prepare(const std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats)
{
    _formats = formats;

    if (!createFilter())
        return false;

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

XPLAYER_PIXEL_FORMAT_TYPE CXPlayerStream::getPixelFormat()
{
    return _pix_format.load();
}

const char * CXPlayerStream::err() const
{
    return _err.c_str();
}

bool CXPlayerStream::createFilter()
{
    _bsf = std::make_unique<CXPlayerFilterBsf>();
    if (nullptr == _bsf)
    {
        xpu_format_string(_err, "Create filter failed");
        return false;
    }

    int ret = _bsf->create(_codecpar);
    if (1 != ret)
    {
        _err = _bsf->err();
        _bsf.reset();
        _bsf = nullptr;
        return 0 == ret;
    }

    return true;
}

void CXPlayerStream::destroyFilter()
{
    if (nullptr != _bsf)
    {
        _bsf->destroy();
        _bsf.reset();
        _bsf = nullptr;
    }
}

bool CXPlayerStream::createDecoder()
{
    // 视频优先使用硬件解码
    if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
        _decoder = CXPlayerDecoderFactory::create(XPLAYER_DECODER_HARDWARE, _codecpar);
    if (nullptr != _decoder)
        return true;

    _decoder = CXPlayerDecoderFactory::create(XPLAYER_DECODER_SOFTWARE, _codecpar);
    if (nullptr == _decoder)
    {
        xpu_format_string(_err, "Create software decoder failed");
        return false;
    }

    return true;
}

void CXPlayerStream::destroyDecoder()
{
    if (nullptr == _decoder)
        return;

    CXPlayerDecoderFactory::destroy(_decoder);
    _decoder = nullptr;
}

bool CXPlayerStream::initRescaler(int format, int width, int height)
{
    if (nullptr != _rescaler)
        return true;

    if (_formats.empty())
    {
        xpu_format_string(_err, "Supported pixel format is empty");
        return false;
    }

    _rescaler = std::make_unique<CXPlayerVideoRescaler>();
    if (nullptr == _rescaler)
    {
        xpu_format_string(_err, "Create CXPlayerVideoRescaler instance failed");
        return false;
    }

    auto pix_fmt = static_cast<AVPixelFormat>(format);
    auto tmp = xpu_f2x(format);
    auto found = std::find(_formats.begin(), _formats.end(), tmp);
    if (_formats.end() == found)
    {
        tmp = _formats[0];
        pix_fmt = static_cast<AVPixelFormat>(xpu_x2f(tmp));
    }
    _pix_format.store(tmp);

    CXPlayerVideoInfo src(static_cast<AVPixelFormat>(format), width, height);
    CXPlayerVideoInfo dst(pix_fmt, width, height);
    if (!_rescaler->create(src, dst))
    {
        _err = _rescaler->err();
        _rescaler.reset();
        _rescaler = nullptr;
        return false;
    }

    return true;
}

void CXPlayerStream::uninitRescaler()
{
    if (nullptr == _rescaler)
        return;

    _rescaler->destroy();
    _rescaler.reset();
    _rescaler = nullptr;
}

bool CXPlayerStream::initResampler(const AVChannelLayout & layout, int format, int sample_rate)
{
    if (nullptr == _codecpar)
    {
        xpu_format_string(_err, "Invalid params");
        return false;
    }

    if (nullptr != _resampler)
        return true;

    _resampler = std::make_unique<CXPlayerAudioResampler>();
    if (nullptr == _resampler)
    {
        xpu_format_string(_err, "Create CXPlayerAudioResampler instance failed");
        return false;
    }

    CXPlayerAudioInfo src(layout, static_cast<AVSampleFormat>(format), sample_rate);
    AVChannelLayout dst_layout{};
    av_channel_layout_default(&dst_layout, 2);
    CXPlayerAudioInfo dst(dst_layout, AV_SAMPLE_FMT_S16, sample_rate);
    if (!_resampler->create(src, dst, _codecpar->frame_size))
    {
        _err = _resampler->err();
        _resampler.reset();
        _resampler = nullptr;
        return false;
    }

    return true;
}

void CXPlayerStream::uninitResampler()
{
    if (nullptr == _resampler)
        return;

    _resampler->destroy();
    _resampler.reset();
    _resampler = nullptr;
}

bool CXPlayerStream::initAudioSpeex(int channels, int sample_rate, int frame_size)
{
    if (_speex)
    {
        return true;
    }

    _speex = std::make_unique<CXPlayerAudioSpeex>();
    if (nullptr == _speex)
    {
        xpu_format_string(_err, "Create CXPlayerAudioSpeex instance failed");
        return false;
    }

    if (!_speex->create(channels, sample_rate, frame_size))
    {
        _err = _speex->err();
        _speex.reset();
        _speex = nullptr;
        return false;
    }

    _speex->setSpeed(_speed.load());

    return true;
}

void CXPlayerStream::uninitAudioSpeex()
{
    if (nullptr != _speex)
    {
        _speex->destroy();
        _speex.reset();
        _speex = nullptr;
    }
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
            AVFrame frm = {};
            succ = _decoder->recv(frm, got, over);
            if (got)
            {
                if (AVMEDIA_TYPE_VIDEO == _codecpar->codec_type)
                    processVideoFrame(frm);
                else if (AVMEDIA_TYPE_AUDIO == _codecpar->codec_type)
                    processAudioFrame(frm);
            }
            av_frame_unref(&frm);

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

bool CXPlayerStream::processVideoFrame(const AVFrame & src)
{
    if (!initRescaler(src.format, src.width, src.height))
        return false;

    uint8_t * data[AV_NUM_DATA_POINTERS]{};
    int linesize[AV_NUM_DATA_POINTERS]{};
    if (!_rescaler->rescale(&src, data, linesize))
    {
        _err = _rescaler->err();
        return false;
    }

    AVFrame * frm = av_frame_alloc();
    if (nullptr == frm)
    {
        xpu_format_string(_err, "No enough memory");
        return false;
    }

    frm->width = src.width;
    frm->height = src.height;
    frm->format = xpu_x2f(_pix_format.load());
    int ret = av_frame_copy_props(frm, &src);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        av_frame_free(&frm);
        return false;
    }

    ret = av_frame_get_buffer(frm, 1);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        av_frame_free(&frm);
        return false;
    }

    av_image_copy(frm->data, frm->linesize, data, linesize, static_cast<AVPixelFormat>(frm->format), src.width, src.height);
    _frms.push(frm);

    return true;
}

bool CXPlayerStream::processAudioFrame(const AVFrame & src)
{
    if (!initResampler(src.ch_layout, src.format, src.sample_rate))
        return false;

    uint8_t * data = nullptr;
    int len = 0;
    if (!_resampler->resample(&src, &data, &len))
    {
        _err = _resampler->err();
        return false;
    }

    if (1.0 == _speed.load())
    {
        auto * frm = makeAudioFrame(src, data, len);
        if (nullptr == frm)
            return false;

        _frms.push(frm);
        return true;
    }

    if (!initAudioSpeex(2, src.sample_rate, _codecpar->frame_size))
        return false;

    if (!_speex->send(data, len))
    {
        _err = _speex->err();
        return false;
    }

    int bytes = 0;
    do 
    {
        std::vector<uint8_t> cache(len);
        bytes = _speex->recv(cache.data(), len);
        if (bytes <= 0)
            continue;

        _cache.insert(_cache.end(), cache.begin(), cache.begin() + bytes);
        if (_cache.size() < len)
            continue;

        auto * frm = makeAudioFrame(src, _cache.data(), len);
        if (nullptr == frm)
            return false;

        _frms.push(frm);
        _cache.erase(_cache.begin(), _cache.begin() + len);
    } while (bytes > 0);

    return true;
}

AVFrame * CXPlayerStream::makeAudioFrame(const AVFrame & src, const uint8_t * data, int len)
{
    AVFrame * frm = av_frame_alloc();
    if (nullptr == frm)
    {
        xpu_format_string(_err, "No enough memory");
        return false;
    }

    frm->channels = 2;
    frm->channel_layout = AV_CH_LAYOUT_STEREO;
    frm->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_default(&frm->ch_layout, 2);
    frm->sample_rate = src.sample_rate;
    frm->nb_samples = src.nb_samples;
    int ret = av_frame_copy_props(frm, &src);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        av_frame_free(&frm);
        return false;
    }

    ret = av_frame_get_buffer(frm, 0);
    if (0 != ret)
    {
        char buff[AV_ERROR_MAX_STRING_SIZE] = {};
        av_make_error_string(buff, AV_ERROR_MAX_STRING_SIZE, ret);
        xpu_format_string(_err, "%s", buff);
        av_frame_free(&frm);
        return false;
    }

    memcpy(frm->data[0], data, len);
    frm->linesize[0] = len;

    return frm;
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
    if (_speex)
        _speex->clear();
}
