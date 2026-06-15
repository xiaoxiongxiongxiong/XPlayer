#ifndef __XPLAYER_DECODER_HARDWARE_H__
#define __XPLAYER_DECODER_HARDWARE_H__

#include "xplayer_decoder.h"

typedef struct AVBufferRef AVBufferRef;

class CXPlayerDecoderHardware : public ICXPlayerDecoder
{
public:
	CXPlayerDecoderHardware() = default;
	~CXPlayerDecoderHardware() = default;

    // 创建解码器
    bool create(const AVCodecParameters * codec_par) override;
    // 销毁解码器
    void destroy() override;

    // 清理残余数据
    bool clear() override;

    //
    bool send(const AVPacket * pkt) override;
    //
    bool recv(AVFrame & frm, bool & got, bool & over) override;

    XPLAYER_DECODER_TYPE getType() const override;

    // 获取错误信息
    const char * err() const override;

private:
    // 硬件设备上下文
    AVBufferRef * _hw_ctx = nullptr;
    // 像素格式
    enum AVPixelFormat _pix_fmt;
};

#endif
