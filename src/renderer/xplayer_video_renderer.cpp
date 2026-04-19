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
		xpu_format_string(m_strError, "Font path or size is invalid");
		return false;
	}

	m_ptrFontCtx = TTF_OpenFont(path.c_str(), size);
	if (nullptr == m_ptrFontCtx)
	{
		xpu_format_string(m_strError, "TTF_OpenFont error: %s", TTF_GetError());
		return false;
	}

	m_strFontPath = path;
	m_iFontSize = size;

	return true;
}

void ICXPlayerVideoRenderer::closeFont()
{
	if (nullptr != m_ptrFontCtx)
	{
		TTF_CloseFont(m_ptrFontCtx);
		m_ptrFontCtx = nullptr;
	}

    m_strFontPath.clear();
    m_iFontSize = 0;
}

bool ICXPlayerVideoRenderer::adjust(int width, int height, bool flag)
{
    if (width < 1 || height < 1)
    {
        xpu_format_string(m_strError, "Input size w * h(%d * %d) is invalid", width, height);
        return false;
    }

	if (flag && (width != m_iWidth || height != m_iHeight))
	{
		m_iWidth.store(width);
		m_iHeight.store(height);
		m_blChanged.store(true);
	}
	else if (!flag && (width != m_iFrameWidth.load() || height != m_iFrameHeight.load()))
	{
        m_iFrameWidth.store(width);
        m_iFrameHeight.store(height);
		m_blChanged.store(true);
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
