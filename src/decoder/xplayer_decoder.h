#ifndef __XPLAYER_DECODER_H__
#define __XPLAYER_DECODER_H__

#include <cstdbool>
#include <string>

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVPacket AVPacket;
typedef struct AVFrame AVFrame;

class CXPlayerDecoder
{
public:
    CXPlayerDecoder() = default;
    ~CXPlayerDecoder() = default;

    // 创建解码器
    bool create(const AVCodecParameters * codec_par);
    // 销毁解码器
    void destroy();

    // 清理残余数据
    bool clear();

    //
    bool send(const AVPacket * pkt);
    //
    bool recv(AVFrame & frm, bool & got, bool & over);

    // 获取错误信息
    const char * err() const;

private:
    // 解码器参数
    AVCodecParameters * _codec_par = nullptr;
    // 解码器实例
    AVCodecContext * _ctx = nullptr;

    // 错误信息
    std::string _err;
};

#endif