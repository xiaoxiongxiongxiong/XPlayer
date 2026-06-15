#ifndef __XPLAYER_FILTER_BSF_H__
#define __XPLAYER_FILTER_BSF_H__

#include <cstdbool>
#include <string>
#include <vector>

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVBSFContext AVBSFContext;
typedef struct AVPacket AVPacket;

class CXPlayerFilterBsf
{
public:
	CXPlayerFilterBsf() = default;
	~CXPlayerFilterBsf() = default;

	// 创建
	int create(AVCodecParameters * par);
    // 销毁
    void destroy();

	// 发送
	bool send(AVPacket & pkt);
	// 接收
	bool recv(AVPacket & pkt, bool & got);

	// 错误信息
	const char * err() const;

private:
	// 上下文
	AVBSFContext * m_ptrCtx = nullptr;

	// 错误信息
	std::string m_strError;
};

#endif
