#ifndef QCPU_KERNEL_H
#define QCPU_KERNEL_H

#include <complex.h>
#include <stddef.h>
#include <stdint.h>

#define QCPU_KERNEL_MAX_QUBITS 20U

typedef enum {
    QCPU_OK = 0,
    QCPU_ERR_ARGUMENT = 1,
    QCPU_ERR_RANGE = 2,
    QCPU_ERR_ALLOC = 3,
    QCPU_ERR_STATE = 4
} QCPUStatus;

typedef struct {
    size_t qubit_count;
    size_t state_count;
    double complex *amplitudes;
    uint64_t rng_state;
    int measured;
    size_t measured_state;
} QCPUKernel;

typedef struct {
    size_t shots;
    size_t state_count;
    uint64_t *counts;
} QCPUShotResult;

QCPUStatus qcpu_kernel_init(
    QCPUKernel *kernel,
    size_t qubit_count,
    uint64_t seed
);

void qcpu_kernel_free(
    QCPUKernel *kernel
);

QCPUStatus qcpu_kernel_reset(
    QCPUKernel *kernel
);

QCPUStatus qcpu_kernel_apply_h(
    QCPUKernel *kernel,
    size_t qubit
);

QCPUStatus qcpu_kernel_apply_x(
    QCPUKernel *kernel,
    size_t qubit
);

QCPUStatus qcpu_kernel_apply_cx(
    QCPUKernel *kernel,
    size_t control,
    size_t target
);

QCPUStatus qcpu_kernel_apply_ghz(
    QCPUKernel *kernel
);

double qcpu_kernel_norm(
    const QCPUKernel *kernel
);

QCPUStatus qcpu_kernel_validate(
    const QCPUKernel *kernel,
    double tolerance
);

QCPUStatus qcpu_kernel_measure(
    QCPUKernel *kernel,
    size_t *measured_state
);

QCPUStatus qcpu_kernel_run_shots(
    QCPUKernel *kernel,
    size_t shots,
    QCPUShotResult *result
);

void qcpu_shot_result_free(
    QCPUShotResult *result
);

const char *qcpu_status_string(
    QCPUStatus status
);

#endif
