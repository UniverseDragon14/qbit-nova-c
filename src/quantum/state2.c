#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "state2.h"

void qstate2_init_00(QState2 *s) {
    for (int i = 0; i < 4; i++) s->amp[i] = 0.0;

    s->amp[0] = 1.0;   /* |00> */
    s->measured = 0;
    s->result = 0;
}

void qstate2_h_first(QState2 *s) {
    double a00 = s->amp[0];
    double a01 = s->amp[1];
    double a10 = s->amp[2];
    double a11 = s->amp[3];

    /*
      Hadamard on first qubit:
      |00> <-> |10>
      |01> <-> |11>
    */
    s->amp[0] = (a00 + a10) / sqrt(2.0);
    s->amp[2] = (a00 - a10) / sqrt(2.0);

    s->amp[1] = (a01 + a11) / sqrt(2.0);
    s->amp[3] = (a01 - a11) / sqrt(2.0);

    s->measured = 0;
}

void qstate2_cnot_first_to_second(QState2 *s) {
    /*
      CNOT control = first qubit, target = second qubit:
      |00> -> |00>
      |01> -> |01>
      |10> -> |11>
      |11> -> |10>
    */
    double temp = s->amp[2];
    s->amp[2] = s->amp[3];
    s->amp[3] = temp;

    s->measured = 0;
}

int qstate2_measure(QState2 *s) {
    double r = (double)rand() / RAND_MAX;
    double acc = 0.0;

    int picked = 3;

    for (int i = 0; i < 4; i++) {
        acc += s->amp[i] * s->amp[i];
        if (r <= acc) {
            picked = i;
            break;
        }
    }

    for (int i = 0; i < 4; i++) s->amp[i] = 0.0;

    s->amp[picked] = 1.0;
    s->measured = 1;
    s->result = picked;

    return picked;
}

void qstate2_print(const QState2 *s) {
    if (s->measured) {
        const char *names[] = {"|00>", "|01>", "|10>", "|11>"};
        printf("%s\n", names[s->result]);
        return;
    }

    printf("|00>=%.3f |01>=%.3f |10>=%.3f |11>=%.3f\n",
           s->amp[0], s->amp[1], s->amp[2], s->amp[3]);
}
