#include "test_framework.hpp"
#include "tensor_order/planner.hpp"

using namespace tensor_order;

void run_planner_tests() {
    std::cout << "Running Planner optimization tests..." << std::endl;

    Planner planner;

    // 1. Single tensor
    Network single;
    single.tensors = {{0, "T", {"x", "y"}}};
    single.dimensions = {{"x", 10}, {"y", 10}};
    PlanResult resSingle = planner.planContractions(single);
    ASSERT_EQ(resSingle.status, PlanStatus::OptimalPlan);
    ASSERT_EQ(resSingle.optimalCost.toString(), "0");

    // 2. Matrix Chain 3 (10x30, 30x5, 5x60)
    Network chain3;
    chain3.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    chain3.dimensions = {{"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}};
    PlanResult res3 = planner.planContractions(chain3);
    ASSERT_EQ(res3.status, PlanStatus::OptimalPlan);
    ASSERT_EQ(res3.optimalCost.toString(), "4500");

    // 3. Matrix Chain 4 (50x20, 20x1, 1x10, 10x64)
    Network chain4;
    chain4.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}},
        {3, "D", {"l", "m"}}
    };
    chain4.dimensions = {{"i", 50}, {"j", 20}, {"k", 1}, {"l", 10}, {"m", 64}};
    PlanResult res4 = planner.planContractions(chain4);
    ASSERT_EQ(res4.status, PlanStatus::OptimalPlan);
    ASSERT_EQ(res4.optimalCost.toString(), "4840");

    // 4. Outer Product
    Network outer;
    outer.tensors = {
        {0, "A", {"i"}},
        {1, "B", {"j"}}
    };
    outer.dimensions = {{"i", 10}, {"j", 20}};
    PlanResult resOuter = planner.planContractions(outer);
    ASSERT_EQ(resOuter.status, PlanStatus::OptimalPlan);
    ASSERT_EQ(resOuter.optimalCost.toString(), "200");

    // 5. Star Network
    Network star;
    star.tensors = {
        {0, "Center", {"i", "j", "k"}},
        {1, "Branch1", {"i", "x"}},
        {2, "Branch2", {"j", "y"}},
        {3, "Branch3", {"k", "z"}}
    };
    star.dimensions = {{"i", 4}, {"j", 8}, {"k", 2}, {"x", 3}, {"y", 2}, {"z", 5}};
    PlanResult resStar = planner.planContractions(star);
    ASSERT_EQ(resStar.status, PlanStatus::OptimalPlan);
    ASSERT_EQ(resStar.optimalCost.toString(), "236");

    // 6. Ceiling tests
    PlanResult resCeilFail = planner.planContractions(chain3, UInt128(4000));
    ASSERT_EQ(resCeilFail.status, PlanStatus::ProvenAboveCeiling);

    PlanResult resCeilPass = planner.planContractions(chain3, UInt128(5000));
    ASSERT_EQ(resCeilPass.status, PlanStatus::OptimalPlan);

    // 7. Budget exhaustion test
    PlanResult resBudget = planner.planContractions(chain4, std::nullopt, 2);
    ASSERT_EQ(resBudget.status, PlanStatus::Incomplete);

    std::cout << "  [PASS] Planner DP optimality, ceiling and budget tests" << std::endl;
}
