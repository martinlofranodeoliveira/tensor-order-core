#include "tensor_order/uint128.hpp"
#include <algorithm>

namespace tensor_order {

UInt128 UInt128::fromString(const std::string& str) {
    if (str.empty()) {
        throw std::invalid_argument("Empty string cannot be converted to UInt128");
    }
    native_t result = 0;
    for (char c : str) {
        if (c < '0' || c > '9') {
            throw std::invalid_argument("Invalid decimal character in UInt128: " + std::string(1, c));
        }
        native_t prev = result;
        result = result * 10 + static_cast<native_t>(c - '0');
        if (result < prev) {
            throw std::overflow_error("UInt128 overflow while parsing string: " + str);
        }
    }
    return UInt128(result);
}

std::string UInt128::toString() const {
    if (val_ == 0) {
        return "0";
    }
    std::string s;
    native_t v = val_;
    while (v > 0) {
        char digit = static_cast<char>('0' + (v % 10));
        s.push_back(digit);
        v /= 10;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

} // namespace tensor_order
