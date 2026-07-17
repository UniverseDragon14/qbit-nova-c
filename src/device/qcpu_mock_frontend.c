#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include "src/device/qcpu_mock_frontend.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define QCPU_MOCK_LOCK_FILE "qcpu_mock.lock"
#define QCPU_MOCK_BACKEND_DEFAULT "stage2-mock"
#define QCPU_MOCK_VERSION_DEFAULT "v4.7-stage2a"
#define QCPU_MOCK_CANCEL_GRACE_NS UINT64_C(100000000)

struct qcpu_mock {
    pthread_mutex_t mutex;
    pthread_cond_t condition;

    char runtime_dir[PATH_MAX];
    char lock_path[PATH_MAX];
    char backend_name[QCPU_UAPI_BACKEND_NAME_BYTES];
    char driver_version[QCPU_UAPI_DRIVER_VERSION_BYTES];

    struct qcpu_backend_ops backend_ops;
    void *backend_context;
    bool backend_attached;

    bool session_open;
    bool busy;
    bool cancel_requested;
    bool cancel_sent;
    int lock_fd;
    struct qcpu_exchange_task *active_task;

    uint32_t device_state;
    int32_t last_errno;
    uint32_t last_protocol_status;
    uint64_t completed_requests;
    uint64_t failed_requests;
};

struct qcpu_exchange_task {
    atomic_uint references;
    struct qcpu_mock *mock;
    struct qcpu_backend_ops backend_ops;
    void *backend_context;
    struct qcpu_uapi_request_v1 request;
    struct qcpu_uapi_response_v1 response;
    uint64_t absolute_deadline_ns;
    int backend_result;
    bool done;
    bool caller_abandoned;
};

static int qcpu_fail(int error_number)
{
    errno = error_number;
    return -1;
}

static void qcpu_task_release(struct qcpu_exchange_task *task)
{
    if (atomic_fetch_sub_explicit(
            &task->references,
            1U,
            memory_order_acq_rel
        ) == 1U) {
        free(task);
    }
}

static void qcpu_copy_text(char *destination, size_t size, const char *source)
{
    if (size == 0U) {
        return;
    }

    memset(destination, 0, size);

    if (source != NULL) {
        (void)snprintf(destination, size, "%s", source);
    }
}

static uint64_t qcpu_timespec_to_ns(const struct timespec *value)
{
    return ((uint64_t)value->tv_sec * UINT64_C(1000000000)) +
           (uint64_t)value->tv_nsec;
}

static struct timespec qcpu_ns_to_timespec(uint64_t value)
{
    struct timespec result;

    result.tv_sec = (time_t)(value / UINT64_C(1000000000));
    result.tv_nsec = (long)(value % UINT64_C(1000000000));

    return result;
}

static void qcpu_set_failure_response(
    struct qcpu_uapi_exchange_v1 *exchange,
    uint32_t protocol_status
)
{
    uint32_t qubits = 0U;
    uint64_t shots = 0U;

    if (exchange != NULL) {
        qubits = exchange->request.qubits;
        shots = exchange->request.shots;
        memset(&exchange->response, 0, sizeof(exchange->response));
        exchange->response.magic = QCPU_UAPI_RESPONSE_MAGIC;
        exchange->response.version = QCPU_UAPI_PROTOCOL_VERSION;
        exchange->response.status = (__u16)protocol_status;
        exchange->response.flags = QCPU_MOCK_REQUIRED_FLAGS;
        exchange->response.qubits = qubits;
        exchange->response.shots = shots;
    }
}

static void qcpu_record_result_locked(
    struct qcpu_mock *mock,
    int error_number,
    uint32_t protocol_status
)
{
    mock->last_errno = (int32_t)error_number;
    mock->last_protocol_status = protocol_status;
}

static int qcpu_status_to_errno(uint32_t status)
{
    switch (status) {
    case QCPU_UAPI_STATUS_OK:
        return 0;
    case QCPU_UAPI_STATUS_BAD_MAGIC:
        return EPROTO;
    case QCPU_UAPI_STATUS_BAD_VERSION:
        return EPROTONOSUPPORT;
    case QCPU_UAPI_STATUS_BAD_COMMAND:
        return EOPNOTSUPP;
    case QCPU_UAPI_STATUS_RANGE:
        return ERANGE;
    case QCPU_UAPI_STATUS_KERNEL:
    case QCPU_UAPI_STATUS_IO:
        return EIO;
    case QCPU_UAPI_STATUS_BACKEND_ABSENT:
        return ENODEV;
    case QCPU_UAPI_STATUS_BUSY:
        return EBUSY;
    case QCPU_UAPI_STATUS_TIMEOUT:
        return ETIMEDOUT;
    case QCPU_UAPI_STATUS_CANCELED:
        return ECANCELED;
    default:
        return EIO;
    }
}

static bool qcpu_status_known(uint32_t status)
{
    return status <= QCPU_UAPI_STATUS_CANCELED;
}

static int qcpu_validate_runtime_directory(
    const char *runtime_dir,
    char *resolved,
    size_t resolved_size
)
{
    struct stat info;
    char *canonical;

    if (runtime_dir == NULL || runtime_dir[0] == '\0') {
        return qcpu_fail(EINVAL);
    }

    canonical = realpath(runtime_dir, NULL);
    if (canonical == NULL) {
        return -1;
    }

    if (strlen(canonical) >= resolved_size) {
        free(canonical);
        return qcpu_fail(ENAMETOOLONG);
    }

    if (stat(canonical, &info) != 0) {
        free(canonical);
        return -1;
    }

    if (!S_ISDIR(info.st_mode)) {
        free(canonical);
        return qcpu_fail(ENOTDIR);
    }

    if (info.st_uid != geteuid()) {
        free(canonical);
        return qcpu_fail(EPERM);
    }

    if ((info.st_mode & (S_IRWXG | S_IRWXO)) != 0) {
        free(canonical);
        return qcpu_fail(EPERM);
    }

    qcpu_copy_text(resolved, resolved_size, canonical);
    free(canonical);

    return 0;
}

static int qcpu_build_lock_path(struct qcpu_mock *mock)
{
    int written = snprintf(
        mock->lock_path,
        sizeof(mock->lock_path),
        "%s/%s",
        mock->runtime_dir,
        QCPU_MOCK_LOCK_FILE
    );

    if (written < 0 || (size_t)written >= sizeof(mock->lock_path)) {
        return qcpu_fail(ENAMETOOLONG);
    }

    return 0;
}

static bool qcpu_request_cancel_locked(struct qcpu_mock *mock)
{
    if (!mock->busy || mock->cancel_sent) {
        return false;
    }

    mock->cancel_requested = true;
    mock->cancel_sent = true;

    return mock->backend_ops.cancel != NULL;
}

static void qcpu_finalize_exchange_locked(
    struct qcpu_mock *mock,
    bool offline,
    bool success,
    int error_number,
    uint32_t protocol_status
)
{
    if (success) {
        mock->completed_requests++;
    } else {
        mock->failed_requests++;
    }

    qcpu_record_result_locked(mock, error_number, protocol_status);

    mock->busy = false;
    mock->cancel_requested = false;
    mock->cancel_sent = false;

    if (offline || !mock->backend_attached) {
        mock->device_state = QCPU_UAPI_DEVICE_OFFLINE;
    } else {
        mock->device_state = QCPU_UAPI_DEVICE_IDLE;
    }

    (void)pthread_cond_broadcast(&mock->condition);
}

static void *qcpu_exchange_worker(void *opaque)
{
    struct qcpu_exchange_task *task = opaque;
    struct qcpu_mock *mock = task->mock;
    int result;

    result = task->backend_ops.exchange(
        task->backend_context,
        &task->request,
        task->absolute_deadline_ns,
        &task->response
    );

    (void)pthread_mutex_lock(&mock->mutex);
    task->backend_result = result;
    task->done = true;

    if (task->caller_abandoned && mock->active_task == task) {
        mock->active_task = NULL;
        mock->busy = false;
        mock->cancel_requested = false;
        mock->cancel_sent = false;
        mock->device_state =
            (!mock->backend_attached || result == -ENODEV)
                ? QCPU_UAPI_DEVICE_OFFLINE
                : QCPU_UAPI_DEVICE_IDLE;
    }

    (void)pthread_cond_broadcast(&mock->condition);
    (void)pthread_mutex_unlock(&mock->mutex);
    qcpu_task_release(task);

    return NULL;
}

static int qcpu_validate_backend_response(
    const struct qcpu_uapi_request_v1 *request,
    const struct qcpu_uapi_response_v1 *response,
    int *error_number,
    uint32_t *protocol_status,
    bool *offline
)
{
    uint64_t all_one_state;

    *offline = false;

    if (response->magic != QCPU_UAPI_RESPONSE_MAGIC) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_IO;
        return -1;
    }

    if (response->version != QCPU_UAPI_PROTOCOL_VERSION) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_IO;
        return -1;
    }

    if (!qcpu_status_known(response->status)) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_IO;
        return -1;
    }

    if (response->status != QCPU_UAPI_STATUS_OK) {
        *error_number = qcpu_status_to_errno(response->status);
        *protocol_status = response->status;
        *offline = response->status == QCPU_UAPI_STATUS_BACKEND_ABSENT;
        return -1;
    }

    if ((response->flags & QCPU_MOCK_REQUIRED_FLAGS) !=
        QCPU_MOCK_REQUIRED_FLAGS) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_IO;
        return -1;
    }

    if (response->qubits != request->qubits ||
        response->shots != request->shots) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_IO;
        return -1;
    }

    if (response->norm_q32_32 > (UINT64_C(1) << 32)) {
        *error_number = EIO;
        *protocol_status = QCPU_UAPI_STATUS_KERNEL;
        return -1;
    }

    if (request->command == QCPU_UAPI_COMMAND_RUN_GHZ) {
        if (response->basis_states != (UINT64_C(1) << request->qubits)) {
            *error_number = EIO;
            *protocol_status = QCPU_UAPI_STATUS_KERNEL;
            return -1;
        }

        all_one_state = (UINT64_C(1) << request->qubits) - UINT64_C(1);

        if (response->measured_state != 0U &&
            response->measured_state != all_one_state) {
            *error_number = EIO;
            *protocol_status = QCPU_UAPI_STATUS_KERNEL;
            return -1;
        }

        if (response->invalid_results != 0U) {
            *error_number = EIO;
            *protocol_status = QCPU_UAPI_STATUS_KERNEL;
            return -1;
        }
    }

    *error_number = 0;
    *protocol_status = QCPU_UAPI_STATUS_OK;

    return 0;
}

static int qcpu_prevalidate_exchange_locked(
    struct qcpu_mock *mock,
    struct qcpu_uapi_exchange_v1 *exchange,
    uint64_t *absolute_deadline_ns
)
{
    struct timespec now;
    uint64_t now_ns;
    uint64_t timeout_ns;
    int error_number = 0;
    uint32_t protocol_status = QCPU_UAPI_STATUS_OK;

    if (!mock->session_open) {
        error_number = EBUSY;
        protocol_status = QCPU_UAPI_STATUS_BUSY;
        goto reject;
    }

    if (exchange == NULL) {
        qcpu_record_result_locked(
            mock,
            EINVAL,
            QCPU_UAPI_STATUS_RANGE
        );
        return qcpu_fail(EINVAL);
    }

    memset(&exchange->response, 0, sizeof(exchange->response));

    if (exchange->request.magic != QCPU_UAPI_REQUEST_MAGIC) {
        error_number = EPROTO;
        protocol_status = QCPU_UAPI_STATUS_BAD_MAGIC;
        goto reject;
    }

    if (exchange->request.version != QCPU_UAPI_PROTOCOL_VERSION) {
        error_number = EPROTONOSUPPORT;
        protocol_status = QCPU_UAPI_STATUS_BAD_VERSION;
        goto reject;
    }

    if (exchange->request.command != QCPU_UAPI_COMMAND_STATUS &&
        exchange->request.command != QCPU_UAPI_COMMAND_RUN_GHZ) {
        error_number = EOPNOTSUPP;
        protocol_status = QCPU_UAPI_STATUS_BAD_COMMAND;
        goto reject;
    }

    if (exchange->request.command == QCPU_UAPI_COMMAND_STATUS) {
        if (exchange->request.qubits != 0U ||
            exchange->request.shots != 0U) {
            error_number = ERANGE;
            protocol_status = QCPU_UAPI_STATUS_RANGE;
            goto reject;
        }
    } else {
        if (exchange->request.qubits == 0U ||
            exchange->request.qubits > QCPU_UAPI_MAX_QUBITS ||
            exchange->request.shots == 0U ||
            exchange->request.shots > QCPU_UAPI_MAX_SHOTS) {
            error_number = ERANGE;
            protocol_status = QCPU_UAPI_STATUS_RANGE;
            goto reject;
        }
    }

    timeout_ns = exchange->timeout_ns;

    if (timeout_ns == 0U) {
        timeout_ns = QCPU_MOCK_DEFAULT_TIMEOUT_NS;
    }

    /*
     * The merged Stage 2 contract is normative here: values above the
     * five-second maximum are rejected rather than clamped. The protected
     * Stage 1 UAPI header is intentionally not modified by this patch.
     */
    if (timeout_ns > QCPU_MOCK_MAX_TIMEOUT_NS) {
        error_number = ERANGE;
        protocol_status = QCPU_UAPI_STATUS_RANGE;
        goto reject;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        error_number = EIO;
        protocol_status = QCPU_UAPI_STATUS_IO;
        goto reject;
    }

    now_ns = qcpu_timespec_to_ns(&now);

    if (UINT64_MAX - now_ns < timeout_ns) {
        error_number = ERANGE;
        protocol_status = QCPU_UAPI_STATUS_RANGE;
        goto reject;
    }

    *absolute_deadline_ns = now_ns + timeout_ns;

    if (mock->busy) {
        error_number = EBUSY;
        protocol_status = QCPU_UAPI_STATUS_BUSY;
        goto reject;
    }

    if (!mock->backend_attached ||
        mock->backend_ops.exchange == NULL) {
        error_number = ENODEV;
        protocol_status = QCPU_UAPI_STATUS_BACKEND_ABSENT;
        mock->device_state = QCPU_UAPI_DEVICE_OFFLINE;
        goto reject;
    }

    return 0;

reject:
    qcpu_set_failure_response(exchange, protocol_status);
    qcpu_record_result_locked(mock, error_number, protocol_status);
    return qcpu_fail(error_number);
}

int qcpu_mock_create(
    struct qcpu_mock **out,
    const struct qcpu_mock_config *config
)
{
    struct qcpu_mock *mock;
    pthread_condattr_t condition_attributes;
    bool mutex_ready = false;
    bool condition_attributes_ready = false;
    bool condition_ready = false;
    int result;

    if (out == NULL || config == NULL) {
        return qcpu_fail(EINVAL);
    }

    *out = NULL;

    mock = calloc(1U, sizeof(*mock));
    if (mock == NULL) {
        return -1;
    }

    mock->lock_fd = -1;

    if (qcpu_validate_runtime_directory(
            config->runtime_dir,
            mock->runtime_dir,
            sizeof(mock->runtime_dir)
        ) != 0) {
        free(mock);
        return -1;
    }

    if (qcpu_build_lock_path(mock) != 0) {
        free(mock);
        return -1;
    }

    result = pthread_mutex_init(&mock->mutex, NULL);
    if (result != 0) {
        free(mock);
        return qcpu_fail(result);
    }
    mutex_ready = true;

    result = pthread_condattr_init(&condition_attributes);
    if (result != 0) {
        (void)pthread_mutex_destroy(&mock->mutex);
        free(mock);
        return qcpu_fail(result);
    }
    condition_attributes_ready = true;

    result = pthread_condattr_setclock(
        &condition_attributes,
        CLOCK_MONOTONIC
    );
    if (result != 0) {
        goto create_failure;
    }

    result = pthread_cond_init(
        &mock->condition,
        &condition_attributes
    );
    if (result != 0) {
        goto create_failure;
    }
    condition_ready = true;

    (void)pthread_condattr_destroy(&condition_attributes);
    condition_attributes_ready = false;

    qcpu_copy_text(
        mock->backend_name,
        sizeof(mock->backend_name),
        config->backend_name != NULL
            ? config->backend_name
            : QCPU_MOCK_BACKEND_DEFAULT
    );
    qcpu_copy_text(
        mock->driver_version,
        sizeof(mock->driver_version),
        config->driver_version != NULL
            ? config->driver_version
            : QCPU_MOCK_VERSION_DEFAULT
    );

    if (config->backend_ops != NULL) {
        if (config->backend_ops->exchange == NULL) {
            goto invalid_backend;
        }

        mock->backend_ops = *config->backend_ops;
        mock->backend_context = config->backend_context;
        mock->backend_attached = true;
        mock->device_state = QCPU_UAPI_DEVICE_IDLE;
    } else {
        memset(&mock->backend_ops, 0, sizeof(mock->backend_ops));
        mock->backend_context = NULL;
        mock->backend_attached = false;
        mock->device_state = QCPU_UAPI_DEVICE_OFFLINE;
    }

    mock->last_protocol_status = QCPU_UAPI_STATUS_OK;
    *out = mock;

    return 0;

invalid_backend:
    errno = EINVAL;

create_failure:
    if (condition_ready) {
        (void)pthread_cond_destroy(&mock->condition);
    }
    if (condition_attributes_ready) {
        (void)pthread_condattr_destroy(&condition_attributes);
    }
    if (mutex_ready) {
        (void)pthread_mutex_destroy(&mock->mutex);
    }
    free(mock);

    if (errno == 0) {
        errno = result;
    }

    return -1;
}

int qcpu_mock_set_backend(
    struct qcpu_mock *mock,
    const struct qcpu_backend_ops *backend_ops,
    void *backend_context,
    const char *backend_name
)
{
    if (mock == NULL) {
        return qcpu_fail(EINVAL);
    }

    (void)pthread_mutex_lock(&mock->mutex);

    if (mock->busy) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(EBUSY);
    }

    if (backend_ops == NULL) {
        memset(&mock->backend_ops, 0, sizeof(mock->backend_ops));
        mock->backend_context = NULL;
        mock->backend_attached = false;
        mock->device_state = QCPU_UAPI_DEVICE_OFFLINE;
        qcpu_copy_text(
            mock->backend_name,
            sizeof(mock->backend_name),
            "absent"
        );
    } else {
        if (backend_ops->exchange == NULL) {
            (void)pthread_mutex_unlock(&mock->mutex);
            return qcpu_fail(EINVAL);
        }

        mock->backend_ops = *backend_ops;
        mock->backend_context = backend_context;
        mock->backend_attached = true;
        mock->device_state = QCPU_UAPI_DEVICE_IDLE;
        qcpu_copy_text(
            mock->backend_name,
            sizeof(mock->backend_name),
            backend_name != NULL
                ? backend_name
                : QCPU_MOCK_BACKEND_DEFAULT
        );
    }

    (void)pthread_mutex_unlock(&mock->mutex);

    return 0;
}

int qcpu_mock_open(struct qcpu_mock *mock)
{
    int descriptor;
    int saved_errno;

    if (mock == NULL) {
        return qcpu_fail(EINVAL);
    }

    (void)pthread_mutex_lock(&mock->mutex);

    if (mock->session_open) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(EBUSY);
    }

    descriptor = open(
        mock->lock_path,
        O_RDWR | O_CREAT | O_CLOEXEC,
        S_IRUSR | S_IWUSR
    );
    if (descriptor < 0) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return -1;
    }

    if (fchmod(descriptor, S_IRUSR | S_IWUSR) != 0) {
        saved_errno = errno;
        (void)close(descriptor);
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(saved_errno);
    }

    if (flock(descriptor, LOCK_EX | LOCK_NB) != 0) {
        saved_errno = errno;
        (void)close(descriptor);
        (void)pthread_mutex_unlock(&mock->mutex);

        if (saved_errno == EWOULDBLOCK || saved_errno == EAGAIN) {
            return qcpu_fail(EBUSY);
        }

        return qcpu_fail(saved_errno);
    }

    mock->lock_fd = descriptor;
    mock->session_open = true;

    (void)pthread_mutex_unlock(&mock->mutex);

    return 0;
}

static int qcpu_fill_caps(
    struct qcpu_mock *mock,
    struct qcpu_uapi_caps_v1 *caps
)
{
    if (caps == NULL) {
        return qcpu_fail(EINVAL);
    }

    memset(caps, 0, sizeof(*caps));
    caps->abi_version = QCPU_UAPI_ABI_VERSION;
    caps->protocol_version = QCPU_UAPI_PROTOCOL_VERSION;
    caps->max_qubits = QCPU_UAPI_MAX_QUBITS;
    caps->max_shots = QCPU_UAPI_MAX_SHOTS;
    caps->max_inflight = QCPU_UAPI_MAX_INFLIGHT;
    caps->flags = QCPU_MOCK_REQUIRED_FLAGS;

    (void)pthread_mutex_lock(&mock->mutex);
    qcpu_copy_text(
        (char *)caps->backend,
        sizeof(caps->backend),
        mock->backend_name
    );
    qcpu_copy_text(
        (char *)caps->driver_version,
        sizeof(caps->driver_version),
        mock->driver_version
    );
    (void)pthread_mutex_unlock(&mock->mutex);

    return 0;
}

static int qcpu_fill_status(
    struct qcpu_mock *mock,
    struct qcpu_uapi_status_v1 *status
)
{
    if (status == NULL) {
        return qcpu_fail(EINVAL);
    }

    memset(status, 0, sizeof(*status));

    (void)pthread_mutex_lock(&mock->mutex);
    status->device_state = mock->device_state;
    status->active_client_sessions = mock->session_open ? 1U : 0U;
    status->last_errno = mock->last_errno;
    status->last_protocol_status = mock->last_protocol_status;
    status->completed_requests = mock->completed_requests;
    status->failed_requests = mock->failed_requests;
    (void)pthread_mutex_unlock(&mock->mutex);

    return 0;
}

static int qcpu_exchange_ioctl(
    struct qcpu_mock *mock,
    struct qcpu_uapi_exchange_v1 *exchange
)
{
    struct qcpu_exchange_task *task = NULL;
    pthread_t worker;
    pthread_attr_t worker_attributes;
    bool worker_attributes_ready = false;
    struct timespec deadline;
    struct timespec cancel_deadline;
    struct timespec now;
    uint64_t absolute_deadline_ns = 0U;
    uint64_t now_ns;
    bool timed_out = false;
    bool call_cancel = false;
    int wait_result;
    int create_result;
    int attribute_result;
    int error_number = 0;
    uint32_t protocol_status = QCPU_UAPI_STATUS_OK;
    bool offline = false;
    int return_value = 0;

    (void)pthread_mutex_lock(&mock->mutex);

    if (qcpu_prevalidate_exchange_locked(
            mock,
            exchange,
            &absolute_deadline_ns
        ) != 0) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return -1;
    }

    task = calloc(1U, sizeof(*task));
    if (task == NULL) {
        qcpu_set_failure_response(exchange, QCPU_UAPI_STATUS_IO);
        qcpu_finalize_exchange_locked(
            mock,
            false,
            false,
            ENOMEM,
            QCPU_UAPI_STATUS_IO
        );
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(ENOMEM);
    }

    atomic_init(&task->references, 2U);
    task->mock = mock;
    task->backend_ops = mock->backend_ops;
    task->backend_context = mock->backend_context;
    task->request = exchange->request;
    task->absolute_deadline_ns = absolute_deadline_ns;

    mock->busy = true;
    mock->cancel_requested = false;
    mock->cancel_sent = false;
    mock->device_state = QCPU_UAPI_DEVICE_BUSY;
    mock->active_task = task;

    attribute_result = pthread_attr_init(&worker_attributes);
    if (attribute_result == 0) {
        worker_attributes_ready = true;
        attribute_result = pthread_attr_setdetachstate(
            &worker_attributes,
            PTHREAD_CREATE_DETACHED
        );
    }

    if (attribute_result != 0) {
        mock->active_task = NULL;
        qcpu_set_failure_response(exchange, QCPU_UAPI_STATUS_IO);
        qcpu_finalize_exchange_locked(
            mock,
            false,
            false,
            attribute_result,
            QCPU_UAPI_STATUS_IO
        );
        if (worker_attributes_ready) {
            (void)pthread_attr_destroy(&worker_attributes);
        }
        free(task);
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(attribute_result);
    }

    create_result = pthread_create(
        &worker,
        &worker_attributes,
        qcpu_exchange_worker,
        task
    );
    (void)pthread_attr_destroy(&worker_attributes);

    if (create_result != 0) {
        mock->active_task = NULL;
        qcpu_set_failure_response(exchange, QCPU_UAPI_STATUS_IO);
        qcpu_finalize_exchange_locked(
            mock,
            false,
            false,
            create_result,
            QCPU_UAPI_STATUS_IO
        );
        free(task);
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(create_result);
    }

    deadline = qcpu_ns_to_timespec(absolute_deadline_ns);

    while (!task->done) {
        wait_result = pthread_cond_timedwait(
            &mock->condition,
            &mock->mutex,
            &deadline
        );

        if (wait_result == ETIMEDOUT && !task->done) {
            timed_out = true;
            call_cancel = qcpu_request_cancel_locked(mock);
            break;
        }

        if (wait_result != 0 && wait_result != EINTR) {
            timed_out = true;
            call_cancel = qcpu_request_cancel_locked(mock);
            break;
        }
    }

    (void)pthread_mutex_unlock(&mock->mutex);

    if (call_cancel) {
        (void)task->backend_ops.cancel(task->backend_context);
    }

    (void)pthread_mutex_lock(&mock->mutex);

    if (timed_out && !task->done) {
        if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
            now_ns = qcpu_timespec_to_ns(&now);
            if (UINT64_MAX - now_ns >= QCPU_MOCK_CANCEL_GRACE_NS) {
                cancel_deadline = qcpu_ns_to_timespec(
                    now_ns + QCPU_MOCK_CANCEL_GRACE_NS
                );

                while (!task->done) {
                    wait_result = pthread_cond_timedwait(
                        &mock->condition,
                        &mock->mutex,
                        &cancel_deadline
                    );

                    if (wait_result == ETIMEDOUT && !task->done) {
                        break;
                    }

                    if (wait_result != 0 && wait_result != EINTR) {
                        break;
                    }
                }
            }
        }
    }

    if (timed_out && !task->done) {
        task->caller_abandoned = true;
        mock->failed_requests++;
        qcpu_record_result_locked(
            mock,
            ETIMEDOUT,
            QCPU_UAPI_STATUS_TIMEOUT
        );
        qcpu_set_failure_response(
            exchange,
            QCPU_UAPI_STATUS_TIMEOUT
        );
        (void)pthread_cond_broadcast(&mock->condition);
        (void)pthread_mutex_unlock(&mock->mutex);
        qcpu_task_release(task);
        return qcpu_fail(ETIMEDOUT);
    }

    if (timed_out) {
        error_number = ETIMEDOUT;
        protocol_status = QCPU_UAPI_STATUS_TIMEOUT;
        qcpu_set_failure_response(exchange, protocol_status);
        return_value = -1;
    } else if (mock->cancel_requested ||
               task->backend_result == -ECANCELED) {
        error_number = ECANCELED;
        protocol_status = QCPU_UAPI_STATUS_CANCELED;
        qcpu_set_failure_response(exchange, protocol_status);
        return_value = -1;
    } else if (task->backend_result != 0) {
        if (task->backend_result == -ENODEV) {
            error_number = ENODEV;
            protocol_status = QCPU_UAPI_STATUS_BACKEND_ABSENT;
            offline = true;
        } else if (task->backend_result == -ETIMEDOUT) {
            error_number = ETIMEDOUT;
            protocol_status = QCPU_UAPI_STATUS_TIMEOUT;
        } else if (task->backend_result == -ECANCELED) {
            error_number = ECANCELED;
            protocol_status = QCPU_UAPI_STATUS_CANCELED;
        } else if (qcpu_status_known(task->response.status) &&
                   task->response.status != QCPU_UAPI_STATUS_OK) {
            protocol_status = task->response.status;
            error_number = qcpu_status_to_errno(protocol_status);
            offline =
                protocol_status ==
                QCPU_UAPI_STATUS_BACKEND_ABSENT;
        } else {
            error_number = EIO;
            protocol_status = QCPU_UAPI_STATUS_IO;
        }

        qcpu_set_failure_response(exchange, protocol_status);
        return_value = -1;
    } else if (qcpu_validate_backend_response(
                   &task->request,
                   &task->response,
                   &error_number,
                   &protocol_status,
                   &offline
               ) != 0) {
        qcpu_set_failure_response(exchange, protocol_status);
        return_value = -1;
    } else {
        exchange->response = task->response;
    }

    mock->active_task = NULL;
    qcpu_finalize_exchange_locked(
        mock,
        offline,
        return_value == 0,
        error_number,
        protocol_status
    );

    (void)pthread_mutex_unlock(&mock->mutex);
    qcpu_task_release(task);

    if (return_value != 0) {
        return qcpu_fail(error_number);
    }

    return 0;
}

int qcpu_mock_ioctl(
    struct qcpu_mock *mock,
    unsigned long request,
    void *argument
)
{
    if (mock == NULL) {
        return qcpu_fail(EINVAL);
    }

    if (request == QCPU_IOC_GET_CAPS_V1) {
        return qcpu_fill_caps(mock, argument);
    }

    if (request == QCPU_IOC_GET_STATUS_V1) {
        return qcpu_fill_status(mock, argument);
    }

    if (request == QCPU_IOC_EXCHANGE_V1) {
        return qcpu_exchange_ioctl(mock, argument);
    }

    return qcpu_fail(ENOTTY);
}

int qcpu_mock_close(struct qcpu_mock *mock)
{
    bool call_cancel = false;
    struct qcpu_backend_ops backend_ops;
    void *backend_context;
    int descriptor;
    int saved_errno = 0;

    if (mock == NULL) {
        return qcpu_fail(EINVAL);
    }

    memset(&backend_ops, 0, sizeof(backend_ops));

    (void)pthread_mutex_lock(&mock->mutex);

    if (!mock->session_open) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(EINVAL);
    }

    if (mock->busy &&
        mock->active_task != NULL &&
        mock->active_task->caller_abandoned) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(EBUSY);
    }

    if (mock->busy) {
        call_cancel = qcpu_request_cancel_locked(mock);
        backend_ops = mock->backend_ops;
        backend_context = mock->backend_context;
        (void)pthread_mutex_unlock(&mock->mutex);

        if (call_cancel) {
            (void)backend_ops.cancel(backend_context);
        }

        (void)pthread_mutex_lock(&mock->mutex);

        while (mock->busy) {
            (void)pthread_cond_wait(
                &mock->condition,
                &mock->mutex
            );
        }
    }

    descriptor = mock->lock_fd;
    mock->lock_fd = -1;
    mock->session_open = false;

    if (descriptor >= 0) {
        if (flock(descriptor, LOCK_UN) != 0) {
            saved_errno = errno;
        }

        if (close(descriptor) != 0 && saved_errno == 0) {
            saved_errno = errno;
        }
    }


    (void)pthread_mutex_unlock(&mock->mutex);

    if (saved_errno != 0) {
        return qcpu_fail(saved_errno);
    }

    return 0;
}

int qcpu_mock_destroy(struct qcpu_mock *mock)
{
    int result;

    if (mock == NULL) {
        return qcpu_fail(EINVAL);
    }

    (void)pthread_mutex_lock(&mock->mutex);

    if (mock->session_open || mock->busy) {
        (void)pthread_mutex_unlock(&mock->mutex);
        return qcpu_fail(EBUSY);
    }

    (void)pthread_mutex_unlock(&mock->mutex);

    result = pthread_cond_destroy(&mock->condition);
    if (result != 0) {
        return qcpu_fail(result);
    }

    result = pthread_mutex_destroy(&mock->mutex);
    if (result != 0) {
        return qcpu_fail(result);
    }

    free(mock);

    return 0;
}

int qcpu_mock_norm_to_q32_32(
    double norm,
    uint64_t *out
)
{
    const double tolerance = 0x1p-40;
    long double clamped;
    long double scaled;
    long double rounded;

    if (out == NULL) {
        return qcpu_fail(EINVAL);
    }

    if (!isfinite(norm) || norm < 0.0) {
        return qcpu_fail(ERANGE);
    }

    if (norm > 1.0 + tolerance) {
        return qcpu_fail(ERANGE);
    }

    clamped = norm > 1.0 ? 1.0L : (long double)norm;
    scaled = clamped * 4294967296.0L;
    rounded = floorl(scaled + 0.5L);

    if (rounded < 0.0L ||
        rounded > 4294967296.0L) {
        return qcpu_fail(ERANGE);
    }

    *out = (uint64_t)rounded;

    return 0;
}
