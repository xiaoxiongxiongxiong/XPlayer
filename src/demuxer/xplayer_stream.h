#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <cstdbool>
#include <string>
#include <thread>
#include <atomic>

extern "C" {
#include "libavcodec/packet.h"
}

#include "utils/xplayer_queue.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVCodecContext AVCodecContext;

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream() = default;

    // 创建
    bool create(const AVCodecParameters * codec_par);
    // 销毁
    void destroy();

    // 是否启用
    void enable(bool flag);

    // 
    bool push(const AVPacket & pkt, bool over = false);

private:
    // 创建解码器
    bool createDecoder();
    // 销毁解码器
    void destroyDecoder();

    // 解码线程
    void decodeThr();

private:
    // 索引
    int _index = -1;

    // 是否运行中
    std::atomic_bool _running = { false };
    // 解码线程句柄
    std::thread _thr;

    // 是否已结束
    std::atomic_bool _is_over = { false };

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 解码器
    AVCodecContext * _codec = nullptr;

    // 最后一次时码
    int64_t _latest_pkt_dts = -1;
    // 数据包
    CXPlayerQueue<AVPacket> _pkts;

    // 错误信息
    std::string _err;
};

#endif
