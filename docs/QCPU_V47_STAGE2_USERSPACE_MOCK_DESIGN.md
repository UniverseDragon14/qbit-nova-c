# QBIT NOVA C v4.7 Stage 2 Userspace Mock Design

## Status

Stage 2 design specification only.

This document defines a pure-userspace mock of the Stage 1 `/dev/qcpu0` UAPI.
It does not implement or emulate a Linux device node, intercept syscalls, load a
kernel module, or modify the existing v4.6 runtime.

## Objective

Prove the Stage 1 ABI semantics before privileged kernel work.

The mock must demonstrate that the fixed-width UAPI can support:

- capability discovery
- device status transitions
- one exclusive client session
- bounded synchronous STATUS and RUN_GHZ exchanges
- deterministic protocol and errno mapping
- backend absence, timeout, cancellation, and crash recovery
- Q32.32 norm conversion in userspace
- the existing verified `qcpud` / `qcpu_kernel` computation path

## Truth boundary

- The host is classical.
- The QCPU is software virtual.
- No physical QPU is present.
- No quantum state or floating-point computation moves into kernel space.
- Stage 2 does not create `/dev/qcpu0`.
- Stage 2 does not claim that a userspace mock is a Linux driver.

## Architecture

```text
test program / future application
               |
               | libqcpu public API
               v
qcpu_mock_frontend
  - open/close session contract
  - ioctl-number dispatch
  - fixed-size structure validation
  - limits and timeout validation
  - status/error mapping
  - counters and state transitions
               |
               | qcpu_backend_ops callbacks
               v
+----------------------------------------------+
| deterministic mock backend                  |
|  - success                                  |
|  - absent                                   |
|  - delayed                                  |
|  - malformed response                       |
|  - crash/disconnect                         |
+----------------------------------------------+
               or
+----------------------------------------------+
| v4.6 qcpud adapter                          |
|  - existing explicit little-endian frames   |
|  - existing STATUS and RUN_GHZ commands     |
|  - existing verified qcpu_kernel            |
+----------------------------------------------+
```

The callback boundary is a Stage 2 test seam, not the final kernel-to-userspace
relay design. Stage 2 must not select Netlink, CUSE, a second character device,
or another privileged transport.

## Mock API shape

The implementation stage may expose an internal interface equivalent to:

```c
struct qcpu_mock;

int qcpu_mock_create(
    struct qcpu_mock **out,
    const struct qcpu_backend_ops *ops,
    void *backend_context
);

int qcpu_mock_open(struct qcpu_mock *mock);
int qcpu_mock_ioctl(
    struct qcpu_mock *mock,
    unsigned long request,
    void *argument
);
int qcpu_mock_close(struct qcpu_mock *mock);
int qcpu_mock_destroy(struct qcpu_mock *mock);
```

This is not a transported ABI and is not added to the Stage 1 UAPI header.

The final implementation may refine names, but must preserve the documented
semantics and remain private to Stage 2 tests until reviewed.

## Backend callback contract

A backend adapter receives only the canonical fixed-width request and returns a
canonical fixed-width response.

Conceptual contract:

```c
struct qcpu_backend_ops {
    int (*exchange)(
        void *context,
        const struct qcpu_uapi_request_v1 *request,
        uint64_t absolute_deadline_ns,
        struct qcpu_uapi_response_v1 *response
    );

    int (*cancel)(void *context);
};
```

Requirements:

- one request at a time
- no unbounded allocation
- no raw state-vector access
- no borrowed pointers retained after return
- no floating-point field crosses the frontend boundary
- response is zero-initialized before backend dispatch
- backend return values are translated through the documented errno map

## ioctl semantics emulated

Stage 2 models exactly the Stage 1 operations:

| ioctl | Stage 2 behavior |
|---|---|
| `QCPU_IOC_GET_CAPS_V1` | returns immutable ABI, protocol, limit, truth-flag, backend, and version values |
| `QCPU_IOC_GET_STATUS_V1` | returns frontend state, active-client count, last error/status, and counters |
| `QCPU_IOC_EXCHANGE_V1` | validates and performs one synchronous STATUS or RUN_GHZ exchange |

Unknown ioctl numbers return `-ENOTTY`.

Stage 2 defines no `read`, `write`, `mmap`, polling, async queue, state-vector
export, gate batching, noise configuration, or arbitrary-circuit command.

## Session model

Stage 2 enforces one exclusive client session across all participating
processes, not merely within one address space.

- first open succeeds
- second open in the same process returns `-EBUSY`
- second open from another process returns `-EBUSY`
- close releases the session
- repeated open/close leaves zero active sessions
- one request may be in flight
- a second exchange while busy returns `-EBUSY`
- `qcpu_mock_destroy()` returns `-EBUSY` while a session is active or an
  exchange callback is in flight
- a failed destroy attempt frees no frontend or backend-owned memory
- destroy succeeds only after the session is closed and the frontend is
  quiescent

Cross-process exclusivity is mandatory. The mock uses a nonblocking advisory
lock on a mode `0600` lock file inside a unique mode `0700` temporary runtime
directory supplied by the test fixture. There is no process-local fallback.
The lock is released on close and is recoverable after normal process exit.
The implementation must not create a global lock, system group, device node,
or persistent runtime path.

## State machine

```text
OFFLINE
  | backend attached
  v
IDLE
  | valid exchange begins
  v
BUSY
  | success, timeout, cancellation, or recoverable backend error
  v
IDLE

BUSY -- backend disconnect ------------------------------> OFFLINE
IDLE -- backend detached --------------------------------> OFFLINE
IDLE/BUSY -- internal invariant or teardown failure -----> ERROR
ERROR -- destroy and recreate frontend ------------------> IDLE or OFFLINE
```

Timeout, cancellation, malformed backend response, and recoverable engine or
transport failure record the error, increment the failed counter after
dispatch, release BUSY, and return to IDLE when the backend remains attached.
A backend disconnect returns the frontend to OFFLINE. ERROR is reserved for an
internal invariant or teardown failure and is not a normal request result.

## Validation order

`QCPU_IOC_EXCHANGE_V1` validation order:

1. session is open
2. argument exists
3. request magic
4. protocol version
5. supported command
6. qubit and shot range
7. timeout conversion and maximum check
8. frontend is not busy
9. backend is available
10. dispatch
11. response validation in the fixed order below
12. update counters and state

Response validation order and outcomes are deterministic:

| Response check | Failure result |
|---|---|
| response magic | `-EIO`, status IO, failed counter +1, IDLE |
| response version | `-EIO`, status IO, failed counter +1, IDLE |
| known non-OK response status | mapped errno/status, failed counter +1, IDLE or OFFLINE for BACKEND_ABSENT |
| unknown response status | `-EIO`, status IO, failed counter +1, IDLE |
| required truth flags present | `-EIO`, status IO, failed counter +1, IDLE |
| response qubits equal request | `-EIO`, status IO, failed counter +1, IDLE |
| response shots equal request | `-EIO`, status IO, failed counter +1, IDLE |
| `basis_states == 1ULL << qubits` for RUN_GHZ | `-EIO`, status KERNEL, failed counter +1, IDLE |
| measured state is all-zero or all-one | `-EIO`, status KERNEL, failed counter +1, IDLE |
| `invalid_results == 0` on success | `-EIO`, status KERNEL, failed counter +1, IDLE |
| `norm_q32_32 <= 1ULL << 32` | `-EIO`, status KERNEL, failed counter +1, IDLE |

The partial backend response is discarded on every failure. Pre-dispatch
validation rejects increment neither execution counter. A backend disconnect
uses `-ENODEV`, status BACKEND_ABSENT, increments the failed counter, and moves
the frontend to OFFLINE.

## Command rules

### STATUS

- accepts `qubits = 0`
- accepts `shots = 0`
- returns truth flags and frontend/backend status
- performs no quantum computation

### RUN_GHZ

- `1 <= qubits <= 20`
- `1 <= shots <= 100`
- measured state must be either all-zero or all-one for the requested width
- `invalid_results` must be zero on success
- `basis_states` must equal `1ULL << qubits`
- truth flags must include software virtual, classical host, physical QPU
  absent, and q0-most-significant

## Q32.32 norm conversion

The existing userspace backend may internally produce a `double` norm.
Conversion occurs only in userspace.

Normative Stage 2 rules:

- `QCPU_MOCK_NORM_TOLERANCE` is exactly `0x1p-40` (`2^-40`)
- reject NaN and infinity
- reject values below `0.0`
- accept values through `1.0 + QCPU_MOCK_NORM_TOLERANCE`
- reject any value above that inclusive boundary
- clamp an accepted overshoot above `1.0` to exactly `1.0`
- scale the clamped value by `2^32`
- round to nearest with exact half-LSB ties rounded upward
- the implementation must not depend on the process floating-point rounding
  mode
- `0.0` becomes `0`
- exact half-LSB `0x1p-33` becomes `1`
- `1.0` becomes `1ULL << 32`
- `1.0 + 0x1p-40` becomes `1ULL << 32`

The kernel-facing UAPI continues to contain only `__u64 norm_q32_32`.

## Timeout and cancellation

Stage 2 uses absolute `CLOCK_MONOTONIC` deadlines.

```text
QCPU_MOCK_DEFAULT_TIMEOUT_NS = 1000000000
QCPU_MOCK_MAX_TIMEOUT_NS     = 5000000000
```

- `timeout_ns == 0` selects the 1-second default
- `1 <= timeout_ns <= QCPU_MOCK_MAX_TIMEOUT_NS` is accepted
- a value above the 5-second maximum returns `-ERANGE` and status RANGE before
  backend dispatch; it is never clamped
- deadline addition is checked for integer overflow before dispatch
- timeout returns `-ETIMEDOUT` and protocol status TIMEOUT
- timeout increments the failed counter, releases BUSY, and returns to IDLE
  while the backend remains attached
- close during an exchange requests cancellation
- cancellation returns `-ECANCELED` and protocol status CANCELED
- cancellation releases BUSY; close then releases the session
- a timeout or cancellation never requires an implicit reopen

## Error and status mapping

The mock follows the Stage 1 threat-model map:

| Condition | errno | protocol status |
|---|---|---|
| success | `0` | OK |
| bad magic | `EPROTO` | BAD_MAGIC |
| unsupported version | `EPROTONOSUPPORT` | BAD_VERSION |
| unknown ioctl | `ENOTTY` | not applicable |
| unsupported command | `EOPNOTSUPP` | BAD_COMMAND |
| range failure | `ERANGE` | RANGE |
| malformed input | `EINVAL` | RANGE |
| backend absent | `ENODEV` | BACKEND_ABSENT |
| session/request busy or non-quiescent destroy | `EBUSY` | BUSY |
| deadline expired | `ETIMEDOUT` | TIMEOUT |
| canceled | `ECANCELED` | CANCELED |
| backend engine failure | `EIO` | KERNEL |
| backend transport/malformed response | `EIO` | IO |

Pure userspace Stage 2 does not claim to prove kernel `copy_from_user` or
`copy_to_user` `EFAULT` behavior. That belongs to the optional Stage 3 module.

## Counters

`completed_requests` increments only after a valid response is accepted.

`failed_requests` increments for a dispatched request that ends in timeout,
cancellation, backend failure, or malformed response.

Input rejected before backend dispatch does not increment either execution
counter. Separate test diagnostics may count validation rejects, but that count
is not part of the Stage 1 UAPI.

## Deterministic backend modes

The mock backend must support deterministic, selectable modes:

- success STATUS
- success RUN_GHZ zero state
- success RUN_GHZ one state
- backend absent
- delay beyond deadline
- cancel acknowledgment
- malformed response magic
- wrong response version
- invalid truth flags
- invalid measured state
- nonzero invalid-results count
- engine failure
- transport failure
- simulated backend disconnect

No random fault injection is required for the first Stage 2 proof.

## v4.6 integration adapter

After deterministic mock tests pass, a separate additive integration case may
connect the Stage 2 frontend to the existing v4.6 `qcpud` Unix-socket endpoint.

The adapter must:

- reuse the existing 24-byte request and 56-byte response frame semantics
- preserve explicit little-endian wire encoding
- convert the userspace norm to Q32.32
- add no TCP or UDP listener
- start `qcpud` only in an isolated temporary runtime directory
- stop it gracefully
- prove socket and process cleanup
- leave existing lifecycle scripts unchanged

## Files allowed in the future implementation stage

This design approval does not create these files, but the implementation stage
may propose an additive set such as:

```text
src/device/qcpu_mock_frontend.c
src/device/qcpu_mock_frontend.h
src/device/qcpu_mock_backend.c
src/device/qcpu_mock_v46_adapter.c
tests/v47/qcpu_mock_test.c
scripts/proof_qcpu_v47_userspace_mock.sh
```

Exact names and source scope require a separate owner approval.

## Explicitly denied in Stage 2

- kernel module source
- kernel headers installation
- `insmod`, `modprobe`, or `rmmod`
- `/dev/qcpu0` creation
- CUSE/FUSE device emulation
- LD_PRELOAD syscall interception
- writable sysfs controls
- Device Tree changes
- systemd or boot persistence
- root-owned group or udev changes
- state-vector `mmap`
- new simulator engine
- cloud submission
- noise model
- Pico or GPIO access
- replacement of `scripts/test_all.sh`
- modification of existing v4.6 tests

## Exit criteria

Stage 2 design is ready for implementation review when:

- all operations map to the merged Stage 1 UAPI
- the transport seam does not prejudge Stage 3
- deterministic fault modes are specified
- Q32.32 conversion is explicit
- limits remain 20 qubits, 100 shots, one in-flight request, one client
- existing v4.6 runtime and tests remain untouched
- no privileged action is required
