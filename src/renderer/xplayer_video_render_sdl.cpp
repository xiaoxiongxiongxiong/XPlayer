#include "xplayer_video_render_sdl.h"

#include <sstream>
#include "SDL2/SDL_video.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_ttf.h"

#include "utils/xplayer_utils.h"

bool CXPlayerVideoRenderSDL::create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height)
{
    if (nullptr == wnd || wnd_width <= 0 || wnd_height <= 0 || frm_width <= 0 || frm_height <= 0)
    {
        xpu_format_string(m_strError, "Input param is invalid");
        return false;
    }

    if (nullptr != _wnd)
    {
        xpu_format_string(m_strError, "Already initialized sdl renderer");
        return false;
    }

    _wnd = SDL_CreateWindowFrom(getHandle(wnd));
    if (nullptr == _wnd)
    {
        xpu_format_string(m_strError, "SDL_CreateWindowFrom failed: %s", SDL_GetError());
        return false;
    }

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_ACCELERATED);
    if (nullptr == _renderer)
    {
        xpu_format_string(m_strError, "SDL_CreateRenderer failed: %s", SDL_GetError());
        destroy();
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frm_width, frm_height);
    if (nullptr == _texture)
    {
        xpu_format_string(m_strError, "SDL_CreateTexture failed: %s", SDL_GetError());
        destroy();
        return false;
    }

    m_iFrameWidth.store(frm_width);
    m_iFrameHeight.store(frm_height);

    m_iWidth.store(wnd_width);
    m_iHeight.store(wnd_height);

    return true;
}

void CXPlayerVideoRenderSDL::destroy()
{
    if (nullptr != _texture)
    {
        SDL_DestroyTexture(_texture);
        _texture = nullptr;
    }

    if (nullptr != _renderer)
    {
        SDL_RenderClear(_renderer);
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }

    if (nullptr != _wnd)
    {
        SDL_DestroyWindow(_wnd);
        _wnd = nullptr;
    }

    uninitFontContext();

    m_iFrameWidth.store(0);
    m_iFrameHeight.store(0);

    m_iWidth.store(0);
    m_iHeight.store(0);
}

bool CXPlayerVideoRenderSDL::initFontContext(const std::string & path, int size)
{
    return openFont(path, size);
}

void CXPlayerVideoRenderSDL::uninitFontContext()
{
    closeFont();
}

bool CXPlayerVideoRenderSDL::resizeWindow(int width, int height)
{
    return adjust(width, height, true);
}

bool CXPlayerVideoRenderSDL::resizeImage(int width, int height)
{
    return adjust(width, height, false);
}

bool CXPlayerVideoRenderSDL::renderer(uint8_t * data[8], int linesize[8], const std::string & str)
{
    if (nullptr == _renderer)
    {
        xpu_format_string(m_strError, "SDL2 renderer not create yet");
        return false;
    }

    if (nullptr == data[0] || linesize[0] <= 0)
    {
        xpu_format_string(m_strError, "Input param is invalid!");
        return false;
    }

    if (!reopenRenderer())
        return false;

    int ret = SDL_UpdateYUVTexture(_texture, nullptr,
                                   data[0], linesize[0],
                                   data[1], linesize[1],
                                   data[2], linesize[2]);
    if (0 != ret)
    {
        xpu_format_string(m_strError, "SDL_UpdateYUVTexture failed: %s", SDL_GetError());
        return false;
    }

    ret = SDL_RenderClear(_renderer);
    if (0 != ret)
    {
        xpu_format_string(m_strError, "SDL_RenderClear failed: %s", SDL_GetError());
        return false;
    }

    SDL_Rect src_rect = { 0, 0, m_iFrameWidth.load(), m_iFrameHeight.load() };
    SDL_Rect rect = { 0, 0, m_iWidth.load(), m_iHeight.load() };
    ret = SDL_RenderCopy(_renderer, _texture, &src_rect, &rect);
    if (0 != ret)
    {
        xpu_format_string(m_strError, "SDL_RenderCopy failed: %s", SDL_GetError());
        return false;
    }

    if (!rendererText(str))
        return false;

    SDL_RenderPresent(_renderer);

    return true;
}

void CXPlayerVideoRenderSDL::clear()
{
    if (nullptr == _renderer)
        return;

    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
    SDL_RenderPresent(_renderer);
}

XPLAYER_VIDEO_RENDERER_TYPE CXPlayerVideoRenderSDL::getType() const
{
    return XPLAYER_VIDEO_RENDERER_SDL2;
}

const char * CXPlayerVideoRenderSDL::err() const
{
    return m_strError.c_str();
}

bool CXPlayerVideoRenderSDL::rendererText(const std::string & str)
{
    if (str.empty() || nullptr == m_ptrFontCtx)
        return true;

    int font_height = TTF_FontHeight(m_ptrFontCtx);
    const int line_space = 12;
    int y = 10;

    std::string val;
    std::istringstream iss(str);
    while (getline(iss, val, '\n'))
    {
        if (val.empty())
            continue;

        SDL_Color font_color = { 255, 0, 0, 255 };
        SDL_Surface * surface = TTF_RenderUTF8_Blended(m_ptrFontCtx, val.c_str(), font_color);
        if (nullptr == surface)
        {
            xpu_format_string(m_strError, "TTF_RenderUTF8_Blended error: %s", TTF_GetError());
            return false;
        }

        SDL_Texture * text = SDL_CreateTextureFromSurface(_renderer, surface);
        if (nullptr == text)
        {
            xpu_format_string(m_strError, "SDL_CreateTextureFromSurface error: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            return false;
        }

        bool succ = true;
        SDL_Rect text_rect = { 10, y, surface->w, surface->h };
        if (0 != SDL_RenderCopy(_renderer, text, nullptr, &text_rect))
        {
            xpu_format_string(m_strError, "SDL_RenderCopy failed: %s", SDL_GetError());
            succ = false;
        }

        SDL_DestroyTexture(text);
        SDL_FreeSurface(surface);
        if (!succ)
            return false;
        y += (font_height + line_space);
    }

    return true;
}

bool CXPlayerVideoRenderSDL::reopenRenderer()
{
    if (!m_blChanged.load())
        return true;

    SDL_RenderClear(_renderer);
    SDL_DestroyRenderer(_renderer);
    _renderer = nullptr;

    SDL_DestroyTexture(_texture);
    _texture = nullptr;

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (nullptr == _renderer)
    {
        xpu_format_string(m_strError, "SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, m_iFrameWidth.load(), m_iFrameHeight.load());
    if (nullptr == _texture)
    {
        xpu_format_string(m_strError, "SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    m_blChanged.store(false);

    return true;
}
