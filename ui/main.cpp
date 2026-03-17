#include "XPlayer.h"
#include <QtWidgets/QApplication>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"

int main(int argc, char * argv[])
{
    QApplication a(argc, argv);

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    atexit(SDL_Quit);

    XPlayer w;

    w.show();
    return a.exec();
}
