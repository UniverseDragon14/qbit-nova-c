#ifndef QCPU_DEVICE_PROTOCOL_H
#define QCPU_DEVICE_PROTOCOL_H

#include <stdint.h>

#define QCPU_DEVICE_PROTOCOL_VERSION UINT16_C(1)

#define QCPU_DEVICE_REQUEST_MAGIC \
    UINT32_C(0x51445631)

#define QCPU_DEVICE_RESPONSE_MAGIC \
    UINT32_C(0x51525631)

#define QCPU_DEVICE_MAX_QUBITS UINT32_C(20)
#define QCPU_DEVICE_MAX_SHOTS UINT32_C(100)

typedef enum {
    QCPU_DEVICE_COMMAND_STATUS = 1,
    QCPU_DEVICE_COMMAND_RUN_GHZ = 2
} QCPUDeviceCommand;

typedef enum {
    QCPU_DEVICE_STATUS_OK = 0,
    QCPU_DEVICE_STATUS_BAD_MAGIC = 1,
    QCPU_DEVICE_STATUS_BAD_VERSION = 2,
    QCPU_DEVICE_STATUS_BAD_COMMAND = 3,
    QCPU_DEVICE_STATUS_RANGE = 4,
    QCPU_DEVICE_STATUS_KERNEL = 5,
    QCPU_DEVICE_STATUS_IO = 6
} QCPUDeviceStatus;

typedef enum {
    QCPU_DEVICE_FLAG_SOFTWARE_VIRTUAL_QCPU =
        UINT32_C(1) << 0,

    QCPU_DEVICE_FLAG_CLASSICAL_HOST =
        UINT32_C(1) << 1,

    QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT =
        UINT32_C(1) << 2,

    QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT =
        UINT32_C(1) << 3
} QCPUDeviceFlags;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t command;
    uint32_t qubits;
    uint32_t shots;
    uint64_t seed;
} QCPUDeviceRequest;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t status;
    uint32_t flags;
    uint32_t qubits;
    uint64_t basis_states;
    uint64_t shots;
    uint64_t measured_state;
    uint64_t invalid_results;
    double norm;
} QCPUDeviceResponse;

_Static_assert(
    sizeof(QCPUDeviceRequest) == 24U,
    "QCPUDeviceRequest layout changed"
);

_Static_assert(
    sizeof(QCPUDeviceResponse) == 56U,
    "QCPUDeviceResponse layout changed"
);

#endif
