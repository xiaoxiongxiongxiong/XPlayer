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

class CXPlayerDemuxImpl;
class CXPlayerStream;

class CXPlayerAudioRender;
class CXPlayerVideoRenderSDL;

class CXPlayerSource final
{
public:
    // 删除拷贝
    CXPlayerSource(const CXPlayerSource & other) = delete;
    // 删除赋值
    CXPlayerSource & operator=(const CXPlayerSource &) = delete;
    // 删除移动
    CXPlayerSource(CXPlayerSource && other) noexcept = delete;
    // 删除移动赋值
    CXPlayerSource & operator=(CXPlayerSource &&) noexcept = delete;

    // 全局唯一访问点
    static CXPlayerSource & getInstance()
    {
        static CXPlayerSource instance;
        return instance;
    }

    // 打开
    bool open(const std::string & url, const std::string & params = "");
    // 关闭
    void close();

    // 获取文件时长
    int64_t duration() const;

    // 获取音视频索引
    void getStreamsInfo(std::vector<int> & ais, std::vector<int> & vis);

    // 播放
    bool play(const void * wnd, int width, int height);

    // 调整窗口大小
    void resize(int width, int height);

    // 暂停
    bool pause();

    // 定位
    bool seek(int64_t pos);

    // 获取播放进度
    int64_t progress();

    // 设置音量
    void setVolume(int volume);

    // 错误信息
    const char * err() const;

private:
    CXPlayerSource() = default;
    ~CXPlayerSource() = default;

    // 创建流
    bool createStreams();
    // 销毁流
    void destroyStreams();

    // 读包线程
    void readPacketsThr();

    // 音频播放线程
    void audioPlayThr();

    // 视频播放线程
    void videoPlayThr();

private:
    // 是否运行中
    std::atomic_bool _is_running = { false };
    // 是否播放中
    std::atomic_bool _is_playing = { false };
    // 是否暂停中
    std::atomic_bool _is_pause = { false };
    // 是否需要跳跃
    std::atomic_bool _is_skip = { false };

    // 当前位置
    std::atomic_int64_t _cur_pos_ms = { 0 };
    // 目标位置
    std::atomic_int64_t _dst_pos_ms = { -1 };
    // 音频时钟
    std::atomic_int64_t _audio_clock = { 0.0 };

    // 音频流索引
    std::atomic_int _audio_stream_index = { -1 };
    // 视频流索引
    std::atomic_int _video_stream_index = { -1 };

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

    // 视频播放线程
    std::thread _video_thr;
    //
    std::mutex _video_mtx;
    //
    std::condition_variable _video_cond;

    // 上下文
    CXPlayerDemuxImpl * _ctx = nullptr;
    // 流
    std::unordered_map<int, std::shared_ptr<CXPlayerStream>> _streams;

    // 音频渲染器
    std::shared_ptr<CXPlayerAudioRender> _audio_render = nullptr;
    // 视频渲染器
    std::shared_ptr<CXPlayerVideoRenderSDL> _video_render = nullptr;

    // 屏幕宽度
    std::atomic_int _wnd_width = { 0 };
    // 屏幕高度
    std::atomic_int _wnd_height = { 0 };
    // 屏幕发生改变
    std::atomic_bool _wnd_changed = { false };

    // 错误信息
    std::string _err;
};

#endif
