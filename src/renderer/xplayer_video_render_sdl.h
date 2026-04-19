#ifndef __XPLAYER_VIDEO_RENDER_SDL_H__
#define __XPLAYER_VIDEO_RENDER_SDL_H__

#include "xplayer_video_renderer.h"
#include <cstdint>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct TTF_Font TTF_Font;

class CXPlayerVideoRenderSDL : public ICXPlayerVideoRenderer
{
public:
	CXPlayerVideoRenderSDL() = default;
	virtual ~CXPlayerVideoRenderSDL() = default;

	// 创建
	bool create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height) override;
	// 销毁
	void destroy() override;

    // 初始化字体上下文
    bool initFontContext(const std::string & path, int size) override;
    // 销毁字体上下文
    void uninitFontContext() override;

	// 改变窗口大小
	bool resizeWindow(int width, int height) override;

	// 改变画面大小
	bool resizeImage(int width, int height) override;

	// 渲染
	bool renderer(uint8_t * data[8], int linesize[8], const std::string & str = "") override;

	// 清空画面
	void clear() override;

    // 获取渲染器类型
    XPLAYER_VIDEO_RENDERER_TYPE getType() const override;

    // 错误信息
    const char * err() const override;

private:
	// 渲染
	bool rendererText(const std::string & str);

	// 重开渲染器
	bool reopenRenderer();

private:
	SDL_Window * _wnd = nullptr;
	SDL_Renderer * _renderer = nullptr;
	SDL_Texture * _texture = nullptr;
};

#endif
