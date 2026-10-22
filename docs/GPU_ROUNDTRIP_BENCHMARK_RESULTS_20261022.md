# Raspberry Pi 5 V3D round-trip benchmark results

Date: 2026-10-22
Host: Raspberry Pi 5, Debian GNU/Linux, Linux 6.18.50+rpt-rpi-2712
GPU: Mesa V3DV / V3D 7.1.7.0
Iterations: 100 per buffer size; warmup: 10

The benchmark compares a deterministic uint32 transform on CPU with a Vulkan V3D GPU path. GPU timing includes upload, device transfer, dispatch, readback, queue synchronization/fence wait, and host copy. The device uses shared physical memory; this is not a discrete-GPU PCIe transfer test.

Two independent runs on the same Pi 5 produced the following median latencies:

| Buffer bytes | CPU median run 1 (µs) | GPU median run 1 (µs) | CPU median run 2 (µs) | GPU median run 2 (µs) | Winner in both runs |
|---:|---:|---:|---:|---:|---|
| 4,096 | 0.768 | 68.213 | 0.796 | 69.426 | CPU |
| 65,536 | 11.482 | 161.166 | 11.333 | 160.213 | CPU |
| 1,048,576 | 187.945 | 2,149.741 | 187.389 | 2,162.972 | CPU |

Run 1 p95 (CPU/GPU, microseconds): 4 KiB 0.834/81.148; 64 KiB 11.945/384.463; 1 MiB 237.704/4,213.166.

Run 2 p95 (CPU/GPU, microseconds): 4 KiB 0.833/78.519; 64 KiB 11.481/170.945; 1 MiB 208.796/3,065.612.

## Interpretation

- CPU median latency won for all three tested buffer sizes in both runs.
- The GPU path was genuinely selected as V3D 7.1.7.0; the benchmark refused llvmpipe fallback.
- Dispatch and synchronization overhead dominate this small transform. These measurements do not establish that CPU is faster for all workloads.
- Do not claim general GPU acceleration, LLM acceleration, or quantum speedup from this benchmark. Repeat with representative workloads before selecting a GPU path for performance-sensitive use.

The generated CSV and raw local run log are artifacts of the benchmark script; this document records two measured executions on the hardware above.
