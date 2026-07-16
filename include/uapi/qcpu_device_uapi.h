#ifndef QCPU_DEVICE_UAPI_H
#define QCPU_DEVICE_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

/*
 * QBIT NOVA C v4.7 Linux device UAPI.
 *
 * This header defines a fixed-width userspace ABI only.
 * It contains no state-vector representation and no floating-point fields.
 *
 * The v4.7 frontend is a software Virtual QCPU interface. Computation remains
 * in the verified userspace qcpud/qcpu_kernel backend on a classical host.
 */

#define QCPU_UAPI_ABI_VERSION             1U
#define QCPU_UAPI_PROTOCOL_VERSION        1U

#define QCPU_UAPI_REQUEST_MAGIC           0x51445631U
#define QCPU_UAPI_RESPONSE_MAGIC          0x51525631U

#define QCPU_UAPI_MAX_QUBITS              20U
#define QCPU_UAPI_MAX_SHOTS               100U
#define QCPU_UAPI_MAX_INFLIGHT            1U

#define QCPU_UAPI_BACKEND_NAME_BYTES      32U
#define QCPU_UAPI_DRIVER_VERSION_BYTES    16U

#define QCPU_UAPI_COMMAND_STATUS          1U
#define QCPU_UAPI_COMMAND_RUN_GHZ         2U

#define QCPU_UAPI_STATUS_OK               0U
#define QCPU_UAPI_STATUS_BAD_MAGIC        1U
#define QCPU_UAPI_STATUS_BAD_VERSION      2U
#define QCPU_UAPI_STATUS_BAD_COMMAND      3U
#define QCPU_UAPI_STATUS_RANGE            4U
#define QCPU_UAPI_STATUS_KERNEL           5U
#define QCPU_UAPI_STATUS_IO               6U
#define QCPU_UAPI_STATUS_BACKEND_ABSENT   7U
#define QCPU_UAPI_STATUS_BUSY             8U
#define QCPU_UAPI_STATUS_TIMEOUT          9U
#define QCPU_UAPI_STATUS_CANCELED         10U

#define QCPU_UAPI_FLAG_SOFTWARE_VIRTUAL_QCPU  (1U << 0)
#define QCPU_UAPI_FLAG_CLASSICAL_HOST         (1U << 1)
#define QCPU_UAPI_FLAG_PHYSICAL_QPU_ABSENT    (1U << 2)
#define QCPU_UAPI_FLAG_Q0_MOST_SIGNIFICANT    (1U << 3)

#define QCPU_UAPI_DEVICE_OFFLINE          0U
#define QCPU_UAPI_DEVICE_IDLE             1U
#define QCPU_UAPI_DEVICE_BUSY             2U
#define QCPU_UAPI_DEVICE_ERROR            3U

/*
 * Semantically mirrors the v4.6 24-byte request.
 * All reserved fields in every UAPI structure must be zero.
 */
struct qcpu_uapi_request_v1 {
    __u32 magic;
    __u16 version;
    __u16 command;
    __u32 qubits;
    __u32 shots;
    __u64 seed;
};

/*
 * Semantically mirrors the v4.6 56-byte response.
 *
 * norm_q32_32 replaces the userspace double used by v4.6:
 *   1.0 == 1ULL << 32
 *
 * Conversion is performed by userspace. Kernel code does no quantum math and
 * no floating-point conversion.
 */
struct qcpu_uapi_response_v1 {
    __u32 magic;
    __u16 version;
    __u16 status;
    __u32 flags;
    __u32 qubits;
    __u64 basis_states;
    __u64 shots;
    __u64 measured_state;
    __u64 invalid_results;
    __u64 norm_q32_32;
};

struct qcpu_uapi_caps_v1 {
    __u32 abi_version;
    __u32 protocol_version;
    __u32 max_qubits;
    __u32 max_shots;
    __u32 max_inflight;
    __u32 flags;
    __u8 backend[QCPU_UAPI_BACKEND_NAME_BYTES];
    __u8 driver_version[QCPU_UAPI_DRIVER_VERSION_BYTES];
    __u8 reserved[8];
};

struct qcpu_uapi_status_v1 {
    __u32 device_state;
    __u32 active_client_sessions;
    __s32 last_errno;
    __u32 last_protocol_status;
    __u64 completed_requests;
    __u64 failed_requests;
};

/*
 * Synchronous bounded transaction.
 *
 * timeout_ns == 0 requests the implementation-defined safe default.
 * A nonzero timeout is measured against CLOCK_MONOTONIC and must be clamped
 * to the documented maximum.
 */
struct qcpu_uapi_exchange_v1 {
    struct qcpu_uapi_request_v1 request;
    __u64 timeout_ns;
    struct qcpu_uapi_response_v1 response;
};

#define QCPU_UAPI_IOC_MAGIC               'Q'

#define QCPU_IOC_GET_CAPS_V1 \
    _IOR(QCPU_UAPI_IOC_MAGIC, 0x00, struct qcpu_uapi_caps_v1)

#define QCPU_IOC_GET_STATUS_V1 \
    _IOR(QCPU_UAPI_IOC_MAGIC, 0x01, struct qcpu_uapi_status_v1)

#define QCPU_IOC_EXCHANGE_V1 \
    _IOWR(QCPU_UAPI_IOC_MAGIC, 0x10, struct qcpu_uapi_exchange_v1)

#endif
