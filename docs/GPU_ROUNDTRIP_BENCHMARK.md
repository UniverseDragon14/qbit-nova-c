# Raspberry Pi 5 CPU vs V3D GPU round-trip benchmark

Run on the Pi 5 itself with scripts/benchmark_gpu_roundtrip.sh.

The benchmark explicitly selects Mesa V3DV's integrated V3D Vulkan GPU and refuses
the llvmpipe CPU fallback. It tests 4 KiB, 64 KiB, and 1 MiB buffers. Both CPU and
GPU paths compute the same deterministic uint32 transform; every timed iteration
compares the complete output buffers.

GPU time includes copying input into mapped upload memory, device transfer, compute
dispatch, device-to-host readback transfer, queue submission/fence wait, and copying
the result back to host memory. CPU time measures the same transform over the input
array. CSV output includes median/p95 latency, CPU-over-GPU median ratio, approximate
two-way payload throughput, and which path measured faster.

This Pi 5 has shared physical memory, so this is not a discrete-GPU PCIe transfer
benchmark. Results include Vulkan command, queue, synchronization and driver
overhead. The benchmark reports observed results and does not assume the GPU wins.
It measures this workload, not general GPU speed or LLM inference.

Optional controls: QBIT_GPU_BENCH_ITERS (10–100000, default 100) and
QBIT_GPU_BENCH_WARMUP (0–10000, default 10). Generated shader, executable and
logs are local ignored artifacts beneath build/ and logs.
