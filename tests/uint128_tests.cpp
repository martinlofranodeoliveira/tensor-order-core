#include "test_framework.hpp"
#include "tensor_order/uint128.hpp"

using namespace tensor_order;

void run_uint128_tests() {
    std::cout << "Running UInt128 tests..." << std::endl;

    UInt128 zero;
    ASSERT_EQ(zero.toString(), "0");

    UInt128 a(12345);
    UInt128 b(67890);
    UInt128 c = a + b;
    ASSERT_EQ(c.toString(), "80235");

    UInt128 p = a * b;
    ASSERT_EQ(p.toString(), "838102050");

    // Overflow check on max 128-bit
    UInt128 max128(static_cast<UInt128::native_t>(~static_cast<UInt128::native_t>(0)));
    ASSERT_THROWS(max128 + UInt128(1), std::overflow_error);
    ASSERT_THROWS(max128 * UInt128(2), std::overflow_error);

    // Parsing
    UInt128 parsed = UInt128::fromString("100000000000000000000");
    ASSERT_EQ(parsed.toString(), "100000000000000000000");

    std::cout << "  [PASS] UInt128 arithmetic and overflow checks" << std::endl;
}
