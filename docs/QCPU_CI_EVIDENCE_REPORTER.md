# QCPU CI Evidence Reporter

QBIT NOVA C v3.6 adds a CI evidence reporter.

Purpose:

- Record local/CI proof.
- Support GitHub Actions where Raspberry Pi hardware commands may not exist.
- Confirm safe fallback when `vcgencmd` is unavailable.
- Confirm no core engine mutation.
- Produce an evidence report.

Outputs:

- `build/qcpu_ci_evidence.md`
- `.qcpu/ci_evidence.env`

Safety boundary:

This reporter is software-only.

It does not mutate hardware.

It does not claim physical quantum hardware.

Physical QCPU remains:

    EXPECTED_FAIL: PHYSICAL_QCPU_NOT_FOUND

Virtual QCPU support remains:

    PASS: VIRTUAL_QCPU_SUPPORTED_BY_CLASSICAL_HOST

Verdict:

    QCPU CI EVIDENCE REPORTER READY
