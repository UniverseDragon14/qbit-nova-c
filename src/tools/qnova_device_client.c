#include "../device/qcpu_device_wire.h"
#include "../device/qcpu_device_io.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static uint64_t parse_unsigned(
    const char *text,
    uint64_t maximum,
    const char *label
) {
    const unsigned char *cursor =
        (const unsigned char *)text;
    char *end = NULL;
    unsigned long long value;

    while (*cursor != '\0' && isspace(*cursor)) {
        ++cursor;
    }

    if (*cursor == '-') {
        fprintf(stderr, "ERROR: invalid %s: %s\n", label, text);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    value = strtoull(text, &end, 0);

    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        value > maximum
    ) {
        fprintf(stderr, "ERROR: invalid %s: %s\n", label, text);
        exit(EXIT_FAILURE);
    }

    return (uint64_t)value;
}

static void print_response(const QCPUDeviceResponse *response) {
    printf("DEVICE=qcpu0\n");
    printf("TYPE=SOFTWARE_VIRTUAL_QCPU\n");
    printf("HOST=CLASSICAL_CPU\n");

    printf(
        "PHYSICAL_QPU_PRESENT=%s\n",
        (
            response->flags &
            QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT
        ) != 0U
            ? "NO"
            : "UNKNOWN"
    );

    printf(
        "Q0_ORDER=%s\n",
        (
            response->flags &
            QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT
        ) != 0U
            ? "MOST_SIGNIFICANT"
            : "UNKNOWN"
    );

    printf("QUBITS=%u\n", (unsigned)response->qubits);
    printf(
        "BASIS_STATES=%llu\n",
        (unsigned long long)response->basis_states
    );
    printf(
        "SHOTS=%llu\n",
        (unsigned long long)response->shots
    );
    printf(
        "MEASURED_STATE=%llu\n",
        (unsigned long long)response->measured_state
    );
    printf(
        "INVALID_RESULTS=%llu\n",
        (unsigned long long)response->invalid_results
    );
    printf("NORM=%.12f\n", response->norm);
    printf("STATUS_CODE=%u\n", (unsigned)response->status);
    printf(
        "STATUS=%s\n",
        response->status == QCPU_DEVICE_STATUS_OK
            ? "PASS"
            : "FAIL"
    );
}

int main(int argc, char **argv) {
    const char *socket_path;
    QCPUDeviceRequest request = {
        .magic = QCPU_DEVICE_REQUEST_MAGIC,
        .version = QCPU_DEVICE_PROTOCOL_VERSION
    };
    QCPUDeviceResponse response;
    uint8_t request_wire[QCPU_DEVICE_REQUEST_WIRE_SIZE];
    uint8_t response_wire[QCPU_DEVICE_RESPONSE_WIRE_SIZE];
    int file_descriptor;
    QCPUDeviceIOStatus io_status;

    if (argc < 4 || strcmp(argv[1], "--socket") != 0) {
        fprintf(
            stderr,
            "USAGE:\n"
            "  %s --socket PATH status\n"
            "  %s --socket PATH run-ghz QUBITS SHOTS SEED\n",
            argv[0],
            argv[0]
        );
        return EXIT_FAILURE;
    }

    socket_path = argv[2];

    if (strcmp(argv[3], "status") == 0) {
        if (argc != 4) {
            fprintf(stderr, "ERROR: status takes no arguments\n");
            return EXIT_FAILURE;
        }

        request.command = QCPU_DEVICE_COMMAND_STATUS;
    } else if (strcmp(argv[3], "run-ghz") == 0) {
        if (argc != 7) {
            fprintf(
                stderr,
                "ERROR: run-ghz requires QUBITS SHOTS SEED\n"
            );
            return EXIT_FAILURE;
        }

        request.command = QCPU_DEVICE_COMMAND_RUN_GHZ;
        request.qubits = (uint32_t)parse_unsigned(
            argv[4],
            QCPU_DEVICE_MAX_QUBITS,
            "qubit count"
        );
        request.shots = (uint32_t)parse_unsigned(
            argv[5],
            QCPU_DEVICE_MAX_SHOTS,
            "shot count"
        );
        request.seed = parse_unsigned(
            argv[6],
            UINT64_MAX,
            "seed"
        );
    } else {
        fprintf(stderr, "ERROR: unknown command: %s\n", argv[3]);
        return EXIT_FAILURE;
    }

    if (
        qcpu_device_encode_request(
            request_wire,
            &request
        ) != QCPU_DEVICE_STATUS_OK
    ) {
        fprintf(stderr, "ERROR: request encoding failed\n");
        return EXIT_FAILURE;
    }

    file_descriptor = qcpu_device_connect_unix(
        socket_path,
        QCPU_DEVICE_IO_TIMEOUT_MS
    );

    if (file_descriptor < 0) {
        if (file_descriptor == QCPU_DEVICE_IO_TIMEOUT) {
            fprintf(stderr, "ERROR: connection timeout\n");
        } else {
            perror("connect");
        }

        return EXIT_FAILURE;
    }

    io_status = qcpu_device_write_all(
        file_descriptor,
        request_wire,
        sizeof(request_wire),
        QCPU_DEVICE_IO_TIMEOUT_MS,
        NULL
    );

    if (io_status != QCPU_DEVICE_IO_OK) {
        fprintf(
            stderr,
            "%s\n",
            io_status == QCPU_DEVICE_IO_TIMEOUT
                ? "ERROR: request write timeout"
                : "ERROR: request write failed"
        );

        close(file_descriptor);
        return EXIT_FAILURE;
    }

    if (shutdown(file_descriptor, SHUT_WR) != 0) {
        perror("shutdown");
        close(file_descriptor);
        return EXIT_FAILURE;
    }

    io_status = qcpu_device_read_all(
        file_descriptor,
        response_wire,
        sizeof(response_wire),
        QCPU_DEVICE_IO_TIMEOUT_MS,
        NULL
    );

    if (io_status != QCPU_DEVICE_IO_OK) {
        fprintf(
            stderr,
            "%s\n",
            io_status == QCPU_DEVICE_IO_TIMEOUT
                ? "ERROR: response read timeout"
                : "ERROR: response read failed"
        );

        close(file_descriptor);
        return EXIT_FAILURE;
    }

    close(file_descriptor);

    if (
        qcpu_device_decode_response(
            &response,
            response_wire,
            sizeof(response_wire)
        ) != QCPU_DEVICE_STATUS_OK
    ) {
        fprintf(stderr, "ERROR: invalid response packet\n");
        return EXIT_FAILURE;
    }

    print_response(&response);

    return response.status == QCPU_DEVICE_STATUS_OK
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
