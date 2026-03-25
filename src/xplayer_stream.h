#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <cstdbool>
#include <string>
#include <atomic>
#include <memory>

extern "C" {
#include "libavutil/frame.h"
#include "libavcodec/packet.h"
}
#include "xplayer_queue.h"

typedef struct AVCodecParameters AVCodecParameters;
class CXPlayerDecoder;
class CXPlayerAudioResampler;
class CXPlayerVideoRescaler;

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream() = default;

    // 创建
    bool init(const AVCodecParameters * codec_par, const AVRational & timebase);
    // 销毁
    void uninit();

    bool send(const AVPacket & pkt, const bool & over = false);
    bool recv(AVFrame & frm, bool & got, bool & over);

    // 清空缓冲区和解码器内部缓冲
    void clear();
    // 缓冲区是否已满
    bool isFull();

    // 准备
    bool prepare();

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
    bool createConvertor();
    // 销毁转换器
    void destroyConvertor();

public:
    // 视频转换器
    std::shared_ptr<CXPlayerVideoRescaler> _video_rescaler = nullptr;
    // 音频重采样器
    std::shared_ptr<CXPlayerAudioResampler> _audio_resampler = nullptr;

private:
    // 索引
    int _index = -1;

    // 是否使用中
    std::atomic_bool _active = { false };

    // flush
    std::atomic_bool _flushed = { false };
    // 是否已结束
    std::atomic_bool _demux_over = { false };

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 时间基
    AVRational _timebase = { 0,1 };

    // 解码器
    std::shared_ptr<CXPlayerDecoder> _decoder = nullptr;

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
