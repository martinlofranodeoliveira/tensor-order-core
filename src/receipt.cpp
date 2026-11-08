#include "tensor_order/types.hpp"
#include <sstream>

namespace tensor_order {

std::string Network::toJsonString() const {
    std::ostringstream ss;
    ss << "{\n  \"tensors\": [\n";
    for (size_t i = 0; i < tensors.size(); ++i) {
        ss << "    {\"id\": " << tensors[i].id << ", \"name\": \"" << tensors[i].name << "\", \"axes\": [";
        for (size_t a = 0; a < tensors[i].axes.size(); ++a) {
            ss << "\"" << tensors[i].axes[a] << "\"" << (a + 1 < tensors[i].axes.size() ? ", " : "");
        }
        ss << "]}" << (i + 1 < tensors.size() ? ",\n" : "\n");
    }
    ss << "  ],\n  \"dimensions\": {\n";
    size_t d = 0;
    for (const auto& [lbl, dim] : dimensions) {
        ss << "    \"" << lbl << "\": " << dim << (++d < dimensions.size() ? ",\n" : "\n");
    }
    ss << "  }\n}";
    return ss.str();
}

} // namespace tensor_order
