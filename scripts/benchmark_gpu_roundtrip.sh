#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
for tool in gcc glslc; do command -v "$tool" >/dev/null 2>&1 || { echo "Missing tool: $tool" >&2; exit 2; }; done
pkg-config --exists vulkan || { echo "Missing Vulkan development files" >&2; exit 2; }
mkdir -p build/gpu-roundtrip logs
glslc benchmarks/gpu_roundtrip.comp -o build/gpu-roundtrip/transform.spv
gcc -std=c11 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers benchmarks/gpu_roundtrip.c -o build/gpu-roundtrip/gpu_roundtrip $(pkg-config --cflags --libs vulkan)
export XDG_RUNTIME_DIR=/tmp
{
 echo "# QBIT NOVA CPU vs Vulkan V3D round-trip benchmark"
 date -Is
 uname -a
 echo "# GPU time includes upload copy, device transfer, compute dispatch, readback transfer and fence wait."
 echo "# CPU time is the same uint32 transform."
 build/gpu-roundtrip/gpu_roundtrip build/gpu-roundtrip/transform.spv
} | tee logs/gpu_roundtrip_benchmark.log
