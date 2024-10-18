#include "xplayer_video_render_sdl.h"

#include "SDL2/SDL_video.h"
#include "SDL2/SDL_render.h"

#include "utils/xplayer_utils.h"

bool CXPlayerVideoRenderSDL::create(const void * wnd, int width, int height)
{
    if (nullptr == wnd || width <= 0 || height <= 0)
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

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_TARGETTEXTURE);
    if (nullptr == _renderer)
    {
        xpu_format_string(_err, "SDL_CreateRenderer failed!");
        destroy();
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (nullptr == _texture)
    {
        xpu_format_string(_err, "SDL_CreateTexture failed!");
        destroy();
        return false;
    }

    _rect = static_cast<SDL_Rect *>(calloc(1, sizeof(SDL_Rect)));
    if (nullptr == _rect)
    {
        xpu_format_string(_err, "No enough memory!");
        destroy();
        return false;
    }

    _width.store(width);
    _height.store(height);

    return true;
}

void CXPlayerVideoRenderSDL::destroy()
{
    if (nullptr != _texture)
    {
        SDL_DestroyTexture(_texture);
        _texture = nullptr;
    }
    if (nullptr != _texture)
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
    if (nullptr != _rect)
    {
        free(_rect);
        _rect = nullptr;
    }

    _width.store(0);
    _height.store(0);
}

bool CXPlayerVideoRenderSDL::resize(int width, int height)
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

bool CXPlayerVideoRenderSDL::renderer(uint8_t * data[8], int linesize[8])
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

    if (!reopenTexture())
    {
        xpu_format_string(_err, "Resize renderer failed!");
        return false;
    }

    int ret = SDL_UpdateYUVTexture(_texture, _rect,
                                   data[0], linesize[0],
                                   data[1], linesize[1],
                                   data[2], linesize[2]);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_UpdateYUVTexture failed");
        return false;
    }

    _rect->x = 0;
    _rect->y = 0;
    _rect->w = _width.load();
    _rect->h = _height.load();

    ret = SDL_RenderClear(_renderer);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderClear failed");
        return false;
    }

    ret = SDL_RenderCopy(_renderer, _texture, nullptr, _rect);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderCopy failed");
        return false;
    }

    SDL_RenderPresent(_renderer);

    return true;
}

bool CXPlayerVideoRenderSDL::reopenTexture()
{
    if (!_changed.load())
        return true;

    SDL_DestroyTexture(_texture);
    _texture = nullptr;

    _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_TARGET, _width.load(), _height.load());
    if (nullptr == _texture)
    {
        xpu_format_string(_err, "SDL_CreateTexture failed!");
        return false;
    }

    _changed.store(false);

    return true;
}
