# QBIT NOVA C — v0.6 Safe Action

Status: historical milestone branch.

## Branch scope

Adds a simulated action allowlist across parser, compiler, and bytecode VM layers.

- Branch: `v0.6-safe-action`
- Inspected tip: `c95584ae1bad`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v0.6-safe-action
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
