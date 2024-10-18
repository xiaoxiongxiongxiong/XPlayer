#ifndef __XPLAYER_VIDEO_RENDER_SDL_H__
#define __XPLAYER_VIDEO_RENDER_SDL_H__

#include <cstdbool>
#include <cstdint>
#include <string>
#include <atomic>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Rect SDL_Rect;

class CXPlayerVideoRenderSDL
{
public:
	CXPlayerVideoRenderSDL() = default;
	~CXPlayerVideoRenderSDL() = default;

	// 创建
	bool create(const void * wnd, int width, int height);
	// 销毁
	void destroy();

	// 改变窗口大小
	bool resize(int width, int height);

	// 渲染
	bool renderer(uint8_t * data[8], int linesize[8]);

private:
	// 重开纹理器
	bool reopenTexture();

private:
	SDL_Window * _wnd = nullptr;
	SDL_Renderer * _renderer = nullptr;
	SDL_Texture * _texture = nullptr;

	SDL_Rect * _rect = nullptr;

	// 宽度
	std::atomic_int _width = { 0 };
	// 高度
	std::atomic_int _height = { 0 };
	// 窗口尺寸是否发生改变
	std::atomic_bool _changed = { false };

	// 错误信息
	std::string _err;
};

#endif
