#ifndef __XPLAYER_SOURCE_H__
#define __XPLAYER_SOURCE_H__

#include <cstdbool>
#include <cstdint>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include <memory>
#include "xplayer_definitions.h"

class CXPlayerDemuxImpl;
class CXPlayerStream;
class CXPlayerAudioRender;
class ICXPlayerVideoRenderer;

class CXPlayerSource final
{
public:
    // 删除拷贝
    CXPlayerSource(const CXPlayerSource &) = delete;
    // 删除赋值
    CXPlayerSource & operator=(const CXPlayerSource &) = delete;
    // 删除移动
    CXPlayerSource(CXPlayerSource &&) noexcept = delete;
    // 删除移动赋值
    CXPlayerSource & operator=(CXPlayerSource &&) noexcept = delete;

    // 全局唯一访问点
    static CXPlayerSource & uniqueInstance()
    {
        static CXPlayerSource instance;
        return instance;
    }

    // 设置字体路径 需在open之前调用
    void setFontPath(const std::string & path);
    // 设置字体大小 需在open之前调用
    void setFontSize(int size);
    // 设置字体颜色
    void setFontColor(int red, int green, int blue, int alpha);

    // 打开
    bool open(const std::string & url, const std::string & params = "");
    // 关闭
    void close();

    // 获取文件时长
    int64_t duration() const;

    // 获取音视频索引
    void getStreamsInfo(std::vector<int> & ais, std::vector<int> & vis);

    // 播放
    bool play(const void * wnd, int width, int height, const std::string & audio_device = "");

    // 调整窗口大小
    void resize(int width, int height);

    // 暂停
    bool pause(bool flag);

    // 定位
    bool seek(int64_t pos);

    // 获取播放进度
    int64_t progress();

    // 选流 -1表示禁用
    bool selectStream(int index, bool is_video);

    // 选择视频渲染器
    void selectVideoRenderer(XPLAYER_VIDEO_RENDERER_TYPE type);

    // 设置音量
    void setVolume(int volume);

    // 设置播放倍速
    void setSpeed(float speed);

    // 显示详细信息
    void showDetail(bool flag);

    // 状态
    XPLAYER_STATE state() const;

    // 错误信息
    const char * err() const;

private:
    CXPlayerSource();
    ~CXPlayerSource();

    // 创建流
    bool createStreams();
    // 销毁流
    void destroyStreams();

    // 初始化渲染器
    bool initAudioRenderer(const std::string & device);
    // 销毁渲染器
    void uninitAudioRenderer();

    // 初始化视频渲染器
    bool initVideoRenderer(const void * wnd, int width, int height);
    // 销毁视频渲染器
    void uninitVideoRenderer();

    // 读包线程
    void readPacketsThr();

    // 音频播放线程
    void audioPlayThr();

    // 视频播放线程
    void videoPlayThr();

    // 切换流
    void changeStream(int & src, const int & dst);

    // 计算实时帧率
    double calcFrameRate();

    // 格式化详细信息
    std::string formatDetailString();

private:
    // 播放状态
    std::atomic<XPLAYER_STATE> _state = { XPLAYER_STATE_NONE };
    // 倍速值
    std::atomic<float> _speed = { 1.0f };
    // 播放倍速改变
    std::atomic_bool _speed_changed = { false };
    // 是否显示详细信息
    std::atomic_bool _show = { false };

    // 是否运行中
    std::atomic_bool _is_running = { false };
    // 是否需要跳跃
    std::atomic_bool _is_skip = { false };

    // 音量
    std::atomic_int _volume = { 64 };
    // 当前位置
    std::atomic_int64_t _cur_pos_ms = { 0 };
    // 目标位置
    std::atomic_int64_t _dst_pos_ms = { -1 };
    // 音频时钟
    std::atomic_int64_t _audio_clock = { 0 };

    // 上次时间
    int64_t _last_ts = 0;
    // 上次帧数
    std::atomic_int64_t _last_frames = { 0 };
    // 当前帧数
    std::atomic_int64_t _cur_frames = { 0 };
    // 当前帧率
    std::atomic<double> _cur_fps = { 0.0 };

    // 线程句柄
    std::thread _thr;
    // 锁
    std::mutex _mtx;
    // 信号量
    std::condition_variable _cond;

    // 音频播放线程
    std::thread _audio_thr;
    //
    std::mutex _audio_mtx;
    // 音频信号量
    std::condition_variable _audio_cond;
    // 是否跳转完成
    std::atomic_bool _audio_skip_over = { true };
    // 是否播放完成
    std::atomic_bool _audio_play_over = { false };
    // 音频流索引
    std::atomic_int _audio_stream_index = { -1 };
    // 音频渲染器
    std::unique_ptr<CXPlayerAudioRender> _audio_renderer = nullptr;

    // 视频播放线程
    std::thread _video_thr;
    //
    std::mutex _video_mtx;
    // 视频信号量
    std::condition_variable _video_cond;
    // 是否跳转完成
    std::atomic_bool _video_skip_over = { true };
    // 是否播放完成
    std::atomic_bool _video_play_over = { false };
    // 视频流索引
    std::atomic_int _video_stream_index = { -1 };
    // 视频渲染器
    ICXPlayerVideoRenderer * _video_renderer = nullptr;
    //
    std::atomic<XPLAYER_VIDEO_RENDERER_TYPE> _video_renderer_type = { XPLAYER_VIDEO_RENDERER_SDL2 };

    // 字体路径
    std::string _font_path;
    // 字体大小
    int _font_size = 24;

    // 上下文
    std::unique_ptr<CXPlayerDemuxImpl> _ctx = nullptr;
    // 流
    std::unordered_map<int, std::shared_ptr<CXPlayerStream>> _streams;

    // 屏幕宽度
    std::atomic_int _wnd_width = { 0 };
    // 屏幕高度
    std::atomic_int _wnd_height = { 0 };
    // 屏幕发生改变
    std::atomic_bool _wnd_changed = { false };

    // 文件名
    std::string _name;

    // 错误信息
    std::string _err;
};

#endif
