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
class CXPlayerAudioResampler;
class CXPlayerVideoRescaler;
class CXPlayerAudioSpeex;
class CXPlayerAudioRender;
class CXPlayerVideoRenderSDL;

// 播放器状态
typedef enum _XPLAYER_STATE
{
    XPLAYER_STATE_NONE,
    XPLAYER_STATE_READY,      // 已就绪
    XPLAYER_STATE_PAUSE,      // 暂停
    XPLAYER_STATE_PLAYING,    // 播放中
    XPLAYER_STATE_OVER,       // 播放完成
    XPLAYER_STATE_ERROR,      // 播放出错
    XPLAYER_STATE_MAX
} XPLAYER_STATE;

// 播放倍速
typedef enum _XPLAYER_SPEED_MODE
{
    XPLAYER_SPEED_ONE_QUATER,  // 0.25
    XPLAYER_SPEED_ONE_HALF,    // 0.5
    XPLAYER_SPEED_NORMAL,      // 1.0
    XPLAYER_SPEED_DOUBLE,      // 2
    XPLAYER_SPEED_QUADRUPLE,   // 4倍速
} XPLAYER_SPEED_MODE;

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
    bool pause(bool flag);

    // 定位
    bool seek(int64_t pos);

    // 获取播放进度
    int64_t progress();

    // 选流 -1表示禁用
    bool selectStream(int index, bool is_video);

    // 设置音量
    void setVolume(int volume);

    // 设置播放倍速
    void setSpeed(XPLAYER_SPEED_MODE speed);

    // 显示详细信息
    void showDetail(bool flag);

    // 状态
    XPLAYER_STATE state() const;

    // 错误信息
    const char * err() const;

private:
    CXPlayerSource() = default;
    ~CXPlayerSource();

    // 创建流
    bool createStreams();
    // 销毁流
    void destroyStreams();

    // 初始化转换器
    bool initConvertor();
    // 销毁转换器
    void uninitConvertor();

    // 初始化过滤器
    bool initFilter();
    // 销毁过滤器
    void uninitFilter();

    // 初始化渲染器
    bool initRenderer(const void * wnd, int width, int height);
    // 销毁渲染器
    void uninitRenderer();

    // 读包线程
    void readPacketsThr();

    // 音频播放线程
    void audioPlayThr();

    // 视频播放线程
    void videoPlayThr();

    // 音频多倍速渲染
    void audioMultiSpeedRenderer(std::vector<std::uint8_t> & buff, int bytes, const bool & over = false);

    // 音频清理
    void audioClear(int stream_index);

    // 处理倍速
    void processSpeed(XPLAYER_SPEED_MODE mode);

    // 处理音频流切换
    bool processAudioStream(int stream_index);

    // 格式化详细信息
    std::string formatDetailString();

private:
    // 播放状态
    std::atomic<XPLAYER_STATE> _state = { XPLAYER_STATE_NONE };
    // 播放倍速
    std::atomic<XPLAYER_SPEED_MODE> _speed_mode = { XPLAYER_SPEED_NORMAL };
    // 倍速值
    std::atomic<double> _speed = { 1.0 };
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

    // 当前帧数
    std::atomic_int64_t _cur_frames = { 0 };
    // 实时帧率
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
    // 音频重采样器
    std::shared_ptr<CXPlayerAudioResampler> _audio_resampler = nullptr;
    // 音频渲染器
    std::shared_ptr<CXPlayerAudioRender> _audio_renderer = nullptr;
    // 音频倍速过滤器
    std::shared_ptr<CXPlayerAudioSpeex> _audio_speex = nullptr;

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
    // 视频画幅转换器
    std::shared_ptr<CXPlayerVideoRescaler> _video_rescaler = nullptr;
    // 视频渲染器
    std::shared_ptr<CXPlayerVideoRenderSDL> _video_renderer = nullptr;

    // 字体路径
    std::string _font_path;
    // 字体大小
    int _font_size;

    // 上下文
    CXPlayerDemuxImpl * _ctx = nullptr;
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
