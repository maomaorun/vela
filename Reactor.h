#ifndef VELA_REACTOR_H
#define VELA_REACTOR_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "moodycamel/concurrentqueue.h"
#include "common.h"

namespace vela
{

class Reactor final
{
public:
    using epoll_callback_t  = std::function<void (epoll_event_t)>;
    using event_callback_t  = std::function<void ()>;
    using exit_callback_t   = std::function<void ()>;
private:
    using exit_callback_vec_t       = std::vector<exit_callback_t>;
    using epoll_callback_map_t      = std::unordered_map<fd_t, epoll_callback_t>;
    using event_callback_queue_t    = moodycamel::ConcurrentQueue<event_callback_t>;
public:
    explicit Reactor(int max_events_num = DEFAULT_EPOLL_EVENTS_NUM) noexcept : 
            epoll_fd_{ UNKNOWN_FD },
            event_fd_{ UNKNOWN_FD },
            max_events_num_{ max_events_num },
            ready_{ false } {}

    ~Reactor() { exit(); }

    Reactor(const Reactor &)                = delete;
    Reactor(Reactor &&)                     = delete;
    Reactor & operator=(const Reactor &)    = delete;
    Reactor & operator=(Reactor &&)         = delete;

    error_t bind(fd_t fd, epoll_event_t epoll_event);
    error_t unbind(fd_t fd);

    void add_epoll_callback(fd_t fd, epoll_callback_t &&epoll_callback)
    { epoll_callback_map_.emplace(fd, std::move(epoll_callback)); }

    void del_epoll_callback(fd_t fd)
    { epoll_callback_map_.erase(fd); }

    void on_exit(exit_callback_t &&exit_callback)
    { exit_callback_vec_.emplace_back(std::move(exit_callback)); }

    error_t post(event_callback_t &&event_callback);

    bool is_ready() const noexcept
    { return ready_; }

    void exit()
    { if (ready_) ready_ = false;}

    error_t init();
    void run_in_loop(int32_t timeout_ms);
private:
    fd_t                    epoll_fd_;
    fd_t                    event_fd_;
    int                     max_events_num_;
    event_callback_queue_t  event_callback_queue_;
    epoll_callback_map_t    epoll_callback_map_;
    exit_callback_vec_t     exit_callback_vec_;
    std::thread::id         thread_id_;
    std::atomic<bool>       ready_;
};

inline error_t Reactor::bind(fd_t fd, epoll_event_t epoll_events)
{
    struct epoll_event event;
    event.data.fd   = fd;
    event.events    = epoll_events;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0 ? ERR_EPOLL_CTL_ADD : ERR_OK;
    
}

inline error_t Reactor::unbind(fd_t fd)
{
    return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) < 0 ? ERR_EPOLL_CTL_DEL : ERR_OK;
}

error_t Reactor::post(event_callback_t &&event_callback)
{
    auto err = ERR_OK;
    if (ready_)
    {
        if (std::this_thread::get_id() == thread_id_)
        {
            event_callback();
        }
        else
        {
            if (event_callback_queue_.enqueue(std::move(event_callback)))
            {
                uint64_t data = 1;
                auto nwrite = write(event_fd_, &data, sizeof(data));
                if (nwrite <= 0)
                {
                    if (errno != EAGAIN)
                    {
                        err = ERR_WRITE;
                    }
                }
            }
            else
            {
                err = ERR_ENQUEUE;
            }
        }
    }
    return err;
}

error_t Reactor::init()
{
    auto err = ERR_OK;

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ > UNKNOWN_FD)
    {
        event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (event_fd_ > UNKNOWN_FD)
        {
            add_epoll_callback
            (
                event_fd_,
                [this](epoll_event_t events)
                {
                    if ((events & EPOLLERR) || (events & EPOLLHUP) || (events & EPOLLRDHUP))
                    {
                        if (errno != EAGAIN)
                        {
                            exit();
                        }
                    }
                    else
                    {
                        if (events & EPOLLIN)
                        {
                            const auto size = sizeof(uint64_t);
                            char data[size];
                            auto nread = read(event_fd_, data, size);
                            if (nread <= 0)
                            {
                                if (errno != EAGAIN)
                                {
                                    exit();
                                }
                            }
                        }
                    }
                }
            );
            err = bind(event_fd_, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
        }
        else
        {
            err = ERR_INVALID_EVENT_FD;
        }
    }
    else
    {
        err = ERR_INVALID_EPOLL_FD;
    }

    return err;
}

void Reactor::run_in_loop(int32_t timeout_ms)
{
    thread_id_ = std::this_thread::get_id();
    struct epoll_event *events = (struct epoll_event *)malloc(max_events_num_ * sizeof(struct epoll_event));
    ready_ = true;
    while (ready_)
    {
        memset(events, 0, max_events_num_ * sizeof(struct epoll_event));
        int nevents = epoll_wait(epoll_fd_, events, max_events_num_, timeout_ms);
        if (nevents >= 0)
        {
            for (int i = 0; i < nevents; ++i)
            {
                int fd = events[i].data.fd;
                epoll_callback_map_[fd](events[i].events);
            }
            event_callback_t event_callback;
            while (event_callback_queue_.try_dequeue(event_callback))
            {
                event_callback();
            }
        }
        else
        {
            exit();
        }
    }
    free(events);

    event_callback_t event_callback;
    while (event_callback_queue_.try_dequeue(event_callback))
    {
        event_callback();
    }

    unbind(event_fd_);
    del_epoll_callback(event_fd_);
    close(event_fd_);
    event_fd_ = UNKNOWN_FD;

    close(epoll_fd_);
    epoll_fd_ = UNKNOWN_FD;
    for (auto &exit_callback : exit_callback_vec_)
    {
        exit_callback();
    }
}

}
#endif