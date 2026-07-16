# QBIT NOVA C v4.7 Stage 2 Userspace Mock Test Plan

## Scope

This plan validates the merged Stage 1 UAPI in pure userspace.

No kernel module is built or loaded. No device node, udev rule, system group,
system service, Device Tree entry, Pico connection, or GPIO action is created.

## Test layers

```text
Layer A: compile-time ABI and layout proof
Layer B: deterministic frontend/mock-backend unit proof
Layer C: process/session and fault-lifecycle proof
Layer D: optional v4.6 qcpud integration proof
Layer E: unchanged full repository regression
```

Every layer is additive.

## Required invariants

```text
ABI version:                 1
protocol version:            1
request size:                24 bytes
response size:               56 bytes
caps size:                   80 bytes
status size:                 32 bytes
exchange size:               88 bytes
max qubits:                  20
max shots:                   100
max in-flight requests:      1
active client sessions:      0 or 1
network listeners added:     0
physical QPU present:        false
```

## Layer A: ABI proof

### A1. Header compilation

Compile the merged `include/uapi/qcpu_device_uapi.h` as a normal userspace
header with:

- C11
- `-Wall`
- `-Wextra`
- `-Werror`

Expected: PASS.

### A2. Structure sizes

Assert:

- `sizeof(qcpu_uapi_request_v1) == 24`
- `sizeof(qcpu_uapi_response_v1) == 56`
- `sizeof(qcpu_uapi_caps_v1) == 80`
- `sizeof(qcpu_uapi_status_v1) == 32`
- `sizeof(qcpu_uapi_exchange_v1) == 88`

### A3. Field offsets

Assert every field offset, including:

- request `seed` at 16
- response `norm_q32_32` at 48
- exchange `timeout_ns` at 24
- exchange `response` at 32

### A4. ioctl identity

Assert that GET_CAPS, GET_STATUS, and EXCHANGE:

- use UAPI magic `'Q'`
- have distinct request numbers
- carry the expected direction bits
- encode the expected structure sizes

### A5. Declaration scan

After removing comments and string literals, reject transported declarations
containing:

- pointers
- bitfields
- `long`
- `float`
- `double`
- variable-length arrays

Comments may accurately mention userspace floating-point conversion.

### A6. v4.6 semantic compatibility

Assert equality of:

- request and response magic
- protocol version
- STATUS and RUN_GHZ command values
- max qubits
- max shots
- request and response semantic field order
- q0-most-significant truth flag

## Layer B: deterministic frontend tests

Each case starts with a fresh frontend unless the case explicitly tests state
transitions.

Every case that calls `QCPU_IOC_EXCHANGE_V1` must use this fixture unless the
case explicitly tests session failure:

1. create the frontend
2. attach the selected backend
3. open exactly one client session
4. perform the exchange assertions
5. close the session
6. destroy the quiescent frontend

GET_CAPS and GET_STATUS may be tested without an open session. Transaction
tests must not bypass the exclusive-session contract.

### B1. GET_CAPS success

Verify:

- ABI version 1
- protocol version 1
- 20 qubits
- 100 shots
- one in-flight request
- required truth flags
- backend name is NUL-terminated
- driver/mock version is NUL-terminated
- all reserved bytes are zero

### B2. GET_STATUS initial states

Verify:

- OFFLINE with no backend
- IDLE after backend attachment
- active sessions initially zero
- counters initially zero

### B3. Unknown ioctl

Submit an unassigned ioctl number.

Expected:

```text
return = -1
errno = ENOTTY
backend dispatches = 0
```

### B4. Exclusive open

- first open succeeds
- second open returns `EBUSY`
- active session count remains one
- close returns the count to zero
- a new open succeeds after close

### B5. STATUS transaction

Use the common open-session fixture with a valid request containing command
STATUS, zero qubits, and zero shots.

Verify:

- response magic and version
- status OK
- truth flags complete
- no quantum execution request
- completed counter increments once
- state returns to IDLE

### B6. RUN_GHZ zero result and norm boundaries

Use the common open-session fixture with 3 qubits, 20 shots, and the
deterministic all-zero backend.

Verify:

- basis states = 8
- measured state = 0
- invalid results = 0
- exact norm `1.0` converts to `1ULL << 32`
- norm `1.0 + 0x1p-40` is accepted, clamped, and converts to `1ULL << 32`
- norm `1.0 + 0x1p-40 + 0x1p-52` is rejected with `EIO`, status KERNEL,
  failed counter +1, and state IDLE
- exact half-LSB `0x1p-33` converts to `1`
- one-and-a-half LSB `0x1.8p-32` converts to `2`
- NaN, infinity, and a negative norm are rejected with `EIO`, status KERNEL
- completed counter increments only for accepted responses

Run the same exact `1.0`, tolerance-edge, outside-tolerance, and half-LSB
conversion cases through both the deterministic STATUS backend path and the
v4.6 STATUS/RUN_GHZ bridge conversion helper.

### B7. RUN_GHZ one result

Use the common open-session fixture with 3 qubits, 20 shots, and the
deterministic all-one backend.

Verify measured state = 7.

### B8. Width boundaries

Verify success at:

- 1 qubit
- 20 qubits
- 1 shot
- 100 shots

Verify `ERANGE` and RANGE status at:

- 0 qubits for RUN_GHZ
- 21 qubits
- 0 shots for RUN_GHZ
- 101 shots

No invalid request reaches the backend.

### B9. Magic, version, and command rejection

Cases:

- wrong request magic
- unsupported protocol version
- unsupported command

Verify deterministic errno and protocol status mapping, zero backend dispatches,
and no completed/failed execution-counter increment.

### B10. Busy exchange

Hold one deterministic request in BUSY state and submit a second exchange.

Expected:

- second exchange returns `EBUSY`
- first request completes or is canceled cleanly
- max concurrent backend dispatches remains one

### B11. Response validation

Inject each failure independently using the common open-session fixture:

| Injected failure | errno | protocol status | final state |
|---|---|---|---|
| bad response magic | `EIO` | IO | IDLE |
| wrong response version | `EIO` | IO | IDLE |
| known non-OK response status | mapped errno | same known status | IDLE, or OFFLINE for BACKEND_ABSENT |
| unknown response status | `EIO` | IO | IDLE |
| missing required truth flag | `EIO` | IO | IDLE |
| qubit mismatch | `EIO` | IO | IDLE |
| shots mismatch | `EIO` | IO | IDLE |
| invalid measured state | `EIO` | KERNEL | IDLE |
| nonzero invalid-results count on success | `EIO` | KERNEL | IDLE |
| impossible basis-state count | `EIO` | KERNEL | IDLE |
| Q32.32 norm above `1ULL << 32` | `EIO` | KERNEL | IDLE |

For every row, verify that the partial response is discarded, the failed
counter increments exactly once, completed does not increment, BUSY is
released, and a following valid exchange succeeds without close/reopen.

### B12. Counter semantics

Verify:

- accepted success increments completed
- dispatched backend failure increments failed
- pre-dispatch validation reject increments neither
- GET_CAPS and GET_STATUS do not change execution counters

## Layer C: timeout, cancel, and lifecycle tests

### C1. Backend absent

With no backend attached:

- GET_CAPS remains available
- GET_STATUS reports OFFLINE
- EXCHANGE returns `ENODEV` and BACKEND_ABSENT
- frontend does not enter BUSY permanently

### C2. Deadline expiry

Use the common open-session fixture with a backend delayed beyond the absolute
monotonic deadline.

Verify:

- `ETIMEDOUT`
- TIMEOUT status
- failed counter increment
- BUSY state released
- state returns directly to IDLE while the backend remains attached
- following valid request succeeds in the same open session

### C3. Timeout boundaries and overflow

Verify:

- `timeout_ns == 0` selects `1000000000` ns
- `timeout_ns == 1` is accepted
- `timeout_ns == 5000000000` is accepted
- `timeout_ns == 5000000001` returns `ERANGE` and RANGE before dispatch
- over-limit values are never clamped
- absolute-deadline addition overflow is rejected before dispatch
- wall-clock changes do not affect monotonic deadline behavior

### C4. Cancellation and destroy quiescence

Begin a delayed exchange, then request close/cancel.

Verify:

- `qcpu_mock_destroy()` during the in-flight callback returns `EBUSY`
- the failed destroy attempt frees no frontend or backend-owned memory
- backend cancel callback is invoked at most once
- waiting operation returns `ECANCELED`
- BUSY is released before close completes
- active session reaches zero
- frontend can reopen
- destroy succeeds only after the session is closed and no callback is running
- no thread remains blocked

### C5. Backend disconnect

Simulate backend death while a request is pending.

Verify:

- client wakes
- request returns `ENODEV` and BACKEND_ABSENT
- failed counter increments exactly once
- partial response is discarded
- state moves from BUSY to OFFLINE
- backend may be reattached, moving OFFLINE to IDLE
- later STATUS succeeds in the same open session

### C6. Repeated session lifecycle

Run at least 100 open/STATUS/close cycles.

Verify:

- active sessions end at zero
- no file descriptors leak
- no threads leak
- no temporary lock remains
- counters match expected values

### C7. Mandatory cross-process exclusive lock

Using a unique mode `0700` temporary directory and mode `0600` lock file:

- process A acquires the mock session with a nonblocking advisory lock
- process B receives `EBUSY`
- no process-local exclusivity fallback is used
- terminate A cleanly
- process B can then acquire the session
- close releases the lock
- the lock file and runtime directory are removed

No global or persistent lock path is permitted.

## Layer D: v4.6 qcpud integration

Run only after Layers A-C pass.

### D1. Isolated qcpud start

- use a unique temporary runtime directory
- socket mode remains `0600`
- no force kill
- no systemd
- no boot persistence
- record PID and socket path
- preserve existing qcpud lifecycle semantics

### D2. STATUS bridge

Submit Stage 2 STATUS through the v4.6 adapter using the common open-session
fixture.

Verify:

- canonical 24-byte request
- canonical 56-byte v4.6 response
- explicit little-endian wire handling
- truth flags preserved
- exact `1.0`, tolerance-edge, outside-tolerance, and half-LSB norm cases use
  the same deterministic Q32.32 conversion contract as Layer B

### D3. RUN_GHZ bridge

Submit 3 qubits and 20 shots using the common open-session fixture.

Verify:

- result is 0 or 7
- invalid results = 0
- basis states = 8
- norm converts to Q32.32 one
- q0-most-significant convention preserved

### D4. qcpud stall

Connect a backend path that accepts but does not complete.

Verify frontend timeout and full cleanup.

### D5. qcpud termination

Terminate qcpud gracefully during a pending request.

Verify deterministic client wakeup and successful daemon restart.

### D6. Cleanup proof

After integration:

- qcpud stopped gracefully
- socket absent
- PID file absent or validly cleaned
- temporary directory removed
- no listener remains
- no process remains
- existing repository worktree unchanged

## Layer E: regression

After all Stage 2 proofs:

```bash
bash scripts/test_all.sh
```

Requirements:

- all existing v4.6 tests remain unchanged
- the existing full PASS marker appears
- Stage 2 test integration is additive
- no test lowers current safety limits
- worktree remains clean after proof

## Negative claims that must be tested

The proof report must state:

```text
PHYSICAL_QPU_PRESENT=NO
KERNEL_MODULE_BUILT=NO
KERNEL_MODULE_LOADED=NO
DEVICE_NODE_CREATED=NO
SYSTEMD_MODIFIED=NO
UDEV_MODIFIED=NO
DEVICE_TREE_MODIFIED=NO
PICO_CONNECTED=NO
GPIO_CONNECTED=NO
NETWORK_LISTENER_ADDED=NO
EXISTING_TESTS_REPLACED=NO
```

## Evidence report

The future Stage 2 proof script must generate:

```text
build/qcpu_v47_userspace_mock_proof.md
.qcpu/qcpu_v47_userspace_mock.env
logs/qcpu_v47_userspace_mock.log
```

Generated runtime files must either be ignored by Git or removed after proof.
No report may include secrets, environment values, phone identifiers, private
socket paths outside the temporary test directory, or raw process environment.

## Pass criteria

Stage 2 implementation is ready for review only when:

- Layers A-C pass
- Layer D passes when the v4.6 adapter is included
- Layer E full regression passes
- no unresolved sanitizer, compiler, or review warning remains
- clean worktree is proven
- timeout and backend-death recovery are proven
- exact source and test scope is documented
- no privileged operation was used
- no physical QPU claim appears

## Deferred to optional Stage 3

The following are intentionally not proven by Stage 2:

- real `open(2)` on `/dev/qcpu0`
- kernel `unlocked_ioctl`
- `copy_from_user` / `copy_to_user`
- kernel `EFAULT` handling
- miscdevice registration
- kernel locking and lifetime rules
- actual udev ownership and mode
- module load/unload behavior
- kernel fuzzing

Those require a separately approved, reviewed, nonpersistent Stage 3 module.
