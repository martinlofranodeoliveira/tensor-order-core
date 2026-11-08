# Copyright 2026 Lofrano Tecnologia
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.25)
project(TensorOrderCore VERSION 0.1.0 LANGUAGES CXX
        DESCRIPTION "Symbolic tensor contraction tree planner"
        HOMEPAGE_URL "https://lofranotecnologia.com.br")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TOC_SANITIZE "Enable address and undefined-behavior sanitizers" OFF)

add_library(tensor_order STATIC
    src/uint128.cpp
    src/network.cpp
    src/boundary.cpp
    src/planner.cpp
    src/receipt.cpp
    src/numerical.cpp
)

target_include_directories(tensor_order PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tensor_order PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror=return-type
    )
endif()

if(TOC_SANITIZE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "TOC_SANITIZE requires GCC or Clang")
    endif()
    target_compile_options(tensor_order PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(tensor_order PUBLIC -fsanitize=address,undefined -fno-sanitize-recover=all)
endif()

add_executable(tensor-order-cli app/main.cpp)
target_link_libraries(tensor-order-cli PRIVATE tensor_order)

include(CTest)
if(BUILD_TESTING)
    add_executable(toc_tests
        tests/main.cpp
        tests/uint128_tests.cpp
        tests/network_tests.cpp
        tests/boundary_tests.cpp
        tests/planner_tests.cpp
        tests/receipt_tests.cpp
        tests/numerical_tests.cpp
    )
    target_link_libraries(toc_tests PRIVATE tensor_order)
    add_test(NAME unit_and_regression COMMAND toc_tests)
    add_test(NAME demo_assert COMMAND tensor-order-cli demo --assert)
    add_test(NAME demo_json COMMAND tensor-order-cli demo --json)
    set_tests_properties(unit_and_regression demo_assert demo_json PROPERTIES TIMEOUT 60)
endif()

install(TARGETS tensor_order ARCHIVE DESTINATION lib)
install(DIRECTORY include/tensor_order DESTINATION include)
