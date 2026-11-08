#pragma once

#include "tensor_order/types.hpp"
#include "tensor_order/boundary.hpp"
#include <optional>

namespace tensor_order {

class Planner {
public:
    explicit Planner(uint64_t maxSplitsBudget = 1'000'000)
        : maxSplitsBudget_(maxSplitsBudget) {}

    PlanResult planContractions(
        Network& network,
        std::optional<UInt128> ceiling = std::nullopt,
        std::optional<uint64_t> budgetOverride = std::nullopt
    );

    static bool verifyReceipt(const ReplayReceipt& receipt, const Network& network, std::string& err);

private:
    uint64_t maxSplitsBudget_;
};

} // namespace tensor_order
