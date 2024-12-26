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

typedef enum _XPLAYER_DECODE_STATE
{
    XPLAYER_DECODE_NONE,
    XPLAYER_DECODE_PREPARE, // 准备
    XPLAYER_DECODE_READY,   // 就绪
    XPLAYER_DECODE_IDLE,    // 空闲
    XPLAYER_DECODE_RUNNING, // 运行中
    XPLAYER_DECODE_SUCC,    // 成功
    XPLAYER_DECODE_FAIL,    // 失败
    XPLAYER_DECODE_MAX,
} XPLAYER_DECODE_STATE;

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

    // 重置 清空缓冲，重开解码器
    void flush();

    // 数据包
    bool push(const AVPacket & pkt, bool over = false);

    // 获取状态
    XPLAYER_DECODE_STATE state() const;

    // 错误信息
    const char * err() const;

private:
    // 创建解码器
    bool createDecoder();
    // 销毁解码器
    void destroyDecoder();
    // 重开解码器
    bool reopenDecoder();

    // 创建渲染器
    bool createRender(const void * wnd, int width, int height);
    // 销毁渲染器
    void destroyRender();

    // 重置
    void reset();

    // 解码线程
    void decodeThr();

private:
    // 索引
    int _index = -1;

    // 是否运行中
    std::atomic_bool _running = { false };
    // 重置
    std::atomic_bool _reset = { false };
    // 是否已结束
    std::atomic_bool _demux_over = { false };

    // 解码线程句柄
    std::thread _thr;

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 解码器
    AVCodecContext * _codec = nullptr;

    // 最后一次时码
    int64_t _latest_pkt_dts = -1;
    // 数据包
    CXPlayerQueue<AVPacket> _pkts;

    // 当前状态
    std::atomic<XPLAYER_DECODE_STATE> _state = { XPLAYER_DECODE_NONE };

    // 错误信息
    std::string _err;
};

#endif
