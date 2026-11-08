#include "tensor_order/boundary.hpp"
#include <stdexcept>

namespace tensor_order {

BoundaryTracker::BoundaryTracker(const Network& net) : net_(net) {
    size_t numTensors = net_.tensors.size();
    size_t totalSubsets = 1u << numTensors;
    subsetBoundaries_.resize(totalSubsets, 0);

    // Precompute label dimensions for fast multiplication
    labelDimensions_.resize(net_.allLabels.size(), 1);
    for (size_t i = 0; i < net_.allLabels.size(); ++i) {
        auto it = net_.dimensions.find(net_.allLabels[i]);
        if (it != net_.dimensions.end()) {
            labelDimensions_[i] = UInt128(it->second);
        }
    }

    // Single tensor leaves
    for (size_t i = 0; i < numTensors; ++i) {
        subsetBoundaries_[1u << i] = net_.tensorAxisMasks[i];
    }

    // Dynamic precomputation of all subsets via bitwise XOR of disjoint parts
    for (uint32_t mask = 1; mask < totalSubsets; ++mask) {
        // Find lowest set bit
        uint32_t lowestBit = mask & (~mask + 1);
        if (mask != lowestBit) {
            subsetBoundaries_[mask] = subsetBoundaries_[lowestBit] ^ subsetBoundaries_[mask ^ lowestBit];
        }
    }
}

UInt128 BoundaryTracker::mergeCost(uint32_t leftMask, uint32_t rightMask) const {
    uint32_t labelsMask = mergeLabelsMask(leftMask, rightMask);
    if (labelsMask == 0) {
        // Scalar merge: 1 multiplication
        return UInt128(1);
    }
    UInt128 cost(1);
    for (size_t i = 0; i < labelDimensions_.size(); ++i) {
        if ((labelsMask & (1u << i)) != 0) {
            cost = cost.checkedMul(labelDimensions_[i]);
        }
    }
    return cost;
}

std::vector<std::string> BoundaryTracker::maskToAxes(uint32_t mask) const {
    std::vector<std::string> axes;
    for (size_t i = 0; i < net_.allLabels.size(); ++i) {
        if ((mask & (1u << i)) != 0) {
            axes.push_back(net_.allLabels[i]);
        }
    }
    return axes;
}

} // namespace tensor_order
