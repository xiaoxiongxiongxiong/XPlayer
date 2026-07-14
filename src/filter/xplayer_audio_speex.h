#ifndef __XPLAYER_AUDIO_SPEEX_H__
#define __XPLAYER_AUDIO_SPEEX_H__

#include <cstdbool>
#include <string>
#include <memory>

namespace soundtouch
{
	class SoundTouch;
}

class CXPlayerAudioSpeex
{
public:
	CXPlayerAudioSpeex();
	~CXPlayerAudioSpeex();

	// 创建
	bool create(int channels, int sample_rate, int samples);
	// 销毁
	void destroy();

	// 更新参数
	void update(int channels, int sample_rate, int samples);

	// 设置倍速
	void setSpeed(double speed);

	// 发送
	bool send(const uint8_t * data, int len);
	// 接收
	int recv(uint8_t * data, int len);

	// flush
	void flush();

	// 清空
	void clear();

	// 错误信息
	const char * err() const;

private:
    // 实例
	std::unique_ptr<soundtouch::SoundTouch> _ctx = nullptr;

	// 声道数
	int _channels = 0;
	// 采样率
	int _sample_rate = 0;
	// 采样数
	int _samples = 0;

	// 错误信息
	std::string _err;
};

#endif
