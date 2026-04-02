#include "xplayer_video_render_sdl.h"

#include "SDL2/SDL_video.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_ttf.h"

#include "utils/xplayer_utils.h"

bool CXPlayerVideoRenderSDL::create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height)
{
    if (nullptr == wnd || wnd_width <= 0 || wnd_height <= 0 || frm_width <= 0 || frm_height <= 0)
    {
        xpu_format_string(_err, "Input param is invalid");
        return false;
    }

    if (nullptr != _wnd)
    {
        xpu_format_string(_err, "Already initialized sdl renderer");
        return false;
    }

    _wnd = SDL_CreateWindowFrom(wnd);
    if (nullptr == _wnd)
    {
        xpu_format_string(_err, "SDL_CreateWindowFrom failed!");
        return false;
    }

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_ACCELERATED);
    if (nullptr == _renderer)
    {
        xpu_format_string(_err, "SDL_CreateRenderer failed!");
        destroy();
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, frm_width, frm_height);
    if (nullptr == _texture)
    {
        xpu_format_string(_err, "SDL_CreateTexture failed!");
        destroy();
        return false;
    }

    if (!initTextRenderer(_font_path, _font_size))
    {
        destroy();
        return false;
    }

    _img_width.store(frm_width);
    _img_height.store(frm_height);

    _width.store(wnd_width);
    _height.store(wnd_height);

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

    _width.store(0);
    _height.store(0);
}

void CXPlayerVideoRenderSDL::setFontPath(const std::string & path)
{
    if (_font_path == path)
        return;

    _font_path = path;
    _font_flag |= 0b01;
}

void CXPlayerVideoRenderSDL::setFontSize(int size)
{
    if (_font_size == size)
        return;

    _font_size = size;
    _font_flag |= 0b10;
}

bool CXPlayerVideoRenderSDL::resizeWindow(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        xpu_format_string(_err, "Input window size w * h(%d * %d) is invalid", width, height);
        return false;
    }

    if (width != _width || height != _height)
    {
        _changed.store(true);
        _width.store(width);
        _height.store(height);
    }

    return true;
}

bool CXPlayerVideoRenderSDL::resizeImage(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        xpu_format_string(_err, "Input image size w * h(%d * %d) is invalid", width, height);
        return false;
    }

    if (width != _img_width.load() || height != _img_height.load())
    {
        _changed.store(true);
        _img_width.store(width);
        _img_height.store(height);
    }

    return true;
}

bool CXPlayerVideoRenderSDL::renderer(uint8_t * data[8], int linesize[8], const std::string & str)
{
    if (nullptr == data[0] || linesize[0] <= 0)
    {
        xpu_format_string(_err, "Input param is invalid!");
        return false;
    }

    if (nullptr == _wnd)
    {
        xpu_format_string(_err, "SDL renderer hasn't created yet!");
        return false;
    }

    if (!reopenRenderer())
    {
        xpu_format_string(_err, "Resize renderer failed!");
        return false;
    }

    int ret = SDL_UpdateYUVTexture(_texture, nullptr,
                                   data[0], linesize[0],
                                   data[1], linesize[1],
                                   data[2], linesize[2]);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_UpdateYUVTexture failed: %s", SDL_GetError());
        return false;
    }

    ret = SDL_RenderClear(_renderer);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderClear failed");
        return false;
    }

    SDL_Rect src_rect = { 0, 0, _img_width.load(), _img_height.load() };
    SDL_Rect rect = { 0, 0, _width.load(), _height.load() };
    ret = SDL_RenderCopy(_renderer, _texture, &src_rect, &rect);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderCopy failed");
        return false;
    }

    rendererText(str);

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

// 错误信息
const char * CXPlayerVideoRenderSDL::err() const
{
    return _err.c_str();
}

bool CXPlayerVideoRenderSDL::initTextRenderer(const std::string & font_path, int font_size)
{
    if (_font_path.empty() || _font_size < 1)
    {
        return true;
    }

    _font = TTF_OpenFont(font_path.c_str(), font_size);
    if (nullptr == _font)
    {
        xpu_format_string(_err, "TTF_OpenFont error: %s", TTF_GetError());
        return false;
    }

    return true;
}

void CXPlayerVideoRenderSDL::rendererText(const std::string & str)
{
    if (str.empty() || nullptr == _font)
        return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface * surface = TTF_RenderUTF8_Blended(_font, str.c_str(), white);
    if (nullptr == surface)
        return;

    SDL_Texture * text = SDL_CreateTextureFromSurface(_renderer, surface);
    if (nullptr != text)
    {
        SDL_Rect text_rect = { 10, 10, surface->w, surface->h };
        SDL_RenderCopy(_renderer, text, nullptr, &text_rect);
        SDL_DestroyTexture(text);
    }
    SDL_FreeSurface(surface);
}

bool CXPlayerVideoRenderSDL::reopenRenderer()
{
    if (!_changed.load())
        return true;

    SDL_RenderClear(_renderer);
    SDL_DestroyRenderer(_renderer);
    _renderer = nullptr;

    SDL_DestroyTexture(_texture);
    _texture = nullptr;

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (nullptr == _renderer)
    {
        xpu_format_string(_err, "SDL_CreateRenderer failed!");
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, _img_width.load(), _img_height.load());
    if (nullptr == _texture)
    {
        xpu_format_string(_err, "SDL_CreateTexture failed!");
        return false;
    }

    _changed.store(false);

    return true;
}
