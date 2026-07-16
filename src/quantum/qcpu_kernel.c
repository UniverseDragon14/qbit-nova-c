#include "qcpu_kernel.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define QCPU_DEFAULT_SEED \
    UINT64_C(0x9e3779b97f4a7c15)

static int qcpu_kernel_ready(
    const QCPUKernel *kernel
) {
    if (kernel == NULL ||
        kernel->amplitudes == NULL ||
        kernel->qubit_count == 0U) {
        return 0;
    }

    return kernel->state_count ==
           ((size_t)1U << kernel->qubit_count);
}

static size_t qcpu_qubit_mask(
    const QCPUKernel *kernel,
    size_t qubit
) {
    return (size_t)1U <<
           (
               kernel->qubit_count -
               1U -
               qubit
           );
}

static uint64_t qcpu_rng_next(
    QCPUKernel *kernel
) {
    uint64_t value = kernel->rng_state;

    if (value == 0U) {
        value = QCPU_DEFAULT_SEED;
    }

    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;

    kernel->rng_state = value;

    return value *
           UINT64_C(2685821657736338717);
}

static double qcpu_rng_unit(
    QCPUKernel *kernel
) {
    uint64_t value = qcpu_rng_next(kernel);

    return (double)(value >> 11) *
           (1.0 / 9007199254740992.0);
}

static QCPUStatus qcpu_kernel_sample(
    QCPUKernel *kernel,
    size_t *state
) {
    double threshold;
    double cumulative = 0.0;

    if (!qcpu_kernel_ready(kernel) ||
        state == NULL) {
        return QCPU_ERR_ARGUMENT;
    }

    threshold = qcpu_rng_unit(kernel);

    for (size_t index = 0U;
         index < kernel->state_count;
         ++index) {
        double real_part =
            creal(kernel->amplitudes[index]);

        double imag_part =
            cimag(kernel->amplitudes[index]);

        cumulative +=
            real_part * real_part +
            imag_part * imag_part;

        if (threshold < cumulative ||
            index + 1U == kernel->state_count) {
            *state = index;
            return QCPU_OK;
        }
    }

    return QCPU_ERR_STATE;
}

QCPUStatus qcpu_kernel_init(
    QCPUKernel *kernel,
    size_t qubit_count,
    uint64_t seed
) {
    if (kernel == NULL) {
        return QCPU_ERR_ARGUMENT;
    }

    if (qubit_count == 0U ||
        qubit_count > QCPU_KERNEL_MAX_QUBITS ||
        qubit_count >= sizeof(size_t) * 8U) {
        return QCPU_ERR_RANGE;
    }

    memset(kernel, 0, sizeof(*kernel));

    kernel->qubit_count = qubit_count;
    kernel->state_count =
        (size_t)1U << qubit_count;
    kernel->rng_state =
        seed == 0U
            ? QCPU_DEFAULT_SEED
            : seed;

    kernel->amplitudes = calloc(
        kernel->state_count,
        sizeof(*kernel->amplitudes)
    );

    if (kernel->amplitudes == NULL) {
        memset(kernel, 0, sizeof(*kernel));
        return QCPU_ERR_ALLOC;
    }

    return qcpu_kernel_reset(kernel);
}

void qcpu_kernel_free(
    QCPUKernel *kernel
) {
    if (kernel == NULL) {
        return;
    }

    free(kernel->amplitudes);
    memset(kernel, 0, sizeof(*kernel));
}

QCPUStatus qcpu_kernel_reset(
    QCPUKernel *kernel
) {
    if (!qcpu_kernel_ready(kernel)) {
        return QCPU_ERR_ARGUMENT;
    }

    memset(
        kernel->amplitudes,
        0,
        kernel->state_count *
            sizeof(*kernel->amplitudes)
    );

    kernel->amplitudes[0] =
        1.0 + 0.0 * I;

    kernel->measured = 0;
    kernel->measured_state = 0U;

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_apply_h(
    QCPUKernel *kernel,
    size_t qubit
) {
    size_t mask;
    double scale;

    if (!qcpu_kernel_ready(kernel)) {
        return QCPU_ERR_ARGUMENT;
    }

    if (qubit >= kernel->qubit_count) {
        return QCPU_ERR_RANGE;
    }

    mask = qcpu_qubit_mask(
        kernel,
        qubit
    );
    scale = 1.0 / sqrt(2.0);

    for (size_t index = 0U;
         index < kernel->state_count;
         ++index) {
        size_t paired;
        double complex first;
        double complex second;

        if ((index & mask) != 0U) {
            continue;
        }

        paired = index | mask;
        first = kernel->amplitudes[index];
        second = kernel->amplitudes[paired];

        kernel->amplitudes[index] =
            (first + second) * scale;

        kernel->amplitudes[paired] =
            (first - second) * scale;
    }

    kernel->measured = 0;

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_apply_x(
    QCPUKernel *kernel,
    size_t qubit
) {
    size_t mask;

    if (!qcpu_kernel_ready(kernel)) {
        return QCPU_ERR_ARGUMENT;
    }

    if (qubit >= kernel->qubit_count) {
        return QCPU_ERR_RANGE;
    }

    mask = qcpu_qubit_mask(
        kernel,
        qubit
    );

    for (size_t index = 0U;
         index < kernel->state_count;
         ++index) {
        size_t paired;
        double complex temporary;

        if ((index & mask) != 0U) {
            continue;
        }

        paired = index | mask;
        temporary =
            kernel->amplitudes[index];

        kernel->amplitudes[index] =
            kernel->amplitudes[paired];

        kernel->amplitudes[paired] =
            temporary;
    }

    kernel->measured = 0;

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_apply_cx(
    QCPUKernel *kernel,
    size_t control,
    size_t target
) {
    size_t control_mask;
    size_t target_mask;

    if (!qcpu_kernel_ready(kernel)) {
        return QCPU_ERR_ARGUMENT;
    }

    if (control >= kernel->qubit_count ||
        target >= kernel->qubit_count ||
        control == target) {
        return QCPU_ERR_RANGE;
    }

    control_mask = qcpu_qubit_mask(
        kernel,
        control
    );

    target_mask = qcpu_qubit_mask(
        kernel,
        target
    );

    for (size_t index = 0U;
         index < kernel->state_count;
         ++index) {
        size_t paired;
        double complex temporary;

        if ((index & control_mask) == 0U ||
            (index & target_mask) != 0U) {
            continue;
        }

        paired = index | target_mask;
        temporary =
            kernel->amplitudes[index];

        kernel->amplitudes[index] =
            kernel->amplitudes[paired];

        kernel->amplitudes[paired] =
            temporary;
    }

    kernel->measured = 0;

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_apply_ghz(
    QCPUKernel *kernel
) {
    QCPUStatus status;

    if (!qcpu_kernel_ready(kernel)) {
        return QCPU_ERR_ARGUMENT;
    }

    status = qcpu_kernel_reset(kernel);

    if (status != QCPU_OK) {
        return status;
    }

    status = qcpu_kernel_apply_h(
        kernel,
        0U
    );

    if (status != QCPU_OK) {
        return status;
    }

    for (size_t qubit = 1U;
         qubit < kernel->qubit_count;
         ++qubit) {
        status = qcpu_kernel_apply_cx(
            kernel,
            qubit - 1U,
            qubit
        );

        if (status != QCPU_OK) {
            return status;
        }
    }

    return QCPU_OK;
}

double qcpu_kernel_norm(
    const QCPUKernel *kernel
) {
    double norm = 0.0;

    if (!qcpu_kernel_ready(kernel)) {
        return 0.0;
    }

    for (size_t index = 0U;
         index < kernel->state_count;
         ++index) {
        double real_part =
            creal(kernel->amplitudes[index]);

        double imag_part =
            cimag(kernel->amplitudes[index]);

        norm +=
            real_part * real_part +
            imag_part * imag_part;
    }

    return norm;
}

QCPUStatus qcpu_kernel_validate(
    const QCPUKernel *kernel,
    double tolerance
) {
    double norm;

    if (!qcpu_kernel_ready(kernel) ||
        tolerance <= 0.0) {
        return QCPU_ERR_ARGUMENT;
    }

    norm = qcpu_kernel_norm(kernel);

    if (!isfinite(norm) ||
        fabs(norm - 1.0) > tolerance) {
        return QCPU_ERR_STATE;
    }

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_measure(
    QCPUKernel *kernel,
    size_t *measured_state
) {
    size_t state = 0U;
    QCPUStatus status;

    if (!qcpu_kernel_ready(kernel) ||
        measured_state == NULL) {
        return QCPU_ERR_ARGUMENT;
    }

    status = qcpu_kernel_sample(
        kernel,
        &state
    );

    if (status != QCPU_OK) {
        return status;
    }

    memset(
        kernel->amplitudes,
        0,
        kernel->state_count *
            sizeof(*kernel->amplitudes)
    );

    kernel->amplitudes[state] =
        1.0 + 0.0 * I;

    kernel->measured = 1;
    kernel->measured_state = state;
    *measured_state = state;

    return QCPU_OK;
}

QCPUStatus qcpu_kernel_run_shots(
    QCPUKernel *kernel,
    size_t shots,
    QCPUShotResult *result
) {
    if (!qcpu_kernel_ready(kernel) ||
        result == NULL ||
        shots == 0U) {
        return QCPU_ERR_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));

    result->counts = calloc(
        kernel->state_count,
        sizeof(*result->counts)
    );

    if (result->counts == NULL) {
        return QCPU_ERR_ALLOC;
    }

    result->shots = shots;
    result->state_count =
        kernel->state_count;

    for (size_t shot = 0U;
         shot < shots;
         ++shot) {
        size_t state = 0U;

        QCPUStatus status =
            qcpu_kernel_sample(
                kernel,
                &state
            );

        if (status != QCPU_OK) {
            qcpu_shot_result_free(result);
            return status;
        }

        result->counts[state] += 1U;
    }

    return QCPU_OK;
}

void qcpu_shot_result_free(
    QCPUShotResult *result
) {
    if (result == NULL) {
        return;
    }

    free(result->counts);
    memset(result, 0, sizeof(*result));
}

const char *qcpu_status_string(
    QCPUStatus status
) {
    switch (status) {
        case QCPU_OK:
            return "QCPU_OK";

        case QCPU_ERR_ARGUMENT:
            return "QCPU_ERR_ARGUMENT";

        case QCPU_ERR_RANGE:
            return "QCPU_ERR_RANGE";

        case QCPU_ERR_ALLOC:
            return "QCPU_ERR_ALLOC";

        case QCPU_ERR_STATE:
            return "QCPU_ERR_STATE";

        default:
            return "QCPU_ERR_UNKNOWN";
    }
}
