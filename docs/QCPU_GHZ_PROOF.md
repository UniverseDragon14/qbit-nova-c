# QCPU GHZ State-N Proof

The GHZ proof extends QBIT NOVA C beyond the two-qubit Bell proof.

It validates N-qubit GHZ-style correlation using a C-based software state-vector simulator.

## Valid outcomes

For N qubits, the valid measured outcomes are:

- all zeros
- all ones

For 3 qubits:

- |000>
- |111>

## Invalid outcomes

Any mixed measured state is invalid for the clean GHZ proof.

Examples:

- |001>
- |010>
- |101>
- |110>

## Safety boundary

This is a software virtual QCPU proof.

It runs on classical hardware such as Raspberry Pi 5.

It does not make Raspberry Pi physical quantum hardware.

It does not prove physical entanglement. It validates software state-vector correlation.

## Run

./scripts/proof_ghz.sh 3 20

Optional:

./scripts/proof_ghz.sh 5 100

## Expected verdict

PASS: QCPU_GHZ_PROOF_READY
