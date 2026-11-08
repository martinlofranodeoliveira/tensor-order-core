# Security Policy

Version 0.1.0 is a development release. There is no formal security certification or SLA.

## Security Model & Boundaries

- **Symbolic Planning**: This library performs offline combinatorial planning and exact cost evaluation for small tensor contraction networks. It allocates symbolic tables only and does not perform large dense GPU allocations, network requests, or subprocess execution.
- **Fail-Closed Admission Limits**: The engine rejects networks with more than 12 input tensors, more than 16 index labels, dimensions exceeding 64, hyperedges (labels appearing > 2 times globally), or repeated labels within the same tensor.
- **Checked Multiprecision Arithmetic**: Merge cost multiplication counts and tree sums use checked 128-bit unsigned integer arithmetic with explicit overflow detection.
- **Resource Bounds**: Dynamic programming split enumerations are bounded by caller-configurable operation limits. Budget exhaustion returns `Incomplete` and is never conflated with impossibility or suboptimal cost.
- **Numerical Adapter Limits**: The optional numerical execution adapter admits at most 4096 elements per tensor and 16384 total elements across the network, checked strictly before dense allocations occur.

## Reporting a Vulnerability

Please do not disclose security vulnerabilities through public GitHub issues. Use GitHub Private Vulnerability Reporting at:

https://github.com/martinlofranodeoliveira/tensor-order-core/security/advisories/new

For general inquiries, visit https://lofranotecnologia.com.br.

Please include:
1. Affected version and compiler/architecture configuration.
2. A minimal synthetic reproducer with network JSON or C++ test case.
3. Expected versus observed behavior.
4. Security and resource impact assessment.
