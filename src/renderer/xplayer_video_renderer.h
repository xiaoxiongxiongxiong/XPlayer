#ifndef __XPLAYER_VIDEO_RENDERER_H__
#define __XPLAYER_VIDEO_RENDERER_H__

#include <cstdbool>
#include <string>
#include <atomic>

#include "xplayer_definitions.h"

typedef struct TTF_Font TTF_Font;

class ICXPlayerVideoRenderer
{
public:
	ICXPlayerVideoRenderer() = default;
	virtual ~ICXPlayerVideoRenderer() = default;

    // 创建
    virtual bool create(const void * wnd, int wnd_width, int wnd_height, int frm_width, int frm_height) = 0;
    // 销毁
    virtual void destroy() = 0;

    // 初始化字体上下文
    virtual bool initFontContext(const std::string & path, int size) = 0;
    // 销毁字体上下文
    virtual void uninitFontContext() = 0;

    // 改变窗口大小
    virtual bool resizeWindow(int width, int height) = 0;

    // 改变画面大小
    virtual bool resizeImage(int width, int height) = 0;

    // 渲染
    virtual bool renderer(uint8_t * data[8], int linesize[8], const std::string & str = "") = 0;

    // 清空画面
    virtual void clear() = 0;

    // 获取渲染器类型
    virtual XPLAYER_VIDEO_RENDERER_TYPE getType() const = 0;

    // 错误信息
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
    std::string m_strFontPath;
    // 字体大小
    int m_iFontSize = 24;
    // 字体上下文
    TTF_Font * m_ptrFontCtx = nullptr;

    // 画面宽度
    std::atomic_int m_iFrameWidth = { 0 };
    // 画面高度
    std::atomic_int m_iFrameHeight = { 0 };

    // 窗口宽度
    std::atomic_int m_iWidth = { 0 };
    // 窗口高度
    std::atomic_int m_iHeight = { 0 };
    // 窗口/画面尺寸是否发生改变
    std::atomic_bool m_blChanged = { false };

    // 错误信息
    std::string m_strError;
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
