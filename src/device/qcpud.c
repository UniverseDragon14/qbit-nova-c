#define _POSIX_C_SOURCE 200809L

#include "qcpu_device_wire.h"
#include "../quantum/qcpu_kernel.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;
static int server_fd_for_signal = -1;

static void handle_stop_signal(int signal_number) {
    int file_descriptor;

    (void)signal_number;
    stop_requested = 1;
    file_descriptor = server_fd_for_signal;

    if (file_descriptor >= 0) {
        (void)close(file_descriptor);
    }
}

static int install_signal_handlers(void) {
    struct sigaction stop_action;
    struct sigaction pipe_action;

    memset(&stop_action, 0, sizeof(stop_action));
    stop_action.sa_handler = handle_stop_signal;

    if (sigemptyset(&stop_action.sa_mask) != 0) {
        return -1;
    }

    if (sigaction(SIGINT, &stop_action, NULL) != 0) {
        return -1;
    }

    if (sigaction(SIGTERM, &stop_action, NULL) != 0) {
        return -1;
    }

    memset(&pipe_action, 0, sizeof(pipe_action));
    pipe_action.sa_handler = SIG_IGN;

    if (sigemptyset(&pipe_action.sa_mask) != 0) {
        return -1;
    }

    if (sigaction(SIGPIPE, &pipe_action, NULL) != 0) {
        return -1;
    }

    return 0;
}

static uint32_t device_flags(void) {
    return
        QCPU_DEVICE_FLAG_SOFTWARE_VIRTUAL_QCPU |
        QCPU_DEVICE_FLAG_CLASSICAL_HOST |
        QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT |
        QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT;
}

static void response_init(QCPUDeviceResponse *response) {
    memset(response, 0, sizeof(*response));
    response->magic = QCPU_DEVICE_RESPONSE_MAGIC;
    response->version = QCPU_DEVICE_PROTOCOL_VERSION;
    response->flags = device_flags();
}

static int read_all(
    int file_descriptor,
    uint8_t *buffer,
    size_t size
) {
    size_t completed = 0U;

    while (completed < size) {
        ssize_t result = read(
            file_descriptor,
            buffer + completed,
            size - completed
        );

        if (result == 0) {
            return -1;
        }

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        completed += (size_t)result;
    }

    return 0;
}

static int write_all(
    int file_descriptor,
    const uint8_t *buffer,
    size_t size
) {
    size_t completed = 0U;

    while (completed < size) {
        ssize_t result = write(
            file_descriptor,
            buffer + completed,
            size - completed
        );

        if (result == 0) {
            return -1;
        }

        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        completed += (size_t)result;
    }

    return 0;
}

static void process_run_ghz(
    const QCPUDeviceRequest *request,
    QCPUDeviceResponse *response
) {
    QCPUKernel kernel;
    QCPUShotResult shot_result = {0};
    QCPUStatus kernel_status;
    uint64_t invalid_results = 0U;
    size_t measured_state = 0U;

    if (
        request->qubits == 0U ||
        request->qubits > QCPU_DEVICE_MAX_QUBITS ||
        request->shots == 0U ||
        request->shots > QCPU_DEVICE_MAX_SHOTS
    ) {
        response->status = QCPU_DEVICE_STATUS_RANGE;
        return;
    }

    kernel_status = qcpu_kernel_init(
        &kernel,
        request->qubits,
        request->seed
    );

    if (kernel_status != QCPU_OK) {
        response->status = QCPU_DEVICE_STATUS_KERNEL;
        return;
    }

    kernel_status = qcpu_kernel_apply_ghz(&kernel);

    if (kernel_status == QCPU_OK) {
        kernel_status = qcpu_kernel_validate(&kernel, 1e-12);
    }

    if (kernel_status == QCPU_OK) {
        kernel_status = qcpu_kernel_run_shots(
            &kernel,
            request->shots,
            &shot_result
        );
    }

    if (kernel_status == QCPU_OK) {
        size_t final_state = kernel.state_count - 1U;
        size_t state;

        for (
            state = 0U;
            state < shot_result.state_count;
            ++state
        ) {
            if (state != 0U && state != final_state) {
                invalid_results += shot_result.counts[state];
            }
        }

        kernel_status = qcpu_kernel_measure(
            &kernel,
            &measured_state
        );
    }

    if (kernel_status != QCPU_OK) {
        qcpu_shot_result_free(&shot_result);
        qcpu_kernel_free(&kernel);
        response->status = QCPU_DEVICE_STATUS_KERNEL;
        return;
    }

    response->status = QCPU_DEVICE_STATUS_OK;
    response->qubits = request->qubits;
    response->basis_states = (uint64_t)kernel.state_count;
    response->shots = request->shots;
    response->measured_state = (uint64_t)measured_state;
    response->invalid_results = invalid_results;
    response->norm = qcpu_kernel_norm(&kernel);

    qcpu_shot_result_free(&shot_result);
    qcpu_kernel_free(&kernel);
}

static void process_request(
    const QCPUDeviceRequest *request,
    QCPUDeviceResponse *response
) {
    response_init(response);

    switch (request->command) {
        case QCPU_DEVICE_COMMAND_STATUS:
            response->status = QCPU_DEVICE_STATUS_OK;
            break;

        case QCPU_DEVICE_COMMAND_RUN_GHZ:
            process_run_ghz(request, response);
            break;

        default:
            response->status = QCPU_DEVICE_STATUS_BAD_COMMAND;
            break;
    }
}

static int serve_connection(int client_fd) {
    uint8_t request_wire[QCPU_DEVICE_REQUEST_WIRE_SIZE];
    uint8_t response_wire[QCPU_DEVICE_RESPONSE_WIRE_SIZE];
    QCPUDeviceRequest request;
    QCPUDeviceResponse response;
    QCPUDeviceStatus protocol_status;
    unsigned command_for_log = 0U;

    if (
        read_all(
            client_fd,
            request_wire,
            sizeof(request_wire)
        ) != 0
    ) {
        fprintf(stderr, "ERROR: incomplete request packet\n");
        return -1;
    }

    protocol_status = qcpu_device_decode_request(
        &request,
        request_wire,
        sizeof(request_wire)
    );

    command_for_log = (unsigned)request.command;

    if (protocol_status != QCPU_DEVICE_STATUS_OK) {
        response_init(&response);
        response.status = (uint16_t)protocol_status;
    } else {
        process_request(&request, &response);
    }

    if (
        qcpu_device_encode_response(
            response_wire,
            &response
        ) != QCPU_DEVICE_STATUS_OK
    ) {
        return -1;
    }

    if (
        write_all(
            client_fd,
            response_wire,
            sizeof(response_wire)
        ) != 0
    ) {
        return -1;
    }

    printf(
        "PASS: QCPUD_REQUEST_SERVED command=%u status=%u\n",
        command_for_log,
        (unsigned)response.status
    );
    fflush(stdout);

    return 0;
}

static int prepare_socket_path(const char *socket_path) {
    struct stat status;

    if (lstat(socket_path, &status) == 0) {
        if (
            !S_ISSOCK(status.st_mode) ||
            status.st_uid != getuid()
        ) {
            fprintf(
                stderr,
                "ERROR: refusing to replace unsafe socket path\n"
            );
            return -1;
        }

        if (unlink(socket_path) != 0) {
            perror("unlink");
            return -1;
        }

        return 0;
    }

    if (errno != ENOENT) {
        perror("lstat");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    const char *socket_path;
    int once = 0;
    int server_fd = -1;
    struct sockaddr_un address;

    if (argc != 3 && argc != 4) {
        fprintf(
            stderr,
            "USAGE: %s --socket PATH [--once]\n",
            argv[0]
        );
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--socket") != 0) {
        fprintf(stderr, "ERROR: --socket is required\n");
        return EXIT_FAILURE;
    }

    socket_path = argv[2];

    if (argc == 4) {
        if (strcmp(argv[3], "--once") != 0) {
            fprintf(
                stderr,
                "ERROR: unsupported option: %s\n",
                argv[3]
            );
            return EXIT_FAILURE;
        }

        once = 1;
    }

    if (strlen(socket_path) >= sizeof(address.sun_path)) {
        fprintf(stderr, "ERROR: socket path too long\n");
        return EXIT_FAILURE;
    }

    umask(0077);

    if (install_signal_handlers() != 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    if (prepare_socket_path(socket_path) != 0) {
        return EXIT_FAILURE;
    }

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    memcpy(
        address.sun_path,
        socket_path,
        strlen(socket_path) + 1U
    );

    if (
        bind(
            server_fd,
            (struct sockaddr *)&address,
            sizeof(address)
        ) != 0
    ) {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (chmod(socket_path, 0600) != 0) {
        perror("chmod");
        close(server_fd);
        unlink(socket_path);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 4) != 0) {
        perror("listen");
        close(server_fd);
        unlink(socket_path);
        return EXIT_FAILURE;
    }

    server_fd_for_signal = server_fd;

    printf("PASS: QCPUD_SOCKET_READY path=%s\n", socket_path);
    printf(
        "DEVICE=qcpu0 TYPE=SOFTWARE_VIRTUAL_QCPU "
        "PHYSICAL_QPU_PRESENT=NO\n"
    );
    fflush(stdout);

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);
        int result;

        if (client_fd < 0) {
            if (stop_requested != 0) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            server_fd_for_signal = -1;
            close(server_fd);
            unlink(socket_path);
            return EXIT_FAILURE;
        }

        result = serve_connection(client_fd);
        close(client_fd);

        if (result != 0) {
            close(server_fd);
            unlink(socket_path);
            return EXIT_FAILURE;
        }

        if (once) {
            break;
        }
    }

    server_fd_for_signal = -1;

    if (close(server_fd) != 0 && errno != EBADF) {
        perror("close server socket");
        unlink(socket_path);
        return EXIT_FAILURE;
    }

    if (unlink(socket_path) != 0) {
        perror("unlink socket");
        return EXIT_FAILURE;
    }

    printf("PASS: QCPUD_CLEAN_SHUTDOWN\n");

    return EXIT_SUCCESS;
}
