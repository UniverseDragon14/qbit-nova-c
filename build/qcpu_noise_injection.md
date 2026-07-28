# QCPU Noise Injection Matrix

Generated UTC: 2026-07-20T10:30:41Z

## Host

- Host: nova-pi
- Architecture: aarch64
- Kernel: 6.18.34+rpt-rpi-2712

## Results

| Test | Result | Meaning |
|---|---|---|
| Clean Bell proof | PASS: CLEAN_BELL_PROOF_READY | Normal Bell proof still passes |
| Synthetic noisy sample | EXPECTED_DETECT: NOISY_BELL_OUTPUT_FOUND | Bad outcomes were detected honestly |
| Bad noisy count | 2 | Number of synthetic invalid Bell outcomes |
| Core mutation check | PASS: CORE_NOT_MUTATED_BY_NOISE_TEST | Noise test did not modify core engine |
| Safety boundary | PASS: NON_DESTRUCTIVE_SOFTWARE_ONLY_TEST | No hardware mutation or destructive action |

## Clean Bell Proof Summary

```text
bad count: 0
BELL PROOF PASSED
```

## Synthetic Noise Sample

```text
|00>
|11>
|00>
|10>
|11>
|01>
|00>
|11>
```

## Boundary Statement

The noisy sample is intentionally invalid.

The detector must catch it.

    EXPECTED_DETECT: NOISY_BELL_OUTPUT_FOUND

The clean Bell proof must remain valid.

    PASS: CLEAN_BELL_PROOF_READY

## Verdict

QCPU NOISE INJECTION MATRIX READY
