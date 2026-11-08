#include "tensor_order/planner.hpp"
#include <algorithm>
#include <bit>

namespace tensor_order {

static std::shared_ptr<ContractionNode> buildTree(
    uint32_t mask,
    const std::vector<uint32_t>& bestLeft,
    const std::vector<uint32_t>& bestRight,
    const std::vector<UInt128>& dpCost,
    const BoundaryTracker& tracker,
    const Network& net
) {
    auto node = std::make_shared<ContractionNode>();
    node->subsetMask = mask;
    node->totalCost = dpCost[mask];
    node->boundaryAxes = tracker.maskToAxes(tracker.boundaryMask(mask));

    uint32_t left = bestLeft[mask];
    uint32_t right = bestRight[mask];

    if (left == 0 && right == 0) {
        // Leaf node
        node->stepCost = UInt128(0);
        // Find which tensor index has bit set
        for (size_t i = 0; i < net.tensors.size(); ++i) {
            if ((mask & (1u << i)) != 0) {
                node->leafTensorId = static_cast<int>(i);
                break;
            }
        }
        return node;
    }

    node->stepCost = tracker.mergeCost(left, right);
    node->left = buildTree(left, bestLeft, bestRight, dpCost, tracker, net);
    node->right = buildTree(right, bestLeft, bestRight, dpCost, tracker, net);

    return node;
}

static void collectReceiptSteps(
    const std::shared_ptr<ContractionNode>& node,
    const BoundaryTracker& tracker,
    std::vector<ReplayReceiptStep>& steps,
    UInt128& runningCost
) {
    if (node->isLeaf()) {
        return;
    }
    // Post-order traversal (left, then right, then merge)
    collectReceiptSteps(node->left, tracker, steps, runningCost);
    collectReceiptSteps(node->right, tracker, steps, runningCost);

    runningCost = runningCost.checkedAdd(node->stepCost);

    ReplayReceiptStep step;
    step.subsetMask = node->subsetMask;
    step.leftMask = node->left->subsetMask;
    step.rightMask = node->right->subsetMask;
    step.contractedAxes = tracker.maskToAxes(tracker.contractedAtMergeMask(step.leftMask, step.rightMask));
    step.resultAxes = node->boundaryAxes;
    step.stepCost = node->stepCost;
    step.cumulativeCost = runningCost;

    steps.push_back(step);
}

PlanResult Planner::planContractions(
    Network& network,
    std::optional<UInt128> ceiling,
    std::optional<uint64_t> budgetOverride
) {
    PlanResult result;
    result.maxSplitsBudget = budgetOverride.value_or(maxSplitsBudget_);

    std::string err;
    if (!network.validate(err)) {
        result.status = PlanStatus::InvalidNetwork;
        result.diagnostic = err;
        return result;
    }

    network.compileBitmasks();
    BoundaryTracker tracker(network);

    size_t n = network.tensors.size();
    if (n == 1) {
        // Single tensor network: 0 cost
        result.status = PlanStatus::OptimalPlan;
        result.optimalCost = UInt128(0);
        auto leaf = std::make_shared<ContractionNode>();
        leaf->subsetMask = 1;
        leaf->stepCost = UInt128(0);
        leaf->totalCost = UInt128(0);
        leaf->leafTensorId = 0;
        leaf->boundaryAxes = network.tensors[0].axes;
        result.root = leaf;
        result.receipt.totalTensors = 1;
        result.receipt.totalCost = UInt128(0);
        result.receipt.verified = true;
        return result;
    }

    size_t totalSubsets = 1u << n;
    std::vector<UInt128> dpCost(totalSubsets, UInt128(0));
    std::vector<bool> dpComputed(totalSubsets, false);
    std::vector<uint32_t> bestLeft(totalSubsets, 0);
    std::vector<uint32_t> bestRight(totalSubsets, 0);

    // Leaves have 0 cost
    for (size_t i = 0; i < n; ++i) {
        uint32_t mask = 1u << i;
        dpCost[mask] = UInt128(0);
        dpComputed[mask] = true;
    }

    uint64_t splitsCount = 0;
    bool budgetExhausted = false;

    // Order subsets by cardinality k from 2 to n
    for (size_t k = 2; k <= n; ++k) {
        for (uint32_t S = 1; S < totalSubsets; ++S) {
            if (static_cast<size_t>(std::popcount(S)) != k) {
                continue;
            }

            UInt128 minCost;
            bool foundFirst = false;
            uint32_t chosenLeft = 0;
            uint32_t chosenRight = 0;

            // Fix the lowest set bit to A to iterate unordered bipartitions exactly once
            uint32_t fixedBit = S & (~S + 1);

            for (uint32_t A = (S - 1) & S; A > 0; A = (A - 1) & S) {
                if ((A & fixedBit) == 0 || A == S) {
                    continue;
                }
                uint32_t B = S ^ A;

                splitsCount++;
                if (result.maxSplitsBudget > 0 && splitsCount > result.maxSplitsBudget) {
                    budgetExhausted = true;
                    break;
                }

                UInt128 stepCost = tracker.mergeCost(A, B);
                UInt128 total = dpCost[A].checkedAdd(dpCost[B]).checkedAdd(stepCost);

                if (!foundFirst || total < minCost) {
                    foundFirst = true;
                    minCost = total;
                    chosenLeft = A;
                    chosenRight = B;
                } else if (total == minCost) {
                    // Canonical tie-breaking: prefer smaller left mask
                    if (A < chosenLeft) {
                        chosenLeft = A;
                        chosenRight = B;
                    }
                }
            }

            if (budgetExhausted) {
                break;
            }

            dpCost[S] = minCost;
            bestLeft[S] = chosenLeft;
            bestRight[S] = chosenRight;
            dpComputed[S] = true;
        }

        if (budgetExhausted) {
            break;
        }
    }

    result.evaluatedSplits = splitsCount;

    uint32_t fullMask = static_cast<uint32_t>(totalSubsets - 1);
    if (budgetExhausted || !dpComputed[fullMask]) {
        result.status = PlanStatus::Incomplete;
        result.diagnostic = "Search budget exhausted (" + std::to_string(splitsCount) + " splits evaluated)";
        return result;
    }

    UInt128 finalCost = dpCost[fullMask];
    result.optimalCost = finalCost;

    if (ceiling.has_value() && finalCost > ceiling.value()) {
        result.status = PlanStatus::ProvenAboveCeiling;
        result.diagnostic = "Optimal cost (" + finalCost.toString() + ") strictly exceeds ceiling (" + ceiling->toString() + ")";
        return result;
    }

    result.status = PlanStatus::OptimalPlan;
    result.root = buildTree(fullMask, bestLeft, bestRight, dpCost, tracker, network);

    // Build receipt
    result.receipt.totalTensors = static_cast<uint32_t>(n);
    result.receipt.totalCost = finalCost;
    UInt128 running(0);
    collectReceiptSteps(result.root, tracker, result.receipt.steps, running);

    std::string verifyErr;
    result.receipt.verified = verifyReceipt(result.receipt, network, verifyErr);
    if (!result.receipt.verified) {
        result.status = PlanStatus::InvalidNetwork;
        result.diagnostic = "Receipt verification failed: " + verifyErr;
    }

    return result;
}

bool Planner::verifyReceipt(const ReplayReceipt& receipt, const Network& network, std::string& err) {
    if (receipt.totalTensors != network.tensors.size()) {
        err = "Receipt tensor count does not match network";
        return false;
    }

    BoundaryTracker tracker(network);
    UInt128 running(0);

    for (size_t i = 0; i < receipt.steps.size(); ++i) {
        const auto& step = receipt.steps[i];
        if ((step.leftMask & step.rightMask) != 0) {
            err = "Step " + std::to_string(i) + " has overlapping child subsets";
            return false;
        }
        if ((step.leftMask | step.rightMask) != step.subsetMask) {
            err = "Step " + std::to_string(i) + " child union does not equal subset mask";
            return false;
        }

        UInt128 expectedCost = tracker.mergeCost(step.leftMask, step.rightMask);
        if (expectedCost != step.stepCost) {
            err = "Step " + std::to_string(i) + " stepCost mismatch";
            return false;
        }

        running = running.checkedAdd(step.stepCost);
        if (running != step.cumulativeCost) {
            err = "Step " + std::to_string(i) + " cumulativeCost mismatch";
            return false;
        }

        auto expectedResultAxes = tracker.maskToAxes(tracker.boundaryMask(step.subsetMask));
        if (expectedResultAxes != step.resultAxes) {
            err = "Step " + std::to_string(i) + " resultAxes mismatch";
            return false;
        }
    }

    if (running != receipt.totalCost) {
        err = "Final cumulative cost (" + running.toString() + ") does not match totalCost (" + receipt.totalCost.toString() + ")";
        return false;
    }

    return true;
}

} // namespace tensor_order
