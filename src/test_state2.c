#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "quantum/state2.h"

int main(void) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    QState2 s;

    printf("=== QBIT NOVA v1.0 STATE VECTOR TEST ===\n");

    qstate2_init_00(&s);
    printf("INIT      ");
    qstate2_print(&s);

    qstate2_h_first(&s);
    printf("H(q)      ");
    qstate2_print(&s);

    qstate2_cnot_first_to_second(&s);
    printf("CNOT q,p  ");
    qstate2_print(&s);

    printf("MEASURE   ");
    qstate2_measure(&s);
    qstate2_print(&s);

    return 0;
}
