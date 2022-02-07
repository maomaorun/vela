#ifndef VELA_MISC_SIGNAL_H
#define VELA_MISC_SIGNAL_H

#include <functional>
#include <vector>
#include <unordered_map>
#include "common.h"
#include "Reactor.h"

namespace vela::misc
{

class Signal final
{
public:
    using signal_t  = int;
    using close_callback_t  = std::function<void (error_t)>;
    using signal_callback_t = std::function<void (signal_t)>;
private:
    using signal_callback_map_t = std::unordered_map<signal_t, signal_callback_t>;
    using close_callback_vec_t  = std::vector<close_callback_t>;
public:
    explicit Signal(Reactor &reactor) noexcept : 
            reactor_{ reactor },
            signal_fd_{ UNKNOWN_FD } {}

    ~Signal() { close(); }

    Signal(const Signal &)              = delete;
    Signal(Signal &&)                   = delete;
    Signal & operator=(const Signal &)  = delete;
    Signal & operator=(Signal &&)       = delete;

    void on_signal(std::initializer_list<signal_t> signals, signal_callback_t &&signal_callback);

    void on_close(close_callback_t &&close_callback)
    { close_callback_vec_.emplace_back(std::move(close_callback)); }

    void run();
    void close();
private:
    Reactor                 &reactor_;
    fd_t                    signal_fd_;
    signal_callback_map_t   signal_callback_map_;
    close_callback_vec_t    close_callback_vec_;
};

void Signal::on_signal(std::initializer_list<signal_t> signals, signal_callback_t &&signal_callback)
{
    for (auto signal : signals)
    {
        signal_callback_map_.emplace(signal, std::move(signal_callback));
    }
}

void Signal::run()
{
    sigset_t mask;
    sigemptyset(&mask);
    for (const auto &[signal, signal_callback] : signal_callback_map_)
    {
        ::sigaddset(&mask, signal);
    }
    if (::sigprocmask(SIG_BLOCK, &mask, NULL) >= 0)
    {
        signal_fd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    }
}

}
#endif