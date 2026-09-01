# QBIT NOVA C — v0.9 CNOT Simulation

Status: historical milestone branch.

## Branch scope

Adds CNOT syntax and simulation across lexer, parser, compiler, and VM.

- Branch: `v0.9-cnot-state-vector`
- Inspected tip: `0635cbb81ae0`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v0.9-cnot-state-vector
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
