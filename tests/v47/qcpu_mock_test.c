#define _POSIX_C_SOURCE 200809L

#include "src/device/qcpu_mock_frontend.h"
#include "tests/v47/qcpu_mock_test_backend.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

_Static_assert(
    sizeof(struct qcpu_uapi_request_v1) == 24U,
    "request layout"
);
_Static_assert(
    sizeof(struct qcpu_uapi_response_v1) == 56U,
    "response layout"
);
_Static_assert(
    sizeof(struct qcpu_uapi_caps_v1) == 80U,
    "caps layout"
);
_Static_assert(
    sizeof(struct qcpu_uapi_status_v1) == 32U,
    "status layout"
);
_Static_assert(
    sizeof(struct qcpu_uapi_exchange_v1) == 88U,
    "exchange layout"
);
_Static_assert(
    offsetof(struct qcpu_uapi_request_v1, seed) == 16U,
    "request seed offset"
);
_Static_assert(
    offsetof(struct qcpu_uapi_response_v1, norm_q32_32) == 48U,
    "response norm offset"
);
_Static_assert(
    offsetof(struct qcpu_uapi_exchange_v1, timeout_ns) == 24U,
    "exchange timeout offset"
);
_Static_assert(
    offsetof(struct qcpu_uapi_exchange_v1, response) == 32U,
    "exchange response offset"
);

#define TEST_TIMEOUT_NS UINT64_C(2000000000)

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf( \
                stderr, \
                "FAIL:%s:%d: %s (errno=%d %s)\n", \
                __FILE__, \
                __LINE__, \
                #condition, \
                errno, \
                strerror(errno) \
            ); \
            return -1; \
        } \
    } while (0)

#define CHECK_ERR(expression, expected_errno) \
    do { \
        errno = 0; \
        CHECK((expression) == -1); \
        CHECK(errno == (expected_errno)); \
    } while (0)

struct test_fixture {
    char runtime_template[128];
    char *runtime_dir;
    struct qcpu_mock *mock;
    struct qcpu_test_backend *backend;
    bool session_open;
};

struct exchange_thread_context {
    struct qcpu_mock *mock;
    struct qcpu_uapi_exchange_v1 exchange;
    int result;
    int error_number;
};

static uint64_t test_timespec_to_ns(const struct timespec *value)
{
    return ((uint64_t)value->tv_sec * UINT64_C(1000000000)) +
           (uint64_t)value->tv_nsec;
}

static int remove_runtime_lock_file(const char *runtime_dir)
{
    char lock_path[256];
    int written;

    written = snprintf(
        lock_path,
        sizeof(lock_path),
        "%s/qcpu_mock.lock",
        runtime_dir
    );
    if (written < 0 || (size_t)written >= sizeof(lock_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    if (unlink(lock_path) != 0 && errno != ENOENT) {
        return -1;
    }

    return 0;
}

static struct qcpu_uapi_exchange_v1 make_status_exchange(
    uint64_t timeout_ns
)
{
    struct qcpu_uapi_exchange_v1 exchange;

    memset(&exchange, 0, sizeof(exchange));
    exchange.request.magic = QCPU_UAPI_REQUEST_MAGIC;
    exchange.request.version = QCPU_UAPI_PROTOCOL_VERSION;
    exchange.request.command = QCPU_UAPI_COMMAND_STATUS;
    exchange.timeout_ns = timeout_ns;

    return exchange;
}

static struct qcpu_uapi_exchange_v1 make_ghz_exchange(
    uint32_t qubits,
    uint32_t shots,
    uint64_t timeout_ns
)
{
    struct qcpu_uapi_exchange_v1 exchange;

    memset(&exchange, 0, sizeof(exchange));
    exchange.request.magic = QCPU_UAPI_REQUEST_MAGIC;
    exchange.request.version = QCPU_UAPI_PROTOCOL_VERSION;
    exchange.request.command = QCPU_UAPI_COMMAND_RUN_GHZ;
    exchange.request.qubits = qubits;
    exchange.request.shots = shots;
    exchange.request.seed = UINT64_C(0x123456789abcdef0);
    exchange.timeout_ns = timeout_ns;

    return exchange;
}

static int fixture_create(
    struct test_fixture *fixture,
    bool attach_backend
)
{
    struct qcpu_mock_config config;

    memset(fixture, 0, sizeof(*fixture));
    (void)snprintf(
        fixture->runtime_template,
        sizeof(fixture->runtime_template),
        "/tmp/qcpu-v47-stage2a-XXXXXX"
    );

    fixture->runtime_dir = mkdtemp(fixture->runtime_template);
    CHECK(fixture->runtime_dir != NULL);
    CHECK(chmod(fixture->runtime_dir, S_IRWXU) == 0);
    CHECK(qcpu_test_backend_create(&fixture->backend) == 0);

    memset(&config, 0, sizeof(config));
    config.runtime_dir = fixture->runtime_dir;
    config.backend_name = "deterministic-test";
    config.driver_version = "v4.7-stage2a";

    if (attach_backend) {
        config.backend_ops = qcpu_test_backend_ops();
        config.backend_context = fixture->backend;
    }

    CHECK(qcpu_mock_create(&fixture->mock, &config) == 0);

    return 0;
}

static int fixture_open(struct test_fixture *fixture)
{
    CHECK(qcpu_mock_open(fixture->mock) == 0);
    fixture->session_open = true;

    return 0;
}

static int fixture_close(struct test_fixture *fixture)
{
    CHECK(qcpu_mock_close(fixture->mock) == 0);
    fixture->session_open = false;

    return 0;
}

static int fixture_destroy(struct test_fixture *fixture)
{
    if (fixture->session_open) {
        CHECK(fixture_close(fixture) == 0);
    }

    if (fixture->mock != NULL) {
        CHECK(qcpu_mock_destroy(fixture->mock) == 0);
        fixture->mock = NULL;
    }

    if (fixture->backend != NULL) {
        CHECK(qcpu_test_backend_destroy(fixture->backend) == 0);
        fixture->backend = NULL;
    }

    if (fixture->runtime_dir != NULL) {
        CHECK(remove_runtime_lock_file(fixture->runtime_dir) == 0);
        CHECK(rmdir(fixture->runtime_dir) == 0);
        fixture->runtime_dir = NULL;
    }

    return 0;
}

static void *exchange_thread(void *opaque)
{
    struct exchange_thread_context *context = opaque;

    context->result = qcpu_mock_ioctl(
        context->mock,
        QCPU_IOC_EXCHANGE_V1,
        &context->exchange
    );
    context->error_number =
        context->result == 0 ? 0 : errno;

    return NULL;
}

static int test_abi_and_ioctl_identity(void)
{
    CHECK(QCPU_UAPI_ABI_VERSION == 1U);
    CHECK(QCPU_UAPI_PROTOCOL_VERSION == 1U);
    CHECK(QCPU_UAPI_MAX_QUBITS == 20U);
    CHECK(QCPU_UAPI_MAX_SHOTS == 100U);
    CHECK(QCPU_UAPI_MAX_INFLIGHT == 1U);

    CHECK(_IOC_TYPE(QCPU_IOC_GET_CAPS_V1) == QCPU_UAPI_IOC_MAGIC);
    CHECK(_IOC_TYPE(QCPU_IOC_GET_STATUS_V1) == QCPU_UAPI_IOC_MAGIC);
    CHECK(_IOC_TYPE(QCPU_IOC_EXCHANGE_V1) == QCPU_UAPI_IOC_MAGIC);

    CHECK(_IOC_NR(QCPU_IOC_GET_CAPS_V1) !=
          _IOC_NR(QCPU_IOC_GET_STATUS_V1));
    CHECK(_IOC_NR(QCPU_IOC_EXCHANGE_V1) !=
          _IOC_NR(QCPU_IOC_GET_STATUS_V1));

    CHECK(_IOC_SIZE(QCPU_IOC_GET_CAPS_V1) ==
          sizeof(struct qcpu_uapi_caps_v1));
    CHECK(_IOC_SIZE(QCPU_IOC_GET_STATUS_V1) ==
          sizeof(struct qcpu_uapi_status_v1));
    CHECK(_IOC_SIZE(QCPU_IOC_EXCHANGE_V1) ==
          sizeof(struct qcpu_uapi_exchange_v1));

    puts("PASS: ABI_LAYOUT_AND_IOCTL_IDENTITY");
    return 0;
}

static int test_caps_status_and_unknown_ioctl(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_caps_v1 caps;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, false) == 0);

    memset(&caps, 0xa5, sizeof(caps));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_CAPS_V1,
        &caps
    ) == 0);

    CHECK(caps.abi_version == 1U);
    CHECK(caps.protocol_version == 1U);
    CHECK(caps.max_qubits == 20U);
    CHECK(caps.max_shots == 100U);
    CHECK(caps.max_inflight == 1U);
    CHECK((caps.flags & QCPU_MOCK_REQUIRED_FLAGS) ==
          QCPU_MOCK_REQUIRED_FLAGS);
    CHECK(caps.backend[QCPU_UAPI_BACKEND_NAME_BYTES - 1U] == 0U);
    CHECK(caps.driver_version[
              QCPU_UAPI_DRIVER_VERSION_BYTES - 1U
          ] == 0U);

    for (size_t index = 0U; index < sizeof(caps.reserved); index++) {
        CHECK(caps.reserved[index] == 0U);
    }

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_OFFLINE);
    CHECK(status.active_client_sessions == 0U);
    CHECK(status.completed_requests == 0U);
    CHECK(status.failed_requests == 0U);

    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            _IO(QCPU_UAPI_IOC_MAGIC, 0x7f),
            NULL
        ),
        ENOTTY
    );

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: CAPS_STATUS_AND_UNKNOWN_IOCTL");
    return 0;
}

static int test_session_exclusivity(void)
{
    struct test_fixture fixture;
    struct qcpu_mock *second_mock = NULL;
    struct qcpu_mock_config second_config;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    CHECK_ERR(qcpu_mock_open(fixture.mock), EBUSY);

    memset(&second_config, 0, sizeof(second_config));
    second_config.runtime_dir = fixture.runtime_dir;
    second_config.backend_ops = qcpu_test_backend_ops();
    second_config.backend_context = fixture.backend;
    second_config.backend_name = "second-test";

    CHECK(qcpu_mock_create(&second_mock, &second_config) == 0);
    CHECK_ERR(qcpu_mock_open(second_mock), EBUSY);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.active_client_sessions == 1U);

    CHECK(fixture_close(&fixture) == 0);
    CHECK(qcpu_mock_open(second_mock) == 0);
    CHECK(qcpu_mock_close(second_mock) == 0);
    CHECK(qcpu_mock_destroy(second_mock) == 0);

    CHECK(fixture_open(&fixture) == 0);
    CHECK(fixture_close(&fixture) == 0);
    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: SAME_PROCESS_EXCLUSIVE_SESSION");
    return 0;
}

static int test_status_and_ghz_success(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_OK);
    CHECK(exchange.response.magic == QCPU_UAPI_RESPONSE_MAGIC);
    CHECK(exchange.response.version == QCPU_UAPI_PROTOCOL_VERSION);
    CHECK((exchange.response.flags & QCPU_MOCK_REQUIRED_FLAGS) ==
          QCPU_MOCK_REQUIRED_FLAGS);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS_ZERO
    );
    exchange = make_ghz_exchange(3U, 20U, 0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.basis_states == 8U);
    CHECK(exchange.response.measured_state == 0U);
    CHECK(exchange.response.invalid_results == 0U);
    CHECK(exchange.response.norm_q32_32 == (UINT64_C(1) << 32));

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS_ONE
    );
    exchange = make_ghz_exchange(3U, 20U, 0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.measured_state == 7U);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
    CHECK(status.completed_requests == 3U);
    CHECK(status.failed_requests == 0U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: STATUS_AND_GHZ_SUCCESS");
    return 0;
}

static int test_predispatch_validation_and_counters(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;
    uint64_t dispatches_before;

    CHECK(fixture_create(&fixture, true) == 0);

    exchange = make_status_exchange(0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EBUSY
    );
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_BUSY);

    CHECK(fixture_open(&fixture) == 0);
    dispatches_before =
        qcpu_test_backend_dispatches(fixture.backend);

    exchange = make_status_exchange(0U);
    exchange.request.magic ^= 1U;
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EPROTO
    );
    CHECK(exchange.response.status ==
          QCPU_UAPI_STATUS_BAD_MAGIC);

    exchange = make_status_exchange(0U);
    exchange.request.version++;
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EPROTONOSUPPORT
    );

    exchange = make_status_exchange(0U);
    exchange.request.command = UINT16_MAX;
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EOPNOTSUPP
    );

    exchange = make_ghz_exchange(0U, 20U, 0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ERANGE
    );

    exchange = make_ghz_exchange(21U, 20U, 0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ERANGE
    );

    exchange = make_ghz_exchange(3U, 101U, 0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ERANGE
    );

    exchange = make_status_exchange(
        QCPU_MOCK_MAX_TIMEOUT_NS + 1U
    );
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ERANGE
    );

    CHECK(qcpu_test_backend_dispatches(fixture.backend) ==
          dispatches_before);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.completed_requests == 0U);
    CHECK(status.failed_requests == 0U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: PREDISPATCH_VALIDATION_AND_COUNTERS");
    return 0;
}

static int test_norm_conversion(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    uint64_t output = 0U;
    const double tolerance = 0x1p-40;

    CHECK(qcpu_mock_norm_to_q32_32(0.0, &output) == 0);
    CHECK(output == 0U);

    CHECK(qcpu_mock_norm_to_q32_32(0x1p-33, &output) == 0);
    CHECK(output == 1U);

    CHECK(qcpu_mock_norm_to_q32_32(0x1.8p-32, &output) == 0);
    CHECK(output == 2U);

    CHECK(qcpu_mock_norm_to_q32_32(1.0, &output) == 0);
    CHECK(output == (UINT64_C(1) << 32));

    CHECK(qcpu_mock_norm_to_q32_32(
        1.0 + tolerance,
        &output
    ) == 0);
    CHECK(output == (UINT64_C(1) << 32));

    CHECK_ERR(
        qcpu_mock_norm_to_q32_32(
            1.0 + tolerance + 0x1p-52,
            &output
        ),
        ERANGE
    );
    CHECK_ERR(qcpu_mock_norm_to_q32_32(-0.1, &output), ERANGE);
    CHECK_ERR(qcpu_mock_norm_to_q32_32(NAN, &output), ERANGE);
    CHECK_ERR(qcpu_mock_norm_to_q32_32(INFINITY, &output), ERANGE);

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    qcpu_test_backend_set_norm(
        fixture.backend,
        1.0 + tolerance
    );
    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.norm_q32_32 ==
          (UINT64_C(1) << 32));

    qcpu_test_backend_set_norm(
        fixture.backend,
        0x1p-33
    );
    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.norm_q32_32 == 1U);

    qcpu_test_backend_set_norm(
        fixture.backend,
        1.0 + tolerance + 0x1p-52
    );
    exchange = make_ghz_exchange(3U, 20U, 0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EIO
    );
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_KERNEL);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: Q32_32_NORM_CONVERSION");
    return 0;
}

struct response_failure_case {
    enum qcpu_test_backend_mode mode;
    uint32_t expected_status;
};

static int test_response_validation(void)
{
    const struct response_failure_case cases[] = {
        {
            QCPU_TEST_BACKEND_BAD_RESPONSE_MAGIC,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_BAD_RESPONSE_VERSION,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_MISSING_FLAGS,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_QUBIT_MISMATCH,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_SHOTS_MISMATCH,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_BAD_BASIS_STATES,
            QCPU_UAPI_STATUS_KERNEL
        },
        {
            QCPU_TEST_BACKEND_BAD_MEASURED_STATE,
            QCPU_UAPI_STATUS_KERNEL
        },
        {
            QCPU_TEST_BACKEND_NONZERO_INVALID_RESULTS,
            QCPU_UAPI_STATUS_KERNEL
        },
        {
            QCPU_TEST_BACKEND_NORM_TOO_HIGH,
            QCPU_UAPI_STATUS_KERNEL
        },
        {
            QCPU_TEST_BACKEND_UNKNOWN_STATUS,
            QCPU_UAPI_STATUS_IO
        },
        {
            QCPU_TEST_BACKEND_KNOWN_KERNEL_STATUS,
            QCPU_UAPI_STATUS_KERNEL
        },
        {
            QCPU_TEST_BACKEND_ENGINE_FAILURE,
            QCPU_UAPI_STATUS_KERNEL
        }
    };

    for (size_t index = 0U;
         index < sizeof(cases) / sizeof(cases[0]);
         index++) {
        struct test_fixture fixture;
        struct qcpu_uapi_exchange_v1 exchange;
        struct qcpu_uapi_status_v1 status;

        CHECK(fixture_create(&fixture, true) == 0);
        CHECK(fixture_open(&fixture) == 0);

        qcpu_test_backend_set_mode(
            fixture.backend,
            cases[index].mode
        );

        exchange = make_ghz_exchange(3U, 20U, 0U);
        CHECK_ERR(
            qcpu_mock_ioctl(
                fixture.mock,
                QCPU_IOC_EXCHANGE_V1,
                &exchange
            ),
            EIO
        );
        CHECK(exchange.response.status ==
              cases[index].expected_status);
        CHECK(exchange.response.measured_state == 0U);
        CHECK(exchange.response.invalid_results == 0U);

        CHECK(qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_GET_STATUS_V1,
            &status
        ) == 0);
        CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
        CHECK(status.completed_requests == 0U);
        CHECK(status.failed_requests == 1U);

        qcpu_test_backend_set_mode(
            fixture.backend,
            QCPU_TEST_BACKEND_SUCCESS_ZERO
        );
        qcpu_test_backend_set_norm(fixture.backend, 1.0);

        exchange = make_ghz_exchange(3U, 20U, 0U);
        CHECK(qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ) == 0);

        CHECK(fixture_destroy(&fixture) == 0);
    }

    puts("PASS: DETERMINISTIC_RESPONSE_VALIDATION");
    return 0;
}

static int test_backend_absent_and_recovery(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, false) == 0);
    CHECK(fixture_open(&fixture) == 0);

    exchange = make_status_exchange(0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ENODEV
    );
    CHECK(exchange.response.status ==
          QCPU_UAPI_STATUS_BACKEND_ABSENT);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_OFFLINE);
    CHECK(status.failed_requests == 0U);

    CHECK(qcpu_mock_set_backend(
        fixture.mock,
        qcpu_test_backend_ops(),
        fixture.backend,
        "reattached-test"
    ) == 0);

    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_DISCONNECT
    );
    exchange = make_status_exchange(0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ENODEV
    );

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_OFFLINE);
    CHECK(status.failed_requests == 1U);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS
    );
    CHECK(qcpu_mock_set_backend(
        fixture.mock,
        qcpu_test_backend_ops(),
        fixture.backend,
        "reattached-test"
    ) == 0);

    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: BACKEND_ABSENT_DISCONNECT_AND_RECOVERY");
    return 0;
}

static int test_timeout_and_same_session_recovery(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_DELAY_UNTIL_CANCELED
    );

    exchange = make_status_exchange(UINT64_C(50000000));
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ETIMEDOUT
    );
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_TIMEOUT);
    CHECK(qcpu_test_backend_cancels(fixture.backend) == 1U);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
    CHECK(status.failed_requests == 1U);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS
    );
    {
        uint64_t dispatches_before =
            qcpu_test_backend_dispatches(fixture.backend);
        int one_ns_result;

        exchange = make_status_exchange(1U);
        errno = 0;
        one_ns_result = qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        );

        if (one_ns_result != 0) {
            CHECK(errno == ETIMEDOUT);
            CHECK(exchange.response.status ==
                  QCPU_UAPI_STATUS_TIMEOUT);
        }

        CHECK(qcpu_test_backend_dispatches(fixture.backend) ==
              dispatches_before + 1U);
    }

    exchange = make_status_exchange(QCPU_MOCK_MAX_TIMEOUT_NS);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: TIMEOUT_AND_SAME_SESSION_RECOVERY");
    return 0;
}

static int test_bounded_uncooperative_timeout(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;
    struct timespec started;
    struct timespec finished;
    struct timespec pause_time = {0, 1000000L};
    uint64_t elapsed_ns;
    bool idle_observed = false;

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_IGNORE_CANCEL_UNTIL_RELEASE
    );

    exchange = make_status_exchange(UINT64_C(50000000));
    CHECK(clock_gettime(CLOCK_MONOTONIC, &started) == 0);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        ETIMEDOUT
    );
    CHECK(clock_gettime(CLOCK_MONOTONIC, &finished) == 0);

    elapsed_ns =
        test_timespec_to_ns(&finished) -
        test_timespec_to_ns(&started);

    CHECK(elapsed_ns < UINT64_C(500000000));
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_TIMEOUT);
    CHECK(qcpu_test_backend_cancels(fixture.backend) == 1U);

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_BUSY);
    CHECK(status.failed_requests == 1U);

    exchange = make_status_exchange(0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ),
        EBUSY
    );
    CHECK_ERR(qcpu_mock_close(fixture.mock), EBUSY);
    CHECK_ERR(qcpu_mock_destroy(fixture.mock), EBUSY);

    qcpu_test_backend_release(fixture.backend);
    CHECK(qcpu_test_backend_wait_idle(
        fixture.backend,
        TEST_TIMEOUT_NS
    ) == 0);

    for (unsigned int index = 0U; index < 500U; index++) {
        CHECK(qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_GET_STATUS_V1,
            &status
        ) == 0);

        if (status.device_state == QCPU_UAPI_DEVICE_IDLE) {
            idle_observed = true;
            break;
        }

        (void)nanosleep(&pause_time, NULL);
    }

    CHECK(idle_observed);
    CHECK(status.failed_requests == 1U);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS
    );
    exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: BOUNDED_UNCOOPERATIVE_TIMEOUT_RETURN");
    return 0;
}

static int test_busy_cancel_and_destroy_quiescence(void)
{
    struct test_fixture fixture;
    struct exchange_thread_context context;
    struct qcpu_uapi_exchange_v1 second_exchange;
    pthread_t thread;

    CHECK(fixture_create(&fixture, true) == 0);
    CHECK(fixture_open(&fixture) == 0);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_DELAY_UNTIL_CANCELED
    );

    memset(&context, 0, sizeof(context));
    context.mock = fixture.mock;
    context.exchange = make_status_exchange(
        QCPU_MOCK_MAX_TIMEOUT_NS
    );

    CHECK(pthread_create(
        &thread,
        NULL,
        exchange_thread,
        &context
    ) == 0);

    CHECK(qcpu_test_backend_wait_started(
        fixture.backend,
        TEST_TIMEOUT_NS
    ) == 0);

    second_exchange = make_status_exchange(0U);
    CHECK_ERR(
        qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &second_exchange
        ),
        EBUSY
    );
    CHECK(second_exchange.response.status ==
          QCPU_UAPI_STATUS_BUSY);

    CHECK_ERR(qcpu_mock_destroy(fixture.mock), EBUSY);

    CHECK(fixture_close(&fixture) == 0);
    CHECK(pthread_join(thread, NULL) == 0);
    CHECK(context.result == -1);
    CHECK(context.error_number == ECANCELED);
    CHECK(context.exchange.response.status ==
          QCPU_UAPI_STATUS_CANCELED);
    CHECK(qcpu_test_backend_cancels(fixture.backend) == 1U);
    CHECK(qcpu_test_backend_max_concurrent(fixture.backend) == 1U);

    qcpu_test_backend_set_mode(
        fixture.backend,
        QCPU_TEST_BACKEND_SUCCESS
    );
    CHECK(fixture_open(&fixture) == 0);
    second_exchange = make_status_exchange(0U);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &second_exchange
    ) == 0);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: BUSY_CANCEL_AND_DESTROY_QUIESCENCE");
    return 0;
}

static int write_byte(int descriptor, char value)
{
    ssize_t count = write(descriptor, &value, 1U);

    return count == 1 ? 0 : -1;
}

static int read_byte(int descriptor, char expected)
{
    char value = '\0';
    ssize_t count = read(descriptor, &value, 1U);

    if (count != 1 || value != expected) {
        errno = EIO;
        return -1;
    }

    return 0;
}

static int process_create_mock(
    const char *runtime_dir,
    struct qcpu_test_backend **backend_out,
    struct qcpu_mock **mock_out
)
{
    struct qcpu_mock_config config;

    if (qcpu_test_backend_create(backend_out) != 0) {
        return -1;
    }

    memset(&config, 0, sizeof(config));
    config.runtime_dir = runtime_dir;
    config.backend_ops = qcpu_test_backend_ops();
    config.backend_context = *backend_out;
    config.backend_name = "cross-process";

    if (qcpu_mock_create(mock_out, &config) != 0) {
        (void)qcpu_test_backend_destroy(*backend_out);
        *backend_out = NULL;
        return -1;
    }

    return 0;
}

static int test_cross_process_lock(void)
{
    char runtime_template[] = "/tmp/qcpu-v47-cross-XXXXXX";
    char *runtime_dir;
    int parent_to_child[2];
    int child_to_parent[2];
    pid_t child;
    int child_status;
    char lock_path[256];
    struct stat lock_before;
    struct stat lock_after_parent_close;
    struct stat lock_after_child_close;
    struct qcpu_test_backend *parent_backend = NULL;
    struct qcpu_mock *parent_mock = NULL;

    runtime_dir = mkdtemp(runtime_template);
    CHECK(runtime_dir != NULL);
    CHECK(chmod(runtime_dir, S_IRWXU) == 0);
    CHECK(snprintf(
        lock_path,
        sizeof(lock_path),
        "%s/qcpu_mock.lock",
        runtime_dir
    ) > 0);
    CHECK(pipe(parent_to_child) == 0);
    CHECK(pipe(child_to_parent) == 0);

    child = fork();
    CHECK(child >= 0);

    if (child == 0) {
        struct qcpu_test_backend *child_backend = NULL;
        struct qcpu_mock *child_mock = NULL;
        int child_result = 0;

        (void)close(parent_to_child[1]);
        (void)close(child_to_parent[0]);

        if (read_byte(parent_to_child[0], 'L') != 0) {
            _exit(10);
        }

        if (process_create_mock(
                runtime_dir,
                &child_backend,
                &child_mock
            ) != 0) {
            _exit(11);
        }

        errno = 0;
        if (qcpu_mock_open(child_mock) != -1 ||
            errno != EBUSY) {
            _exit(12);
        }

        if (write_byte(child_to_parent[1], 'B') != 0) {
            _exit(13);
        }

        if (read_byte(parent_to_child[0], 'R') != 0) {
            _exit(14);
        }

        if (qcpu_mock_open(child_mock) != 0) {
            _exit(15);
        }

        if (qcpu_mock_close(child_mock) != 0) {
            _exit(16);
        }

        if (qcpu_mock_destroy(child_mock) != 0) {
            _exit(17);
        }

        if (qcpu_test_backend_destroy(child_backend) != 0) {
            _exit(18);
        }

        if (write_byte(child_to_parent[1], 'S') != 0) {
            child_result = 19;
        }

        (void)close(parent_to_child[0]);
        (void)close(child_to_parent[1]);
        _exit(child_result);
    }

    (void)close(parent_to_child[0]);
    (void)close(child_to_parent[1]);

    CHECK(process_create_mock(
        runtime_dir,
        &parent_backend,
        &parent_mock
    ) == 0);
    CHECK(qcpu_mock_open(parent_mock) == 0);
    CHECK(stat(lock_path, &lock_before) == 0);

    CHECK(write_byte(parent_to_child[1], 'L') == 0);
    CHECK(read_byte(child_to_parent[0], 'B') == 0);

    CHECK(qcpu_mock_close(parent_mock) == 0);
    CHECK(stat(lock_path, &lock_after_parent_close) == 0);
    CHECK(lock_before.st_dev == lock_after_parent_close.st_dev);
    CHECK(lock_before.st_ino == lock_after_parent_close.st_ino);

    CHECK(write_byte(parent_to_child[1], 'R') == 0);
    CHECK(read_byte(child_to_parent[0], 'S') == 0);
    CHECK(stat(lock_path, &lock_after_child_close) == 0);
    CHECK(lock_before.st_dev == lock_after_child_close.st_dev);
    CHECK(lock_before.st_ino == lock_after_child_close.st_ino);

    CHECK(waitpid(child, &child_status, 0) == child);
    CHECK(WIFEXITED(child_status));
    CHECK(WEXITSTATUS(child_status) == 0);

    CHECK(qcpu_mock_destroy(parent_mock) == 0);
    CHECK(qcpu_test_backend_destroy(parent_backend) == 0);

    (void)close(parent_to_child[1]);
    (void)close(child_to_parent[0]);

    CHECK(remove_runtime_lock_file(runtime_dir) == 0);
    CHECK(rmdir(runtime_dir) == 0);

    puts("PASS: MANDATORY_CROSS_PROCESS_EXCLUSIVE_LOCK");
    puts("PASS: ADVISORY_LOCK_INODE_STABLE_ACROSS_SESSIONS");
    return 0;
}

static int test_repeated_lifecycle(void)
{
    struct test_fixture fixture;
    struct qcpu_uapi_exchange_v1 exchange;
    struct qcpu_uapi_status_v1 status;

    CHECK(fixture_create(&fixture, true) == 0);

    for (unsigned int index = 0U; index < 100U; index++) {
        CHECK(fixture_open(&fixture) == 0);

        exchange = make_status_exchange(0U);
        CHECK(qcpu_mock_ioctl(
            fixture.mock,
            QCPU_IOC_EXCHANGE_V1,
            &exchange
        ) == 0);

        CHECK(fixture_close(&fixture) == 0);
    }

    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.active_client_sessions == 0U);
    CHECK(status.completed_requests == 100U);
    CHECK(status.failed_requests == 0U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: REPEATED_100_SESSION_LIFECYCLE");
    return 0;
}

int main(void)
{
    CHECK(test_abi_and_ioctl_identity() == 0);
    CHECK(test_caps_status_and_unknown_ioctl() == 0);
    CHECK(test_session_exclusivity() == 0);
    CHECK(test_status_and_ghz_success() == 0);
    CHECK(test_predispatch_validation_and_counters() == 0);
    CHECK(test_norm_conversion() == 0);
    CHECK(test_response_validation() == 0);
    CHECK(test_backend_absent_and_recovery() == 0);
    CHECK(test_timeout_and_same_session_recovery() == 0);
    CHECK(test_bounded_uncooperative_timeout() == 0);
    CHECK(test_busy_cancel_and_destroy_quiescence() == 0);
    CHECK(test_cross_process_lock() == 0);
    CHECK(test_repeated_lifecycle() == 0);

    puts("PASS: QCPU_V47_STAGE2A_USERSPACE_MOCK_CORE_READY");
    return 0;
}
