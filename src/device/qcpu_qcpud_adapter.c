#define _POSIX_C_SOURCE 200809L

#include "qcpu_qcpud_adapter.h"

#include "qcpu_device_io.h"
#include "qcpu_device_wire.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

struct qcpu_qcpud_adapter {
    pthread_mutex_t mutex;
    char socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];
    int active_fd;
    bool active;
    bool cancel_requested;
};

static const struct qcpu_backend_ops QCPU_QCPUD_ADAPTER_OPS;

static int qcpu_adapter_fail(int error_number)
{
    errno = error_number;
    return -1;
}

static uint64_t qcpu_adapter_now_ns(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return UINT64_MAX;
    }

    return ((uint64_t)now.tv_sec * UINT64_C(1000000000)) +
           (uint64_t)now.tv_nsec;
}

static int qcpu_adapter_remaining_ms(uint64_t absolute_deadline_ns)
{
    uint64_t now_ns = qcpu_adapter_now_ns();
    uint64_t remaining_ns;
    uint64_t remaining_ms;

    if (now_ns == UINT64_MAX || now_ns >= absolute_deadline_ns) {
        return 0;
    }

    remaining_ns = absolute_deadline_ns - now_ns;
    remaining_ms = remaining_ns / UINT64_C(1000000);

    if ((remaining_ns % UINT64_C(1000000)) != 0U) {
        remaining_ms++;
    }

    if (remaining_ms > (uint64_t)INT_MAX) {
        return INT_MAX;
    }

    return (int)remaining_ms;
}

static bool qcpu_adapter_is_canceled(
    struct qcpu_qcpud_adapter *adapter
)
{
    bool canceled;

    (void)pthread_mutex_lock(&adapter->mutex);
    canceled = adapter->cancel_requested;
    (void)pthread_mutex_unlock(&adapter->mutex);

    return canceled;
}

static int qcpu_adapter_begin(
    struct qcpu_qcpud_adapter *adapter
)
{
    int result = 0;

    (void)pthread_mutex_lock(&adapter->mutex);

    if (adapter->active) {
        result = -EBUSY;
    } else {
        adapter->active = true;
        adapter->cancel_requested = false;
        adapter->active_fd = -1;
    }

    (void)pthread_mutex_unlock(&adapter->mutex);

    return result;
}

static bool qcpu_adapter_register_fd(
    struct qcpu_qcpud_adapter *adapter,
    int file_descriptor
)
{
    bool canceled;

    (void)pthread_mutex_lock(&adapter->mutex);
    adapter->active_fd = file_descriptor;
    canceled = adapter->cancel_requested;

    if (canceled) {
        (void)shutdown(file_descriptor, SHUT_RDWR);
    }

    (void)pthread_mutex_unlock(&adapter->mutex);

    return canceled;
}

static int qcpu_adapter_finish(
    struct qcpu_qcpud_adapter *adapter,
    int file_descriptor,
    int result
)
{
    bool canceled;

    (void)pthread_mutex_lock(&adapter->mutex);
    canceled = adapter->cancel_requested;
    adapter->active_fd = -1;
    adapter->active = false;
    adapter->cancel_requested = false;
    (void)pthread_mutex_unlock(&adapter->mutex);

    /*
     * Publish active_fd == -1 before close so a concurrent cancel can never
     * issue shutdown() against a descriptor number that close() has already
     * made available for reuse.
     */
    if (file_descriptor >= 0) {
        (void)close(file_descriptor);
    }

    return canceled ? -ECANCELED : result;
}

static uint16_t qcpu_adapter_status(uint16_t status)
{
    switch (status) {
    case QCPU_DEVICE_STATUS_OK:
        return QCPU_UAPI_STATUS_OK;
    case QCPU_DEVICE_STATUS_BAD_MAGIC:
        return QCPU_UAPI_STATUS_BAD_MAGIC;
    case QCPU_DEVICE_STATUS_BAD_VERSION:
        return QCPU_UAPI_STATUS_BAD_VERSION;
    case QCPU_DEVICE_STATUS_BAD_COMMAND:
        return QCPU_UAPI_STATUS_BAD_COMMAND;
    case QCPU_DEVICE_STATUS_RANGE:
        return QCPU_UAPI_STATUS_RANGE;
    case QCPU_DEVICE_STATUS_KERNEL:
        return QCPU_UAPI_STATUS_KERNEL;
    case QCPU_DEVICE_STATUS_IO:
        return QCPU_UAPI_STATUS_IO;
    default:
        return QCPU_UAPI_STATUS_IO;
    }
}

static int qcpu_adapter_status_result(uint16_t status)
{
    switch (status) {
    case QCPU_UAPI_STATUS_OK:
        return 0;
    case QCPU_UAPI_STATUS_BAD_MAGIC:
        return -EPROTO;
    case QCPU_UAPI_STATUS_BAD_VERSION:
        return -EPROTONOSUPPORT;
    case QCPU_UAPI_STATUS_BAD_COMMAND:
        return -EOPNOTSUPP;
    case QCPU_UAPI_STATUS_RANGE:
        return -ERANGE;
    case QCPU_UAPI_STATUS_KERNEL:
    case QCPU_UAPI_STATUS_IO:
    default:
        return -EIO;
    }
}

static uint32_t qcpu_adapter_flags(uint32_t flags)
{
    uint32_t translated = 0U;

    if ((flags & QCPU_DEVICE_FLAG_SOFTWARE_VIRTUAL_QCPU) != 0U) {
        translated |= QCPU_UAPI_FLAG_SOFTWARE_VIRTUAL_QCPU;
    }

    if ((flags & QCPU_DEVICE_FLAG_CLASSICAL_HOST) != 0U) {
        translated |= QCPU_UAPI_FLAG_CLASSICAL_HOST;
    }

    if ((flags & QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT) != 0U) {
        translated |= QCPU_UAPI_FLAG_PHYSICAL_QPU_ABSENT;
    }

    if ((flags & QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT) != 0U) {
        translated |= QCPU_UAPI_FLAG_Q0_MOST_SIGNIFICANT;
    }

    return translated;
}

static int qcpu_adapter_translate_request(
    const struct qcpu_uapi_request_v1 *request,
    QCPUDeviceRequest *translated
)
{
    memset(translated, 0, sizeof(*translated));
    translated->magic = QCPU_DEVICE_REQUEST_MAGIC;
    translated->version = QCPU_DEVICE_PROTOCOL_VERSION;

    if (request->command == QCPU_UAPI_COMMAND_STATUS) {
        translated->command = QCPU_DEVICE_COMMAND_STATUS;
    } else if (request->command == QCPU_UAPI_COMMAND_RUN_GHZ) {
        translated->command = QCPU_DEVICE_COMMAND_RUN_GHZ;
    } else {
        return -EOPNOTSUPP;
    }

    translated->qubits = request->qubits;
    translated->shots = request->shots;
    translated->seed = request->seed;

    return 0;
}

static int qcpu_adapter_translate_response(
    const QCPUDeviceResponse *source,
    struct qcpu_uapi_response_v1 *response
)
{
    uint64_t norm_q32_32 = 0U;
    int result;

    memset(response, 0, sizeof(*response));
    response->magic = QCPU_UAPI_RESPONSE_MAGIC;
    response->version = QCPU_UAPI_PROTOCOL_VERSION;
    response->status = qcpu_adapter_status(source->status);
    response->flags = qcpu_adapter_flags(source->flags);
    response->qubits = source->qubits;
    response->basis_states = source->basis_states;
    response->shots = source->shots;
    response->measured_state = source->measured_state;
    response->invalid_results = source->invalid_results;

    if (qcpu_mock_norm_to_q32_32(
            source->norm,
            &norm_q32_32
        ) != 0) {
        response->status = QCPU_UAPI_STATUS_KERNEL;
        return -EIO;
    }

    response->norm_q32_32 = (__u64)norm_q32_32;

    result = qcpu_adapter_status_result(response->status);

    return result;
}

static int qcpu_adapter_io_result(
    struct qcpu_qcpud_adapter *adapter,
    QCPUDeviceIOStatus io_status
)
{
    if (qcpu_adapter_is_canceled(adapter)) {
        return -ECANCELED;
    }

    if (io_status == QCPU_DEVICE_IO_TIMEOUT) {
        return -ETIMEDOUT;
    }

    return -EIO;
}

static int qcpu_qcpud_exchange(
    void *context,
    const struct qcpu_uapi_request_v1 *request,
    uint64_t absolute_deadline_ns,
    struct qcpu_uapi_response_v1 *response
)
{
    struct qcpu_qcpud_adapter *adapter = context;
    QCPUDeviceRequest translated_request;
    QCPUDeviceResponse translated_response;
    uint8_t request_wire[QCPU_DEVICE_REQUEST_WIRE_SIZE];
    uint8_t response_wire[QCPU_DEVICE_RESPONSE_WIRE_SIZE];
    QCPUDeviceIOStatus io_status;
    int timeout_ms;
    int file_descriptor = -1;
    int result;

    if (adapter == NULL || request == NULL || response == NULL) {
        return -EINVAL;
    }

    memset(response, 0, sizeof(*response));

    result = qcpu_adapter_begin(adapter);
    if (result != 0) {
        return result;
    }

    result = qcpu_adapter_translate_request(
        request,
        &translated_request
    );
    if (result != 0) {
        return qcpu_adapter_finish(adapter, -1, result);
    }

    if (qcpu_device_encode_request(
            request_wire,
            &translated_request
        ) != QCPU_DEVICE_STATUS_OK) {
        return qcpu_adapter_finish(adapter, -1, -EIO);
    }

    timeout_ms = qcpu_adapter_remaining_ms(absolute_deadline_ns);
    if (timeout_ms <= 0) {
        return qcpu_adapter_finish(adapter, -1, -ETIMEDOUT);
    }

    file_descriptor = qcpu_device_connect_unix(
        adapter->socket_path,
        timeout_ms
    );

    if (file_descriptor < 0) {
        if (file_descriptor == QCPU_DEVICE_IO_TIMEOUT) {
            result = -ETIMEDOUT;
        } else if (errno == ENOENT || errno == ECONNREFUSED) {
            result = -ENODEV;
        } else {
            result = -EIO;
        }

        return qcpu_adapter_finish(adapter, -1, result);
    }

    if (qcpu_adapter_register_fd(adapter, file_descriptor)) {
        return qcpu_adapter_finish(
            adapter,
            file_descriptor,
            -ECANCELED
        );
    }

    timeout_ms = qcpu_adapter_remaining_ms(absolute_deadline_ns);
    if (timeout_ms <= 0) {
        return qcpu_adapter_finish(
            adapter,
            file_descriptor,
            -ETIMEDOUT
        );
    }

    io_status = qcpu_device_write_all(
        file_descriptor,
        request_wire,
        sizeof(request_wire),
        timeout_ms,
        NULL
    );
    if (io_status != QCPU_DEVICE_IO_OK) {
        result = qcpu_adapter_io_result(adapter, io_status);
        return qcpu_adapter_finish(adapter, file_descriptor, result);
    }

    if (shutdown(file_descriptor, SHUT_WR) != 0) {
        result = qcpu_adapter_is_canceled(adapter)
            ? -ECANCELED
            : -EIO;
        return qcpu_adapter_finish(adapter, file_descriptor, result);
    }

    timeout_ms = qcpu_adapter_remaining_ms(absolute_deadline_ns);
    if (timeout_ms <= 0) {
        return qcpu_adapter_finish(
            adapter,
            file_descriptor,
            -ETIMEDOUT
        );
    }

    io_status = qcpu_device_read_all(
        file_descriptor,
        response_wire,
        sizeof(response_wire),
        timeout_ms,
        NULL
    );
    if (io_status != QCPU_DEVICE_IO_OK) {
        result = qcpu_adapter_io_result(adapter, io_status);
        return qcpu_adapter_finish(adapter, file_descriptor, result);
    }

    if (qcpu_device_decode_response(
            &translated_response,
            response_wire,
            sizeof(response_wire)
        ) != QCPU_DEVICE_STATUS_OK) {
        return qcpu_adapter_finish(adapter, file_descriptor, -EIO);
    }

    result = qcpu_adapter_translate_response(
        &translated_response,
        response
    );

    return qcpu_adapter_finish(adapter, file_descriptor, result);
}

static int qcpu_qcpud_cancel(void *context)
{
    struct qcpu_qcpud_adapter *adapter = context;

    if (adapter == NULL) {
        return -EINVAL;
    }

    (void)pthread_mutex_lock(&adapter->mutex);

    if (adapter->active) {
        adapter->cancel_requested = true;

        if (adapter->active_fd >= 0) {
            (void)shutdown(adapter->active_fd, SHUT_RDWR);
        }
    }

    (void)pthread_mutex_unlock(&adapter->mutex);

    return 0;
}

static const struct qcpu_backend_ops QCPU_QCPUD_ADAPTER_OPS = {
    .exchange = qcpu_qcpud_exchange,
    .cancel = qcpu_qcpud_cancel
};

int qcpu_qcpud_adapter_create(
    struct qcpu_qcpud_adapter **out,
    const char *socket_path
)
{
    struct qcpu_qcpud_adapter *adapter;
    int mutex_result;

    if (out == NULL || socket_path == NULL || socket_path[0] == '\0') {
        return qcpu_adapter_fail(EINVAL);
    }

    *out = NULL;

    if (strlen(socket_path) >=
        sizeof(((struct qcpu_qcpud_adapter *)0)->socket_path)) {
        return qcpu_adapter_fail(ENAMETOOLONG);
    }

    adapter = calloc(1U, sizeof(*adapter));
    if (adapter == NULL) {
        return -1;
    }

    mutex_result = pthread_mutex_init(&adapter->mutex, NULL);
    if (mutex_result != 0) {
        free(adapter);
        return qcpu_adapter_fail(mutex_result);
    }

    memcpy(
        adapter->socket_path,
        socket_path,
        strlen(socket_path) + 1U
    );
    adapter->active_fd = -1;
    *out = adapter;

    return 0;
}

int qcpu_qcpud_adapter_destroy(
    struct qcpu_qcpud_adapter *adapter
)
{
    int mutex_result;

    if (adapter == NULL) {
        return qcpu_adapter_fail(EINVAL);
    }

    (void)pthread_mutex_lock(&adapter->mutex);

    if (adapter->active) {
        (void)pthread_mutex_unlock(&adapter->mutex);
        return qcpu_adapter_fail(EBUSY);
    }

    (void)pthread_mutex_unlock(&adapter->mutex);

    mutex_result = pthread_mutex_destroy(&adapter->mutex);
    if (mutex_result != 0) {
        return qcpu_adapter_fail(mutex_result);
    }

    free(adapter);

    return 0;
}

const struct qcpu_backend_ops *qcpu_qcpud_adapter_ops(void)
{
    return &QCPU_QCPUD_ADAPTER_OPS;
}
