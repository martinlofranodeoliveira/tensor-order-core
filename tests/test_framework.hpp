#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdlib>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) os << ", ";
        os << vec[i];
    }
    return os << "]";
}

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_FALSE(cond) \
    do { \
        if (cond) { \
            std::cerr << "Assertion failed: !(" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::cerr << "Assertion failed: (" #a " == " #b ") [got " << (a) << " vs " << (b) << "] at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define ASSERT_THROWS(expr, ExcType) \
    do { \
        bool caught = false; \
        try { \
            (expr); \
        } catch (const ExcType&) { \
            caught = true; \
        } catch (...) { \
            std::cerr << "Caught wrong exception for " #expr " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
        if (!caught) { \
            std::cerr << "Expected exception " #ExcType " for " #expr " was not thrown at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)
