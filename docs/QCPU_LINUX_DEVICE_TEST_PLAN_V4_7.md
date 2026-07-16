# QBIT NOVA C v4.7 Linux QCPU Device Test Plan

## Principle

v4.7 tests are additive.

The existing v4.6 protocol, Unix-socket, lifecycle, and complete regression
tests remain unchanged.

## Stage 1 checks

Stage 1 is software-interface specification only. It creates no privileged implementation and makes no physical QPU claim.

Stage 1 validates only the specification and UAPI header:

1. header compiles as a userspace include
2. fixed structure sizes remain stable
3. ioctl numbers and directions are unique
4. no floating-point fields exist
5. no raw pointers, `long`, bitfields, or variable arrays exist
6. v4.6 magic, version, limits, commands, and packet sizes match
7. existing `scripts/test_all.sh` checksum is unchanged
8. only approved Stage 1 files are committed

Expected structure sizes:

| Structure | Size |
|---|---:|
| `qcpu_uapi_request_v1` | 24 bytes |
| `qcpu_uapi_response_v1` | 56 bytes |
| `qcpu_uapi_caps_v1` | 80 bytes |
| `qcpu_uapi_status_v1` | 32 bytes |
| `qcpu_uapi_exchange_v1` | 88 bytes |

## Stage 2 userspace mock

Requires separate approval.

A userspace mock validates the ABI without loading a custom kernel module.

Tests:

- GET_CAPS values
- GET_STATUS state transitions
- STATUS transaction
- RUN_GHZ with 3 qubits and 20 shots
- result restricted to `|000>` or `|111>`
- wrong magic rejection
- wrong version rejection
- unknown command rejection
- qubit and shot range rejection
- nonzero reserved-field rejection
- backend absent error
- deadline expiry
- second client receives `EBUSY`
- backend crash wakes client
- repeated open/close does not leak sessions

No existing v4.6 test file is replaced.

## Stage 3 optional miscdevice implementation

Requires separate approval and a reviewed implementation.

Preconditions:

- Stage 1 specification merged
- Stage 2 mock passes
- clean backup and rollback plan
- matching Raspberry Pi kernel headers available
- no boot persistence
- no automatic module load

Tests:

1. build module with warnings treated as errors
2. inspect module metadata
3. load manually under explicit approval
4. verify only expected device/class entries appear
5. verify mode and ownership
6. run Stage 2 functional cases against `/dev/qcpu0`
7. run malformed-frame and timeout fault tests
8. simulate qcpud disconnect and restart
9. unload module cleanly
10. confirm no process, node, class, or memory allocation remains
11. run full V1-v4.7 regression
12. verify temperature and throttling remain safe

## ABI compatibility tests

A dedicated layout test must assert:

- request size 24
- response size 56
- exchange size 88
- exact offsets for every field
- ioctl command values
- Q32.32 norm value for 0.0 and 1.0
- all reserved bytes are zero
- 32-bit and 64-bit userspace layout compatibility where supported

An incompatible structure change requires a new ioctl number.

## Fuzz and fault tests

Before any release claim:

- random ioctl numbers
- truncated user buffers
- unreadable user pointers
- all command values
- boundary qubit and shot values
- maximum timeout and overflow attempts
- signals during a blocked transaction
- client exit during backend work
- backend exit during client wait
- repeated timeout recovery
- request counter wrap analysis

## Performance and safety limits

Initial v4.7 limits remain:

```text
max qubits: 20
max shots: 100
max in-flight requests: 1
client sessions: 1
network listeners: 0
```

Performance testing must never weaken these limits merely to obtain a larger
benchmark number.

## Release evidence

A v4.7 release candidate requires:

- Stage 1 UAPI and threat model review
- userspace mock PASS
- optional module test PASS if a module is included
- all v4.6 tests unchanged and PASS
- full regression PASS
- worktree clean
- no force push
- no boot persistence
- physical QPU claim absent
- Raspberry Pi temperature safe
- `get_throttled=0x0`

## Test-plan verdict

The test plan proves the ABI first, userspace behavior second, and privileged
kernel behavior last. Each stage has a separate approval boundary.
