#ifndef __XPLAYER_DECODER_H__
#define __XPLAYER_DECODER_H__

#include <cstdbool>
#include <string>
#include "xplayer_definitions.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVPacket AVPacket;
typedef struct AVFrame AVFrame;

class ICXPlayerDecoder
{
public:
	ICXPlayerDecoder() = default;
	virtual ~ICXPlayerDecoder() = default;

    // 创建解码器
    virtual bool create(const AVCodecParameters * codec_par) = 0;
    
    // 销毁解码器
    virtual void destroy();

    // 清理残余数据
    virtual bool clear();

    // 发包
    virtual bool send(const AVPacket * pkt);
    
    // 收帧
    virtual bool recv(AVFrame & frm, bool & got, bool & over);

    // 获取解码器类型
    virtual XPLAYER_DECODER_TYPE getType() const = 0;

    // 获取错误信息
    virtual const char * err() const = 0;

protected:
    // 解码器实例
    AVCodecContext * _ctx = nullptr;

    // 错误信息
    std::string _err;
};

class CXPlayerDecoderFactory
{
public:
    // 创建解码器实例
    static ICXPlayerDecoder * create(XPLAYER_DECODER_TYPE type, const AVCodecParameters * codec_par);

    // 销毁解码器实例
    static void destroy(ICXPlayerDecoder * decoder);
};

#endif
