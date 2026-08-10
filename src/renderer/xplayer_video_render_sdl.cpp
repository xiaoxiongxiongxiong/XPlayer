#include "xplayer_video_render_sdl.h"

#include <sstream>
#include "SDL2/SDL_video.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_ttf.h"

#include "utils/xplayer_utils.h"

bool CXPlayerVideoRenderSDL::supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats)
{
    formats.clear();
    formats.push_back(XPLAYER_PIXEL_FORMAT_YUV420P);
    formats.push_back(XPLAYER_PIXEL_FORMAT_YUY2);
    formats.push_back(XPLAYER_PIXEL_FORMAT_UYVY);
    formats.push_back(XPLAYER_PIXEL_FORMAT_YVYU);

    return true;
}

void CXPlayerVideoRenderSDL::setFontPath(const std::string & path)
{
    _font_path = path;
}

void CXPlayerVideoRenderSDL::setFontSize(int size)
{
    _font_size = size;
}

void CXPlayerVideoRenderSDL::setFontColor(int red, int green, int blue, int alpha)
{
    SDL_Color color{};
    color.r = static_cast<uint8_t>(red);
    color.g = static_cast<uint8_t>(green);
    color.b = static_cast<uint8_t>(blue);

    auto a = static_cast<float>(alpha) * 2.55f;
    color.a = static_cast<uint8_t>(std::round(a));
}

bool CXPlayerVideoRenderSDL::create(const void * wnd, int width, int height, const std::string & path, const int & size)
{
    if (nullptr == wnd || width <= 0 || height <= 0 || path.empty() || size <= 0)
    {
        xpu_format_string(_err, "Input param is invalid");
        return false;
    }

    if (nullptr != _wnd)
    {
        xpu_format_string(_err, "Already initialized SDL2 renderer");
        return false;
    }

    _wnd = SDL_CreateWindowFrom(getHandle(wnd));
    if (nullptr == _wnd)
    {
        xpu_format_string(_err, "SDL_CreateWindowFrom failed: %s", SDL_GetError());
        return false;
    }

    if (!openFont(path, size))
    {
        SDL_DestroyWindow(_wnd);
        _wnd = nullptr;
        return false;
    }

    _wnd_width.store(width);
    _wnd_height.store(height);

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

    closeFont();

    _img_width.store(0);
    _img_height.store(0);

    _wnd_width.store(0);
    _wnd_height.store(0);
}

bool CXPlayerVideoRenderSDL::resize(int width, int height)
{
    return adjust(width, height, true);
}

bool CXPlayerVideoRenderSDL::renderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format,
                                      uint8_t * data[8], int linesize[8], const std::string & str)
{
    if (nullptr == _wnd)
    {
        xpu_format_string(_err, "SDL2 renderer not create yet");
        return false;
    }

    if (nullptr == data[0] || linesize[0] <= 0)
    {
        xpu_format_string(_err, "Input param is invalid!");
        return false;
    }

    if (!reopenRenderer(width, height, format))
        return false;

    int ret = -1;
    if (XPLAYER_PIXEL_FORMAT_YUV420P == _format.load())
    {
        ret = SDL_UpdateYUVTexture(_texture, nullptr,
                                   data[0], linesize[0],
                                   data[1], linesize[1],
                                   data[2], linesize[2]);
    }
    else
    {
        ret = SDL_UpdateTexture(_texture, nullptr, data[0], linesize[0]);
    }
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_UpdateYUVTexture failed: %s", SDL_GetError());
        return false;
    }

    ret = SDL_RenderClear(_renderer);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderClear failed: %s", SDL_GetError());
        return false;
    }

    SDL_Rect img_rect = { 0, 0, _img_width.load(), _img_height.load() };
    SDL_Rect wnd_rect = { 0, 0, _wnd_width.load(), _wnd_height.load() };
    ret = SDL_RenderCopy(_renderer, _texture, &img_rect, &wnd_rect);
    if (0 != ret)
    {
        xpu_format_string(_err, "SDL_RenderCopy failed: %s", SDL_GetError());
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
    return _err.c_str();
}

bool CXPlayerVideoRenderSDL::rendererText(const std::string & str)
{
    if (str.empty() || nullptr == _font_ctx)
        return true;

    int font_height = TTF_FontHeight(_font_ctx);
    const int line_space = 12;
    int y = 10;

    std::string val;
    std::istringstream iss(str);
    while (getline(iss, val, '\n'))
    {
        if (val.empty())
            continue;

        SDL_Color font_color = { 255, 0, 0, 255 };
        SDL_Surface * surface = TTF_RenderUTF8_Blended(_font_ctx, val.c_str(), font_color);
        if (nullptr == surface)
        {
            xpu_format_string(_err, "TTF_RenderUTF8_Blended error: %s", TTF_GetError());
            return false;
        }

        SDL_Texture * text = SDL_CreateTextureFromSurface(_renderer, surface);
        if (nullptr == text)
        {
            xpu_format_string(_err, "SDL_CreateTextureFromSurface error: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            return false;
        }

        bool succ = true;
        SDL_Rect text_rect = { 10, y, surface->w, surface->h };
        if (0 != SDL_RenderCopy(_renderer, text, nullptr, &text_rect))
        {
            xpu_format_string(_err, "SDL_RenderCopy failed: %s", SDL_GetError());
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

bool CXPlayerVideoRenderSDL::reopenRenderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format)
{
    if (_img_width.load() == width && _img_height.load() == height && _format.load() == format)
        return true;

    if (nullptr != _renderer)
    {
        SDL_RenderClear(_renderer);
        SDL_DestroyRenderer(_renderer);
        _renderer = nullptr;
    }

    if (nullptr != _texture)
    {
        SDL_DestroyTexture(_texture);
        _texture = nullptr;
    }

    _renderer = SDL_CreateRenderer(_wnd, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (nullptr == _renderer)
    {
        xpu_format_string(_err, "SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    auto tmp = getPixelFormat(format);
    if (-1 == tmp)
    {
        xpu_format_string(_err, "Unsupported pixel format: %d", format);
        return false;
    }

    _texture = SDL_CreateTexture(_renderer, static_cast<SDL_PixelFormatEnum>(tmp), SDL_TEXTUREACCESS_STREAMING, width, height);
    if (nullptr == _texture)
    {
        xpu_format_string(_err, "SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    _changed.store(false);
    _format.store(format);
    _img_width.store(width);
    _img_height.store(height);

    return true;
}

int CXPlayerVideoRenderSDL::getPixelFormat(XPLAYER_PIXEL_FORMAT_TYPE format)
{
    int res = -1;

    switch (format)
    {
    case XPLAYER_PIXEL_FORMAT_YUV420P:
        res = SDL_PIXELFORMAT_IYUV;
        break;
    case XPLAYER_PIXEL_FORMAT_YUY2:
        res = SDL_PIXELFORMAT_YUY2;
        break;
    case XPLAYER_PIXEL_FORMAT_UYVY:
        res = SDL_PIXELFORMAT_UYVY;
        break;
    case XPLAYER_PIXEL_FORMAT_YVYU:
        res = SDL_PIXELFORMAT_YVYU;
        break;
    default:
        break;
    }

    return res;
}
