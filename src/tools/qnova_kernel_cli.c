#include "../quantum/qcpu_kernel.h"

#include <complex.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void print_basis(
    size_t state,
    size_t qubit_count
) {
    putchar('|');

    for (
        size_t bit = qubit_count;
        bit > 0U;
        --bit
    ) {
        size_t value =
            (
                state >>
                (bit - 1U)
            ) &
            1U;

        putchar(
            value != 0U
                ? '1'
                : '0'
        );
    }

    putchar('>');
}

static size_t parse_size(
    const char *text,
    const char *label
) {
    char *end = NULL;
    unsigned long long value;

    errno = 0;

    value = strtoull(
        text,
        &end,
        10
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0' ||
        value > (unsigned long long)SIZE_MAX
    ) {
        fprintf(
            stderr,
            "ERROR: invalid %s: %s\n",
            label,
            text
        );

        exit(EXIT_FAILURE);
    }

    return (size_t)value;
}

static uint64_t parse_seed(
    const char *text
) {
    char *end = NULL;
    unsigned long long value;

    errno = 0;

    value = strtoull(
        text,
        &end,
        0
    );

    if (
        errno != 0 ||
        end == text ||
        *end != '\0'
    ) {
        fprintf(
            stderr,
            "ERROR: invalid seed: %s\n",
            text
        );

        exit(EXIT_FAILURE);
    }

    return (uint64_t)value;
}

static int fail_status(
    const char *operation,
    QCPUStatus status
) {
    fprintf(
        stderr,
        "ERROR: %s failed: %s\n",
        operation,
        qcpu_status_string(status)
    );

    return EXIT_FAILURE;
}

int main(
    int argc,
    char **argv
) {
    size_t qubit_count = 3U;
    size_t shots = 20U;
    uint64_t seed = UINT64_C(424242);

    QCPUKernel kernel;
    QCPUShotResult result;
    QCPUStatus status;

    if (argc > 4) {
        fprintf(
            stderr,
            "USAGE: %s [qubits] [shots] [seed]\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    if (argc >= 2) {
        qubit_count = parse_size(
            argv[1],
            "qubit count"
        );
    }

    if (argc >= 3) {
        shots = parse_size(
            argv[2],
            "shot count"
        );
    }

    if (argc >= 4) {
        seed = parse_seed(argv[3]);
    }

    status = qcpu_kernel_init(
        &kernel,
        qubit_count,
        seed
    );

    if (status != QCPU_OK) {
        return fail_status(
            "kernel initialization",
            status
        );
    }

    status = qcpu_kernel_apply_ghz(
        &kernel
    );

    if (status != QCPU_OK) {
        qcpu_kernel_free(&kernel);

        return fail_status(
            "GHZ preparation",
            status
        );
    }

    status = qcpu_kernel_validate(
        &kernel,
        1e-12
    );

    if (status != QCPU_OK) {
        qcpu_kernel_free(&kernel);

        return fail_status(
            "state validation",
            status
        );
    }

    printf(
        "[INPUT] compact_ghz_ops=%zu qubits=%zu\n",
        qubit_count,
        qubit_count
    );

    printf(
        "[EXPAND] basis_states=%zu norm=%.12f\n",
        kernel.state_count,
        qcpu_kernel_norm(&kernel)
    );

    printf("[STATE]");

    for (
        size_t state = 0U;
        state < kernel.state_count;
        ++state
    ) {
        double magnitude =
            cabs(kernel.amplitudes[state]);

        if (magnitude <= 1e-12) {
            continue;
        }

        putchar(' ');
        print_basis(
            state,
            qubit_count
        );

        printf(
            "=%.6f",
            magnitude
        );
    }

    putchar('\n');

    status = qcpu_kernel_run_shots(
        &kernel,
        shots,
        &result
    );

    if (status != QCPU_OK) {
        qcpu_kernel_free(&kernel);

        return fail_status(
            "shot execution",
            status
        );
    }

    size_t final_state =
        kernel.state_count - 1U;

    uint64_t bad_count = 0U;

    for (
        size_t state = 0U;
        state < result.state_count;
        ++state
    ) {
        if (
            state != 0U &&
            state != final_state
        ) {
            bad_count +=
                result.counts[state];
        }
    }

    printf(
        "[SHOTS] total=%zu ",
        shots
    );

    print_basis(
        0U,
        qubit_count
    );

    printf(
        "=%llu ",
        (unsigned long long)
            result.counts[0]
    );

    print_basis(
        final_state,
        qubit_count
    );

    printf(
        "=%llu bad=%llu\n",
        (unsigned long long)
            result.counts[final_state],
        (unsigned long long)
            bad_count
    );

    qcpu_shot_result_free(&result);

    size_t measured_state = 0U;

    status = qcpu_kernel_measure(
        &kernel,
        &measured_state
    );

    if (status != QCPU_OK) {
        qcpu_kernel_free(&kernel);

        return fail_status(
            "measurement",
            status
        );
    }

    printf(
        "[COLLAPSE] measured="
    );

    print_basis(
        measured_state,
        qubit_count
    );

    printf(
        " output_bits=%zu\n",
        qubit_count
    );

    if (
        bad_count != 0U ||
        (
            measured_state != 0U &&
            measured_state != final_state
        )
    ) {
        fprintf(
            stderr,
            "FAIL: invalid GHZ result detected\n"
        );

        qcpu_kernel_free(&kernel);

        return EXIT_FAILURE;
    }

    printf(
        "PASS: "
        "QCPU_EXPANSION_COLLAPSE_KERNEL_READY\n"
    );

    qcpu_kernel_free(&kernel);

    return EXIT_SUCCESS;
}
