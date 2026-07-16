#include "device/qcpu_device_wire.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require_true(int condition, const char *label) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    QCPUDeviceRequest request = {
        .magic = QCPU_DEVICE_REQUEST_MAGIC,
        .version = QCPU_DEVICE_PROTOCOL_VERSION,
        .command = QCPU_DEVICE_COMMAND_RUN_GHZ,
        .qubits = 3U,
        .shots = 20U,
        .seed = UINT64_C(424242)
    };
    QCPUDeviceRequest decoded_request;
    QCPUDeviceResponse response = {
        .magic = QCPU_DEVICE_RESPONSE_MAGIC,
        .version = QCPU_DEVICE_PROTOCOL_VERSION,
        .status = QCPU_DEVICE_STATUS_OK,
        .flags =
            QCPU_DEVICE_FLAG_SOFTWARE_VIRTUAL_QCPU |
            QCPU_DEVICE_FLAG_CLASSICAL_HOST |
            QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT |
            QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT,
        .qubits = 3U,
        .basis_states = UINT64_C(8),
        .shots = UINT64_C(20),
        .measured_state = UINT64_C(7),
        .invalid_results = UINT64_C(0),
        .norm = 1.0
    };
    QCPUDeviceResponse decoded_response;
    uint8_t request_wire[QCPU_DEVICE_REQUEST_WIRE_SIZE];
    uint8_t response_wire[QCPU_DEVICE_RESPONSE_WIRE_SIZE];
    uint8_t bad_magic[QCPU_DEVICE_REQUEST_WIRE_SIZE];
    uint8_t bad_version[QCPU_DEVICE_REQUEST_WIRE_SIZE];

    static const uint8_t expected_request[
        QCPU_DEVICE_REQUEST_WIRE_SIZE
    ] = {
        0x31U, 0x56U, 0x44U, 0x51U,
        0x01U, 0x00U, 0x02U, 0x00U,
        0x03U, 0x00U, 0x00U, 0x00U,
        0x14U, 0x00U, 0x00U, 0x00U,
        0x32U, 0x79U, 0x06U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U
    };

    static const uint8_t expected_response[
        QCPU_DEVICE_RESPONSE_WIRE_SIZE
    ] = {
        0x31U, 0x56U, 0x52U, 0x51U,
        0x01U, 0x00U, 0x00U, 0x00U,
        0x0fU, 0x00U, 0x00U, 0x00U,
        0x03U, 0x00U, 0x00U, 0x00U,
        0x08U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x14U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x07U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0xf0U, 0x3fU
    };

    require_true(
        qcpu_device_encode_request(
            request_wire,
            &request
        ) == QCPU_DEVICE_STATUS_OK,
        "encode_request"
    );

    require_true(
        memcmp(
            request_wire,
            expected_request,
            sizeof(expected_request)
        ) == 0,
        "request_golden_packet"
    );

    require_true(
        qcpu_device_decode_request(
            &decoded_request,
            request_wire,
            sizeof(request_wire)
        ) == QCPU_DEVICE_STATUS_OK,
        "decode_request"
    );

    require_true(
        decoded_request.command ==
            QCPU_DEVICE_COMMAND_RUN_GHZ &&
        decoded_request.qubits == 3U &&
        decoded_request.shots == 20U &&
        decoded_request.seed == UINT64_C(424242),
        "request_round_trip"
    );

    memcpy(bad_magic, request_wire, sizeof(bad_magic));
    bad_magic[0] ^= UINT8_C(1);

    require_true(
        qcpu_device_decode_request(
            &decoded_request,
            bad_magic,
            sizeof(bad_magic)
        ) == QCPU_DEVICE_STATUS_BAD_MAGIC,
        "bad_magic_rejected"
    );

    memcpy(bad_version, request_wire, sizeof(bad_version));
    bad_version[4] = UINT8_C(0xff);
    bad_version[5] = UINT8_C(0xff);

    require_true(
        qcpu_device_decode_request(
            &decoded_request,
            bad_version,
            sizeof(bad_version)
        ) == QCPU_DEVICE_STATUS_BAD_VERSION,
        "bad_version_rejected"
    );

    require_true(
        qcpu_device_encode_response(
            response_wire,
            &response
        ) == QCPU_DEVICE_STATUS_OK,
        "encode_response"
    );

    require_true(
        memcmp(
            response_wire,
            expected_response,
            sizeof(expected_response)
        ) == 0,
        "response_golden_packet"
    );

    require_true(
        qcpu_device_decode_response(
            &decoded_response,
            response_wire,
            sizeof(response_wire)
        ) == QCPU_DEVICE_STATUS_OK,
        "decode_response"
    );

    require_true(
        decoded_response.basis_states == UINT64_C(8) &&
        decoded_response.measured_state == UINT64_C(7) &&
        decoded_response.invalid_results == UINT64_C(0),
        "response_round_trip"
    );

    require_true(
        fabs(decoded_response.norm - 1.0) < 1e-12,
        "response_norm_round_trip"
    );

    printf("PASS: QCPU_DEVICE_LITTLE_ENDIAN_WIRE_READY\n");
    printf("PASS: QCPU_DEVICE_BAD_MAGIC_REJECTED\n");
    printf("PASS: QCPU_DEVICE_BAD_VERSION_REJECTED\n");
    printf("PASS: QCPU_DEVICE_WIRE_ROUND_TRIP_READY\n");
    printf("PASS: QCPU_DEVICE_GOLDEN_PACKETS_READY\n");

    return EXIT_SUCCESS;
}
