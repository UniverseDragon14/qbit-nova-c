# QBIT NOVA C - Proof Pack

This file records the current working proof points for QBIT NOVA C.

## Proof 1: Repeat VM

Command:

    ./qnova examples/full_test.qn

Expected:

    0012: REPEAT 3
    Loop!
    Loop!
    Loop!

## Proof 2: Bell State STATE2

Command:

    ./qnova examples/bell_state2.qn

Expected:

    [RUNNER] mode: STATE2
    0004: CNOT q p
    [STATE2] MEASURE pair |00> or |11>

## Proof 3: QMSG Packet

Command:

    ./qnova examples/qmsg_hi.qn

Expected:

    [QMSG] message loaded: HI
    [QMSG] bits: 0100100001001001
    [QMSG] decoded message: HI

## Proof 4: QMSG Register

Command:

    ./qnova examples/qmsg_register.qn

Expected:

    [QMSG] virtual qbit register:
    q0=|0> q1=|1> q2=|0> q3=|0> q4=|1> q5=|0> q6=|0> q7=|0>
    q8=|0> q9=|1> q10=|0> q11=|0> q12=|1> q13=|0> q14=|0> q15=|1>

## Safety Statement

This project is a software simulation and virtual processor layer.
It does not claim to convert phone or Raspberry Pi hardware into physical quantum hardware.
