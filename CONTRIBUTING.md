# Contributing

Thank you for considering a contribution to Lofrano Tecnologia's Tensor Order Core.

## Local Development & Testing

Requirements: Modern C++20 compiler (GCC 13+ or Clang 16+) and CMake 3.25+.

```sh
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j$(nproc)

# Run test suite
ctest --test-dir build --output-on-failure

# Run demonstration scenarios
./build/tensor-order-cli demo --assert
./build/tensor-order-cli demo --json
```

## Guidelines

- All contributions must be original and compatible with the MIT License (© 2026 Lofrano Tecnologia).
- Zero external runtime dependencies: all planning, checked 128-bit math, and receipt verification must remain self-contained.
- Preserve exact arithmetic bounds: all scalar multiplication products must use checked `uint128_t` math with overflow guards.
- Maintain deterministic tie-breaking for equal-cost bipartitions.
- Accompany all changes with unit tests and bilingual documentation (`README.md` and `README.pt-BR.md`).
- Follow `SECURITY.md` for reporting any vulnerability.
