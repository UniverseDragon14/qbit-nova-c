#define _POSIX_C_SOURCE 200809L

#include "src/device/qcpu_qcpud_adapter.h"
#include "src/device/qcpu_device_io.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <unistd.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf( \
            stderr, \
            "FAIL: %s:%d: %s (errno=%d)\n", \
            __FILE__, \
            __LINE__, \
            #condition, \
            errno \
        ); \
        return -1; \
    } \
} while (0)

struct adapter_fixture {
    struct qcpu_qcpud_adapter *adapter;
    struct qcpu_mock *mock;
    int session_open;
};

static uint64_t monotonic_ns(void)
{
    struct timespec now;

    CHECK(clock_gettime(CLOCK_MONOTONIC, &now) == 0);

    return ((uint64_t)now.tv_sec * UINT64_C(1000000000)) +
           (uint64_t)now.tv_nsec;
}

static struct qcpu_uapi_exchange_v1 status_exchange(uint64_t timeout_ns)
{
    struct qcpu_uapi_exchange_v1 exchange;

    memset(&exchange, 0, sizeof(exchange));
    exchange.request.magic = QCPU_UAPI_REQUEST_MAGIC;
    exchange.request.version = QCPU_UAPI_PROTOCOL_VERSION;
    exchange.request.command = QCPU_UAPI_COMMAND_STATUS;
    exchange.timeout_ns = timeout_ns;

    return exchange;
}

static struct qcpu_uapi_exchange_v1 ghz_exchange(void)
{
    struct qcpu_uapi_exchange_v1 exchange;

    memset(&exchange, 0, sizeof(exchange));
    exchange.request.magic = QCPU_UAPI_REQUEST_MAGIC;
    exchange.request.version = QCPU_UAPI_PROTOCOL_VERSION;
    exchange.request.command = QCPU_UAPI_COMMAND_RUN_GHZ;
    exchange.request.qubits = 3U;
    exchange.request.shots = 20U;
    exchange.request.seed = UINT64_C(424242);
    exchange.timeout_ns = QCPU_MOCK_DEFAULT_TIMEOUT_NS;

    return exchange;
}

static int test_sigpipe_suppression(void)
{
    uint8_t byte = UINT8_C(0x5a);
    int sockets[2] = {-1, -1};
    QCPUDeviceIOStatus io_status;

    CHECK(signal(SIGPIPE, SIG_DFL) != SIG_ERR);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    CHECK(close(sockets[1]) == 0);
    sockets[1] = -1;

    errno = 0;
    io_status = qcpu_device_write_all(
        sockets[0],
        &byte,
        sizeof(byte),
        100,
        NULL
    );

    CHECK(io_status == QCPU_DEVICE_IO_ERROR);
    CHECK(errno == EPIPE || errno == ECONNRESET);
    CHECK(close(sockets[0]) == 0);

    puts("PASS: QCPU_V47_STAGE2B_SIGPIPE_SUPPRESSED_READY");
    return 0;
}

static int fixture_create(
    struct adapter_fixture *fixture,
    const char *runtime_dir,
    const char *socket_path
)
{
    struct qcpu_mock_config config;

    memset(fixture, 0, sizeof(*fixture));
    memset(&config, 0, sizeof(config));

    CHECK(qcpu_qcpud_adapter_create(
        &fixture->adapter,
        socket_path
    ) == 0);

    config.runtime_dir = runtime_dir;
    config.backend_name = QCPU_QCPUD_ADAPTER_BACKEND_NAME;
    config.driver_version = "4.7-stage2b";
    config.backend_ops = qcpu_qcpud_adapter_ops();
    config.backend_context = fixture->adapter;

    CHECK(qcpu_mock_create(&fixture->mock, &config) == 0);
    CHECK(qcpu_mock_open(fixture->mock) == 0);
    fixture->session_open = 1;

    return 0;
}

static int fixture_destroy(struct adapter_fixture *fixture)
{
    if (fixture->session_open) {
        CHECK(qcpu_mock_close(fixture->mock) == 0);
        fixture->session_open = 0;
    }

    CHECK(qcpu_mock_destroy(fixture->mock) == 0);
    fixture->mock = NULL;
    CHECK(qcpu_qcpud_adapter_destroy(fixture->adapter) == 0);
    fixture->adapter = NULL;

    return 0;
}

static int test_happy_path(
    const char *runtime_dir,
    const char *socket_path
)
{
    struct adapter_fixture fixture;
    struct qcpu_uapi_caps_v1 caps;
    struct qcpu_uapi_status_v1 status;
    struct qcpu_uapi_exchange_v1 exchange;
    uint64_t all_one_state;

    CHECK(fixture_create(&fixture, runtime_dir, socket_path) == 0);

    memset(&caps, 0, sizeof(caps));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_CAPS_V1,
        &caps
    ) == 0);
    CHECK(caps.abi_version == QCPU_UAPI_ABI_VERSION);
    CHECK(caps.protocol_version == QCPU_UAPI_PROTOCOL_VERSION);
    CHECK((caps.flags & QCPU_MOCK_REQUIRED_FLAGS) ==
          QCPU_MOCK_REQUIRED_FLAGS);
    CHECK(strncmp(
        (const char *)caps.backend,
        QCPU_QCPUD_ADAPTER_BACKEND_NAME,
        sizeof(caps.backend)
    ) == 0);

    exchange = status_exchange(QCPU_MOCK_DEFAULT_TIMEOUT_NS);
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_OK);
    CHECK(exchange.response.qubits == 0U);
    CHECK(exchange.response.shots == 0U);
    CHECK((exchange.response.flags & QCPU_MOCK_REQUIRED_FLAGS) ==
          QCPU_MOCK_REQUIRED_FLAGS);

    exchange = ghz_exchange();
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == 0);
    all_one_state = (UINT64_C(1) << 3) - UINT64_C(1);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_OK);
    CHECK(exchange.response.qubits == 3U);
    CHECK(exchange.response.basis_states == 8U);
    CHECK(exchange.response.shots == 20U);
    CHECK(exchange.response.measured_state == 0U ||
          exchange.response.measured_state == all_one_state);
    CHECK(exchange.response.invalid_results == 0U);
    CHECK(exchange.response.norm_q32_32 == (UINT64_C(1) << 32));

    memset(&status, 0, sizeof(status));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
    CHECK(status.completed_requests == 2U);
    CHECK(status.failed_requests == 0U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: QCPU_V47_STAGE2B_STATUS_BRIDGE_READY");
    puts("PASS: QCPU_V47_STAGE2B_GHZ_BRIDGE_READY");
    puts("PASS: QCPU_V47_STAGE2B_Q32_32_BRIDGE_READY");
    return 0;
}

static int test_offline(
    const char *runtime_dir,
    const char *socket_path
)
{
    struct adapter_fixture fixture;
    struct qcpu_uapi_status_v1 status;
    struct qcpu_uapi_exchange_v1 exchange;

    CHECK(fixture_create(&fixture, runtime_dir, socket_path) == 0);

    exchange = status_exchange(UINT64_C(200000000));
    errno = 0;
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == -1);
    CHECK(errno == ENODEV);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_BACKEND_ABSENT);

    memset(&status, 0, sizeof(status));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_OFFLINE);
    CHECK(status.completed_requests == 0U);
    CHECK(status.failed_requests == 1U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: QCPU_V47_STAGE2B_BACKEND_ABSENT_READY");
    return 0;
}

static int test_stall(
    const char *runtime_dir,
    const char *socket_path
)
{
    struct adapter_fixture fixture;
    struct qcpu_uapi_status_v1 status;
    struct qcpu_uapi_exchange_v1 exchange;
    uint64_t start_ns;
    uint64_t elapsed_ns;

    CHECK(fixture_create(&fixture, runtime_dir, socket_path) == 0);

    exchange = status_exchange(UINT64_C(100000000));
    start_ns = monotonic_ns();
    errno = 0;
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == -1);
    elapsed_ns = monotonic_ns() - start_ns;

    CHECK(errno == ETIMEDOUT);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_TIMEOUT);
    CHECK(elapsed_ns < UINT64_C(1000000000));

    memset(&status, 0, sizeof(status));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
    CHECK(status.completed_requests == 0U);
    CHECK(status.failed_requests == 1U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: QCPU_V47_STAGE2B_BOUNDED_STALL_READY");
    return 0;
}

static int test_disconnect(
    const char *runtime_dir,
    const char *socket_path
)
{
    struct adapter_fixture fixture;
    struct qcpu_uapi_status_v1 status;
    struct qcpu_uapi_exchange_v1 exchange;

    CHECK(fixture_create(&fixture, runtime_dir, socket_path) == 0);

    exchange = status_exchange(QCPU_MOCK_DEFAULT_TIMEOUT_NS);
    errno = 0;
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_EXCHANGE_V1,
        &exchange
    ) == -1);
    CHECK(errno == EIO);
    CHECK(exchange.response.status == QCPU_UAPI_STATUS_IO);

    memset(&status, 0, sizeof(status));
    CHECK(qcpu_mock_ioctl(
        fixture.mock,
        QCPU_IOC_GET_STATUS_V1,
        &status
    ) == 0);
    CHECK(status.device_state == QCPU_UAPI_DEVICE_IDLE);
    CHECK(status.completed_requests == 0U);
    CHECK(status.failed_requests == 1U);

    CHECK(fixture_destroy(&fixture) == 0);

    puts("PASS: QCPU_V47_STAGE2B_TERMINATION_WAKEUP_READY");
    return 0;
}

int main(int argc, char **argv)
{
    int result;

    if (argc != 4) {
        fprintf(
            stderr,
            "USAGE: %s MODE RUNTIME_DIR SOCKET_PATH\n",
            argv[0]
        );
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "sigpipe") == 0) {
        result = test_sigpipe_suppression();
    } else if (strcmp(argv[1], "happy") == 0) {
        result = test_happy_path(argv[2], argv[3]);
    } else if (strcmp(argv[1], "offline") == 0) {
        result = test_offline(argv[2], argv[3]);
    } else if (strcmp(argv[1], "stall") == 0) {
        result = test_stall(argv[2], argv[3]);
    } else if (strcmp(argv[1], "disconnect") == 0) {
        result = test_disconnect(argv[2], argv[3]);
    } else {
        fprintf(stderr, "ERROR: unknown mode: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
