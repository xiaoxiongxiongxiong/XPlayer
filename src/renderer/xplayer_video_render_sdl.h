#ifndef __XPLAYER_VIDEO_RENDER_SDL_H__
#define __XPLAYER_VIDEO_RENDER_SDL_H__

#include <cstdbool>
#include <cstdint>
#include <string>
#include <atomic>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

class CXPlayerVideoRenderSDL
{
public:
	CXPlayerVideoRenderSDL() = default;
	~CXPlayerVideoRenderSDL() = default;

	// 创建
	bool create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height);
	// 销毁
	void destroy();

	// 改变窗口大小
	bool resizeWindow(int width, int height);

	// 改变画面大小
	bool resizeImage(int width, int height);

	// 渲染
	bool renderer(uint8_t * data[8], int linesize[8]);

	// 清空画面
	void clear();

    // 错误信息
    const char * err() const;

private:
	// 重开纹理器
	bool reopenTexture();

private:
	SDL_Window * _wnd = nullptr;
	SDL_Renderer * _renderer = nullptr;
	SDL_Texture * _texture = nullptr;

	// 画面宽度
	std::atomic_int _img_width = { 0 };
	// 画面高度
	std::atomic_int _img_height = { 0 };

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
