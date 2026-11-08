# Tensor Order Core: Architecture & Implementation Design

## 1. Architectural Philosophy

`tensor-order-core` is engineered as a zero-dependency, high-reliability C++20 core engine for exact tensor contraction path optimization. Rather than relying on heuristic stochastic search or greedy algorithms that fail on strongly coupled or non-planar graphs, this library implements an exact, bitmask-accelerated dynamic programming (DP) formulation over all subset bipartitions.

```
       Network JSON / C++ API
                 │
                 ▼
         Network Compiler
        (Sanity & Bitmasks)
                 │
                 ▼
     Subset Dynamic Programming
       (261,625 Bipartitions)
                 │
                 ├──► Ceiling Pruning
                 ├──► 128-bit Checked Math
                 └──► Canonical Tie-Breaking
                 │
                 ▼
          Contraction Tree
                 │
        ┌────────┴────────┐
        ▼                 ▼
  Replay Receipt    Numerical Adapter
 (Self-Auditing)    (Dense Validation)
```

---

## 2. Core Data Structures & Memory Layout

### 2.1 Bitmask Representation
To minimize memory bandwidth and maximize CPU L1 cache residence during DP exploration, all symbolic entities are mapped to compact integer registers:
- **Tensors**: Indexed $0 \le i < 12$. A subset $S \subseteq \{0, \dots, N-1\}$ is represented by a 32-bit unsigned integer `mask` where bit $i$ is set if $T_i \in S$.
- **Axis Labels**: Up to 16 unique labels per network. Each label is assigned a unique index $0 \le \lambda < 16$.
- **Tensor Axis Mask**: For each tensor $T_i$, `tensorAxisMasks[i]` is a `uint32_t` whose $\lambda$-th bit is 1 iff $T_i$ contains label $\lambda$.

### 2.2 Precomputed Boundary Table
Calculating the boundary set $\mathcal{B}(S)$ dynamically during every inner loop iteration would be costly.
`BoundaryTracker` precomputes arrays for all $2^N$ subsets during initialization:
- `boundaryMasks[mask]`: bitmask of active boundary labels for subset `mask`.
- `boundarySizes[mask]`: number of active boundary labels.
- `boundaryCosts[mask]`: $\prod_{\lambda \in \mathcal{B}(mask)} d_\lambda$ computed via checked 128-bit multiplication.

**Boundary Calculation Logic:**
```cpp
for (uint32_t mask = 1; mask < totalStates; ++mask) {
    uint32_t counts[16] = {0};
    for (size_t i = 0; i < N; ++i) {
        if (mask & (1 << i)) {
            for (size_t l = 0; l < numLabels; ++l) {
                if (tensorAxisMasks[i] & (1 << l)) counts[l]++;
            }
        }
    }
    uint32_t bmask = 0;
    for (size_t l = 0; l < numLabels; ++l) {
        if (counts[l] == 1) {
            bmask |= (1 << l);
        }
    }
    boundaryMasks[mask] = bmask;
}
```

---

## 3. Dynamic Programming & Submask Iteration

### 3.1 DP State Array
A flat contiguous vector `std::vector<DPSlot> dp(1 << N)` holds the optimal solution for every subset mask $m$:
```cpp
struct DPSlot {
    UInt128 minCost = UINT128_MAX;
    uint32_t bestLeft = 0;
    uint32_t bestRight = 0;
    UInt128 bestStepCost = 0;
    bool reached = false;
};
```
For $N = 12$, $|dp| = 4096$ slots, consuming $\approx 192$ KB—fitting comfortably inside modern CPU L2/L3 caches.

### 3.2 Canonical Bipartition Enumeration
Subsets are solved in strictly increasing order of their cardinality ($|S| = 2, 3, \dots, N$) to satisfy optimal substructure dependencies.
For a given target subset mask $S$:
```cpp
// Iterate over all proper non-empty submasks A of S
for (uint32_t A = (S - 1) & S; A > 0; A = (A - 1) & S) {
    uint32_t B = S ^ A;
    if (A >= B) continue; // Enforce canonical symmetry A < B
    
    // Evaluate cost: dp[A] + dp[B] + mergeCost(A, B)
    UInt128 stepCost = boundaryTracker.computeMergeCost(A, B);
    UInt128 totalCandidate = dp[A].minCost + dp[B].minCost + stepCost;
    
    // Relax DP state with canonical tie-breaking
    if (totalCandidate < dp[S].minCost) {
        dp[S].minCost = totalCandidate;
        dp[S].bestLeft = A;
        dp[S].bestRight = B;
        dp[S].bestStepCost = stepCost;
    }
}
```

### 3.3 Combinatorial Complexity
The number of valid non-trivial bipartitions across all subsets of size $N$ is:
$$\sum_{k=2}^N \binom{N}{k} (2^{k-1} - 1) = \frac{3^N - 2^{N+1} + 1}{2}$$

| $N$ (Tensors) | Total Subsets ($2^N - 1$) | Total Bipartition Pairs | Typical Run Time |
|:---:|:---:|:---:|:---:|
| 4 | 15 | 25 | $< 1\,\mu\text{s}$ |
| 6 | 63 | 301 | $< 10\,\mu\text{s}$ |
| 8 | 255 | 3,025 | $< 90\,\mu\text{s}$ |
| 10 | 1,023 | 28,501 | $< 800\,\mu\text{s}$ |
| 12 | 4,095 | 261,625 | $\approx 4.5\,\text{ms}$ |

---

## 4. Ceiling Pruning & Budget Limits

Users may supply an optional arithmetic ceiling (`maxAllowedCost`) or an evaluation budget (`maxSplitsBudget`):
1. **Ceiling Pruning**: If at any point the lower bound for subset $S$ exceeds `maxAllowedCost`, or if all bipartitions of the final state exceed the ceiling, the planner exits with `PlanStatus::ProvenAboveCeiling`.
2. **Budget Limits**: To prevent pathological CPU hangs in hostile multi-tenant environments, split evaluations are counted. If `evaluatedSplits > maxSplitsBudget`, the solver terminates deterministically with `PlanStatus::Incomplete`.

---

## 5. Numerical Execution Adapter

To bridge the gap between abstract symbolic plans and concrete tensor computations, `tensor-order-core` includes a lightweight, zero-dependency dense multidimensional tensor engine:
- Tensors are stored in dense row-major contiguous vectors of double-precision floating point numbers (`std::vector<double>`).
- Direct multidimensional index conversion and coordinate-strided contraction loops.
- Used to numerically contract test tensors ($\le 4096$ elements) along the optimal tree vs a naive linear chain, verifying numerical identity within $10^{-9}$ absolute residual.
