#define _POSIX_C_SOURCE 200809L

#include "qcpu_device_io.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int64_t monotonic_milliseconds(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
    }

    return
        ((int64_t)now.tv_sec * INT64_C(1000)) +
        ((int64_t)now.tv_nsec / INT64_C(1000000));
}

static QCPUDeviceIOStatus set_nonblocking(
    int file_descriptor
) {
    int flags = fcntl(file_descriptor, F_GETFL, 0);

    if (flags < 0) {
        return QCPU_DEVICE_IO_ERROR;
    }

    if ((flags & O_NONBLOCK) != 0) {
        return QCPU_DEVICE_IO_OK;
    }

    if (
        fcntl(
            file_descriptor,
            F_SETFL,
            flags | O_NONBLOCK
        ) != 0
    ) {
        return QCPU_DEVICE_IO_ERROR;
    }

    return QCPU_DEVICE_IO_OK;
}

static QCPUDeviceIOStatus wait_until_ready(
    int file_descriptor,
    short events,
    int64_t deadline_ms,
    const volatile sig_atomic_t *stop_flag
) {
    struct pollfd descriptor = {
        .fd = file_descriptor,
        .events = events,
        .revents = 0
    };

    for (;;) {
        int64_t now_ms;
        int64_t remaining_ms;
        int poll_timeout;
        int result;

        if (stop_flag != NULL && *stop_flag != 0) {
            return QCPU_DEVICE_IO_STOPPED;
        }

        now_ms = monotonic_milliseconds();

        if (now_ms < 0) {
            return QCPU_DEVICE_IO_ERROR;
        }

        remaining_ms = deadline_ms - now_ms;

        if (remaining_ms <= 0) {
            return QCPU_DEVICE_IO_TIMEOUT;
        }

        poll_timeout =
            remaining_ms > INT_MAX
                ? INT_MAX
                : (int)remaining_ms;

        descriptor.revents = 0;

        result = poll(
            &descriptor,
            1,
            poll_timeout
        );

        if (result > 0) {
            if ((descriptor.revents & POLLNVAL) != 0) {
                return QCPU_DEVICE_IO_ERROR;
            }

            if ((descriptor.revents & events) != 0) {
                return QCPU_DEVICE_IO_OK;
            }

            if (
                (descriptor.revents & (POLLERR | POLLHUP)) != 0
            ) {
                return QCPU_DEVICE_IO_ERROR;
            }

            continue;
        }

        if (result == 0) {
            return QCPU_DEVICE_IO_TIMEOUT;
        }

        if (errno == EINTR) {
            continue;
        }

        return QCPU_DEVICE_IO_ERROR;
    }
}

static QCPUDeviceIOStatus prepare_deadline(
    int timeout_ms,
    int64_t *deadline_ms
) {
    int64_t now_ms;

    if (timeout_ms <= 0 || deadline_ms == NULL) {
        return QCPU_DEVICE_IO_ERROR;
    }

    now_ms = monotonic_milliseconds();

    if (now_ms < 0) {
        return QCPU_DEVICE_IO_ERROR;
    }

    *deadline_ms = now_ms + (int64_t)timeout_ms;

    return QCPU_DEVICE_IO_OK;
}

static ssize_t write_without_sigpipe(
    int file_descriptor,
    const uint8_t *buffer,
    size_t size
) {
#ifdef MSG_NOSIGNAL
    return send(
        file_descriptor,
        buffer,
        size,
        MSG_NOSIGNAL
    );
#else
    return write(
        file_descriptor,
        buffer,
        size
    );
#endif
}

QCPUDeviceIOStatus qcpu_device_read_all(
    int file_descriptor,
    uint8_t *buffer,
    size_t size,
    int timeout_ms,
    const volatile sig_atomic_t *stop_flag
) {
    size_t completed = 0U;
    int64_t deadline_ms;

    if (
        file_descriptor < 0 ||
        buffer == NULL ||
        prepare_deadline(
            timeout_ms,
            &deadline_ms
        ) != QCPU_DEVICE_IO_OK ||
        set_nonblocking(file_descriptor) != QCPU_DEVICE_IO_OK
    ) {
        return QCPU_DEVICE_IO_ERROR;
    }

    while (completed < size) {
        ssize_t result;

        if (stop_flag != NULL && *stop_flag != 0) {
            return QCPU_DEVICE_IO_STOPPED;
        }

        result = read(
            file_descriptor,
            buffer + completed,
            size - completed
        );

        if (result > 0) {
            completed += (size_t)result;
            continue;
        }

        if (result == 0) {
            return QCPU_DEVICE_IO_ERROR;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            QCPUDeviceIOStatus wait_status =
                wait_until_ready(
                    file_descriptor,
                    POLLIN,
                    deadline_ms,
                    stop_flag
                );

            if (wait_status != QCPU_DEVICE_IO_OK) {
                return wait_status;
            }

            continue;
        }

        return QCPU_DEVICE_IO_ERROR;
    }

    return QCPU_DEVICE_IO_OK;
}

QCPUDeviceIOStatus qcpu_device_write_all(
    int file_descriptor,
    const uint8_t *buffer,
    size_t size,
    int timeout_ms,
    const volatile sig_atomic_t *stop_flag
) {
    size_t completed = 0U;
    int64_t deadline_ms;

    if (
        file_descriptor < 0 ||
        buffer == NULL ||
        prepare_deadline(
            timeout_ms,
            &deadline_ms
        ) != QCPU_DEVICE_IO_OK ||
        set_nonblocking(file_descriptor) != QCPU_DEVICE_IO_OK
    ) {
        return QCPU_DEVICE_IO_ERROR;
    }

    while (completed < size) {
        ssize_t result;

        if (stop_flag != NULL && *stop_flag != 0) {
            return QCPU_DEVICE_IO_STOPPED;
        }

        result = write_without_sigpipe(
            file_descriptor,
            buffer + completed,
            size - completed
        );

        if (result > 0) {
            completed += (size_t)result;
            continue;
        }

        if (result == 0) {
            return QCPU_DEVICE_IO_ERROR;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            QCPUDeviceIOStatus wait_status =
                wait_until_ready(
                    file_descriptor,
                    POLLOUT,
                    deadline_ms,
                    stop_flag
                );

            if (wait_status != QCPU_DEVICE_IO_OK) {
                return wait_status;
            }

            continue;
        }

        return QCPU_DEVICE_IO_ERROR;
    }

    return QCPU_DEVICE_IO_OK;
}

int qcpu_device_connect_unix(
    const char *socket_path,
    int timeout_ms
) {
    int file_descriptor;
    struct sockaddr_un address;
    int64_t deadline_ms;
    QCPUDeviceIOStatus wait_status;
    int socket_error = 0;
    socklen_t socket_error_size =
        (socklen_t)sizeof(socket_error);

    if (
        socket_path == NULL ||
        timeout_ms <= 0 ||
        strlen(socket_path) >= sizeof(address.sun_path)
    ) {
        errno = EINVAL;
        return QCPU_DEVICE_IO_ERROR;
    }

    file_descriptor = socket(
        AF_UNIX,
        SOCK_STREAM,
        0
    );

    if (file_descriptor < 0) {
        return QCPU_DEVICE_IO_ERROR;
    }

    if (
        set_nonblocking(file_descriptor) !=
        QCPU_DEVICE_IO_OK
    ) {
        close(file_descriptor);
        return QCPU_DEVICE_IO_ERROR;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    memcpy(
        address.sun_path,
        socket_path,
        strlen(socket_path) + 1U
    );

    if (
        connect(
            file_descriptor,
            (struct sockaddr *)&address,
            sizeof(address)
        ) == 0
    ) {
        return file_descriptor;
    }

    if (errno != EINPROGRESS && errno != EAGAIN) {
        close(file_descriptor);
        return QCPU_DEVICE_IO_ERROR;
    }

    if (
        prepare_deadline(
            timeout_ms,
            &deadline_ms
        ) != QCPU_DEVICE_IO_OK
    ) {
        close(file_descriptor);
        return QCPU_DEVICE_IO_ERROR;
    }

    wait_status = wait_until_ready(
        file_descriptor,
        POLLOUT,
        deadline_ms,
        NULL
    );

    if (wait_status != QCPU_DEVICE_IO_OK) {
        close(file_descriptor);
        return (int)wait_status;
    }

    if (
        getsockopt(
            file_descriptor,
            SOL_SOCKET,
            SO_ERROR,
            &socket_error,
            &socket_error_size
        ) != 0
    ) {
        close(file_descriptor);
        return QCPU_DEVICE_IO_ERROR;
    }

    if (socket_error != 0) {
        errno = socket_error;
        close(file_descriptor);
        return QCPU_DEVICE_IO_ERROR;
    }

    return file_descriptor;
}
