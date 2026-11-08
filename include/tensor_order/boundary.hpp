#pragma once

#include "tensor_order/types.hpp"
#include <vector>

namespace tensor_order {

class BoundaryTracker {
public:
    explicit BoundaryTracker(const Network& net);

    // Returns boundary bitmask for subset mask (0 < mask < 2^N)
    uint32_t boundaryMask(uint32_t subsetMask) const noexcept {
        return subsetBoundaries_[subsetMask];
    }

    // Returns labels involved in the binary merge A + B (boundary(A) | boundary(B))
    uint32_t mergeLabelsMask(uint32_t leftMask, uint32_t rightMask) const noexcept {
        return subsetBoundaries_[leftMask] | subsetBoundaries_[rightMask];
    }

    // Returns labels contracted during merge A + B (boundary(A) & boundary(B))
    uint32_t contractedAtMergeMask(uint32_t leftMask, uint32_t rightMask) const noexcept {
        return subsetBoundaries_[leftMask] & subsetBoundaries_[rightMask];
    }

    // Calculates scalar multiplication cost for merging A and B
    UInt128 mergeCost(uint32_t leftMask, uint32_t rightMask) const;

    // Convert bitmask to list of string axis names
    std::vector<std::string> maskToAxes(uint32_t mask) const;

private:
    const Network& net_;
    std::vector<uint32_t> subsetBoundaries_;
    std::vector<UInt128> labelDimensions_; // indexed by label bit index
};

} // namespace tensor_order
