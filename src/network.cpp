#include "tensor_order/types.hpp"
#include <set>
#include <sstream>
#include <algorithm>

namespace tensor_order {

bool Network::validate(std::string& err) const {
    if (tensors.empty()) {
        err = "Network must have at least 1 tensor";
        return false;
    }
    if (tensors.size() > MAX_TENSORS) {
        err = "Network exceeds maximum tensor limit (12): " + std::to_string(tensors.size());
        return false;
    }

    std::map<std::string, int> labelFrequency;
    for (size_t i = 0; i < tensors.size(); ++i) {
        const auto& t = tensors[i];
        std::set<std::string> seenAxes;
        for (const auto& axis : t.axes) {
            if (axis.empty()) {
                err = "Empty axis name in tensor " + std::to_string(i);
                return false;
            }
            if (seenAxes.count(axis)) {
                err = "Repeated axis '" + axis + "' within single tensor " + std::to_string(i) + " is unsupported";
                return false;
            }
            seenAxes.insert(axis);
            labelFrequency[axis]++;
        }
    }

    if (labelFrequency.size() > MAX_LABELS) {
        err = "Network exceeds maximum distinct labels limit (16): " + std::to_string(labelFrequency.size());
        return false;
    }

    for (const auto& [label, freq] : labelFrequency) {
        if (freq != 1 && freq != 2) {
            err = "Label '" + label + "' has unsupported occurrence count " + std::to_string(freq) + " (must be 1 for free, 2 for contracted)";
            return false;
        }
        auto dimIt = dimensions.find(label);
        if (dimIt == dimensions.end()) {
            err = "Missing dimension for label: " + label;
            return false;
        }
        if (dimIt->second < MIN_DIMENSION || dimIt->second > MAX_DIMENSION) {
            err = "Dimension for label '" + label + "' out of range [1, 64]: " + std::to_string(dimIt->second);
            return false;
        }
    }

    return true;
}

void Network::compileBitmasks() {
    allLabels.clear();
    for (const auto& [label, _] : dimensions) {
        allLabels.push_back(label);
    }
    std::sort(allLabels.begin(), allLabels.end());

    std::map<std::string, int> labelToIndex;
    for (size_t i = 0; i < allLabels.size(); ++i) {
        labelToIndex[allLabels[i]] = static_cast<int>(i);
    }

    outputAxes.clear();
    contractedAxes.clear();
    outputAxesMask = 0;
    contractedAxesMask = 0;

    std::map<std::string, int> freqs;
    for (const auto& t : tensors) {
        for (const auto& ax : t.axes) {
            freqs[ax]++;
        }
    }

    for (const auto& [label, freq] : freqs) {
        int idx = labelToIndex[label];
        if (freq == 1) {
            outputAxes.push_back(label);
            outputAxesMask |= (1u << idx);
        } else if (freq == 2) {
            contractedAxes.push_back(label);
            contractedAxesMask |= (1u << idx);
        }
    }
    std::sort(outputAxes.begin(), outputAxes.end());
    std::sort(contractedAxes.begin(), contractedAxes.end());

    tensorAxisMasks.resize(tensors.size(), 0);
    for (size_t i = 0; i < tensors.size(); ++i) {
        uint32_t mask = 0;
        for (const auto& ax : tensors[i].axes) {
            mask |= (1u << labelToIndex[ax]);
        }
        tensorAxisMasks[i] = mask;
    }
}

} // namespace tensor_order
