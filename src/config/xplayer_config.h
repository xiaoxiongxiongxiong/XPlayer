#ifndef __XPLAYER_CONFIG_H__
#define __XPLAYER_CONFIG_H__

#include <cstdbool>
#include <string>

class CXPlayerConfig
{
public:
    CXPlayerConfig(const CXPlayerConfig &) = delete;
    CXPlayerConfig & operator=(const CXPlayerConfig &) = delete;

    CXPlayerConfig(CXPlayerConfig &&) = delete;
    CXPlayerConfig & operator= (const CXPlayerConfig &&) = delete;

    static CXPlayerConfig & getInstance()
    {
        static CXPlayerConfig instance;
        return instance;
    }

    // 加载配置文件
    bool loadConfig(const std::string & path);
    // 卸载配置文件
    void unloadConfig();

    // 设置音量
    void setVolume(int vol);
    // 获取音量
    int getVolume();

    // 设置播放记录显示标记
    void setRecordVisible(bool flag);
    // 获取播放记录显示标记
    bool getRecordVisible();

private:
    CXPlayerConfig() = default;
    ~CXPlayerConfig() = default;

    // 音量
    int _vol = 64;

    // 播放记录标记
    bool _record_flag = false;
};

#endif
