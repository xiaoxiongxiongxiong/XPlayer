#ifndef __XPLAYER_AUDIO_VOLUME_H__
#define __XPLAYER_AUDIO_VOLUME_H__

#include <string>

typedef struct AVFrame AVFrame;
typedef struct AVFilterGraph AVFilterGraph;
typedef struct AVFilterContext AVFilterContext;

class CXPlayerAudioVolume
{
public:
	CXPlayerAudioVolume() = default;
	~CXPlayerAudioVolume() = default;

	// flag-true(百分比) false(分贝)
	bool create(int sample_rate, int format, uint64_t channel_layout, float val, const bool & flag = false);

	// 销毁
	void destroy();

	// 发送
	bool send(AVFrame & frm);
	// 接收
	bool recv(AVFrame & frm, bool & got, bool & over);

	// 错误信息
	const char * err() const;

private:
	// 过滤器图
	AVFilterGraph * _graph = nullptr;

	// 过滤器上下文
	AVFilterContext * _ctx = nullptr;

	// 过滤器输出上下文
	AVFilterContext * _sink_ctx = nullptr;

	// 错误信息
	std::string _err;
};

#endif
