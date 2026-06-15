#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <cstdbool>
#include <string>
#include <atomic>
#include <memory>
#include <thread>

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/packet.h"
}
#include "xplayer_queue.h"
#include "xplayer_definitions.h"

typedef struct AVCodecParameters AVCodecParameters;
class ICXPlayerDecoder;
class CXPlayerFilterBsf;
class CXPlayerVideoRescaler;
class CXPlayerAudioResampler;
class CXPlayerAudioSpeex;

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream();

    // 创建
    bool init(const AVCodecParameters * codec_par, const AVRational & timebase);
    // 销毁
    void uninit();

    // 发包
    bool send(AVPacket & pkt, const bool & over = false);
    // 收帧
    bool recv(AVFrame & frm, bool & got, bool & over);

    // 清空缓冲区和解码器内部缓冲
    void clear();
    // 缓冲区是否已满
    bool isFull();

    // 准备
    bool prepare(const std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats);

    // 时间戳
    int64_t timestamp(int64_t timecode);

    // 单帧时长
    int64_t frameDuration();

    // 获取最终的像素格式
    XPLAYER_PIXEL_FORMAT_TYPE getPixelFormat();

    // 错误信息
    const char * err() const;

private:
    // 创建过滤器
    bool createFilter();
    // 销毁过滤器
    void destroyFilter();

    // 创建解码器
    bool createDecoder();
    // 销毁解码器
    void destroyDecoder();

    // 初始化画幅转换器
    bool initRescaler(int format, int width, int height);
    // 销毁画幅转换器
    void uninitRescaler();

    // 初始化重采样器
    bool initResampler(const AVChannelLayout & layout, int format, int sample_rate);
    // 销毁重采样器
    void uninitResampler();

    // 初始化音频倍速过滤器
    bool initAudioSpeex(int channels, int sample_rate, int frame_size);
    // 销毁音频倍速过滤器
    void uninitAudioSpeex();

    // 解码线程
    void decodeThr();

    // 处理视频帧
    bool processVideoFrame(const AVFrame & src);

    // 处理音频帧
    bool processAudioFrame(const AVFrame & src);

    // 重置
    void reset();

private:
    // 索引
    int _index = -1;

    // flush
    std::atomic_bool _flushed = { false };
    // 是否已结束
    std::atomic_bool _demux_over = { false };
    // 是否解码结束
    std::atomic_bool _decode_over = { false };
    // 是否解码错误
    std::atomic_bool _decode_error = { false };
    // 是否需要重置
    std::atomic_bool _need_reset = { false };

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 时间基
    AVRational _timebase = { 0,1 };

    // 支持的像素格式集合
    std::vector<XPLAYER_PIXEL_FORMAT_TYPE> _formats;

    // 解码器
    ICXPlayerDecoder * _decoder = nullptr;
    std::unique_ptr<CXPlayerFilterBsf> _bsf = nullptr;

    // 画幅转换器
    std::unique_ptr<CXPlayerVideoRescaler> _rescaler = nullptr;
    // 最终像素格式
    std::atomic<XPLAYER_PIXEL_FORMAT_TYPE> _pix_format = { XPLAYER_PIXEL_FORMAT_NONE };
    // 重采样器
    std::unique_ptr<CXPlayerAudioResampler> _resampler = nullptr;

    // 音频倍速过滤器
    std::unique_ptr<CXPlayerAudioSpeex> _speex = nullptr;

    // 队列长度上限
    int _max_pkts = 0;
    // 最后一包时码
    int64_t _pkt_dts = 0;
    // 数据包队列
    CXPlayerQueue<AVPacket> _pkts;

    // 缓冲帧个数
    int _max_frms = 5;
    // 解码后数据
    CXPlayerQueue<AVFrame *> _frms;

    // 运行标记
    std::atomic_bool _running = { false };
    // 解码线程
    std::thread _thr;

    // 错误信息
    std::string _err;
};

#endif
