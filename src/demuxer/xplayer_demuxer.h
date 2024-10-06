#ifndef __XPLAYER_DEMUXER_H__
#define __XPLAYER_DEMUXER_H__

#include <stdint.h>
#include <stdbool.h>
#include <string>
#include <vector>
#include <memory>

class CXPlayerStream;
typedef struct AVFormatContext AVFormatContext;

class CXPlayerDemuxImpl
{
public:
    CXPlayerDemuxImpl() = default;
    ~CXPlayerDemuxImpl() = default;

    // 打开
    bool open(const std::string & url, const std::string & specs);
    // 关闭
    void close();

    // 时长
    int64_t duration() const;

    // 获取错误信息
    const char * err() const;

private:
    // 创建流
    bool createStreams();
    // 销毁流
    void destroyStreams();

private:
    // 源时长
    int64_t _duration_ms = 0;

    // 源上下文
    AVFormatContext * _ctx = nullptr;

    //
    std::vector<std::shared_ptr<CXPlayerStream>> _streams;

    // 错误信息
    std::string _err;
};

#endif
