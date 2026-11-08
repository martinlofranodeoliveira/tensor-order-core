#include "test_framework.hpp"
#include "tensor_order/boundary.hpp"

using namespace tensor_order;

void run_boundary_tests() {
    std::cout << "Running Boundary tracker tests..." << std::endl;

    Network net;
    net.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    net.dimensions = {{"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}};
    std::string err;
    ASSERT_TRUE(net.validate(err));
    net.compileBitmasks();

    BoundaryTracker tracker(net);

    // Leaf boundaries
    auto axes0 = tracker.maskToAxes(tracker.boundaryMask(1u << 0));
    ASSERT_EQ(axes0.size(), 2);

    // Merge A and B (mask 1 | mask 2 = 3): 'j' is contracted, boundary should be 'i' and 'k'
    uint32_t maskAB = 3;
    auto axesAB = tracker.maskToAxes(tracker.boundaryMask(maskAB));
    ASSERT_EQ(axesAB.size(), 2);
    ASSERT_EQ(axesAB[0], "i");
    ASSERT_EQ(axesAB[1], "k");

    // Merge cost of A and B: dim(i)*dim(j)*dim(k) = 10 * 30 * 5 = 1500
    UInt128 costAB = tracker.mergeCost(1u << 0, 1u << 1);
    ASSERT_EQ(costAB.toString(), "1500");

    // Merge cost of (AB) and C: dim(i)*dim(k)*dim(l) = 10 * 5 * 60 = 3000
    UInt128 costABC = tracker.mergeCost(maskAB, 1u << 2);
    ASSERT_EQ(costABC.toString(), "3000");

    std::cout << "  [PASS] Boundary tracker and merge cost tests" << std::endl;
}
