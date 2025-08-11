# Security Overhead Benchmark

## Overview

This project measures the performance impact of compiler and linker security hardening features by comparing execution times of several microbenchmarks compiled with and without such protections enabled. It supports Windows (MSVC), Linux (GCC/Clang), and macOS (Clang).

The benchmark runs four different tests:

1. Empty function calls (tests function call prologue/epilogue overhead)
2. `snprintf` to a fixed stack buffer
3. `memcpy` on fixed stack buffers
4. Frequent small `malloc`/`free` operations

By running the same code with security features enabled and disabled, you can observe potential runtime overhead.

---

## Problem Description

Modern compilers and linkers provide options for runtime security features like stack protection, FORTIFY\_SOURCE, RELRO, PIE, and Control Flow Guard. These features improve security but may introduce measurable overhead.

The goal of this project is to:

* Quantify the performance cost of enabling security hardening options.
* Provide a reproducible and portable benchmark setup.
* Allow tuning of buffer sizes, allocation sizes, and iteration counts via CLI parameters.

---

## Example Output

**Without security hardening (`SECURE_BUILD=OFF`):**

```
Iters = 50000000, buf size = 64, malloc size = 32

1) empty function calls: 82.7365 ms
2) snprintf to stack buffer: 15474.9 ms
3) memcpy on stack buffer: 12820.6 ms
4) malloc/free small blocks: 4381.71 ms
```

**With security hardening (`SECURE_BUILD=ON`):**

```
Iters = 50000000, buf size = 64, malloc size = 32

1) empty function calls: 86.9924 ms
2) snprintf to stack buffer: 10658.1 ms
3) memcpy on stack buffer: 11078 ms
4) malloc/free small blocks: 4752.31 ms
```

---

## Explanation of Output

* **1) empty function calls** — Measures pure call overhead; can be affected by stack canaries in functions with local buffers.
* **2) snprintf to stack buffer** — Writes formatted data to a fixed-size stack buffer; may trigger extra runtime checks with `FORTIFY_SOURCE` and stack protector.
* **3) memcpy on stack buffer** — Copies fixed-size stack buffers; same security checks can apply.
* **4) malloc/free small blocks** — Allocates and frees small heap blocks repeatedly; may be impacted by allocator security features or runtime library changes.

**Why differences can be small** — On many platforms, certain security features only affect functions with specific patterns (e.g., local arrays on the stack), indirect calls, or standard library functions that can be fortified. If a benchmark’s code paths don’t meet these conditions, the runtime cost of enabling hardening can be minimal, and differences between ON and OFF may be within normal measurement noise. Additionally, run-to-run variations from CPU turbo boost, cache effects, and heap allocator state can mask or exaggerate real differences.

---

## How to Compile and Run

### 1. Clone the Repository

```bash
git clone https://github.com/LyudmilaKostanyan/Security-Overhead-Benchmark.git
cd Security-Overhead-Benchmark
```

### 2. Build the Project

#### For Linux/macOS

Secure build:

```
cmake -S . -B build_on -DSECURE_BUILD=ON
cmake --build build_on
```

Unsecure build:

```
cmake -S . -B build_off -DSECURE_BUILD=OFF
cmake --build build_off
```

#### For Windows (MSVC)

Secure build:

```
cmake -S . -B build_on -DSECURE_BUILD=ON
cmake --build build_on
```

Unsecure build:

```
cmake -S . -B build_off -DSECURE_BUILD=OFF
cmake --build build_off
```

---

### 3. Run the Program

#### Linux/macOS

```bash
./build_on/main --iters 50000000 --buf 64 --malloc 32
./build_off/main --iters 50000000 --buf 64 --malloc 32
```

#### Windows

```powershell
.\build_on\main.exe --iters 50000000 --buf 64 --malloc 32
.\build_off\main.exe --iters 50000000 --buf 64 --malloc 32
```

---

## CLI Parameters

* `--iters, -n <N>` — Number of loop iterations per test (default: 50,000,000)
* `--buf <B>` — Buffer size for snprintf/memcpy tests (default: 64)
* `--malloc <M>` — Allocation size for malloc/free test (default: 32)
