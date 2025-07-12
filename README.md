# Apex: Arbitrary Precision Expressions for JimTcl

**Apex** is a JimTcl extension that integrates [Gavin Howard’s `bc`](https://github.com/gavinhoward/bc) for arbitrary precision arithmetic directly within the JimTcl scripting environment.

It allows developers to evaluate numeric expressions—including floating-point and high-precision math—by communicating with a `bc` subprocess, safely and interactively.

---

## Features

- Arbitrary precision floating-point and integer arithmetic using bc syntax (C-like syntax, similar to JimTcl expr syntax)
- Communication with `bc` through three non-blocking unidirectional pipes for stdin, stdout and stderr.
- Clean integration with JimTcl as a `apex` command
- Auto-trims output and handles errors
- Reusable subprocess; minimal process overhead

---

## Requirements

- [JimTcl](http://jim.tcl.tk/)
- [Gavin Howard’s `bc`](https://github.com/gavinhoward/bc) (must be available in `$PATH`)
  - This `bc` is powerful and supports arbitrary precision math.
  - Run `bc --version` to confirm it is installed.
- Standard Unix-like environment (uses `fork()`, `pipe()`, etc.)

---

## Building

To build the extension:

```sh
make
```

This will produce a shared object (apex.so) for dynamic loading in JimTcl.

To clean the build:

```sh
make clean
```

## Usage
Loading the Extension in JimTcl

```tcl
load ./apex.so
```

This registers the apex command in your Jim interpreter.

Evaluating Expressions
```tcl
puts [apex {5 * (3 + 2)}]
# Output: 25
```

The command accepts a single string argument containing a valid bc expression. Expressions are evaluated using bc -lLq, which:

-l: loads the math library

-L: allows output lines of unlimited length

-q: suppresses startup message

Floating Point Example
```tcl
puts [apex {scale=1000; 4*a(1)}] ;# approximation of pi
```

Note: a(1) returns arctangent(1), i.e., π/4.

Integer Math
To force integer math (i.e., truncate decimals), set scale=0:

```tcl
puts [apex {scale=0; 5 / 2}]
# Output: 2
```

Functions
To create functions in apex:

```tcl
# Define factorial function in bc
apex {
define fact(n) {
  if (n <= 1) return 1
  return n * fact(n - 1)
}
}

# Call the function with some example inputs
puts "20! = [apex {scale = 1000; fact(20)}]"
puts "40! = [apex {fact(40)}]"

# Output: 2433902008176640000
# Output: 815915283247897734345611269596115894272000000000
```

Closing the Subprocess
To shut down the underlying bc process and clean up:

```tcl
apex close
```

You can reinitialize by invoking apex again later.

Error Handling
Errors from bc (e.g., syntax errors) will be caught and returned in the Jim result. Example:

```tcl
puts [apex {5 + }]
# Output: ./SCRIPT:LINENO: Error:
# Parse error: bad expression
#   <stdin>: N
#
# Traceback (most recent call last):
# ...
```

## Internals
A single persistent bc subprocess is launched upon the first use.

Data is sent to bc via a pipe; results are read asynchronously until a sentinel (@) is detected.

On failure or apex close, the subprocess and pipes are closed and cleaned.

## License
BSD-2-Clause License

Copyright (c) Geoffrey P. Messer

This project uses and interacts with JimTcl and Gavin Howard's bc.
See apex.c for full license details.

Example Script
```tcl
load ./apex.so

# Floating point calculation
puts "pi ≈ [apex {scale=30; 4*a(1)}]"

# Integer division
puts "10 div 3 = [apex {scale=0; 10 / 3}]"

# Cleanup
apex close
```

## See Also
JimTcl

Gavin Howard's bc

POSIX bc Manual
