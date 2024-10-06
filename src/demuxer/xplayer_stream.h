#ifndef __XPLAYER_STREAM_H__
#define __XPLAYER_STREAM_H__

#include <stdbool.h>

class CXPlayerStream
{
public:
    CXPlayerStream(int index);
    ~CXPlayerStream() = default;

    // 创建
    bool create();
    // 销毁
    void destroy();

    // 

private:
    // 创建解码器
    bool createDecoder();
    // 销毁解码器
    void destroyDecoder();

private:
    int _index = -1;
};

#endif
