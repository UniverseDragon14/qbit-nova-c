# QBIT NOVA C — v1.3 QMSG Packet Layer

Status: historical milestone branch.

## Branch scope

Adds the QMSG virtual packet layer and its language/compiler/VM path.

- Branch: `v1.3-qcpu-qmsg`
- Inspected tip: `3296c796743e`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v1.3-qcpu-qmsg
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
