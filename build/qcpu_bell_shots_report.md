# QCPU Bell Shots Reporter

Generated UTC: 2026-07-20T10:30:48Z

## Host

| Field | Value |
|---|---|
| Host | nova-pi |
| Architecture | aarch64 |
| Kernel | 6.18.34+rpt-rpi-2712 |
| Commit | ca48ac4 Document v4.7 Stage 2B GPT-5.6 verification evidence |
| Latest tag | v4.3 |

## Bell Shots Summary

| Result | Count | Status |
|---|---:|---|
| \|00> | 15 | valid measured Bell outcome |
| \|11> | 5 | valid measured Bell outcome |
| \|01> | 0 | invalid measured outcome for clean Bell proof |
| \|10> | 0 | invalid measured outcome for clean Bell proof |
| Unknown | 0 | invalid or unreadable measurement |

## Decision

| Field | Value |
|---|---|
| Shots | 20 |
| Good count | 20 |
| Bad count | 0 |
| Decision | ALLOW_BELL_SHOTS_REPORT |
| Status | PASS: QCPU_BELL_SHOTS_REPORT_READY |

## Parser Rule

The reporter reads only this measured-result line:

[STATE2] MEASURE pair

It ignores state-vector amplitude trace lines such as:

\|01>=0.000

because those are basis labels, not measured outcomes.

## Boundary Statement

This Bell shots reporter is software-only.

It does not claim physical quantum hardware.

It checks that the clean Bell proof produces only valid correlated measured outcomes:

- \|00>
- \|11>

Invalid measured outcomes are blocked:

- \|01>
- \|10>

## Verdict

QCPU BELL SHOTS REPORTER READY

PASS: QCPU_BELL_SHOTS_REPORT_READY
