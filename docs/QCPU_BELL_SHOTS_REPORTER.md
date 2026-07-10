# QCPU Bell Shots Reporter

The QCPU Bell Shots Reporter runs the clean Bell proof multiple times and records the measured Bell outcomes.

## Purpose

It strengthens the public QBIT NOVA C proof chain by showing that the Bell demo remains inside the expected software-simulator boundary.

## Important parser rule

The reporter parses only the measured-result line:

[STATE2] MEASURE pair

It does not treat state-vector amplitude labels as measured outcomes.

Example amplitude trace:

|01>=0.000

This is not an invalid measurement. It is a basis label with zero amplitude.

## Valid measured outcomes

For the clean Bell proof, the valid correlated measured outcomes are:

- |00>
- |11>

## Invalid measured outcomes

The reporter blocks the report if these appear as measured results:

- |01>
- |10>

## Safety boundary

QBIT NOVA C is a C-based software virtual QCPU runtime.

It runs on classical hardware such as Raspberry Pi 5.

It is not a physical quantum computer.

The Bell shots reporter does not mutate hardware and does not claim physical quantum hardware.

## Output files

- build/qcpu_bell_shots_report.md
- .qcpu/bell_shots_report.env
- build/qcpu_bell_shots_raw.log

## Run

./scripts/qcpu_bell_shots_reporter.sh

Optional:

QCPU_BELL_SHOTS=100 ./scripts/qcpu_bell_shots_reporter.sh

## Expected verdict

QCPU BELL SHOTS REPORTER READY
PASS: QCPU_BELL_SHOTS_REPORT_READY
