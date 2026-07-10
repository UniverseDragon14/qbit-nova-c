/*
  QBIT NOVA C - N-qubit GHZ state-vector core

  Software virtual QCPU on classical hardware.
  Not physical quantum hardware.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

static void print_bits(long index, int n) {
    printf("|");
    for (int j = 0; j < n; j++) {
        int bit = (index >> (n - 1 - j)) & 1;
        printf("%d", bit);
    }
    printf(">");
}

static void h_gate(double *amp, int n, int q) {
    int p = n - 1 - q;
    long dim = 1L << n;
    double inv = 1.0 / sqrt(2.0);

    for (long i = 0; i < dim; i++) {
        if ((i >> p) & 1) continue;
        long j = i | (1L << p);
        double a = amp[i];
        double b = amp[j];
        amp[i] = (a + b) * inv;
        amp[j] = (a - b) * inv;
    }
}

static void cnot(double *amp, int n, int c, int t) {
    int pc = n - 1 - c;
    int pt = n - 1 - t;
    long dim = 1L << n;

    for (long i = 0; i < dim; i++) {
        if (!((i >> pc) & 1)) continue;
        if ((i >> pt) & 1) continue;

        long j = i | (1L << pt);
        double tmp = amp[i];
        amp[i] = amp[j];
        amp[j] = tmp;
    }
}

static long measure(double *amp, long dim) {
    double r = (double)rand() / RAND_MAX;
    double acc = 0.0;
    long picked = dim - 1;

    for (long i = 0; i < dim; i++) {
        acc += amp[i] * amp[i];
        if (r <= acc) {
            picked = i;
            break;
        }
    }

    for (long i = 0; i < dim; i++) amp[i] = 0.0;
    amp[picked] = 1.0;
    return picked;
}

int main(int argc, char **argv) {
    int n = 3;

    if (argc > 1) n = atoi(argv[1]);
    if (n < 2) n = 2;
    if (n > 16) n = 16;

    srand((unsigned)(time(NULL) ^ ((unsigned)getpid() << 16) ^ (unsigned)clock()));

    long dim = 1L << n;
    double *amp = calloc((size_t)dim, sizeof(double));

    if (!amp) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }

    amp[0] = 1.0;

    printf("=== QBIT NOVA STATEN VM ===\n");
    printf("[STATEN] boundary: software virtual QCPU, not physical quantum hardware\n");
    printf("[STATEN] qubits: %d amplitudes: %ld\n", n, dim);
    printf("[STATEN] building GHZ state: H q0 + CNOT chain\n");

    h_gate(amp, n, 0);

    for (int q = 0; q < n - 1; q++) {
        cnot(amp, n, q, q + 1);
    }

    printf("[STATEN] state: ");
    for (long i = 0; i < dim; i++) {
        if (fabs(amp[i]) > 1e-9) {
            print_bits(i, n);
            printf("=%.3f ", amp[i]);
        }
    }
    printf("\n");

    long m = measure(amp, dim);

    printf("[STATEN] MEASURE ");
    print_bits(m, n);
    printf("\n");

    free(amp);
    return 0;
}
