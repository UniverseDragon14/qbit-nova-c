#ifndef QCPU_MOCK_TEST_BACKEND_H
#define QCPU_MOCK_TEST_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

#include "src/device/qcpu_mock_frontend.h"

enum qcpu_test_backend_mode {
    QCPU_TEST_BACKEND_SUCCESS = 0,
    QCPU_TEST_BACKEND_SUCCESS_ZERO,
    QCPU_TEST_BACKEND_SUCCESS_ONE,
    QCPU_TEST_BACKEND_DELAY_UNTIL_CANCELED,
    QCPU_TEST_BACKEND_IGNORE_CANCEL_UNTIL_RELEASE,
    QCPU_TEST_BACKEND_DISCONNECT,
    QCPU_TEST_BACKEND_BAD_RESPONSE_MAGIC,
    QCPU_TEST_BACKEND_BAD_RESPONSE_VERSION,
    QCPU_TEST_BACKEND_MISSING_FLAGS,
    QCPU_TEST_BACKEND_QUBIT_MISMATCH,
    QCPU_TEST_BACKEND_SHOTS_MISMATCH,
    QCPU_TEST_BACKEND_BAD_BASIS_STATES,
    QCPU_TEST_BACKEND_BAD_MEASURED_STATE,
    QCPU_TEST_BACKEND_NONZERO_INVALID_RESULTS,
    QCPU_TEST_BACKEND_NORM_TOO_HIGH,
    QCPU_TEST_BACKEND_UNKNOWN_STATUS,
    QCPU_TEST_BACKEND_KNOWN_KERNEL_STATUS,
    QCPU_TEST_BACKEND_ENGINE_FAILURE
};

struct qcpu_test_backend;

int qcpu_test_backend_create(
    struct qcpu_test_backend **out
);

int qcpu_test_backend_destroy(
    struct qcpu_test_backend *backend
);

void qcpu_test_backend_set_mode(
    struct qcpu_test_backend *backend,
    enum qcpu_test_backend_mode mode
);

void qcpu_test_backend_set_norm(
    struct qcpu_test_backend *backend,
    double norm
);

int qcpu_test_backend_wait_started(
    struct qcpu_test_backend *backend,
    uint64_t timeout_ns
);

uint64_t qcpu_test_backend_dispatches(
    struct qcpu_test_backend *backend
);

uint64_t qcpu_test_backend_cancels(
    struct qcpu_test_backend *backend
);

uint32_t qcpu_test_backend_max_concurrent(
    struct qcpu_test_backend *backend
);

uint32_t qcpu_test_backend_waiters(
    struct qcpu_test_backend *backend
);

void qcpu_test_backend_release(
    struct qcpu_test_backend *backend
);

int qcpu_test_backend_wait_idle(
    struct qcpu_test_backend *backend,
    uint64_t timeout_ns
);

const struct qcpu_backend_ops *qcpu_test_backend_ops(void);

#endif
