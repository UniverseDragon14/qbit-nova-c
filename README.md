# QBIT NOVA C — v0.4 Bytecode

Status: historical milestone branch.

## Branch scope

Early AST interpreter and bytecode foundation; the tip fixes the interpreter switch default path.

- Branch: `v0.4-bytecode`
- Inspected tip: `faeb9cbdece866eac90a95bc93f8bc1755c9f3c1`
- Project owner: Universal Dragon Aslam
- Runtime boundary: software running on classical hardware
- Physical QPU: **false**

This branch is a preserved development checkpoint. It does not include every feature documented on `main`; later milestones build on or supersede this snapshot.

## Inspect this checkpoint

```bash
git switch v0.4-bytecode
git log -1 --oneline
find . -maxdepth 3 -type f | sort
```

Use the source, tests, and commit at this branch tip as the authority for supported behavior. Quantum-style gates and qbit registers here are software simulation, not physical quantum hardware.
