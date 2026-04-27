#ifndef __XPLAYER_VIDEO_DECODER_D3D11_H__
#define __XPLAYER_VIDEO_DECODER_D3D11_H__

#include <cstdbool>
#include <string>
#include <vector>
#include <atomic>

class ID3D11Device;
class ID3D11DeviceContext;
class ID3D11VideoDevice;
class ID3D11VideoContext;
class ID3D11VideoDecoder;
class ID3D11Texture2D;
class ID3D11VideoDecoderOutputView;

class CXPlayerVideoDecoderD3D11
{
public:
	CXPlayerVideoDecoderD3D11() = default;
	~CXPlayerVideoDecoderD3D11() = default;

	bool create(int width, int height, int profile);
	void destroy();

	bool send(const uint8_t * data, int size);

private:
	bool initialize();

private:
	ID3D11Device * m_ptrDevice = nullptr;
	ID3D11DeviceContext * m_ptrContext = nullptr;
    ID3D11VideoDevice * m_ptrVideoDevice = nullptr;
    ID3D11VideoContext * m_ptrVideoContext = nullptr;
    ID3D11VideoDecoder * m_ptrVideoDecoder = nullptr;
	std::vector<ID3D11Texture2D *> m_ptrTextures;
	std::vector<ID3D11VideoDecoderOutputView *> m_ptrOutputViews;

	std::atomic_int m_iFrameCount = { 0 };

	// 错误信息
	std::string m_strError;
};

#endif
