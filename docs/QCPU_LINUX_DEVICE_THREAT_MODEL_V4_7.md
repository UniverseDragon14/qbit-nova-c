# QBIT NOVA C v4.7 Linux QCPU Device Threat Model

## Scope

This threat model covers the proposed `/dev/qcpu0` frontend UAPI and its future
connection to the existing userspace `qcpud` backend.

Stage 1 is documentation and UAPI definition only. No privileged code is
installed or executed.

## Protected assets

1. Raspberry Pi 5 kernel and host availability
2. integrity of the verified `qcpu_kernel`
3. confidentiality of process and kernel memory
4. correctness of request and measurement results
5. stable v4.7 userspace ABI
6. honest software-only QCPU boundary
7. repository and release-chain integrity

## Trust zones

### Untrusted client

A process in the future `qcpu` group may submit malformed or adversarial input.

### Device frontend

The frontend validates fixed-size UAPI messages and enforces bounded execution.
It is not trusted to perform quantum computation.

### qcpud backend

`qcpud` is a userspace service. It may crash, stall, disconnect, or return a
malformed response. The frontend must fail closed and recover.

### qcpu_kernel

The existing userspace state-vector engine is the only approved quantum
computation implementation for v4.7.

## Threats and required controls

| Threat | Required control |
|---|---|
| malformed magic or version | reject before backend dispatch |
| unknown ioctl | return `-ENOTTY` |
| unknown protocol command | return `-EOPNOTSUPP` and BAD_COMMAND |
| oversized qubits or shots | enforce 20 qubits and 100 shots |
| nonzero reserved bytes | reject with `-EINVAL` |
| ABI padding leak | explicit reserved bytes, zero initialization |
| pointer or word-size confusion | fixed-width UAPI fields only |
| floating-point use in kernel | forbidden |
| state-vector exposure | no `mmap`, no GET_STATE in v4.7.0 |
| backend absent | deterministic `-ENODEV` |
| backend stall | absolute CLOCK_MONOTONIC deadline |
| request queue exhaustion | one in-flight request, `-EBUSY` or bounded wait |
| second client | exclusive client session, `-EBUSY` |
| client death | cancel or drain request, release session |
| backend death | wake client, discard partial response |
| stale response | transaction generation check in implementation stage |
| response spoofing | validate magic, version, command context, and limits |
| uninitialized kernel memory | zero every response before copy to userspace |
| integer overflow | checked size and timeout arithmetic |
| privilege escalation | device node mode `0660`, dedicated `qcpu` group |
| network exposure | no TCP or UDP listener introduced by v4.7 |
| truth-boundary drift | flags always state software virtual/classical/no physical QPU |
| repository regression | additive files only; existing v4.6 tests unchanged |

## Permission model

Proposed device-node policy:

```text
owner: root
group: qcpu
mode: 0660
```

This is a specification only. No group, device node, or udev rule is created in
Stage 1.

Normal clients require `qcpu` group membership. v4.7.0 exposes no state-vector
debug interface and therefore defines no `CAP_SYS_ADMIN` data path.

## Error mapping

When an ioctl itself cannot be processed, the syscall returns a negative errno.
When the transaction reaches the protocol layer, the response also carries a
QCPU protocol status.

| Condition | errno | protocol status |
|---|---|---|
| success | 0 | OK |
| bad magic | `EPROTO` | BAD_MAGIC |
| unsupported version | `EPROTONOSUPPORT` | BAD_VERSION |
| unknown ioctl | `ENOTTY` | not applicable |
| unsupported command | `EOPNOTSUPP` | BAD_COMMAND |
| qubit/shot/timeout range | `ERANGE` | RANGE |
| malformed reserved fields | `EINVAL` | RANGE |
| backend unavailable | `ENODEV` | BACKEND_ABSENT |
| active client exists | `EBUSY` | BUSY |
| backend deadline expired | `ETIMEDOUT` | TIMEOUT |
| request canceled | `ECANCELED` | CANCELED |
| userspace copy fault | `EFAULT` | not applicable |
| backend kernel-engine failure | `EIO` | KERNEL |
| backend transport failure | `EIO` | IO |

## Denied designs

The following are explicitly denied in v4.7 Stage 1:

- quantum math in kernel space
- kernel state-vector storage
- kernel floating-point
- variable-sized ioctl payloads
- raw userspace pointers in transported structures
- state-vector `mmap`
- writable sysfs controls
- Device Tree node for a pure software device
- noise or density-matrix implementation
- network listener
- automatic module loading
- root service installation
- direct Pico or GPIO access

## Residual risks

Even with bounded fixed-size frames, a future kernel implementation can contain
lifetime, locking, or copy-to-user bugs. Therefore:

1. userspace mock validation comes first
2. a custom module requires separate approval
3. module testing occurs without boot persistence
4. unload and backend-death recovery are mandatory tests
5. no production claim is made until fault tests pass

## Security verdict

The v4.7 design minimizes kernel responsibility to validation, permissions,
bounded queuing, and deterministic error mapping. Quantum computation and all
floating-point work remain in the verified userspace backend.
