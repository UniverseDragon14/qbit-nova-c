OPENQASM 3.0;
include "stdgates.inc";

qubit[2] q;
bit[2] c;

// QBIT NOVA name map
// q -> q[0]
// p -> q[1]

h q[0];
cx q[0], q[1];
c[0] = measure q[0];
c[1] = measure q[1];
