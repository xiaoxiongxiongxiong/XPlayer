﻿#ifndef __XPLAYER_QUEUE_H__
#define __XPLAYER_QUEUE_H__

#include <cstdio>
#include <cstdbool>
#include <queue>
#include <mutex>

template <typename T>
class CXPlayerQueue
{
public:
    explicit CXPlayerQueue() = default;
    virtual ~CXPlayerQueue() = default;

public:
    void push(const T & item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.emplace_back(item);
    }

    bool front(T & item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty())
        {
            return false;
        }
        item = _queue.front();

        return true;
    }

    bool pop(T & item)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty())
        {
            return false;
        }
        item = _queue.front();
        _queue.pop_front();

        return true;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.clear();
    }

    bool empty()
    {
        return _queue.empty();
    }

    size_t size()
    {
        return _queue.size();
    }

private:
    std::mutex _mutex;
    std::deque<T> _queue;
};

#endif
