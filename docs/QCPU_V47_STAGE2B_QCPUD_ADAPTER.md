# QCPU v4.7 Stage 2B qcpud Adapter

Project brand: **Novakutty**

Creator and owner: **Universal Dragon Aslam**

Core technology: **QBIT NOVA C**

User-facing assistant identity: **NOVA / EVE**

## Outcome

Stage 2B connects the v4.7 pure-userspace mock frontend to the verified v4.6
`qcpud` Unix-socket backend. It makes the stable v4.7 UAPI contract runnable
without creating a kernel module or `/dev/qcpu0`.

## Data path

```text
qcpu_mock_ioctl(QCPU_IOC_EXCHANGE_V1)
        |
        v
qcpu_qcpud_adapter
        |
        | 24-byte request / 56-byte response
        | explicit little-endian framing
        v
owner-only Unix socket (mode 0600)
        |
        v
qcpud -> qcpu_kernel -> canonical response
```

## Guarantees

- STATUS and RUN_GHZ are translated explicitly.
- The adapter uses the absolute `CLOCK_MONOTONIC` deadline supplied by the
  Stage 2 frontend.
- Connect, write, and read operations each use the remaining deadline rather
  than restarting a fixed timeout.
- Cancellation shuts down only the active adapter-owned socket.
- Linux socket writes suppress `SIGPIPE`, so an early backend disconnect is
  returned as an I/O failure instead of terminating the caller process.
- A missing or refused qcpud socket maps to `ENODEV` / `BACKEND_ABSENT`.
- Transport failures map to `EIO` / `IO`.
- Backend stalls return a bounded `ETIMEDOUT` / `TIMEOUT`.
- The v4.6 userspace `double` norm is converted through the reviewed Stage 2
  Q32.32 conversion routine before it reaches the frontend response.
- Truth flags continue to state software Virtual QCPU, classical host,
  physical QPU absent, and q0 most significant.

## Files

- `src/device/qcpu_qcpud_adapter.h`
- `src/device/qcpu_qcpud_adapter.c`
- `tests/v47/qcpu_qcpud_adapter_test.c`
- `scripts/proof_qcpu_v47_qcpud_adapter.sh`

## Targeted proof

```bash
bash scripts/proof_qcpu_v47_qcpud_adapter.sh
```

The proof builds with strict C11 warnings-as-errors and verifies:

1. owner-only isolated qcpud socket startup;
2. Stage 2 STATUS through the v4.6 wire protocol;
3. Stage 2 three-qubit, twenty-shot GHZ execution;
4. exact Q32.32 norm conversion for the live qcpud response;
5. missing-backend mapping;
6. bounded accepted-but-stalled backend behavior;
7. deterministic wake-up when the connected backend terminates;
8. suppression of process-terminating `SIGPIPE` on an early peer disconnect;
9. graceful qcpud termination, socket cleanup, and restart recovery; and
10. absence of kernel artifacts, device-node actions, and network listeners.

## Build Week demo

```bash
bash scripts/qnova_build_week_demo.sh
```

The judge demo first runs the existing approval gate, proving that a standard
request is admitted while a heavy request is rejected. It then runs the live
Stage 2B adapter proof and writes a concise evidence receipt under `build/`.

## Safety boundary

Stage 2B is entirely unprivileged userspace code. It does not build or load a
kernel module, create `/dev/qcpu0`, call `mknod`, use root, install a service,
change udev or Device Tree state, contact a physical QPU, or open a TCP/UDP
listener.
