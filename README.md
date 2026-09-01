# QBIT NOVA C — v0.8 Quantum Core

Status: historical milestone branch.

## Branch scope

Adds the early entangle-style quantum simulation path.

- Branch: `v0.8-quantum-core`
- Inspected tip: `83b5c7ce7692`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v0.8-quantum-core
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
