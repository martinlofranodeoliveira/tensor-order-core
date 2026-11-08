#include "test_framework.hpp"
#include "tensor_order/planner.hpp"

using namespace tensor_order;

void run_receipt_tests() {
    std::cout << "Running Receipt verification tests..." << std::endl;

    Network chain3;
    chain3.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    chain3.dimensions = {{"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}};

    Planner planner;
    PlanResult res = planner.planContractions(chain3);
    ASSERT_EQ(res.status, PlanStatus::OptimalPlan);
    ASSERT_TRUE(res.receipt.verified);

    // Tamper with receipt stepCost
    ReplayReceipt tamperedCost = res.receipt;
    tamperedCost.steps[0].stepCost = UInt128(9999);
    std::string err;
    ASSERT_FALSE(Planner::verifyReceipt(tamperedCost, chain3, err));

    // Tamper with leftMask
    ReplayReceipt tamperedMask = res.receipt;
    tamperedMask.steps[0].leftMask = 0;
    ASSERT_FALSE(Planner::verifyReceipt(tamperedMask, chain3, err));

    std::cout << "  [PASS] Receipt tamper resistance and verification tests" << std::endl;
}
