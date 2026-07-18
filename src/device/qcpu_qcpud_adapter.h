#ifndef QCPU_QCPUD_ADAPTER_H
#define QCPU_QCPUD_ADAPTER_H

#include "qcpu_mock_frontend.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QCPU_QCPUD_ADAPTER_BACKEND_NAME "qcpud-v4.6"

struct qcpu_qcpud_adapter;

int qcpu_qcpud_adapter_create(
    struct qcpu_qcpud_adapter **out,
    const char *socket_path
);

int qcpu_qcpud_adapter_destroy(
    struct qcpu_qcpud_adapter *adapter
);

const struct qcpu_backend_ops *qcpu_qcpud_adapter_ops(void);

#ifdef __cplusplus
}
#endif

#endif
