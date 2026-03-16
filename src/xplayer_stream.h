#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <cstdbool>
#include <string>
#include <atomic>
#include <memory>

extern "C" {
#include "libavcodec/packet.h"
}
#include "xplayer_queue.h"

typedef struct AVCodecParameters AVCodecParameters;
class CXPlayerDecoder;
class CXPlayerAudioResampler;
class CXPlayerVideoRescaler;
class CXPlayerVideoRenderSDL;
class CXPlayerAudioRender;

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream() = default;

    // 创建
    bool init(const AVCodecParameters * codec_par, const AVRational & timebase);
    // 销毁
    void uninit();

    // 
    bool pushPacket(const AVPacket & pkt);
    // 
    bool popPacket(AVPacket & pkt);
    // 清空缓冲区
    void clearPackets();
    // 缓冲区是否已满
    bool isCacheFull();

    // 准备
    bool setup(const void * wnd, int width, int height);

    // 时间戳
    int64_t timestamp(int64_t timecode);

    // 单帧时长
    int64_t frameDuration();

    // 错误信息
    const char * err() const;

private:
    // 创建解码器
    bool createDecoder();
    // 销毁解码器
    void destroyDecoder();

    // 创建转换器
    bool createConvertor(int width, int height);
    // 销毁转换器
    void destroyConvertor();

    // 创建渲染器
    bool createRenderer(const void * wnd, int width, int height);
    // 销毁渲染器
    void destroyRenderer();

public:
    // 解码器
    std::shared_ptr<CXPlayerDecoder> _decoder = nullptr;

    // 视频转换器
    std::shared_ptr<CXPlayerVideoRescaler> _video_rescaler = nullptr;
    // 音频重采样器
    std::shared_ptr<CXPlayerAudioResampler> _audio_resampler = nullptr;

    // 视频渲染器
    std::shared_ptr<CXPlayerVideoRenderSDL> _video_renderer = nullptr;
    // 音频渲染器
    std::shared_ptr<CXPlayerAudioRender> _audio_renderer = nullptr;

private:
    // 索引
    int _index = -1;

    // 是否使用中
    std::atomic_bool _active = { false };

    // 重置
    std::atomic_bool _reset = { false };
    // 是否已结束
    std::atomic_bool _demux_over = { false };

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 时间基
    AVRational _timebase = { 0,1 };

    // 队列长度上限
    int _max_pkts = 0;
    // 最后一包时码
    int64_t _pkt_dts = 0;
    // 数据包队列
    CXPlayerQueue<AVPacket> _pkts;

    // 错误信息
    std::string _err;
};

#endif
