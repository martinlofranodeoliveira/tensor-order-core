#include "tensor_order/types.hpp"
#include "tensor_order/planner.hpp"
#include "tensor_order/numerical.hpp"
#include <iostream>
#include <string>
#include <cmath>

using namespace tensor_order;

static bool runDemoScenarios(bool jsonOutput) {
    Planner planner;

    // Scenario 1: Matrix Chain 3 (10x30, 30x5, 5x60)
    Network chain3;
    chain3.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    chain3.dimensions = {{"i", 10}, {"j", 30}, {"k", 5}, {"l", 60}};
    PlanResult res1 = planner.planContractions(chain3);
    bool s1Pass = (res1.status == PlanStatus::OptimalPlan && res1.optimalCost == UInt128(4500) && res1.receipt.verified);

    // Scenario 2: Star Network 4
    Network star;
    star.tensors = {
        {0, "Center", {"i", "j", "k"}},
        {1, "Branch1", {"i", "x"}},
        {2, "Branch2", {"j", "y"}},
        {3, "Branch3", {"k", "z"}}
    };
    star.dimensions = {{"i", 4}, {"j", 8}, {"k", 2}, {"x", 3}, {"y", 2}, {"z", 5}};
    PlanResult res2 = planner.planContractions(star);
    bool s2Pass = (res2.status == PlanStatus::OptimalPlan && res2.optimalCost == UInt128(236) && res2.receipt.verified);

    // Scenario 3: Proven Above Ceiling
    PlanResult res3 = planner.planContractions(chain3, UInt128(4000));
    bool s3Pass = (res3.status == PlanStatus::ProvenAboveCeiling);

    // Scenario 4: Numerical Execution Verification
    DenseTensor tA{{"i", "j"}, {2, 3}, {1, 2, 3, 4, 5, 6}};
    DenseTensor tB{{"j", "k"}, {3, 4}, {1, 0, 2, 1, 0, 1, 1, 0, 2, 1, 0, 1}};
    DenseTensor tC{{"k", "l"}, {4, 2}, {1, 2, 0, 1, 1, 0, 2, 1}};
    Network netNum;
    netNum.tensors = {
        {0, "A", {"i", "j"}},
        {1, "B", {"j", "k"}},
        {2, "C", {"k", "l"}}
    };
    netNum.dimensions = {{"i", 2}, {"j", 3}, {"k", 4}, {"l", 2}};
    PlanResult planNum = planner.planContractions(netNum);
    DenseTensor treeRes = NumericalAdapter::executeTree(planNum.root, {tA, tB, tC}, netNum.dimensions);
    DenseTensor naiveRes = NumericalAdapter::naiveContractAll({tA, tB, tC}, netNum.dimensions);
    bool s4Pass = (treeRes.shape == naiveRes.shape && treeRes.data.size() == naiveRes.data.size());
    if (s4Pass) {
        for (size_t i = 0; i < treeRes.data.size(); ++i) {
            if (std::fabs(treeRes.data[i] - naiveRes.data[i]) > 1e-9) {
                s4Pass = false;
                break;
            }
        }
    }

    if (jsonOutput) {
        std::cout << "{\n"
                  << "  \"scenario1_matrix_chain\": {\"status\": \"" << statusToString(res1.status) << "\", \"optimalCost\": \"" << res1.optimalCost.toString() << "\", \"pass\": " << (s1Pass ? "true" : "false") << "},\n"
                  << "  \"scenario2_star_network\": {\"status\": \"" << statusToString(res2.status) << "\", \"optimalCost\": \"" << res2.optimalCost.toString() << "\", \"pass\": " << (s2Pass ? "true" : "false") << "},\n"
                  << "  \"scenario3_ceiling\": {\"status\": \"" << statusToString(res3.status) << "\", \"pass\": " << (s3Pass ? "true" : "false") << "},\n"
                  << "  \"scenario4_numerical\": {\"pass\": " << (s4Pass ? "true" : "false") << "}\n"
                  << "}\n";
    } else {
        std::cout << "[DEMO] Tensor Order Core Scenarios\n";
        std::cout << "  1. Matrix Chain 3 (10x30, 30x5, 5x60): " << (s1Pass ? "PASS (Cost: 4500)" : "FAIL") << "\n";
        std::cout << "  2. Star Network 4: " << (s2Pass ? "PASS (Cost: 236)" : "FAIL") << "\n";
        std::cout << "  3. Ceiling Rejection: " << (s3Pass ? "PASS (ProvenAboveCeiling)" : "FAIL") << "\n";
        std::cout << "  4. Numerical Einsum Equivalence: " << (s4Pass ? "PASS (Residual < 1e-9)" : "FAIL") << "\n";
        if (s1Pass && s2Pass && s3Pass && s4Pass) {
            std::cout << "All demo assertions passed successfully.\n";
        } else {
            std::cerr << "Some demo assertions failed.\n";
        }
    }

    return s1Pass && s2Pass && s3Pass && s4Pass;
}

int main(int argc, char* argv[]) {
    std::string cmd = (argc > 1) ? argv[1] : "--help";

    if (cmd == "--help" || cmd == "-h") {
        std::cout << "Tensor Order Core (C++20)\n\n"
                  << "Usage:\n"
                  << "  tensor-order-cli demo [--assert|--json]\n"
                  << "  tensor-order-cli --help\n\n"
                  << "Commands:\n"
                  << "  demo    Execute demonstration scenarios with exact cost verification.\n";
        return 0;
    }

    if (cmd == "demo") {
        std::string mode = (argc > 2) ? argv[2] : "--assert";
        bool json = (mode == "--json");
        bool ok = runDemoScenarios(json);
        return ok ? 0 : 1;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    return 1;
}
