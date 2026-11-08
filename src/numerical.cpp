#include "tensor_order/numerical.hpp"
#include <stdexcept>
#include <set>
#include <algorithm>

namespace tensor_order {

DenseTensor NumericalAdapter::contractPair(
    const DenseTensor& left,
    const DenseTensor& right,
    const std::map<std::string, uint32_t>& dimensions
) {
    std::set<std::string> setL(left.axes.begin(), left.axes.end());
    std::set<std::string> setR(right.axes.begin(), right.axes.end());

    std::vector<std::string> contracted;
    std::vector<std::string> resultAxes;

    for (const auto& a : left.axes) {
        if (setR.count(a)) {
            contracted.push_back(a);
        } else {
            resultAxes.push_back(a);
        }
    }
    for (const auto& a : right.axes) {
        if (!setL.count(a)) {
            resultAxes.push_back(a);
        }
    }

    DenseTensor result;
    result.axes = resultAxes;
    size_t totalRes = 1;
    for (const auto& a : resultAxes) {
        auto it = dimensions.find(a);
        if (it == dimensions.end()) {
            throw std::runtime_error("Missing dimension for axis: " + a);
        }
        size_t d = it->second;
        result.shape.push_back(d);
        totalRes *= d;
    }

    if (totalRes > MAX_ELEMENTS_PER_TENSOR) {
        throw std::runtime_error("Contracted tensor elements (" + std::to_string(totalRes) + ") exceed maximum limit (4096)");
    }

    result.data.resize(totalRes, 0.0);

    // Compute strides for Left
    std::vector<size_t> strideL(left.axes.size(), 1);
    if (!left.axes.empty()) {
        for (int i = static_cast<int>(left.axes.size()) - 2; i >= 0; --i) {
            strideL[i] = strideL[i + 1] * left.shape[i + 1];
        }
    }

    // Compute strides for Right
    std::vector<size_t> strideR(right.axes.size(), 1);
    if (!right.axes.empty()) {
        for (int i = static_cast<int>(right.axes.size()) - 2; i >= 0; --i) {
            strideR[i] = strideR[i + 1] * right.shape[i + 1];
        }
    }

    // Compute strides for Result
    std::vector<size_t> strideRes(resultAxes.size(), 1);
    if (!resultAxes.empty()) {
        for (int i = static_cast<int>(resultAxes.size()) - 2; i >= 0; --i) {
            strideRes[i] = strideRes[i + 1] * result.shape[i + 1];
        }
    }

    // Map each axis to its stride in L, R, Result
    // All unique axes involved:
    std::vector<std::string> allUnion = resultAxes;
    for (const auto& c : contracted) {
        allUnion.push_back(c);
    }

    std::vector<size_t> unionShape;
    size_t totalIterations = 1;
    for (const auto& a : allUnion) {
        size_t d = dimensions.at(a);
        unionShape.push_back(d);
        totalIterations *= d;
    }

    std::vector<size_t> mapToL(allUnion.size(), 0);
    std::vector<bool> inL(allUnion.size(), false);
    for (size_t u = 0; u < allUnion.size(); ++u) {
        auto it = std::find(left.axes.begin(), left.axes.end(), allUnion[u]);
        if (it != left.axes.end()) {
            inL[u] = true;
            mapToL[u] = strideL[std::distance(left.axes.begin(), it)];
        }
    }

    std::vector<size_t> mapToR(allUnion.size(), 0);
    std::vector<bool> inR(allUnion.size(), false);
    for (size_t u = 0; u < allUnion.size(); ++u) {
        auto it = std::find(right.axes.begin(), right.axes.end(), allUnion[u]);
        if (it != right.axes.end()) {
            inR[u] = true;
            mapToR[u] = strideR[std::distance(right.axes.begin(), it)];
        }
    }

    std::vector<size_t> mapToRes(allUnion.size(), 0);
    std::vector<bool> inRes(allUnion.size(), false);
    for (size_t u = 0; u < allUnion.size(); ++u) {
        auto it = std::find(resultAxes.begin(), resultAxes.end(), allUnion[u]);
        if (it != resultAxes.end()) {
            inRes[u] = true;
            mapToRes[u] = strideRes[std::distance(resultAxes.begin(), it)];
        }
    }

    // Iterate over union domain
    std::vector<size_t> coord(allUnion.size(), 0);
    for (size_t iter = 0; iter < totalIterations; ++iter) {
        size_t idxL = 0;
        size_t idxR = 0;
        size_t idxRes = 0;

        for (size_t u = 0; u < allUnion.size(); ++u) {
            if (inL[u]) idxL += coord[u] * mapToL[u];
            if (inR[u]) idxR += coord[u] * mapToR[u];
            if (inRes[u]) idxRes += coord[u] * mapToRes[u];
        }

        result.data[idxRes] += left.data[idxL] * right.data[idxR];

        // Increment coordinate odometer
        for (int u = static_cast<int>(allUnion.size()) - 1; u >= 0; --u) {
            coord[u]++;
            if (coord[u] < unionShape[u]) {
                break;
            }
            coord[u] = 0;
        }
    }

    return result;
}

DenseTensor NumericalAdapter::executeTree(
    const std::shared_ptr<ContractionNode>& node,
    const std::vector<DenseTensor>& leaves,
    const std::map<std::string, uint32_t>& dimensions
) {
    if (node->isLeaf()) {
        return leaves[node->leafTensorId];
    }
    DenseTensor l = executeTree(node->left, leaves, dimensions);
    DenseTensor r = executeTree(node->right, leaves, dimensions);
    return contractPair(l, r, dimensions);
}

DenseTensor NumericalAdapter::naiveContractAll(
    const std::vector<DenseTensor>& tensors,
    const std::map<std::string, uint32_t>& dimensions
) {
    if (tensors.empty()) {
        throw std::invalid_argument("Empty tensor list");
    }
    DenseTensor current = tensors[0];
    for (size_t i = 1; i < tensors.size(); ++i) {
        current = contractPair(current, tensors[i], dimensions);
    }
    return current;
}

} // namespace tensor_order
