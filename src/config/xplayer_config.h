#ifndef __XPLAYER_CONFIG_H__
#define __XPLAYER_CONFIG_H__

#include <cstdbool>
#include <string>

class CXPlayerConfig final
{
public:
    CXPlayerConfig(const CXPlayerConfig &) = delete;
    CXPlayerConfig & operator=(const CXPlayerConfig &) = delete;

    CXPlayerConfig(CXPlayerConfig &&) = delete;
    CXPlayerConfig & operator= (const CXPlayerConfig &&) = delete;

    static CXPlayerConfig & uniqueInstance()
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

    // 设置字体文件路径
    void setFontPath(const std::string & path);
    // 获取字体文件路径
    const std::string & getFontPath();

    // 设置字体大小
    void setFontSize(int size);
    // 获取字体大小
    int getFontSize();

    // 错误信息
    const char * err() const;

private:
    CXPlayerConfig() = default;
    ~CXPlayerConfig() = default;

    // 音量
    int _vol = 64;

    // 播放记录标记
    bool _record_flag = false;

    // 字体文件路径
    std::string _font_path;
    // 字体大小
    int _font_size = 24;

    // 配置文件路径
    std::string _path;

    // 错误信息
    std::string _err;
};

#endif
