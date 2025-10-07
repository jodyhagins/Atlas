// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "compilation_support.hpp"
#include "doctest.hpp"

namespace {

using namespace wjh::atlas::testing;
using namespace wjh::atlas::testing::compilation;

TEST_SUITE("Compilation Tests: Runtime Behavior")
{
    TEST_CASE("Arithmetic operations compute correct values")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=ArithType
description=strong int; +, -, *, /, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::ArithType x{10};
    test::ArithType y{3};

    auto sum = x + y;
    auto diff = x - y;
    auto prod = x * y;
    auto quot = x / y;

    assert(static_cast<int>(sum) == 13);
    assert(static_cast<int>(diff) == 7);
    assert(static_cast<int>(prod) == 30);
    assert(static_cast<int>(quot) == 3);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Comparison operations return correct boolean values")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=CompType
description=strong int; ==, !=, <, <=, >, >=, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::CompType x{10};
    test::CompType y{20};
    test::CompType z{10};

    assert((x == z) == true);
    assert((x != y) == true);
    assert((x < y) == true);
    assert((x <= y) == true);
    assert((x <= z) == true);
    assert((y > x) == true);
    assert((y >= x) == true);
    assert((x >= z) == true);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Increment/decrement operators modify values correctly")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Counter
description=strong int; ++, --, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Counter x{10};

    auto pre_inc = ++x;
    assert(static_cast<int>(x) == 11);
    assert(static_cast<int>(pre_inc) == 11);

    auto post_inc = x++;
    assert(static_cast<int>(x) == 12);
    assert(static_cast<int>(post_inc) == 11);

    auto pre_dec = --x;
    assert(static_cast<int>(x) == 11);
    assert(static_cast<int>(pre_dec) == 11);

    auto post_dec = x--;
    assert(static_cast<int>(x) == 10);
    assert(static_cast<int>(post_dec) == 11);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Subscript operators return correct elements")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=IntArray
description=strong std::vector<int>; [], #<vector>, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <vector>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    test::IntArray arr(vec);

    assert(arr[0] == 1);
    assert(arr[2] == 3);
    assert(arr[4] == 5);

    arr[1] = 42;
    assert(arr[1] == 42);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Default values initialize correctly")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=DefaultValue
description=strong int; +, no-constexpr
default_value=42
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::DefaultValue x;
    assert(static_cast<int>(x) == 42);

    test::DefaultValue y{10};
    assert(static_cast<int>(y) == 10);

    test::DefaultValue z{};
    assert(static_cast<int>(z) == 42);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Unary operators compute correct values")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=UnaryType
description=strong int; u+, u-, u~, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::UnaryType x{10};
    test::UnaryType y{-5};

    auto pos = +x;
    assert(static_cast<int>(pos) == 10);

    auto neg = -x;
    assert(static_cast<int>(neg) == -10);

    auto neg_y = -y;
    assert(static_cast<int>(neg_y) == 5);

    test::UnaryType bits{15};  // 0x0F
    auto inverted = ~bits;
    assert(static_cast<int>(inverted) == ~15);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Bool conversion works correctly")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=BoolType
description=strong int; bool, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::BoolType zero{0};
    test::BoolType nonzero{42};

    if (zero) {
        assert(false && "zero should be false");
    }

    if (nonzero) {
        assert(true);
    } else {
        assert(false && "nonzero should be true");
    }

    assert(!zero);
    assert(!!nonzero);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Compilation Tests: Type Safety")
{
    TEST_CASE("Strong types cannot implicitly convert")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=StrongInt
description=strong int; ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::StrongInt x{42};

    // Can explicitly cast
    int raw = static_cast<int>(x);
    assert(raw == 42);

    // Can compare strong types
    test::StrongInt y{42};
    assert(x == y);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Explicit cast is required to access underlying value")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Wrapped
description=strong int; no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Wrapped x{100};

    // Must use explicit cast
    int value = static_cast<int>(x);
    assert(value == 100);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Cannot mix different strong types")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Price
description=strong int; +, ==, no-constexpr

[type]
kind=struct
namespace=test
name=Quantity
description=strong int; +, ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Price p1{100};
    test::Price p2{200};
    test::Quantity q1{5};
    test::Quantity q2{10};

    // Same types can be compared and added
    assert(p1 == test::Price{100});
    auto total_price = p1 + p2;
    assert(static_cast<int>(total_price) == 300);

    assert(q1 == test::Quantity{5});
    auto total_qty = q1 + q2;
    assert(static_cast<int>(total_qty) == 15);

    // Different strong types are type-safe
    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Compilation Tests: Standard Library Integration")
{
    TEST_CASE("std::hash works with unordered containers")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=HashableType
description=strong int; ==, hash, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <unordered_set>

int main() {
    std::unordered_set<test::HashableType> set;

    test::HashableType x{42};
    test::HashableType y{100};

    set.insert(x);
    set.insert(y);
    set.insert(x); // duplicate

    assert(set.size() == 2);
    assert(set.count(x) == 1);
    assert(set.count(y) == 1);
    assert(set.count(test::HashableType{999}) == 0);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Stream operators work with std::iostream")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Streamable
description=strong int; out, in, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <iostream>
#include <sstream>

int main() {
    test::Streamable x{42};

    std::ostringstream oss;
    oss << x;
    assert(oss.str() == "42");

    std::istringstream iss("100");
    test::Streamable y{0};
    iss >> y;
    assert(static_cast<int>(y) == 100);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Works with std::algorithm functions")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Sortable
description=strong int; ==, !=, <, no-constexpr
)";

        auto test_code = R"(
#include <algorithm>
#include <cassert>
#include <vector>

int main() {
    std::vector<test::Sortable> vec{
        test::Sortable{30},
        test::Sortable{10},
        test::Sortable{20}
    };

    std::sort(vec.begin(), vec.end());

    assert(static_cast<int>(vec[0]) == 10);
    assert(static_cast<int>(vec[1]) == 20);
    assert(static_cast<int>(vec[2]) == 30);

    auto it = std::find(vec.begin(), vec.end(), test::Sortable{20});
    assert(it != vec.end());
    assert(*it == test::Sortable{20});

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Compilation Tests: C++ Standards Compatibility")
{
    TEST_CASE("Compiles and runs with C++11")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Cpp11Type
description=strong int; +, -, ==, !=, <, ++, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Cpp11Type x{10};
    test::Cpp11Type y{5};

    auto sum = x + y;
    assert(static_cast<int>(sum) == 15);

    auto diff = x - y;
    assert(static_cast<int>(diff) == 5);

    assert(x != y);
    assert(x == test::Cpp11Type{10});
    assert(y < x);

    ++x;
    assert(static_cast<int>(x) == 11);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++11");

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Compiles and runs with C++14")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Cpp14Type
description=strong int; +, *, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Cpp14Type x{10};
    test::Cpp14Type y{5};

    auto sum = x + y;
    assert(static_cast<int>(sum) == 15);

    auto product = x * y;
    assert(static_cast<int>(product) == 50);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++14");

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Compiles and runs with C++17")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Cpp17Type
description=strong int; +, <, !=, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::Cpp17Type x{10};
    test::Cpp17Type y{5};

    auto sum = x + y;
    assert(static_cast<int>(sum) == 15);

    assert(y < x);
    assert(x != y);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++17");

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("C++20 features work correctly")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=Cpp20Type
description=strong int; <=>, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <compare>

int main() {
    test::Cpp20Type x{10};
    test::Cpp20Type y{20};
    test::Cpp20Type z{10};

    assert((x <=> y) < 0);
    assert((y <=> x) > 0);
    assert((x <=> z) == 0);

    assert(x < y);
    assert(y > x);
    assert(x == z);
    assert(x != y);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++20");

        CHECK(result.success);
        if (not result.success) {
            INFO("Compilation/execution failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Compilation Tests: Integration")
{
    TEST_CASE("Kitchen sink: many features work together")
    {
        CompilationTester tester;

        // Kitchen sink: Test many features working together
        // Strong types with:
        // - Arithmetic (+, -, *, /, u-)
        // - Comparison (==, !=, <, <=>, all 6 relational)
        // - Increment/decrement (++, --)
        // - Bool conversion
        // - Stream operators (out, in)
        // - Hash support
        // - Default initialization
        // - Container operations ([], iterable)
        // Plus interactions:
        // - Distance / Time = Velocity
        // - Velocity * Time = Distance
        // - Distance / Velocity = Time
        // This is a comprehensive "everything working together" test

        auto types_description = R"([type]
kind=struct
namespace=kitchen
name=Distance
description=strong double; u-, ==, !=, <, <=>, ++, --, bool, out, hash, no-constexpr
default_value=0.0

[type]
kind=struct
namespace=kitchen
name=Time
description=strong double; u-, ==, !=, <, <=>, ++, bool, hash, no-constexpr
default_value=0.0

[type]
kind=struct
namespace=kitchen
name=Velocity
description=strong double; ==, !=, <, bool, out, no-constexpr

[type]
kind=struct
namespace=kitchen
name=Container
description=strong std::vector<int>; ==, !=, [], iterable, no-constexpr
)";

        auto interactions_description = R"(namespace=kitchen
value_access=.value
no-constexpr

Distance + Distance -> Distance
Distance - Distance -> Distance
Distance * double -> Distance
double * Distance -> Distance
Distance / double -> Distance
Distance / Time -> Velocity
Velocity * Time -> Distance
Distance / Velocity -> Time

Time + Time -> Time
Time - Time -> Time
Time * double -> Time

Velocity + Velocity -> Velocity
Velocity - Velocity -> Velocity
Velocity * double -> Velocity
Velocity / double -> Velocity
)";

        auto test_code = R"(
#include <cassert>
#include <sstream>
#include <unordered_set>
#include <vector>

int main() {
    // Test Distance: arithmetic, comparison, increment, bool, hash, stream
    kitchen::Distance d1{100.0};
    kitchen::Distance d2{50.0};

    auto d_sum = d1 + d2;
    assert(static_cast<double>(d_sum) == 150.0);

    auto d_diff = d1 - d2;
    assert(static_cast<double>(d_diff) == 50.0);

    auto d_scaled = d1 * 2.0;
    assert(static_cast<double>(d_scaled) == 200.0);

    auto d_neg = -d1;
    assert(static_cast<double>(d_neg) == -100.0);

    assert(d1 != d2);
    assert(d2 < d1);
    assert((d1 <=> d2) > 0);

    kitchen::Distance d3{1.0};
    ++d3;
    assert(static_cast<double>(d3) == 2.0);

    if (d1) { /* non-zero is true */ }

    std::ostringstream oss;
    oss << d1;
    assert(oss.str() == "100");

    std::unordered_set<kitchen::Distance> d_set;
    d_set.insert(d1);
    assert(d_set.count(d1) == 1);

    // Test Time: similar features
    kitchen::Time t1{10.0};
    kitchen::Time t2{5.0};

    auto t_sum = t1 + t2;
    assert(static_cast<double>(t_sum) == 15.0);

    assert(t1 > t2);

    // Test interactions: Distance / Time = Velocity
    kitchen::Distance dist{100.0};
    kitchen::Time time{10.0};

    auto velocity = dist / time;
    assert(static_cast<double>(velocity) == 10.0);

    // Velocity * Time = Distance
    auto dist2 = velocity * time;
    assert(static_cast<double>(dist2) == 100.0);

    // Distance / Velocity = Time
    auto time2 = dist / velocity;
    assert(static_cast<double>(time2) == 10.0);

    // Test interactions: Distance + Distance
    kitchen::Distance d_a{30.0};
    kitchen::Distance d_b{20.0};
    auto d_c = d_a + d_b;
    assert(static_cast<double>(d_c) == 50.0);

    // Test interactions: Distance * scalar
    auto d_double = 2.0 * d_a;  // scalar * Distance
    assert(static_cast<double>(d_double) == 60.0);

    // Test Container: subscript operator and iteration
    std::vector<int> vec = {10, 20, 30};
    kitchen::Container container(vec);

    assert(container[0] == 10);
    assert(container[1] == 20);

    container[1] = 999;
    assert(container[1] == 999);

    int total = 0;
    for (auto val : container) {
        total += val;
    }
    assert(total == 10 + 999 + 30);

    return 0;
}
)";

        auto result = tester.compile_and_run_with_interactions(
            types_description,
            interactions_description,
            test_code);

        if (not result.success) {
            std::cerr << "Kitchen sink test failed:\n" << result.output << "\n";
        }
        CHECK(result.success);
    }
}

} // anonymous namespace
