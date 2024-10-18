#ifndef __XPLAYER_DEMUXER_H__
#define __XPLAYER_DEMUXER_H__

#include <cstdint>
#include <cstdbool>
#include <string>

typedef struct AVFormatContext AVFormatContext;
typedef struct AVStream AVStream;
typedef struct AVPacket AVPacket;

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

    // 获取流个数
    int getStreamsCount();

    // 获取流信息
    AVStream * getStreamInfo(int index);

    // 定位
    int seek(int stream_index, int64_t timestamp);

    // 读包
    int readPacket(AVPacket * pkt);

    // 获取错误信息
    const char * err() const;

private:
    // 源时长
    int64_t _duration_ms = 0;

    // 源上下文
    AVFormatContext * _ctx = nullptr;

    // 错误信息
    std::string _err;
};

#endif
