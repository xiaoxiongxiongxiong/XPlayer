#ifndef __XPLAYER_VIDEO_RESCALER_H__
#define __XPLAYER_VIDEO_RESCALER_H__

#include <cstdio>
#include <cstdint>
#include <cstdbool>
#include <string>

extern "C" {
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

// 视频信息
class CXPlayerVideoInfo
{
public:
    CXPlayerVideoInfo() = default;

    CXPlayerVideoInfo(enum AVPixelFormat fmt, int width, int height);

    bool operator==(const CXPlayerVideoInfo & other) const;

public:
    // 像素格式
    enum AVPixelFormat _fmt = AVPixelFormat::AV_PIX_FMT_NONE;

    // 图像宽度
    int _width = 0;

    // 图像高度
    int _height = 0;
};

class CXPlayerVideoRescaler
{
public:
    CXPlayerVideoRescaler() = default;
    ~CXPlayerVideoRescaler() = default;

    // 创建转换器
    bool create(const CXPlayerVideoInfo & src, const CXPlayerVideoInfo & dst);

    // 销毁
    void destroy();

    // 转换
    bool rescale(const AVFrame * in_frm, AVFrame * out_frm);

    // 错误信息
    const char * err() const;

protected:
    // 拷贝数据帧
    void copyFrame(AVFrame * dst_frm, const AVFrame * src_frm);

private:
    // 是否需要转换
    bool _need_rescale = true;

    // 源
    CXPlayerVideoInfo _src;

    // 目标
    CXPlayerVideoInfo _dst;

    // 输出数据
    uint8_t * _out_data[AV_NUM_DATA_POINTERS] = { nullptr };
    // 输出行
    int _out_linesize[AV_NUM_DATA_POINTERS] = { 0 };

    // 视频转换器
    SwsContext * _rescaler = nullptr;

    // 错误信息
    std::string _err;
};

#endif
