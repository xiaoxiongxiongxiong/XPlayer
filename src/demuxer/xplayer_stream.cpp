#include "xplayer_stream.h"

CXPlayerStream::CXPlayerStream(int index):
    _index(index)
{
}

bool CXPlayerStream::create()
{
    return true;
}

void CXPlayerStream::destroy()
{
}

bool CXPlayerStream::createDecoder()
{
    return true;
}

void CXPlayerStream::destroyDecoder()
{
}
