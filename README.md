# Apex: Arbitrary Precision Expressions for JimTcl

**Apex** is a JimTcl extension that integrates [Gavin Howard’s `bc`](https://github.com/gavinhoward/bc) for powerful arbitrary precision arithmetic directly within the JimTcl scripting environment.

It allows developers to evaluate numeric expressions—including floating-point and high-precision math—by communicating with a `bc` subprocess, safely and interactively.

---

## Features

- 🔢 Arbitrary precision floating-point and integer arithmetic
- 📡 Communication with `bc` through bidirectional pipes
- 🧮 Clean integration with JimTcl as a `apex` command
- 🧼 Auto-trims output and handles errors
- 🧰 Reusable subprocess; minimal process overhead

---

## Requirements

- [JimTcl](http://jim.tcl.tk/)
- [Gavin Howard’s `bc`](https://github.com/gavinhoward/bc) (must be available in `$PATH`)
  - This `bc` is POSIX-compatible and supports arbitrary precision math.
  - Run `bc --version` to confirm it is installed.
- Standard Unix-like environment (uses `fork()`, `pipe()`, etc.)

---

## Building

To build the extension:

```sh
make
