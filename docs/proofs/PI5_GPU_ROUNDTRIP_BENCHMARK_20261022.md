# Pi5 GPU round-trip benchmark evidence

Measured on Raspberry Pi 5 with V3D 7.1.7.0 on 2026-10-22.

## Results

Debian aarch64, Linux `6.18.50+rpt-rpi-2712`, Mesa V3DV integrated GPU. Each case used 10 warm-up iterations and 100 timed iterations. Every timed GPU output was compared byte-for-byte with the CPU reference.

| Input buffer | CPU median (µs) | CPU p95 (µs) | GPU round-trip median (µs) | GPU p95 (µs) | Approx. two-way payload throughput | Winner |
| ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 4 KiB | 0.796 | 0.870 | 68.815 | 76.055 | 119.044 MB/s | CPU |
| 64 KiB | 11.417 | 11.741 | 159.343 | 311.203 | 822.580 MB/s | CPU |
| 1 MiB | 187.722 | 207.797 | 2,091.963 | 3,913.037 | 1,002.481 MB/s | CPU |

## Interpretation and limits

- GPU timing includes upload copy, device transfer, compute dispatch, readback transfer, queue submission/fence wait, and copying the result back to host memory.
- Raspberry Pi 5 uses shared physical memory. These numbers are not a discrete-GPU PCIe transfer benchmark.
- For these sizes and this deterministic `uint32` transform, Vulkan dispatch, synchronization, and upload/readback overhead dominate CPU execution. CPU wins in median latency for all three cases.
- This does not imply that V3D is slower for every parallel workload. It means this measurement does not support a GPU speedup claim for this particular end-to-end round trip.
- The benchmark proves the selected V3D compute path ran and matched the CPU reference. It does not prove quantum acceleration, physical QPU execution, or LLM inference acceleration.

## Reproduction

On a Raspberry Pi 5 with Mesa V3DV, Vulkan development files, `gcc`, `glslc`, `vulkaninfo`, and `pkg-config` installed, run:

```sh
QBIT_GPU_BENCH_ITERS=100 QBIT_GPU_BENCH_WARMUP=10 bash scripts/benchmark_gpu_roundtrip.sh
```

The CSV log is generated at `logs/gpu_roundtrip_benchmark.log`; binaries and logs are local ignored artifacts. No SD-card or firmware changes are part of this test.
