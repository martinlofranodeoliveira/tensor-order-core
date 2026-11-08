# Tensor Order Core: Mathematical Contract & Invariants Specification

## 1. Abstract & Scope

`tensor-order-core` provides an exact combinatorial optimization engine in C++20 for determining the optimal binary contraction order of arbitrary tensor networks of size $N \le 12$. The contract establishes formal guarantees regarding:
- Computational complexity of contraction steps.
- Bitmask boundary formulation for intermediate tensor shapes.
- Deterministic canonical tie-breaking among degenerate equal-cost trees.
- Exact checked 128-bit integer arithmetic preventing silent integer wrap-around.
- Self-verifying replay receipts for zero-trust execution.

---

## 2. Network Topology & Structural Restrictions

### 2.1 Definitions
A tensor network $\mathcal{N} = (\mathcal{T}, \mathcal{D})$ is specified by:
1. An indexed sequence of tensors $\mathcal{T} = [T_0, T_1, \dots, T_{N-1}]$ where $1 \le N \le 12$.
2. Each tensor $T_i$ possesses an ordered tuple of axis label identifiers $\text{axes}(T_i) \in \Sigma^*$.
3. A dimension mapping $\mathcal{D}: \Sigma \to [1, 64]$ associating each label $\alpha \in \Sigma$ with a strictly positive integer dimension $d_\alpha$.

### 2.2 Invariants & Validation Rules
The network $\mathcal{N}$ is valid if and only if the following invariants hold:
1. **Network Size**: $1 \le N \le 12$.
2. **Distinct Tensor Axes**: For every tensor $T_i$, all axis labels are strictly distinct ($\forall \alpha, \beta \in \text{axes}(T_i), \alpha \ne \beta$). Self-traces are rejected.
3. **Axis Multiplicity (No Hyperedges)**: Every label $\alpha \in \bigcup_{i=0}^{N-1} \text{axes}(T_i)$ must appear **at most twice** across all tensors in $\mathcal{T}$. Labels appearing 3 or more times (hyperedges) violate pairwise contraction symmetry and must be rejected with `PlanStatus::UnsupportedNetwork`.
4. **Dimension Bounds**: Every referenced axis label $\alpha$ must have an explicitly defined dimension $d_\alpha \in [1, 64]$. Zero dimensions are strictly forbidden.
5. **Label Capacity**: Total unique labels $|\Sigma| \le 16$, permitting compact bitmask representation in standard 16-bit registers.

---

## 3. Boundary Algebra & Merge Cost Model

### 3.1 Global Classification of Axes
Let $\text{deg}(\alpha) = \sum_{i=0}^{N-1} \mathbf{1}_{\alpha \in \text{axes}(T_i)}$ denote the global degree of label $\alpha$:
- **Output Axes (Free Indices)**: $\mathcal{F} = \{ \alpha \in \Sigma \mid \text{deg}(\alpha) = 1 \}$.
- **Contracted Axes (Bound Indices)**: $\mathcal{C} = \{ \alpha \in \Sigma \mid \text{deg}(\alpha) = 2 \}$.

### 3.2 Intermediate Boundary Set $\mathcal{B}(S)$
For any non-empty tensor subset $S \subseteq \{0, 1, \dots, N-1\}$, let the local occurrence count be:
$$\text{deg}_S(\alpha) = \sum_{i \in S} \mathbf{1}_{\alpha \in \text{axes}(T_i)}$$

The active boundary labels of intermediate composite tensor $T_S$ are defined as:
$$\mathcal{B}(S) = \{ \alpha \in \Sigma \mid \text{deg}_S(\alpha) = 1 \}$$

**Proof of Invariant:**
- If $\alpha \in \mathcal{F}$, $\text{deg}(\alpha) = 1$. When a tensor containing $\alpha$ enters $S$, $\text{deg}_S(\alpha) = 1$. It remains in $\mathcal{B}(S)$ for all supersets, exiting only when the entire network is evaluated.
- If $\alpha \in \mathcal{C}$, $\text{deg}(\alpha) = 2$. When the first tensor containing $\alpha$ enters $S$, $\text{deg}_S(\alpha) = 1$, so $\alpha \in \mathcal{B}(S)$. When the second tensor enters $S$, $\text{deg}_S(\alpha) = 2 \ne 1$, so $\alpha \notin \mathcal{B}(S)$. The label has been completely contracted inside $S$.

### 3.3 Binary Merge Step Cost
For two disjoint subsets $A, B \subset \{0, \dots, N-1\}$ where $A \cap B = \emptyset$ and $S = A \cup B$:
- **Inner Contracted Axes**: $\mathcal{I}(A, B) = \mathcal{B}(A) \cap \mathcal{B}(B)$.
- **Combined Loop Space**: $\mathcal{L}(A, B) = \mathcal{B}(A) \cup \mathcal{B}(B)$.
- **Arithmetic Contraction Cost**:
  $$\text{StepCost}(A, B) = \prod_{\alpha \in \mathcal{L}(A, B)} d_\alpha$$

The total number of scalar multiplications required to contract intermediate $T_A$ with intermediate $T_B$ is the product of all dimensions in their union of boundary axes. When $\mathcal{I}(A, B) = \emptyset$, this corresponds to a tensor outer product.

---

## 4. Subset Dynamic Programming Formulation

### 4.1 State Space
The search space comprises all non-empty subsets $S \subseteq \{0, \dots, N-1\}$ represented by binary bitmasks $m \in [1, 2^N - 1]$.
- Size of state space: $2^N - 1 \le 4095$ entries.
- For singletons ($|S| = 1$), base cost $C(S) = 0$.

### 4.2 Recurrence Relation
For any subset $S$ with $|S| \ge 2$:
$$C(S) = \min_{\substack{A \subset S, A \ne \emptyset \\ B = S \setminus A \\ A < B}} \left[ C(A) + C(B) + \text{StepCost}(A, B) \right]$$

### 4.3 Canonical Tie-Breaking
When multiple bipartitions $(A_1, B_1)$ and $(A_2, B_2)$ produce identical minimum cumulative cost $C_1 = C_2$, determinism is guaranteed by the canonical lexicographic ordering of the partition masks:
$$\text{Partition } (A_1, B_1) \prec (A_2, B_2) \iff (A_1 < A_2) \lor (A_1 == A_2 \land B_1 < B_2)$$
This guarantees reproducible, compiler-independent optimal tree topologies across all platforms.

---

## 5. Checked 128-bit Arithmetic Guarantees

Although dimensions are bounded by $d_\alpha \le 64$, intermediate products over 16 axes can reach:
$$\prod_{j=1}^{16} 64 = 64^{16} = 2^{96} \approx 7.92 \times 10^{28}$$
Standard 64-bit unsigned integers (`uint64_t`, max $\approx 1.84 \times 10^{19}$) overflow silently.

`tensor-order-core` enforces `UInt128` checked arithmetic:
1. Checked Multiplication: $\text{checkedMul}(a, b)$ traps overflow via $a \cdot b / a \ne b$.
2. Checked Addition: $\text{checkedAdd}(a, b)$ traps overflow via $a + b < a$.
3. When any step cost or cumulative tree cost exceeds $2^{128} - 1$, an explicit `std::overflow_error` is thrown and caught, yielding `PlanStatus::ProvenAboveCeiling`.

---

## 6. Execution Replay Receipt Contract

A `PlanResult` contains a verifiable `ReplayReceipt` containing an ordered sequence of $N-1$ post-order binary steps:
$$\text{Step}_k = (m_k, m_{L, k}, m_{R, k}, \text{Cost}_k, \text{CumulativeCost}_k)$$

The verification routine `verifyReceipt(network, receipt)` independently evaluates:
1. **Partition Validity**: $m_{L, k} \cap m_{R, k} = \emptyset$ and $m_{L, k} \cup m_{R, k} = m_k$.
2. **Dimension Independence**: Recalculates $\prod_{\alpha \in \mathcal{B}(m_L) \cup \mathcal{B}(m_R)} d_\alpha$ directly from raw network specifications.
3. **Cumulative Sum**: $\text{CumulativeCost}_k = \text{CumulativeCost}_{L} + \text{CumulativeCost}_{R} + \text{Cost}_k$.
4. **Final Coverage**: Final step mask $m_{N-1} = 2^N - 1$.
Failure of any invariant results in immediate receipt invalidation.
