# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-08

### Added
- Symbolic tensor contraction planner supporting 1 to 12 input tensors and up to 16 index labels.
- Exact dynamic programming over 12-bit subset masks (4096 states, up to 261,625 bipartitions).
- Checked 128-bit unsigned integer arithmetic (`uint128_t`) with overflow detection.
- Fast bitmask boundary calculation separating globally free labels from contracted internal labels.
- Deterministic canonical split tie-breaking for equal-cost binary contractions.
- Replayable cost receipts verifying child subsets, boundary labels, and exact cost additions.
- Dense numerical execution adapter for small verified networks ($\le 4096$ elements per tensor).
- Independent Python reference oracle and exhaustive binary tree enumerator for $N \le 6$.
- CLI tool supporting `demo --assert`, `demo --json`, and custom JSON network evaluation.
- Bilingual documentation in English and Portuguese, formal contract, and architecture design documents.
