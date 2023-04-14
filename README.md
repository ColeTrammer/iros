# Iros

[![Linux CI](https://github.com/ColeTrammer/iros/actions/workflows/linux.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/linux.yml)
[![Iros CI](https://github.com/ColeTrammer/iros/actions/workflows/iros.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/iros.yml)
[![Deploy and Generate Documentation](https://github.com/ColeTrammer/iros/actions/workflows/docs.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/docs.yml)
[![Lint](https://github.com/ColeTrammer/iros/actions/workflows/lint.yml/badge.svg)](https://github.com/ColeTrammer/iros/actions/workflows/lint.yml)
[![Codecov](https://codecov.io/gh/ColeTrammer/iros/branch/iris/graph/badge.svg?token=XOF3ERG8YK)](https://codecov.io/gh/ColeTrammer/iros)

Operating System focused on asynchronicity, minimalism, and performance.

## Features

- Supprt x86_64 architecture
- Kernel Initramfs
- Kernel Level Tasks
- Userspace Processes and Threads
- Userspace FPU/SIMD (tested on AVX2)
- Preemptive Multitasking
- Preemptible Kernel
- Symmetric Multi-Processing (SMP)
- Serial Console Input/Output
- Extremely Minimal POSIX libc
- Extremely Minimal Shell
- Boot using [Limine](https://github.com/limine-bootloader/limine)
- Targets only [QEMU](https://www.qemu.org/) for now
- Compilation using GCC 12 or Clang 16

## Screenshots

### Serial Console Shell

![Figlet Demo](/docs/assets/figlet-demo.png)

This is Iros running the [Figlet](https://github.com/cmatsuoka/figlet) program, which generates ASCII art from text.

## Build Instructions

[See here](https://coletrammer.github.io/iros/md_docs_build.html).

## Project Documentation

[See here](https://coletrammer.github.io/iros).

## Kernel Overview

[See here](https://coletrammer.github.io/iros/md_docs_iris_design_overview.html).

## Legacy Code for this repo

[See here](https://github.com/ColeTrammer/iros/tree/legacy).
