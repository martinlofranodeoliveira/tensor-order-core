#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>
#include <ostream>

namespace tensor_order {

class UInt128 {
public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#if defined(__SIZEOF_INT128__)
    using native_t = unsigned __int128;
#else
    #error "128-bit integer support is required for checked tensor contraction bounds"
#endif
#pragma GCC diagnostic pop

    constexpr UInt128() noexcept : val_(0) {}
    constexpr UInt128(native_t val) noexcept : val_(val) {}

    static UInt128 fromString(const std::string& str);

    constexpr native_t native() const noexcept { return val_; }
    constexpr explicit operator uint64_t() const noexcept { return static_cast<uint64_t>(val_); }

    UInt128 checkedAdd(const UInt128& other) const {
        native_t sum = val_ + other.val_;
        if (sum < val_) {
            throw std::overflow_error("UInt128 addition overflow");
        }
        return UInt128(sum);
    }

    UInt128 checkedMul(const UInt128& other) const {
        if (val_ == 0 || other.val_ == 0) {
            return UInt128(0ULL);
        }
        native_t prod = val_ * other.val_;
        if (prod / val_ != other.val_) {
            throw std::overflow_error("UInt128 multiplication overflow");
        }
        return UInt128(prod);
    }

    constexpr bool operator==(const UInt128& other) const noexcept { return val_ == other.val_; }
    constexpr bool operator!=(const UInt128& other) const noexcept { return val_ != other.val_; }
    constexpr bool operator<(const UInt128& other) const noexcept { return val_ < other.val_; }
    constexpr bool operator<=(const UInt128& other) const noexcept { return val_ <= other.val_; }
    constexpr bool operator>(const UInt128& other) const noexcept { return val_ > other.val_; }
    constexpr bool operator>=(const UInt128& other) const noexcept { return val_ >= other.val_; }

    UInt128 operator+(const UInt128& other) const { return checkedAdd(other); }
    UInt128 operator*(const UInt128& other) const { return checkedMul(other); }

    std::string toString() const;

private:
    native_t val_;
};

inline std::ostream& operator<<(std::ostream& os, const UInt128& v) {
    return os << v.toString();
}

} // namespace tensor_order
