#ifndef VELA_COMMON_H
#define VELA_COMMON_H

#include <cstdint>

namespace vela
{

/** typedef */
using error_t       = int32_t;
using fd_t          = int;
using epoll_event_t = uint32_t;

/** constants */
static constexpr fd_t UNKNOWN_FD                = 0;
static constexpr int DEFAULT_EPOLL_EVENTS_NUM   = 128;

/** errors */
static constexpr error_t ERR_OK                 = 0;
static constexpr error_t ERR_INVALID_FD         = 1;
static constexpr error_t ERR_FD_NOT_FOUND       = 2;
static constexpr error_t ERR_EVENT              = 3;
static constexpr error_t ERR_EPOLL_CTL_ADD      = 4;
static constexpr error_t ERR_EPOLL_CTL_DEL      = 5;
static constexpr error_t ERR_ENQUEUE            = 6;
static constexpr error_t ERR_DEQUEUE            = 7;
static constexpr error_t ERR_WRITE              = 8;
static constexpr error_t ERR_READ               = 9;
static constexpr error_t ERR_INVALID_EPOLL_FD   = 10;
static constexpr error_t ERR_INVALID_EVENT_FD   = 11;
static constexpr error_t ERR_EPOLL_WAIT         = 12;

}

#endif