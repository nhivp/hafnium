# Code structure

The Hafnium repository contains Hafnium itself, along with unit tests and
integration tests, a small client library for VMs, a Linux kernel module for the
primary VM, prebuilt binaries of tools needed for building it and running tests.
Everything is built with [GN](https://gn.googlesource.com/gn/).

Hafnium can be built for an **architecture**, currently including:

*   `aarch64`: 64-bit Armv8
*   `fake`: A dummy architecture used for running unit tests on the host system.

And for a **platform**, such as:

*   `aem_v8a_fvp`: The Arm [Fixed Virtual Platform](FVP.md) emulator.
*   `qemu_aarch64`: QEMU emulating an AArch64 device.
*   `rpi4`: A Raspberry Pi 4 board.

Each platform has a single associated architecture.

