# tensor-order-core

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Standard-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/Build-CMake%203.25+-informational.svg)](CMakeLists.txt)
[![Zero Dependencies](https://img.shields.io/badge/Dependencies-Zero-brightgreen.svg)](#zero-dependencies)
[![Checked 128-bit](https://img.shields.io/badge/Arithmetic-Checked%20128--bit-orange.svg)](#checked-128-bit-arithmetic)

> **Exact combinatorial optimization engine in C++20 for finding the provably minimal-multiplication binary contraction tree of arbitrary tensor networks with checked 128-bit arithmetic, canonical tie-breaking, and self-auditing replay receipts.**

---

## Overview

In tensor network computations—spanning quantum circuit simulation, tensor train decompositions, state preparation, and multi-index Einstein summations (`einsum`)—the order in which pairwise tensors are contracted dictates algorithmic performance. A poorly chosen contraction sequence can result in intermediate tensors whose dimension products exceed memory or CPU budgets by orders of magnitude (e.g. $10^{15}$ operations instead of $10^4$).

`tensor-order-core` solves this combinatorial optimization problem exactly for networks up to $N = 12$ tensors by formulating an exhaustive bitmask-accelerated dynamic programming (DP) search over all 261,625 bipartitions. It guarantees:
- **Provable Optimality**: Finds the global minimum arithmetic multiplication count.
- **Checked 128-bit Math**: Traps arithmetic overflow when intermediate volume products exceed $2^{64}-1$.
- **Zero Dependencies**: Pure ISO C++20 standard library with no external third-party dependencies.
- **Replay Receipts**: Generates post-order execution manifests that can be independently audited in linear time.
- **Canonical Determinism**: Purely deterministic tie-breaking across compilers, CPU architectures, and runtime threads.

---

## Key Features

1. **Exact Dynamic Programming**: Explores all $2^N$ states and canonical $A < B$ bipartitions in under 5 milliseconds for 12 tensors.
2. **Boundary-Bitmask Tracker**: Instantaneous intermediate boundary calculation via bitwise XOR and degree-tracker tables.
3. **Checked 128-Bit Unsigned Math (`UInt128`)**: Fully checked operations with overflow detection during tensor product evaluation.
4. **Pruning & Safety Bounds**:
   - `maxAllowedCost`: Provably exits early (`ProvenAboveCeiling`) if minimum cost exceeds a predefined ceiling.
   - `maxSplitsBudget`: Prevents resource exhaustion in multi-tenant or real-time query engines.
5. **Self-Auditing Replay Receipts**: Generates an unambiguous post-order contraction schedule with step-by-step arithmetic verification.
6. **Numerical Demonstration Engine**: Includes a dense multidimensional tensor contraction executor to verify symbolic tree outputs against direct naive contractions.

---

## Performance & Complexity

The total number of bipartition pairs evaluated across all non-empty subsets of size $N$ is:
$$\text{Bipartitions}(N) = \frac{3^N - 2^{N+1} + 1}{2}$$

| Tensors ($N$) | Subsets ($2^N - 1$) | Evaluated Bipartitions | Memory Footprint | Solve Time (GCC 14 / x86_64) |
|:---:|:---:|:---:|:---:|:---:|
| 4 | 15 | 25 | $< 1\,\text{KB}$ | $< 1\,\mu\text{s}$ |
| 6 | 63 | 301 | $3\,\text{KB}$ | $< 8\,\mu\text{s}$ |
| 8 | 255 | 3,025 | $12\,\text{KB}$ | $< 85\,\mu\text{s}$ |
| 10 | 1,023 | 28,501 | $48\,\text{KB}$ | $< 750\,\mu\text{s}$ |
| 12 | 4,095 | 261,625 | $192\,\text{KB}$ | $\approx 4.5\,\text{ms}$ |

---

## Quickstart & CLI Usage

### Build from Source
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
```

### Run Unit Tests
```bash
ctest --test-dir build --output-on-failure
```

### Run Scenarios Demo
```bash
./build/tensor-order-cli demo --assert
```

### Emit JSON Diagnostics
```bash
./build/tensor-order-cli demo --json
```

---

## C++ API Usage Example

```cpp
#include "tensor_order/planner.hpp"
#include <iostream>

int main() {
    using namespace tensor_order;

    // Define network: 3-matrix chain (10x30) x (30x5) x (5x60)
    Network net;
    net.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    net.dimensions = {
        {"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}
    };

    // Compile bitmasks and validate
    std::string err;
    if (!net.validate(err)) {
        std::cerr << "Validation error: " << err << std::endl;
        return 1;
    }
    net.compileBitmasks();

    // Plan optimal contraction sequence
    Planner planner;
    PlanResult result = planner.plan(net);

    if (result.status == PlanStatus::OptimalPlan) {
        std::cout << "Optimal scalar multiplications: " << result.optimalCost << std::endl;
        std::cout << "Evaluated splits: " << result.evaluatedSplits << std::endl;

        // Verify receipt
        bool valid = verifyReceipt(net, result.receipt);
        std::cout << "Receipt verified: " << (valid ? "YES" : "NO") << std::endl;
    }

    return 0;
}
```

---

## Verification & Architecture Documentation

For formal proofs, boundary invariants, and implementation architecture, consult:
- [Contract Specification](docs/contract.md): Formal algebra, boundary definitions, and overflow guarantees.
- [Architecture & Design](docs/design.md): DP state machine, memory layout, and complexity breakdown.

---

## Security

Please report vulnerabilities via GitHub Private Vulnerability Reporting or refer to [SECURITY.md](SECURITY.md).

---

## License

This project is licensed under the [MIT License](LICENSE).
Copyright © 2026 Lofrano Tecnologia.
