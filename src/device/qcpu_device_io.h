#ifndef QCPU_DEVICE_IO_H
#define QCPU_DEVICE_IO_H

#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#define QCPU_DEVICE_IO_TIMEOUT_MS 2000

typedef enum {
    QCPU_DEVICE_IO_OK = 0,
    QCPU_DEVICE_IO_ERROR = -1,
    QCPU_DEVICE_IO_TIMEOUT = -2,
    QCPU_DEVICE_IO_STOPPED = -3
} QCPUDeviceIOStatus;

QCPUDeviceIOStatus qcpu_device_read_all(
    int file_descriptor,
    uint8_t *buffer,
    size_t size,
    int timeout_ms,
    const volatile sig_atomic_t *stop_flag
);

QCPUDeviceIOStatus qcpu_device_write_all(
    int file_descriptor,
    const uint8_t *buffer,
    size_t size,
    int timeout_ms,
    const volatile sig_atomic_t *stop_flag
);

int qcpu_device_connect_unix(
    const char *socket_path,
    int timeout_ms
);

#endif
