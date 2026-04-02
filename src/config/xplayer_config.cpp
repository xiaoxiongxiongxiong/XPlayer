#include "xplayer_config.h"

bool CXPlayerConfig::loadConfig(const std::string & path)
{
    return true;
}

void CXPlayerConfig::unloadConfig()
{
}

void CXPlayerConfig::setVolume(int vol)
{
    _vol = vol;
}

int CXPlayerConfig::getVolume()
{
    return _vol;
}

void CXPlayerConfig::setRecordVisible(bool flag)
{
    _record_flag = flag;
}

bool CXPlayerConfig::getRecordVisible()
{
    return _record_flag;
}

