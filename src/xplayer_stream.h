#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <cstdbool>
#include <string>
#include <atomic>
#include <memory>

typedef struct AVCodecParameters AVCodecParameters;
class CXPlayerDecoder;

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream() = default;

    // 创建
    bool create(const AVCodecParameters * codec_par);
    // 销毁
    void destroy();

    // 准备
    bool prepare(const void * wnd, int width, int height);

    // 获取解码器
    const std::shared_ptr<CXPlayerDecoder> & decoder();

    // 获取转换器


    // 获取渲染器
    //const std::shared_ptr<CXPlayerRenderer>

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

private:
    // 索引
    int _index = -1;

    // 是否使用中
    std::atomic_bool _active = { false };

    // 是否运行中
    std::atomic_bool _running = { false };
    // 重置
    std::atomic_bool _reset = { false };
    // 是否已结束
    std::atomic_bool _demux_over = { false };

    // 编解码器参数
    AVCodecParameters * _codecpar = nullptr;
    // 解码器
    std::shared_ptr<CXPlayerDecoder> _decoder = nullptr;

    // 错误信息
    std::string _err;
};

#endif
