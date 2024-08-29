#include "XPlayer.h"

#include "renderer/CVideoRender.h"
#include "renderer/CAudioRender.h"

XPlayer::XPlayer(QWidget * parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
}

XPlayer::~XPlayer()
{}
