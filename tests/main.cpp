#include <iostream>

void run_uint128_tests();
void run_network_tests();
void run_boundary_tests();
void run_planner_tests();
void run_receipt_tests();
void run_numerical_tests();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Tensor Order Core Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    run_uint128_tests();
    run_network_tests();
    run_boundary_tests();
    run_planner_tests();
    run_receipt_tests();
    run_numerical_tests();

    std::cout << "========================================" << std::endl;
    std::cout << "All test suites PASSED successfully." << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
