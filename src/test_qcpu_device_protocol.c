#include "device/qcpu_device_protocol.h"

#include <stdio.h>
#include <stdlib.h>

static void require_true(
    int condition,
    const char *label
) {
    if (!condition) {
        fprintf(
            stderr,
            "FAIL: %s\n",
            label
        );

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

    require_true(
        sizeof(request) == 24U,
        "request_size_is_24"
    );

    require_true(
        sizeof(response) == 56U,
        "response_size_is_56"
    );

    require_true(
        request.magic ==
            QCPU_DEVICE_REQUEST_MAGIC,
        "request_magic"
    );

    require_true(
        request.version ==
            QCPU_DEVICE_PROTOCOL_VERSION,
        "protocol_version"
    );

    require_true(
        request.qubits <=
            QCPU_DEVICE_MAX_QUBITS,
        "qubit_limit"
    );

    require_true(
        request.shots <=
            QCPU_DEVICE_MAX_SHOTS,
        "shot_limit"
    );

    require_true(
        response.flags &
            QCPU_DEVICE_FLAG_SOFTWARE_VIRTUAL_QCPU,
        "software_virtual_qcpu_flag"
    );

    require_true(
        response.flags &
            QCPU_DEVICE_FLAG_CLASSICAL_HOST,
        "classical_host_flag"
    );

    require_true(
        response.flags &
            QCPU_DEVICE_FLAG_PHYSICAL_QPU_ABSENT,
        "physical_qpu_absent_flag"
    );

    require_true(
        response.flags &
            QCPU_DEVICE_FLAG_Q0_MOST_SIGNIFICANT,
        "q0_msb_flag"
    );

    require_true(
        response.basis_states ==
            UINT64_C(8),
        "three_qubits_expand_to_eight_states"
    );

    require_true(
        response.invalid_results ==
            UINT64_C(0),
        "invalid_result_count_zero"
    );

    require_true(
        response.norm == 1.0,
        "response_norm_one"
    );

    printf(
        "PASS: QCPU_DEVICE_REQUEST_LAYOUT_READY\n"
    );

    printf(
        "PASS: QCPU_DEVICE_RESPONSE_LAYOUT_READY\n"
    );

    printf(
        "PASS: QCPU_DEVICE_TRUTH_FLAGS_READY\n"
    );

    printf(
        "PASS: QCPU_VIRTUAL_DEVICE_PROTOCOL_READY\n"
    );

    return EXIT_SUCCESS;
}
