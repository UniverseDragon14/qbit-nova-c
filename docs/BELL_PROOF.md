# QBIT NOVA C - Bell State Correlation Proof

This proof checks the QBIT NOVA 2-qbit state-vector Bell circuit.

## Circuit

    qbit q = |0>
    qbit p = |0>

    hadamard q
    cnot q p

    measure q
    say q
    say p

## Expected result

After H(q) and CNOT(q, p), the state becomes:

    (|00> + |11>) / sqrt(2)

So measurement must produce only:

    |00>
    |11>

It must never produce:

    |01>
    |10>

## Meaning

This proves QBIT NOVA's Bell-state simulation preserves measurement correlation in the virtual 2-qbit state-vector engine.

## Safety statement

This is a software simulation proof on classical hardware.
It is not a claim of physical quantum hardware.
