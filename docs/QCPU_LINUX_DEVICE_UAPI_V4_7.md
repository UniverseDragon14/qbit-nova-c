# QBIT NOVA C v4.7 Linux QCPU Device UAPI

## Status

Stage 1 specification only.

This stage defines the stable userspace contract for a future `/dev/qcpu0`
frontend. It does not implement, build, load, install, or autoload a kernel
module. It does not create a device node.

## Truth boundary

- `/dev/qcpu0` represents a software Virtual QCPU.
- Raspberry Pi 5 remains a classical computer.
- Physical quantum hardware is absent.
- Quantum state-vector computation remains in the verified userspace
  `qcpud` and `qcpu_kernel` path.
- Kernel-facing code performs no quantum math, no state-vector allocation,
  and no floating-point operations.

## Architecture

```text
qnova-device / future applications
               |
               | libqcpu
               v
/dev/qcpu0 stable UAPI
               |
               | bounded, fixed-width request/response frames
               v
qcpud userspace backend
               |
               v
existing verified qcpu_kernel
```

The UAPI is implementation-neutral. A userspace mock or CUSE proof may be used
before any custom kernel module. A later miscdevice implementation must preserve
the same ABI.

Stage 1 does not select Netlink, a second control device, or another kernel to
userspace relay. Backend transport selection requires a separate design review
and approval. The canonical backend payload remains the v4.6 protocol frame.

## Existing v4.6 contract preserved

The current v4.6 device protocol defines:

- request magic `0x51445631`
- response magic `0x51525631`
- protocol version `1`
- maximum qubits `20`
- maximum shots `100`
- command `1`: STATUS
- command `2`: RUN_GHZ
- fixed 24-byte request
- fixed 56-byte response
- q0 mapped to the most-significant basis bit

v4.7 keeps these values and command meanings. It does not invent a second
quantum engine or claim gates that the current verified kernel does not expose.

## UAPI files

The normative header is:

```text
include/uapi/qcpu_device_uapi.h
```

The ABI uses Linux fixed-width UAPI types only:

- `__u8`
- `__u16`
- `__u32`
- `__s32`
- `__u64`

The ABI forbids:

- `long` and `unsigned long`
- pointers
- bitfields
- C enums in transported structures
- `float` and `double`
- variable-length arrays
- implicit state-vector layouts

## v4.7.0 command scope

Only the already verified v4.6 operations are exposed:

| Command | Value | Meaning |
|---|---:|---|
| `QCPU_UAPI_COMMAND_STATUS` | 1 | Return device truth and capability status |
| `QCPU_UAPI_COMMAND_RUN_GHZ` | 2 | Execute a bounded GHZ request |

Gate batching, arbitrary circuit loading, rotations, noise configuration,
state export, and asynchronous execution are deferred. Each incompatible ABI
addition receives a new ioctl number rather than changing an existing layout.

## Structure layout

### Request

`struct qcpu_uapi_request_v1` is 24 bytes and mirrors the v4.6 request
semantics.

### Response

`struct qcpu_uapi_response_v1` is 56 bytes and mirrors the v4.6 response
semantics, except that `norm` is represented as unsigned Q32.32 fixed point.

```text
1.0 = 1 << 32
```

The userspace backend converts between its internal `double` norm and
`norm_q32_32`. The kernel frontend transports and validates the fixed-width
integer without performing floating-point work.

### Exchange

`struct qcpu_uapi_exchange_v1` is a synchronous, bounded transaction:

```text
request + timeout_ns + response
```

Only one request may be in flight in v4.7.0.

## ioctl table

| ioctl | Direction | Purpose |
|---|---|---|
| `QCPU_IOC_GET_CAPS_V1` | read | Return ABI, protocol, limits, flags, backend name, and driver version |
| `QCPU_IOC_GET_STATUS_V1` | read | Return frontend state and request counters |
| `QCPU_IOC_EXCHANGE_V1` | read/write | Submit one bounded v4.6-compatible request and receive one response |

Unknown ioctl numbers return `-ENOTTY`.

The v4.7.0 interface does not define `read`, `write`, `mmap`, or state-vector
export semantics. Those operations are not silently repurposed later.

## Open contract

The logical contract is:

- at most one active client session
- a second client receives `-EBUSY`
- backend registration is a separate internal concern
- closing the client cancels or drains its bounded request
- backend loss wakes the client with a deterministic error
- no request waits forever

An implementation may allow one internal backend connection in addition to the
one client session. That backend connection is not counted as a second client.

## Sysfs contract

Future read-only metadata may expose:

```text
/sys/class/qcpu/qcpu0/backend
/sys/class/qcpu/qcpu0/driver_version
/sys/class/qcpu/qcpu0/max_qubits
/sys/class/qcpu/qcpu0/protocol_version
/sys/class/qcpu/qcpu0/status
```

No writable sysfs control is part of v4.7.0.

## Canonical backend frame

When a future frontend relays a request to `qcpud`, the canonical payload is the
existing explicit little-endian v4.6 request and response format.

The frontend must validate:

1. structure size
2. magic
3. protocol version
4. command
5. qubit and shot limits
6. zeroed reserved fields
7. bounded monotonic timeout

The frontend does not inspect or modify amplitudes.

## Deferred work

Not part of Stage 1:

- kernel module source
- `misc_register`
- module loading or unloading
- `/dev/qcpu0` creation
- udev rule installation
- systemd integration
- boot persistence
- Device Tree binding
- Pico or GPIO activity
- Netlink family registration
- CUSE daemon implementation
- cloud submission
- noise model
- state-vector `mmap`
- physical QPU claims

## Verdict

QBIT NOVA C v4.7 Stage 1 defines a fixed-width, bounded, honest Linux device
UAPI while keeping all quantum computation in the existing verified userspace
backend.
