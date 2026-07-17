#define _POSIX_C_SOURCE 200809L

#include "tests/v47/qcpu_mock_test_backend.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct qcpu_test_backend {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    enum qcpu_test_backend_mode mode;
    double norm;
    bool cancel_requested;
    bool started;
    uint64_t dispatches;
    uint64_t cancels;
    uint32_t active;
    uint32_t max_concurrent;
};

static uint64_t qcpu_test_timespec_to_ns(const struct timespec *value)
{
    return ((uint64_t)value->tv_sec * UINT64_C(1000000000)) +
           (uint64_t)value->tv_nsec;
}

static struct timespec qcpu_test_ns_to_timespec(uint64_t value)
{
    struct timespec result;

    result.tv_sec = (time_t)(value / UINT64_C(1000000000));
    result.tv_nsec = (long)(value % UINT64_C(1000000000));

    return result;
}

static void qcpu_test_fill_success_response(
    const struct qcpu_uapi_request_v1 *request,
    struct qcpu_uapi_response_v1 *response,
    uint64_t norm_q32_32,
    bool one_state
)
{
    memset(response, 0, sizeof(*response));

    response->magic = QCPU_UAPI_RESPONSE_MAGIC;
    response->version = QCPU_UAPI_PROTOCOL_VERSION;
    response->status = QCPU_UAPI_STATUS_OK;
    response->flags = QCPU_MOCK_REQUIRED_FLAGS;
    response->qubits = request->qubits;
    response->shots = request->shots;
    response->norm_q32_32 = norm_q32_32;

    if (request->command == QCPU_UAPI_COMMAND_RUN_GHZ) {
        response->basis_states = UINT64_C(1) << request->qubits;

        if (one_state) {
            response->measured_state =
                (UINT64_C(1) << request->qubits) -
                UINT64_C(1);
        }
    }
}

static int qcpu_test_backend_exchange(
    void *context,
    const struct qcpu_uapi_request_v1 *request,
    uint64_t absolute_deadline_ns,
    struct qcpu_uapi_response_v1 *response
)
{
    struct qcpu_test_backend *backend = context;
    enum qcpu_test_backend_mode mode;
    double norm;
    uint64_t converted_norm = 0U;
    int result = 0;

    (void)absolute_deadline_ns;

    if (backend == NULL || request == NULL || response == NULL) {
        return -EINVAL;
    }

    (void)pthread_mutex_lock(&backend->mutex);

    backend->dispatches++;
    backend->active++;

    if (backend->active > backend->max_concurrent) {
        backend->max_concurrent = backend->active;
    }

    backend->started = true;
    backend->cancel_requested = false;
    mode = backend->mode;
    norm = backend->norm;

    (void)pthread_cond_broadcast(&backend->condition);

    if (mode == QCPU_TEST_BACKEND_DELAY_UNTIL_CANCELED) {
        while (!backend->cancel_requested) {
            (void)pthread_cond_wait(
                &backend->condition,
                &backend->mutex
            );
        }

        backend->active--;
        (void)pthread_cond_broadcast(&backend->condition);
        (void)pthread_mutex_unlock(&backend->mutex);

        return -ECANCELED;
    }

    (void)pthread_mutex_unlock(&backend->mutex);

    if (mode == QCPU_TEST_BACKEND_DISCONNECT) {
        result = -ENODEV;
        goto complete;
    }

    if (mode == QCPU_TEST_BACKEND_ENGINE_FAILURE) {
        memset(response, 0, sizeof(*response));
        response->status = QCPU_UAPI_STATUS_KERNEL;
        result = -EIO;
        goto complete;
    }

    if (qcpu_mock_norm_to_q32_32(norm, &converted_norm) != 0) {
        memset(response, 0, sizeof(*response));
        response->magic = QCPU_UAPI_RESPONSE_MAGIC;
        response->version = QCPU_UAPI_PROTOCOL_VERSION;
        response->status = QCPU_UAPI_STATUS_KERNEL;
        response->flags = QCPU_MOCK_REQUIRED_FLAGS;
        response->qubits = request->qubits;
        response->shots = request->shots;
        result = 0;
        goto complete;
    }

    qcpu_test_fill_success_response(
        request,
        response,
        converted_norm,
        mode == QCPU_TEST_BACKEND_SUCCESS_ONE
    );

    switch (mode) {
    case QCPU_TEST_BACKEND_SUCCESS:
    case QCPU_TEST_BACKEND_SUCCESS_ZERO:
    case QCPU_TEST_BACKEND_SUCCESS_ONE:
        break;
    case QCPU_TEST_BACKEND_BAD_RESPONSE_MAGIC:
        response->magic ^= 1U;
        break;
    case QCPU_TEST_BACKEND_BAD_RESPONSE_VERSION:
        response->version++;
        break;
    case QCPU_TEST_BACKEND_MISSING_FLAGS:
        response->flags &=
            ~QCPU_UAPI_FLAG_PHYSICAL_QPU_ABSENT;
        break;
    case QCPU_TEST_BACKEND_QUBIT_MISMATCH:
        response->qubits++;
        break;
    case QCPU_TEST_BACKEND_SHOTS_MISMATCH:
        response->shots++;
        break;
    case QCPU_TEST_BACKEND_BAD_BASIS_STATES:
        response->basis_states++;
        break;
    case QCPU_TEST_BACKEND_BAD_MEASURED_STATE:
        response->measured_state = 1U;
        if (request->qubits == 1U) {
            response->measured_state = 2U;
        }
        break;
    case QCPU_TEST_BACKEND_NONZERO_INVALID_RESULTS:
        response->invalid_results = 1U;
        break;
    case QCPU_TEST_BACKEND_NORM_TOO_HIGH:
        response->norm_q32_32 =
            (UINT64_C(1) << 32) + UINT64_C(1);
        break;
    case QCPU_TEST_BACKEND_UNKNOWN_STATUS:
        response->status = UINT16_MAX;
        break;
    case QCPU_TEST_BACKEND_KNOWN_KERNEL_STATUS:
        response->status = QCPU_UAPI_STATUS_KERNEL;
        break;
    case QCPU_TEST_BACKEND_DELAY_UNTIL_CANCELED:
    case QCPU_TEST_BACKEND_DISCONNECT:
    case QCPU_TEST_BACKEND_ENGINE_FAILURE:
        break;
    }

complete:
    (void)pthread_mutex_lock(&backend->mutex);
    backend->active--;
    (void)pthread_cond_broadcast(&backend->condition);
    (void)pthread_mutex_unlock(&backend->mutex);

    return result;
}

static int qcpu_test_backend_cancel(void *context)
{
    struct qcpu_test_backend *backend = context;

    if (backend == NULL) {
        return -EINVAL;
    }

    (void)pthread_mutex_lock(&backend->mutex);
    backend->cancels++;
    backend->cancel_requested = true;
    (void)pthread_cond_broadcast(&backend->condition);
    (void)pthread_mutex_unlock(&backend->mutex);

    return 0;
}

static const struct qcpu_backend_ops QCPU_TEST_BACKEND_OPS = {
    .exchange = qcpu_test_backend_exchange,
    .cancel = qcpu_test_backend_cancel
};

int qcpu_test_backend_create(
    struct qcpu_test_backend **out
)
{
    struct qcpu_test_backend *backend;
    pthread_condattr_t attributes;
    bool attributes_ready = false;
    bool mutex_ready = false;
    int result;

    if (out == NULL) {
        errno = EINVAL;
        return -1;
    }

    *out = NULL;

    backend = calloc(1U, sizeof(*backend));
    if (backend == NULL) {
        return -1;
    }

    backend->mode = QCPU_TEST_BACKEND_SUCCESS;
    backend->norm = 1.0;

    result = pthread_mutex_init(&backend->mutex, NULL);
    if (result != 0) {
        free(backend);
        errno = result;
        return -1;
    }
    mutex_ready = true;

    result = pthread_condattr_init(&attributes);
    if (result != 0) {
        (void)pthread_mutex_destroy(&backend->mutex);
        free(backend);
        errno = result;
        return -1;
    }
    attributes_ready = true;

    result = pthread_condattr_setclock(
        &attributes,
        CLOCK_MONOTONIC
    );
    if (result != 0) {
        goto failure;
    }

    result = pthread_cond_init(
        &backend->condition,
        &attributes
    );
    if (result != 0) {
        goto failure;
    }

    (void)pthread_condattr_destroy(&attributes);
    *out = backend;

    return 0;

failure:
    if (attributes_ready) {
        (void)pthread_condattr_destroy(&attributes);
    }
    if (mutex_ready) {
        (void)pthread_mutex_destroy(&backend->mutex);
    }
    free(backend);
    errno = result;

    return -1;
}

int qcpu_test_backend_destroy(
    struct qcpu_test_backend *backend
)
{
    if (backend == NULL) {
        errno = EINVAL;
        return -1;
    }

    (void)pthread_mutex_lock(&backend->mutex);

    if (backend->active != 0U) {
        (void)pthread_mutex_unlock(&backend->mutex);
        errno = EBUSY;
        return -1;
    }

    (void)pthread_mutex_unlock(&backend->mutex);
    (void)pthread_cond_destroy(&backend->condition);
    (void)pthread_mutex_destroy(&backend->mutex);
    free(backend);

    return 0;
}

void qcpu_test_backend_set_mode(
    struct qcpu_test_backend *backend,
    enum qcpu_test_backend_mode mode
)
{
    (void)pthread_mutex_lock(&backend->mutex);
    backend->mode = mode;
    backend->started = false;
    backend->cancel_requested = false;
    (void)pthread_mutex_unlock(&backend->mutex);
}

void qcpu_test_backend_set_norm(
    struct qcpu_test_backend *backend,
    double norm
)
{
    (void)pthread_mutex_lock(&backend->mutex);
    backend->norm = norm;
    backend->started = false;
    (void)pthread_mutex_unlock(&backend->mutex);
}

int qcpu_test_backend_wait_started(
    struct qcpu_test_backend *backend,
    uint64_t timeout_ns
)
{
    struct timespec now;
    struct timespec deadline;
    uint64_t now_ns;
    int result = 0;

    if (backend == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return -1;
    }

    now_ns = qcpu_test_timespec_to_ns(&now);

    if (UINT64_MAX - now_ns < timeout_ns) {
        errno = ERANGE;
        return -1;
    }

    deadline = qcpu_test_ns_to_timespec(now_ns + timeout_ns);

    (void)pthread_mutex_lock(&backend->mutex);

    while (!backend->started) {
        result = pthread_cond_timedwait(
            &backend->condition,
            &backend->mutex,
            &deadline
        );

        if (result == ETIMEDOUT) {
            (void)pthread_mutex_unlock(&backend->mutex);
            errno = ETIMEDOUT;
            return -1;
        }

        if (result != 0 && result != EINTR) {
            (void)pthread_mutex_unlock(&backend->mutex);
            errno = result;
            return -1;
        }
    }

    (void)pthread_mutex_unlock(&backend->mutex);

    return 0;
}

uint64_t qcpu_test_backend_dispatches(
    struct qcpu_test_backend *backend
)
{
    uint64_t value;

    (void)pthread_mutex_lock(&backend->mutex);
    value = backend->dispatches;
    (void)pthread_mutex_unlock(&backend->mutex);

    return value;
}

uint64_t qcpu_test_backend_cancels(
    struct qcpu_test_backend *backend
)
{
    uint64_t value;

    (void)pthread_mutex_lock(&backend->mutex);
    value = backend->cancels;
    (void)pthread_mutex_unlock(&backend->mutex);

    return value;
}

uint32_t qcpu_test_backend_max_concurrent(
    struct qcpu_test_backend *backend
)
{
    uint32_t value;

    (void)pthread_mutex_lock(&backend->mutex);
    value = backend->max_concurrent;
    (void)pthread_mutex_unlock(&backend->mutex);

    return value;
}

const struct qcpu_backend_ops *qcpu_test_backend_ops(void)
{
    return &QCPU_TEST_BACKEND_OPS;
}
