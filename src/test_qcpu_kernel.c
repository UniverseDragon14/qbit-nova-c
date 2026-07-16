#include "quantum/qcpu_kernel.h"

#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void require_status(
    QCPUStatus actual,
    QCPUStatus expected,
    const char *label
) {
    if (actual != expected) {
        fprintf(
            stderr,
            "FAIL: %s expected=%s actual=%s\n",
            label,
            qcpu_status_string(expected),
            qcpu_status_string(actual)
        );

        exit(EXIT_FAILURE);
    }
}

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
    QCPUKernel kernel;
    QCPUShotResult result;

    const uint64_t seed =
        UINT64_C(0x514249544e4f5641);

    const double expected =
        1.0 / sqrt(2.0);

    require_status(
        qcpu_kernel_init(
            &kernel,
            3U,
            seed
        ),
        QCPU_OK,
        "kernel_init"
    );

    require_true(
        kernel.qubit_count == 3U,
        "qubit_count_is_three"
    );

    require_true(
        kernel.state_count == 8U,
        "three_qubits_expand_to_eight_states"
    );

    printf(
        "PASS: QCPU_KERNEL_INIT_READY\n"
    );

    printf(
        "PASS: SMALL_INPUT_EXPANDED_TO_8_STATES\n"
    );

    require_status(
        qcpu_kernel_apply_ghz(&kernel),
        QCPU_OK,
        "apply_ghz"
    );

    require_status(
        qcpu_kernel_validate(
            &kernel,
            1e-12
        ),
        QCPU_OK,
        "state_normalization"
    );

    require_true(
        fabs(
            qcpu_kernel_norm(&kernel) -
            1.0
        ) < 1e-12,
        "norm_equals_one"
    );

    require_true(
        cabs(
            kernel.amplitudes[0] -
            expected
        ) < 1e-12,
        "amplitude_000"
    );

    require_true(
        cabs(
            kernel.amplitudes[7] -
            expected
        ) < 1e-12,
        "amplitude_111"
    );

    for (
        size_t state = 1U;
        state < 7U;
        ++state
    ) {
        require_true(
            cabs(
                kernel.amplitudes[state]
            ) < 1e-12,
            "middle_amplitudes_zero"
        );
    }

    printf(
        "PASS: GHZ_STATE_NORMALIZED\n"
    );

    printf(
        "STATE: |000>=%.6f |111>=%.6f\n",
        cabs(kernel.amplitudes[0]),
        cabs(kernel.amplitudes[7])
    );

    require_status(
        qcpu_kernel_run_shots(
            &kernel,
            100U,
            &result
        ),
        QCPU_OK,
        "run_100_shots"
    );

    uint64_t total = 0U;
    uint64_t bad = 0U;

    for (
        size_t state = 0U;
        state < result.state_count;
        ++state
    ) {
        total += result.counts[state];

        if (
            state != 0U &&
            state != 7U
        ) {
            bad += result.counts[state];
        }
    }

    require_true(
        total == 100U,
        "shot_total_is_100"
    );

    require_true(
        bad == 0U,
        "no_invalid_ghz_results"
    );

    require_true(
        result.counts[0] > 0U,
        "state_000_observed"
    );

    require_true(
        result.counts[7] > 0U,
        "state_111_observed"
    );

    printf(
        "PASS: GHZ_SHOTS_ONLY_000_OR_111\n"
    );

    printf(
        "SHOTS: total=100 |000>=%llu "
        "|111>=%llu bad=%llu\n",
        (unsigned long long)
            result.counts[0],
        (unsigned long long)
            result.counts[7],
        (unsigned long long)
            bad
    );

    qcpu_shot_result_free(&result);

    size_t measured_state = 0U;

    require_status(
        qcpu_kernel_measure(
            &kernel,
            &measured_state
        ),
        QCPU_OK,
        "single_measurement"
    );

    require_true(
        measured_state == 0U ||
        measured_state == 7U,
        "collapsed_state_is_valid"
    );

    require_status(
        qcpu_kernel_validate(
            &kernel,
            1e-12
        ),
        QCPU_OK,
        "collapsed_state_normalized"
    );

    require_true(
        cabs(
            kernel.amplitudes[
                measured_state
            ] - 1.0
        ) < 1e-12,
        "collapsed_amplitude_is_one"
    );

    printf(
        "PASS: LARGE_STATE_COLLAPSED_TO_VALID_BASIS\n"
    );

    printf(
        "COLLAPSE: measured_state=%zu\n",
        measured_state
    );

    require_status(
        qcpu_kernel_apply_cx(
            &kernel,
            0U,
            0U
        ),
        QCPU_ERR_RANGE,
        "same_control_target_rejected"
    );

    require_status(
        qcpu_kernel_apply_h(
            &kernel,
            3U
        ),
        QCPU_ERR_RANGE,
        "invalid_qubit_rejected"
    );

    printf(
        "PASS: STRUCTURED_RANGE_ERRORS_READY\n"
    );

    qcpu_kernel_free(&kernel);

    printf(
        "QCPU EXPANSION-COLLAPSE "
        "KERNEL TEST PASSED\n"
    );

    return EXIT_SUCCESS;
}
