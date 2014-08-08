#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <sys/epoll.h>
#include "Status.h"

namespace catchdb
{

enum EventFlag
{
    EVENT_NONE = 0,
    EVENT_IN = 1,
    EVENT_OUT = 2,
    EVENT_ALL = 3
};

class EventManager;
//typedef std::function<void (EventManager&, int, void *)> EventHandler;
typedef void (*EventHandler) (EventManager&, int, void*);

struct Event
{
    int flag;
    EventHandler inHandler;
    EventHandler outHandler;
    void *data;

    Event() : flag(EVENT_NONE), data(nullptr) {}

    Event(int f, EventHandler handler, void *d) : flag(f), data(d)
    {
        if (flag & EVENT_IN) inHandler = handler;
        if (flag & EVENT_OUT) outHandler = handler;
    }
};

class EventManager
{
public:
    EventManager(int maxSize);
    ~EventManager();

    Status addEvent(int fd, const Event &event);
    void delEvent(int fd, int flag);

    void run();

    // non-copyable
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

private:
    int maxSize_;
    int epollfd_;
    int maxfd_;
    struct epoll_event *epollEvents_;

    std::vector<Event> events_; // fd -> Event
};

} // namespace catchdb
