#include "test_framework.hpp"
#include "tensor_order/numerical.hpp"
#include "tensor_order/planner.hpp"
#include <cmath>

using namespace tensor_order;

void run_numerical_tests() {
    std::cout << "Running Numerical adapter execution tests..." << std::endl;

    // A: 2x3, B: 3x4, C: 4x2
    Network net;
    net.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    net.dimensions = {{"i", 2}, {"j", 3}, {"k", 4}, {"l", 2}};

    Planner planner;
    PlanResult plan = planner.planContractions(net);
    ASSERT_EQ(plan.status, PlanStatus::OptimalPlan);

    DenseTensor tA;
    tA.axes = {"i", "j"};
    tA.shape = {2, 3};
    tA.data = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

    DenseTensor tB;
    tB.axes = {"j", "k"};
    tB.shape = {3, 4};
    tB.data = {
        1.0, 0.0, 2.0, 1.0,
        0.0, 1.0, 1.0, 0.0,
        2.0, 1.0, 0.0, 1.0
    };

    DenseTensor tC;
    tC.axes = {"k", "l"};
    tC.shape = {4, 2};
    tC.data = {
        1.0, 2.0,
        0.0, 1.0,
        1.0, 0.0,
        2.0, 1.0
    };

    std::vector<DenseTensor> leaves = {tA, tB, tC};

    DenseTensor treeResult = NumericalAdapter::executeTree(plan.root, leaves, net.dimensions);
    DenseTensor naiveResult = NumericalAdapter::naiveContractAll(leaves, net.dimensions);

    ASSERT_EQ(treeResult.shape, naiveResult.shape);
    ASSERT_EQ(treeResult.data.size(), naiveResult.data.size());

    for (size_t i = 0; i < treeResult.data.size(); ++i) {
        ASSERT_TRUE(std::fabs(treeResult.data[i] - naiveResult.data[i]) < 1e-9);
    }

    std::cout << "  [PASS] Numerical contraction tree matches naive execution" << std::endl;
}
