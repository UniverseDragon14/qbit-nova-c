# QNOVA Public Install Script

v4.2 adds a simple local install script for QBIT NOVA C public demo users.

## Install

From the repository root, run:

    ./install.sh

This creates a local user command:

    qnova-demo

## Dry run

To test without installing files:

    ./install.sh --dry-run

## What it installs

The script creates this wrapper:

    ~/.local/bin/qnova-demo

The wrapper runs:

    scripts/qnova_demo.sh

## Safety boundary

This installer is local-user only.

It does not use sudo.

It does not mutate hardware.

It does not claim physical quantum hardware.

It only creates a convenience command for the safe public virtual QCPU demo.

## Expected output

    QNOVA PUBLIC INSTALL SCRIPT READY
    PASS: QNOVA_PUBLIC_INSTALL_READY

## Verdict

QNOVA PUBLIC INSTALL SCRIPT READY
PASS: QNOVA_PUBLIC_INSTALL_READY
