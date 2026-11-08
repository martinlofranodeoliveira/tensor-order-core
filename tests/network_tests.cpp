#include "test_framework.hpp"
#include "tensor_order/types.hpp"

using namespace tensor_order;

void run_network_tests() {
    std::cout << "Running Network validation tests..." << std::endl;

    // Empty network
    Network emptyNet;
    std::string err;
    ASSERT_FALSE(emptyNet.validate(err));

    // Valid 2-tensor network
    Network net;
    net.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}}
    };
    net.dimensions = {{"i", 10}, {"j", 20}, {"k", 30}};
    ASSERT_TRUE(net.validate(err));
    net.compileBitmasks();
    ASSERT_EQ(net.allLabels.size(), 3);
    ASSERT_EQ(net.contractedAxes.size(), 1);
    ASSERT_EQ(net.contractedAxes[0], "j");
    ASSERT_EQ(net.outputAxes.size(), 2);

    // Repeated axis in single tensor
    Network badRepeat;
    badRepeat.tensors = {{0, "A", {"i", "i"}}};
    badRepeat.dimensions = {{"i", 10}};
    ASSERT_FALSE(badRepeat.validate(err));

    // Hyperedge (3 occurrences)
    Network badHyper;
    badHyper.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"j", "l"}}
    };
    badHyper.dimensions = {{"i", 2}, {"j", 3}, {"k", 4}, {"l", 5}};
    ASSERT_FALSE(badHyper.validate(err));

    // Out of range dimension (> 64)
    Network badDim;
    badDim.tensors = {{0, "A", {"i"}}};
    badDim.dimensions = {{"i", 65}};
    ASSERT_FALSE(badDim.validate(err));

    std::cout << "  [PASS] Network validation and bounds checks" << std::endl;
}
