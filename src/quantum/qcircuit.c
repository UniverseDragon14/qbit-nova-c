/*
  QBIT NOVA C - Circuit VM

  Software virtual QCPU on classical hardware.
  Not physical quantum hardware.

  Gates:
    h x y z s t cx swap

  Macro:
    ghz = h q0 + cx chain

  Qubit 0 is the most-significant bit.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>

#define QPI 3.14159265358979323846
#define MAX_QUBITS 16
#define MAX_OPS 4096

typedef struct {
    char op[8];
    int a;
    int b;
} Op;

static int NQ = 0;
static int HAS_MEASURE = 0;
static Op OPS[MAX_OPS];
static int NOPS = 0;

static void fail(const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

static void add_op(const char *op, int a, int b) {
    if (NOPS >= MAX_OPS) fail("too many circuit operations");
    snprintf(OPS[NOPS].op, sizeof(OPS[NOPS].op), "%s", op);
    OPS[NOPS].a = a;
    OPS[NOPS].b = b;
    NOPS++;
}

static void require_qubit(int q) {
    if (q < 0 || q >= NQ) fail("qubit index out of range");
}

static void add_ghz(void) {
    if (NQ < 2) fail("ghz requires qubits N before ghz, with N >= 2");
    add_op("h", 0, 0);
    for (int q = 0; q < NQ - 1; q++) {
        add_op("cx", q, q + 1);
    }
}

static void print_bits(long index, int n) {
    printf("|");
    for (int j = 0; j < n; j++) {
        printf("%d", (int)((index >> (n - 1 - j)) & 1));
    }
    printf(">");
}

static void g_single(double complex *amp, int n, int q,
                     double complex m00,
                     double complex m01,
                     double complex m10,
                     double complex m11) {
    int p = n - 1 - q;
    long dim = 1L << n;

    for (long i = 0; i < dim; i++) {
        if ((i >> p) & 1) continue;

        long j = i | (1L << p);
        double complex x = amp[i];
        double complex y = amp[j];

        amp[i] = m00 * x + m01 * y;
        amp[j] = m10 * x + m11 * y;
    }
}

static void g_h(double complex *amp, int n, int q) {
    double s = 1.0 / sqrt(2.0);
    g_single(amp, n, q, s, s, s, -s);
}

static void g_x(double complex *amp, int n, int q) {
    g_single(amp, n, q, 0, 1, 1, 0);
}

static void g_y(double complex *amp, int n, int q) {
    g_single(amp, n, q, 0, -I, I, 0);
}

static void g_z(double complex *amp, int n, int q) {
    g_single(amp, n, q, 1, 0, 0, -1);
}

static void g_s(double complex *amp, int n, int q) {
    g_single(amp, n, q, 1, 0, 0, I);
}

static void g_t(double complex *amp, int n, int q) {
    g_single(amp, n, q, 1, 0, 0, cexp(I * QPI / 4.0));
}

static void g_cx(double complex *amp, int n, int c, int t) {
    if (c == t) fail("cx control and target cannot be same");

    int pc = n - 1 - c;
    int pt = n - 1 - t;
    long dim = 1L << n;

    for (long i = 0; i < dim; i++) {
        if (!((i >> pc) & 1)) continue;
        if ((i >> pt) & 1) continue;

        long j = i | (1L << pt);
        double complex tmp = amp[i];
        amp[i] = amp[j];
        amp[j] = tmp;
    }
}

static void g_swap(double complex *amp, int n, int a, int b) {
    if (a == b) return;
    g_cx(amp, n, a, b);
    g_cx(amp, n, b, a);
    g_cx(amp, n, a, b);
}

static long measure_all(double complex *amp, long dim) {
    double r = (double)rand() / RAND_MAX;
    double acc = 0.0;
    long picked = dim - 1;

    for (long i = 0; i < dim; i++) {
        double p = creal(amp[i]) * creal(amp[i]) + cimag(amp[i]) * cimag(amp[i]);
        acc += p;
        if (r <= acc) {
            picked = i;
            break;
        }
    }

    return picked;
}

static void load_circuit(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        exit(1);
    }

    char line[256];

    while (fgets(line, sizeof(line), f)) {
        char kw[64] = {0};
        int q1 = 0;
        int q2 = 0;

        if (line[0] == '#' || line[0] == '\n') continue;

        int got = sscanf(line, "%63s %d %d", kw, &q1, &q2);
        if (got < 1) continue;

        if (!strcmp(kw, "qubits")) {
            NQ = q1;
            if (NQ < 1 || NQ > MAX_QUBITS) fail("qubits must be 1..16");
        } else if (!strcmp(kw, "ghz")) {
            add_ghz();
        } else if (!strcmp(kw, "h") || !strcmp(kw, "x") || !strcmp(kw, "y") ||
                   !strcmp(kw, "z") || !strcmp(kw, "s") || !strcmp(kw, "t")) {
            require_qubit(q1);
            add_op(kw, q1, 0);
        } else if (!strcmp(kw, "cx") || !strcmp(kw, "swap")) {
            require_qubit(q1);
            require_qubit(q2);
            add_op(kw, q1, q2);
        } else if (!strcmp(kw, "measure")) {
            HAS_MEASURE = 1;
        } else {
            fprintf(stderr, "ERROR: unknown circuit op: %s\n", kw);
            exit(1);
        }
    }

    fclose(f);
}

static void emit_qasm(void) {
    printf("OPENQASM 3.0;\n");
    printf("include \"stdgates.inc\";\n\n");
    printf("qubit[%d] q;\n", NQ);
    printf("bit[%d] c;\n\n", NQ);

    for (int i = 0; i < NOPS; i++) {
        Op o = OPS[i];

        if (!strcmp(o.op, "h")) printf("h q[%d];\n", o.a);
        else if (!strcmp(o.op, "x")) printf("x q[%d];\n", o.a);
        else if (!strcmp(o.op, "y")) printf("y q[%d];\n", o.a);
        else if (!strcmp(o.op, "z")) printf("z q[%d];\n", o.a);
        else if (!strcmp(o.op, "s")) printf("s q[%d];\n", o.a);
        else if (!strcmp(o.op, "t")) printf("t q[%d];\n", o.a);
        else if (!strcmp(o.op, "cx")) printf("cx q[%d], q[%d];\n", o.a, o.b);
        else if (!strcmp(o.op, "swap")) printf("swap q[%d], q[%d];\n", o.a, o.b);
    }

    if (HAS_MEASURE) {
        printf("\n");
        for (int q = 0; q < NQ; q++) {
            printf("c[%d] = measure q[%d];\n", q, q);
        }
    }
}

static void simulate(void) {
    long dim = 1L << NQ;
    double complex *amp = calloc((size_t)dim, sizeof(double complex));

    if (!amp) fail("state allocation failed");

    amp[0] = 1.0;

    printf("=== QBIT NOVA CIRCUIT VM ===\n");
    printf("[CIRCUIT] boundary: software virtual QCPU, not physical quantum hardware\n");
    printf("[CIRCUIT] qubits: %d amplitudes: %ld gates: %d\n", NQ, dim, NOPS);

    for (int i = 0; i < NOPS; i++) {
        Op o = OPS[i];

        if (!strcmp(o.op, "h")) g_h(amp, NQ, o.a);
        else if (!strcmp(o.op, "x")) g_x(amp, NQ, o.a);
        else if (!strcmp(o.op, "y")) g_y(amp, NQ, o.a);
        else if (!strcmp(o.op, "z")) g_z(amp, NQ, o.a);
        else if (!strcmp(o.op, "s")) g_s(amp, NQ, o.a);
        else if (!strcmp(o.op, "t")) g_t(amp, NQ, o.a);
        else if (!strcmp(o.op, "cx")) g_cx(amp, NQ, o.a, o.b);
        else if (!strcmp(o.op, "swap")) g_swap(amp, NQ, o.a, o.b);
    }

    printf("[CIRCUIT] state: ");

    for (long i = 0; i < dim; i++) {
        double re = creal(amp[i]);
        double im = cimag(amp[i]);
        double p = re * re + im * im;

        if (p > 1e-9) {
            print_bits(i, NQ);

            if (fabs(im) < 1e-9) {
                printf("=%.3f ", re);
            } else if (fabs(re) < 1e-9) {
                printf("=%.3fi ", im);
            } else {
                printf("=(%.3f%+.3fi) ", re, im);
            }
        }
    }

    printf("\n");

    if (HAS_MEASURE) {
        long m = measure_all(amp, dim);
        printf("[CIRCUIT] MEASURE ");
        print_bits(m, NQ);
        printf("\n");
    }

    free(amp);
}

int main(int argc, char **argv) {
    srand((unsigned)(time(NULL) ^ ((unsigned)getpid() << 16) ^ (unsigned)clock()));

    int qasm = 0;
    int argi = 1;

    if (argc > 1 && !strcmp(argv[1], "--qasm")) {
        qasm = 1;
        argi = 2;
    }

    if (argi >= argc) {
        fprintf(stderr, "usage: qnova-circuit [--qasm] <file|N>\n");
        return 1;
    }

    char *arg = argv[argi];
    int is_num = 1;

    for (char *p = arg; *p; p++) {
        if (*p < '0' || *p > '9') {
            is_num = 0;
            break;
        }
    }

    if (is_num) {
        NQ = atoi(arg);
        if (NQ < 2) NQ = 2;
        if (NQ > MAX_QUBITS) NQ = MAX_QUBITS;
        add_ghz();
        HAS_MEASURE = 1;
    } else {
        load_circuit(arg);
        if (NQ < 1) fail("circuit missing qubits N");
    }

    if (qasm) emit_qasm();
    else simulate();

    return 0;
}
