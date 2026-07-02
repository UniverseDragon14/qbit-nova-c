# QBIT NOVA C Quick Demo

Run the full demo:

    ./scripts/demo.sh

Run all proof tests:

    ./scripts/test_all.sh

Run Bell proof only:

    ./scripts/proof_bell.sh 20

Run QMSG register example:

    ./qnova examples/qmsg_register.qn

Run OpenQASM export:

    ./qnova-qasm examples/bell_qasm.qn

Expected major proofs:

- QMSG decodes HI
- QMSG shows virtual qbit register
- OpenQASM export prints valid OpenQASM 3.0 style circuit
- Bell proof shows only |00> and |11>
- bad count remains 0
