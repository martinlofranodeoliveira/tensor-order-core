#pragma once

#include "tensor_order/types.hpp"
#include <vector>
#include <string>
#include <map>

namespace tensor_order {

constexpr size_t MAX_ELEMENTS_PER_TENSOR = 4096;
constexpr size_t MAX_TOTAL_INPUT_ELEMENTS = 16384;

struct DenseTensor {
    std::vector<std::string> axes;
    std::vector<size_t> shape;
    std::vector<double> data;

    size_t numElements() const {
        size_t n = 1;
        for (size_t d : shape) n *= d;
        return n;
    }
};

class NumericalAdapter {
public:
    static DenseTensor contractPair(
        const DenseTensor& left,
        const DenseTensor& right,
        const std::map<std::string, uint32_t>& dimensions
    );

    static DenseTensor executeTree(
        const std::shared_ptr<ContractionNode>& node,
        const std::vector<DenseTensor>& leaves,
        const std::map<std::string, uint32_t>& dimensions
    );

    static DenseTensor naiveContractAll(
        const std::vector<DenseTensor>& tensors,
        const std::map<std::string, uint32_t>& dimensions
    );
};

} // namespace tensor_order
