# QBIT NOVA C — v1.1 QN Pipeline

Status: historical milestone branch.

## Branch scope

Connects `.qn` input to the two-qbit state-vector test pipeline.

- Branch: `v1.1-qn-state-vector-pipeline`
- Inspected tip: `11deda011c67`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v1.1-qn-state-vector-pipeline
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
