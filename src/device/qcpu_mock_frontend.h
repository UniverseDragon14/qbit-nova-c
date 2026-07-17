#ifndef QCPU_MOCK_FRONTEND_H
#define QCPU_MOCK_FRONTEND_H

#include <stddef.h>
#include <stdint.h>

#include "include/uapi/qcpu_device_uapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QCPU_MOCK_DEFAULT_TIMEOUT_NS UINT64_C(1000000000)
#define QCPU_MOCK_MAX_TIMEOUT_NS     UINT64_C(5000000000)
#define QCPU_MOCK_REQUIRED_FLAGS \
    (QCPU_UAPI_FLAG_SOFTWARE_VIRTUAL_QCPU | \
     QCPU_UAPI_FLAG_CLASSICAL_HOST | \
     QCPU_UAPI_FLAG_PHYSICAL_QPU_ABSENT | \
     QCPU_UAPI_FLAG_Q0_MOST_SIGNIFICANT)

struct qcpu_mock;

struct qcpu_backend_ops {
    int (*exchange)(
        void *context,
        const struct qcpu_uapi_request_v1 *request,
        uint64_t absolute_deadline_ns,
        struct qcpu_uapi_response_v1 *response
    );
    int (*cancel)(void *context);
};

struct qcpu_mock_config {
    const char *runtime_dir;
    const char *backend_name;
    const char *driver_version;
    const struct qcpu_backend_ops *backend_ops;
    void *backend_context;
};

int qcpu_mock_create(
    struct qcpu_mock **out,
    const struct qcpu_mock_config *config
);

int qcpu_mock_set_backend(
    struct qcpu_mock *mock,
    const struct qcpu_backend_ops *backend_ops,
    void *backend_context,
    const char *backend_name
);

int qcpu_mock_open(struct qcpu_mock *mock);

int qcpu_mock_ioctl(
    struct qcpu_mock *mock,
    unsigned long request,
    void *argument
);

int qcpu_mock_close(struct qcpu_mock *mock);

int qcpu_mock_destroy(struct qcpu_mock *mock);

int qcpu_mock_norm_to_q32_32(
    double norm,
    uint64_t *out
);

#ifdef __cplusplus
}
#endif

#endif
