#include "XPlayer.h"
#include <QtWidgets/QApplication>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

int main(int argc, char * argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 如果是 Qt 5.14 以上，还可以加上这个（更精细的控制）
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    // PassThrough: 允许非整数缩放 (如 125%, 150%)，避免界面抖动
    // RoundPreferFloor: 向下取整，可能导致轻微模糊但布局稳定
#endif

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    atexit(SDL_Quit);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    QApplication a(argc, argv);

    XPlayer w;

    w.show();
    return a.exec();
}
