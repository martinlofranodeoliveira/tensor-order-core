#pragma once

#include "tensor_order/uint128.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace tensor_order {

constexpr size_t MAX_TENSORS = 12;
constexpr size_t MAX_LABELS = 16;
constexpr uint32_t MAX_DIMENSION = 64;
constexpr uint32_t MIN_DIMENSION = 1;

enum class PlanStatus {
    OptimalPlan,
    ProvenAboveCeiling,
    InvalidNetwork,
    UnsupportedNetwork,
    Incomplete
};

inline const char* statusToString(PlanStatus s) {
    switch (s) {
        case PlanStatus::OptimalPlan: return "OptimalPlan";
        case PlanStatus::ProvenAboveCeiling: return "ProvenAboveCeiling";
        case PlanStatus::InvalidNetwork: return "InvalidNetwork";
        case PlanStatus::UnsupportedNetwork: return "UnsupportedNetwork";
        case PlanStatus::Incomplete: return "Incomplete";
    }
    return "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, PlanStatus s) {
    return os << statusToString(s);
}

struct TensorSpec {
    int id = -1;
    std::string name;
    std::vector<std::string> axes;
};

struct ContractionNode {
    uint32_t subsetMask = 0; // Bitmask of participating tensors
    UInt128 stepCost{};      // Cost of this single binary merge
    UInt128 totalCost{};     // Cumulative cost of subtree
    std::vector<std::string> boundaryAxes; // Output axes of this intermediate
    std::shared_ptr<ContractionNode> left;
    std::shared_ptr<ContractionNode> right;
    int leafTensorId = -1; // Valid if leaf (left == nullptr && right == nullptr)

    bool isLeaf() const noexcept { return left == nullptr && right == nullptr; }
};

struct ReplayReceiptStep {
    uint32_t subsetMask = 0;
    uint32_t leftMask = 0;
    uint32_t rightMask = 0;
    std::vector<std::string> contractedAxes;
    std::vector<std::string> resultAxes;
    UInt128 stepCost{};
    UInt128 cumulativeCost{};
};

struct ReplayReceipt {
    uint32_t totalTensors = 0;
    UInt128 totalCost{};
    std::vector<ReplayReceiptStep> steps; // Ordered post-order execution
    bool verified = false;
};

struct PlanResult {
    PlanStatus status = PlanStatus::InvalidNetwork;
    std::string diagnostic;
    UInt128 optimalCost{};
    std::shared_ptr<ContractionNode> root;
    ReplayReceipt receipt;
    uint64_t evaluatedSplits = 0;
    uint64_t maxSplitsBudget = 0;
};

class Network {
public:
    std::vector<TensorSpec> tensors;
    std::map<std::string, uint32_t> dimensions;

    // Derived metadata
    std::vector<std::string> allLabels;
    std::vector<std::string> outputAxes;
    std::vector<std::string> contractedAxes;

    // Bitmask representations (indices corresponding to allLabels)
    std::vector<uint32_t> tensorAxisMasks; // 1 << labelIndex
    uint32_t outputAxesMask = 0;
    uint32_t contractedAxesMask = 0;

    static Network fromJsonString(const std::string& json);
    std::string toJsonString() const;

    bool validate(std::string& err) const;
    void compileBitmasks();
};

} // namespace tensor_order
