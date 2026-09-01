# QBIT NOVA C — v0.5 Bytecode VM

Status: historical milestone branch.

## Branch scope

Adds bytecode execution for bounded repeat loops.

- Branch: `v0.5-bytecode-vm`
- Inspected tip: `72fc501b5d85`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v0.5-bytecode-vm
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
