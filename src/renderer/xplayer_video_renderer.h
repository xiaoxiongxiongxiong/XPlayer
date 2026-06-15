#ifndef __XPLAYER_VIDEO_RENDERER_H__
#define __XPLAYER_VIDEO_RENDERER_H__

#include <cstdbool>
#include <string>
#include <atomic>
#include <vector>

#include "xplayer_definitions.h"

typedef struct TTF_Font TTF_Font;

class ICXPlayerVideoRenderer
{
public:
	ICXPlayerVideoRenderer() = default;
	virtual ~ICXPlayerVideoRenderer() = default;

    /*
    * supportedPixelFormat
    * @brief 是否支持对应像素格式
    * @param  format  像素格式
    * @return true/false
    */
    virtual bool supportedPixelFormat(std::vector<XPLAYER_PIXEL_FORMAT_TYPE> & formats) = 0;

    /*
    * create
    * @brief 创建渲染器
    * @param  wnd    窗口句柄
    * @param  width  窗口宽度
    * @param  height 窗口高度
    * @param  path   字体路径
    * @param  size   字体大小
    * @return true/false
    */
    virtual bool create(const void * wnd, int width, int height, const std::string & path, const int & size) = 0;
    
    /*
    * destroy
    * @brief 销毁渲染器
    */
    virtual void destroy() = 0;

    /*
    * resize
    * @brief  改变窗口大小
    * @param  width  窗口宽度
    * @param  height 窗口高度
    * @return true/false
    */
    virtual bool resize(int width, int height) = 0;

    // 渲染
    /*
    * renderer
    * @brief  渲染
    * @param  width    画面宽度
    * @param  height   画面高度
    * @param  format   像素格式
    * @param  data     画面数据
    * @param  linesize 画面行大小
    * @param  str      画面要叠加的字符串
    * @return true/false
    */
    virtual bool renderer(int width, int height, XPLAYER_PIXEL_FORMAT_TYPE format, 
                          uint8_t * data[8], int linesize[8], const std::string & str = "") = 0;

    /*
    * clear
    * @brief 清空画面
    */
    virtual void clear() = 0;

    /*
    * getType
    * @brief 获取渲染器类型
    * @return 渲染器类型
    */
    virtual XPLAYER_VIDEO_RENDERER_TYPE getType() const = 0;

    /*
    * err
    * @brief 获取错误信息
    * @return 错误信息
    */
    virtual const char * err() const = 0;

protected:
    // 获取句柄
    void * getHandle(const void * wnd);

    // 打开字体
    bool openFont(const std::string & path, int size);
    // 关闭字体
    void closeFont();
    
    // flag true-window false-frame
    bool adjust(int width, int height, bool flag);

protected:
    // 字体路径
    std::string _font_path;
    // 字体大小
    int _font_size = 24;
    // 字体上下文
    TTF_Font * _font_ctx = nullptr;

    // 画面宽度
    std::atomic_int _img_width = { 0 };
    // 画面高度
    std::atomic_int _img_height = { 0 };

    // 窗口宽度
    std::atomic_int _wnd_width = { 0 };
    // 窗口高度
    std::atomic_int _wnd_height = { 0 };
    // 窗口/画面尺寸是否发生改变
    std::atomic_bool _changed = { false };

    // 像素格式
    std::atomic<XPLAYER_PIXEL_FORMAT_TYPE> _format = {};

    // 错误信息
    std::string _err;
};

class CXPlayerVideoRendererFactory
{
public:
    CXPlayerVideoRendererFactory() = default;
    ~CXPlayerVideoRendererFactory() = default;

    // 创建
    static ICXPlayerVideoRenderer * create(XPLAYER_VIDEO_RENDERER_TYPE type, const void * wnd);
    // 销毁
    static void destroy(ICXPlayerVideoRenderer *& ctx);
};

#endif
