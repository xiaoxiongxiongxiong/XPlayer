#ifndef __XPLAYER_DECODER_SOFTWARE_H__
#define __XPLAYER_DECODER_SOFTWARE_H__

#include "xplayer_decoder.h"

class CXPlayerDecoderSoftware : public ICXPlayerDecoder
{
public:
    CXPlayerDecoderSoftware() = default;
    ~CXPlayerDecoderSoftware() = default;

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
};

#endif