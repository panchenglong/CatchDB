#include "EventManager.h"
#include "Util.h"
#include "Logger.h"
#include <algorithm>
#include <errno.h>
#include <cstdlib>

namespace catchdb
{

EventManager::EventManager(int maxSize)
    : maxSize_(maxSize), maxfd_(-1)
{
    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0) {
        LogFatal("epoll create error: %s", ErrorDescription(errno));
        exit(EXIT_FAILURE);
    }

    epollEvents_ = new epoll_event[maxSize_];

    events_.clear();
    events_.resize(maxSize_);
}

EventManager::~EventManager()
{
    delete epollEvents_;
}

Status EventManager::addEvent(int fd, const Event &event)
{
    if (fd >= maxSize_)
        return Status::OutOfRange;
    int op = (events_[fd].flag == EVENT_NONE) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    if (events_[fd].flag == EVENT_NONE) {
        events_[fd] = event;
        maxfd_ = std::max(maxfd_, fd);
    } else {
        if (event.flag & EVENT_IN) {
            events_[fd].flag |= EVENT_IN;
            events_[fd].inHandler = event.inHandler;
        }
        if (event.flag & EVENT_OUT) {
            events_[fd].flag |= EVENT_OUT;
            events_[fd].outHandler = event.outHandler;
        }
    }

    struct epoll_event e;
    e.events = 0;
    if (event.flag & EVENT_IN) e.events |= EPOLLIN;
    if (event.flag & EVENT_OUT) e.events |= EPOLLOUT;
    e.data.fd = fd;

    if (epoll_ctl(epollfd_, op, fd, &e) == -1) {
        LogDebug("epoll_ctl: %s", ErrorDescription(errno));
        return Status::Error;
    }
    return Status::OK;
}

void EventManager::delEvent(int fd, int flag)
{
    if (events_[fd].flag == EVENT_NONE)
        return;

    events_[fd].flag &= ~flag;
    if (events_[fd].flag != EVENT_NONE) {
        struct epoll_event e;
        e.events = 0;
        if (events_[fd].flag & EVENT_IN) e.events |= EPOLLIN;
        if (events_[fd].flag & EVENT_OUT) e.events |= EPOLLOUT;
        e.data.fd = fd;

        epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &e);
    } else {
        epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, nullptr);

        if (fd == maxfd_) {
            int i = fd;
            while (--i > -1 && events_[i].flag == EVENT_NONE);
            maxfd_ = i;
        }
    }
}

void EventManager::run()
{
    while (true) {
        int num = epoll_wait(epollfd_, epollEvents_, maxfd_ + 1, -1);
        if (num == -1) {
            if (errno == EINTR)
                continue;
            else
                num = 0;
        }

        for (int i = 0; i < num; ++i) {
            int fd = epollEvents_[i].data.fd;
            Event &e = events_[fd];
            int flag = epollEvents_[i].events;
            bool fired = false;
            if (flag & EPOLLIN) {
                e.inHandler(*this, fd, e.data); 
                fired = true;
            }
            if (flag & EPOLLOUT) {
                if (!fired)
                    e.outHandler(*this, fd, e.data);
            }
            // ignore EPOLL_ERR and EPOLL_HUP
        }
    }
}

} // namespace catchdb
