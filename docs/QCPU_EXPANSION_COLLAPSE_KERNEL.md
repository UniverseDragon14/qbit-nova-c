# QCPU Expansion-Collapse Kernel

QBIT NOVA C v4.5 introduces a reusable C statevector kernel.

## Execution model

A compact circuit describing N virtual qubits maps to a statevector
containing 2^N complex amplitudes.

For a three-qubit GHZ circuit:

    H q0
    CX q0 q1
    CX q1 q2

The software state is:

    |000> = 1/sqrt(2)
    |111> = 1/sqrt(2)

Measurement collapses the state to one classical basis result:

    000 or 111

## Kernel capabilities

- Reusable C API
- Deterministic seeded sampling
- H and X gates
- Controlled-X gate
- GHZ preparation
- State normalization validation
- Repeated shot distributions
- Single-measurement collapse
- Structured status codes
- Maximum configured statevector size of 20 qubits

## Scientific boundary

Expansion means mathematical state-space expansion from compact circuit
instructions. It does not create information from nothing.

Collapse means probabilistic measurement into one classical basis result.

This implementation is a software Virtual QCPU. It does not represent
physical quantum hardware.
