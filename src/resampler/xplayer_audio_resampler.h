#ifndef __XPLAYER_AUDIO_RESCALER_H__
#define __XPLAYER_AUDIO_RESCALER_H__

#include <cstdio>
#include <cstdint>
#include <cstdbool>
#include <string>

extern "C" {
#include "libswresample/swresample.h"
}

// 音频信息
class CXPlayerAudioInfo
{
public:
    CXPlayerAudioInfo() = default;

    CXPlayerAudioInfo(AVChannelLayout layout, enum AVSampleFormat fmt, int sample_rate);

    bool operator==(const CXPlayerAudioInfo & other) const;

public:
    // 声道排列
    AVChannelLayout _layout{};

    // 采样格式
    enum AVSampleFormat _fmt = AVSampleFormat::AV_SAMPLE_FMT_NONE;

    // 采样率
    int _sample_rate = 0;
};

class CXPlayerAudioResampler
{
public:
    CXPlayerAudioResampler() = default;
    ~CXPlayerAudioResampler() = default;

    // 创建转换器
    bool create(const CXPlayerAudioInfo & src, const CXPlayerAudioInfo & dst, int frame_size);

    // 销毁
    void destroy();

    // 转换
    bool resample(const AVFrame * in_frm, uint8_t ** out_data, int * out_len);

    // 错误信息
    const char * err() const;

protected:
    // 调整缓冲区大小
    bool resizeCache(int nb_samples);

private:
    // 是否需要转换
    bool _need_rescale = true;

    // 输入帧大小
    int _in_nb_samples = 0;
    // 输入音频信息
    CXPlayerAudioInfo _in;

    // 输出音频信息
    CXPlayerAudioInfo _out;

    int _out_max_nb_samples = 0;

    // 输出内容长度
    int _out_size[AV_NUM_DATA_POINTERS] = { 0 };
    // 输出内容
    uint8_t * _out_data[AV_NUM_DATA_POINTERS] = { nullptr };

    // 转换器
    SwrContext * _swr_ctx = nullptr;

    // 错误信息
    std::string _err;
};

#endif
