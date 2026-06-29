#ifndef QBIT_NOVA_STATE2_H
#define QBIT_NOVA_STATE2_H

typedef struct {
    double amp[4];   /* |00>, |01>, |10>, |11> */
    int measured;
    int result;      /* 0=|00>, 1=|01>, 2=|10>, 3=|11> */
} QState2;

void qstate2_init_00(QState2 *s);
void qstate2_h_first(QState2 *s);
void qstate2_cnot_first_to_second(QState2 *s);
int qstate2_measure(QState2 *s);
void qstate2_print(const QState2 *s);

#endif
