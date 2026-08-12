#include "xplayer_video_renderer.h"

#include "SDL2/SDL_ttf.h"
#include "xplayer_utils.h"
#include "xplayer_video_render_sdl.h"
#include "xplayer_video_render_opengl.h"

void * ICXPlayerVideoRenderer::getHandle(const void * wnd)
{
	auto * ctx = reinterpret_cast<CXPlayerVideoRenderOpengl *>(const_cast<void *>(wnd));
	if (nullptr == ctx)
		return nullptr;

	return reinterpret_cast<HWND>(ctx->winId());
}

bool ICXPlayerVideoRenderer::openFont(const std::string & path, int size)
{
	if (path.empty() || size < 1)
	{
		xpu_format_string(_err, "Font path or size is invalid");
		return false;
	}

	_font_ctx = TTF_OpenFont(path.c_str(), size);
	if (nullptr == _font_ctx)
	{
		xpu_format_string(_err, "TTF_OpenFont error: %s", TTF_GetError());
		return false;
	}

	return true;
}

void ICXPlayerVideoRenderer::closeFont()
{
	if (nullptr != _font_ctx)
	{
		TTF_CloseFont(_font_ctx);
		_font_ctx = nullptr;
	}

    _font_path.clear();
    _font_size = 0;
}

bool ICXPlayerVideoRenderer::adjust(int width, int height, bool flag)
{
    if (width < 1 || height < 1)
    {
        xpu_format_string(_err, "Input size w * h(%d * %d) is invalid", width, height);
        return false;
    }

	if (flag && (width != _wnd_width || height != _wnd_height))
	{
		_wnd_width.store(width);
		_wnd_height.store(height);
		_changed.store(true);
	}
	else if (!flag && (width != _img_width.load() || height != _img_height.load()))
	{
        _img_width.store(width);
        _img_height.store(height);
		_changed.store(true);
	}

	return true;
}

ICXPlayerVideoRenderer * CXPlayerVideoRendererFactory::create(XPLAYER_VIDEO_RENDERER_TYPE type, const void * wnd)
{
	ICXPlayerVideoRenderer * ctx = nullptr;

	if (XPLAYER_VIDEO_RENDERER_SDL2 == type)
	{
		auto * sdl_ctx = new (std::nothrow) CXPlayerVideoRenderSDL();
		ctx = dynamic_cast<ICXPlayerVideoRenderer *>(sdl_ctx);
	}
	else if (XPLAYER_VIDEO_RENDERER_OPENGL == type)
	{
		auto * gl_ctx = reinterpret_cast<CXPlayerVideoRenderOpengl *>(const_cast<void *>(wnd));
		//gl_ctx->winId();
		ctx = dynamic_cast<ICXPlayerVideoRenderer *>(gl_ctx);
	}

	return ctx;
}

void CXPlayerVideoRendererFactory::destroy(ICXPlayerVideoRenderer *& ctx)
{
	if (nullptr == ctx)
		return;

	const auto type = ctx->getType();
	if (XPLAYER_VIDEO_RENDERER_SDL2 == type)
	{
		auto * sdl_ctx = dynamic_cast<CXPlayerVideoRenderSDL *>(ctx);
		delete sdl_ctx;
		ctx = nullptr;
	}
	else if (XPLAYER_VIDEO_RENDERER_OPENGL == type)
	{
		ctx = nullptr;
	}
}
