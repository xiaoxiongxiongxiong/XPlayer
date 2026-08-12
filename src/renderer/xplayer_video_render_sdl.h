#ifndef __XPLAYER_VIDEO_RENDER_SDL_H__
#define __XPLAYER_VIDEO_RENDER_SDL_H__

#include "xplayer_video_renderer.h"

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

class CXPlayerVideoRenderSDL : public ICXPlayerVideoRenderer
{
public:
	CXPlayerVideoRenderSDL() = default;
	virtual ~CXPlayerVideoRenderSDL() = default;

	// 是否支持对应像素格式
	bool supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats) override;

	// 设置字体路径
	void setFontPath(const std::string & path) override;
	// 设置字体大小
	void setFontSize(int size) override;
	// 设置字体颜色
	void setFontColor(const xplayer_color_t & color) override;

	// 创建
	bool create(const void * wnd, int width, int height) override;
	// 销毁
	void destroy() override;

	// 改变窗口大小
	bool resize(int width, int height) override;

	// 渲染
	bool renderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format, 
				  uint8_t * data[8], int linesize[8], const std::string & str = "") override;

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
	bool reopenRenderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format);

	// 获取像素格式
	int getPixelFormat(XPLAYER_PIXEL_FORMAT_TYPE format);

private:
	SDL_Window * _wnd = nullptr;
	SDL_Renderer * _renderer = nullptr;
	SDL_Texture * _texture = nullptr;
};

#endif
