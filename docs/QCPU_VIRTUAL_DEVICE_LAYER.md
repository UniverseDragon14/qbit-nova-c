# QBIT NOVA C v4.6 Virtual QCPU Device Layer

## Purpose

QBIT NOVA C v4.6 exposes the existing software statevector
kernel through a hardware-style device boundary.

The statevector computation still runs on the Raspberry Pi 5
ARM CPU and system memory.

Huawei Termux acts as a remote control terminal over SSH.

A Raspberry Pi Pico may later act as a physical controller for
status LEDs, approval input, display output, and watchdog duties.

## Truth boundary

- Software Virtual QCPU: yes
- Classical Raspberry Pi 5 host: yes
- Physical quantum processor: no
- Physical qubits: no
- Quantum hardware claim: no

## Transport

The initial transport is a local Unix domain socket.

Planned socket:

    /run/user/<uid>/qbit-nova/qcpu.sock

The socket will be owner-only.

## Commands

Protocol version 1 defines:

- STATUS
- RUN_GHZ

Future commands may include:

- RESET
- LOAD_CIRCUIT
- RUN_CIRCUIT
- MEASURE

## Runtime limits

- Maximum virtual qubits: 20
- Maximum shots per device request: 100
- One request processed at a time
- No network listener
- No privileged kernel module
- No hardware mutation

## Device truth flags

Every successful response identifies the device as:

- software virtual QCPU
- classical host execution
- physical QPU absent
- q0 mapped to the most-significant basis bit

## Hardware roadmap

The Pico controller is not required for the v4.6 software layer.

Pico integration will begin only after its exact model, cable,
power source, and available modules have been visually verified.

## Stage 3B Unix socket device

The first executable device boundary uses an owner-only Unix
domain socket and does not expose a TCP or UDP listener.

Components:

- `qcpud`: local C device daemon
- `qnova-device`: local C device client
- explicit little-endian request and response encoding
- fixed 24-byte request packet
- fixed 56-byte response packet
- malformed magic and protocol-version rejection
- STATUS and RUN_GHZ commands
- owner-only socket mode `0600`
- one request per test daemon instance

Example:

    qcpud --socket /tmp/qbit-nova/qcpu.sock
    qnova-device --socket /tmp/qbit-nova/qcpu.sock status
    qnova-device --socket /tmp/qbit-nova/qcpu.sock run-ghz 3 20 424242

The daemon runs the statevector kernel on the classical host CPU.

The socket is not a physical QPU and the Pico is not connected
during this stage.
