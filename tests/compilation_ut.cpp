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

    assert(atlas_value_for(sum) == 13);
    assert(atlas_value_for(diff) == 7);
    assert(atlas_value_for(prod) == 30);
    assert(atlas_value_for(quot) == 3);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(x) == 11);
    assert(atlas_value_for(pre_inc) == 11);

    auto post_inc = x++;
    assert(atlas_value_for(x) == 12);
    assert(atlas_value_for(post_inc) == 11);

    auto pre_dec = --x;
    assert(atlas_value_for(x) == 11);
    assert(atlas_value_for(pre_dec) == 11);

    auto post_dec = x--;
    assert(atlas_value_for(x) == 10);
    assert(atlas_value_for(post_dec) == 11);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(x) == 42);

    test::DefaultValue y{10};
    assert(atlas_value_for(y) == 10);

    test::DefaultValue z{};
    assert(atlas_value_for(z) == 42);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(pos) == 10);

    auto neg = -x;
    assert(atlas_value_for(neg) == -10);

    auto neg_y = -y;
    assert(atlas_value_for(neg_y) == 5);

    test::UnaryType bits{15};  // 0x0F
    auto inverted = ~bits;
    assert(atlas_value_for(inverted) == ~15);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

    // Can access underlying value via atlas_value_for()
    int raw = atlas_value_for(x);
    assert(raw == 42);

    // Can compare strong types
    test::StrongInt y{42};
    assert(x == y);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("atlas_value_for() is required to access underlying value")
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

    // Must use atlas_value_for() to access underlying value
    int value = atlas_value_for(x);
    assert(value == 100);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(total_price) == 300);

    assert(q1 == test::Quantity{5});
    auto total_qty = q1 + q2;
    assert(atlas_value_for(total_qty) == 15);

    // Different strong types are type-safe
    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(y) == 100);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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

    assert(atlas_value_for(vec[0]) == 10);
    assert(atlas_value_for(vec[1]) == 20);
    assert(atlas_value_for(vec[2]) == 30);

    auto it = std::find(vec.begin(), vec.end(), test::Sortable{20});
    assert(it != vec.end());
    assert(*it == test::Sortable{20});

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        INFO("Compilation/execution output:");
        INFO(result.output);
        CHECK(result.success);
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
    assert(atlas_value_for(sum) == 15);

    auto diff = x - y;
    assert(atlas_value_for(diff) == 5);

    assert(x != y);
    assert(x == test::Cpp11Type{10});
    assert(y < x);

    ++x;
    assert(atlas_value_for(x) == 11);

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
    assert(atlas_value_for(sum) == 15);

    auto product = x * y;
    assert(atlas_value_for(product) == 50);

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
    assert(atlas_value_for(sum) == 15);

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
    assert(atlas_value_for(d_sum) == 150.0);

    auto d_diff = d1 - d2;
    assert(atlas_value_for(d_diff) == 50.0);

    auto d_scaled = d1 * 2.0;
    assert(atlas_value_for(d_scaled) == 200.0);

    auto d_neg = -d1;
    assert(atlas_value_for(d_neg) == -100.0);

    assert(d1 != d2);
    assert(d2 < d1);
    assert((d1 <=> d2) > 0);

    kitchen::Distance d3{1.0};
    ++d3;
    assert(atlas_value_for(d3) == 2.0);

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
    assert(atlas_value_for(t_sum) == 15.0);

    assert(t1 > t2);

    // Test interactions: Distance / Time = Velocity
    kitchen::Distance dist{100.0};
    kitchen::Time time{10.0};

    auto velocity = dist / time;
    assert(atlas_value_for(velocity) == 10.0);

    // Velocity * Time = Distance
    auto dist2 = velocity * time;
    assert(atlas_value_for(dist2) == 100.0);

    // Distance / Velocity = Time
    auto time2 = dist / velocity;
    assert(atlas_value_for(time2) == 10.0);

    // Test interactions: Distance + Distance
    kitchen::Distance d_a{30.0};
    kitchen::Distance d_b{20.0};
    auto d_c = d_a + d_b;
    assert(atlas_value_for(d_c) == 50.0);

    // Test interactions: Distance * scalar
    auto d_double = 2.0 * d_a;  // scalar * Distance
    assert(atlas_value_for(d_double) == 60.0);

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

    TEST_CASE("Arrow operator forwards correctly for pointer types")
    {
        CompilationTester tester;

        auto description = R"(
# Test arrow operator forwarding with different pointer types

[IntPtrWrapper]
kind=struct
namespace=test
name=IntPtrWrapper
description=strong int*; ->, no-constexpr

[SharedPtrWrapper]
kind=struct
namespace=test
name=SharedPtrWrapper
description=strong std::shared_ptr<std::string>; ->, no-constexpr

[UniquePtrWrapper]
kind=struct
namespace=test
name=UniquePtrWrapper
description=strong std::unique_ptr<std::string>; ->, no-constexpr

[StringWrapper]
kind=struct
namespace=test
name=StringWrapper
description=strong std::string; ->, no-constexpr
)";

        auto test_code = R"(
#include <memory>
#include <cassert>
#include <string>

int main() {
    // Test 1: Raw pointer forwarding
    int value = 42;
    test::IntPtrWrapper ptr(&value);
    // For raw pointer to int, operator-> returns the pointer
    // C++ then applies -> again, giving us *ptr (the int)
    // So we can't test ptr->something, but we can verify it returns right type
    int* raw = ptr.operator->();
    assert(*raw == 42);

    // Test 2: shared_ptr forwarding
    auto str_ptr = std::make_shared<std::string>("Hello");
    test::SharedPtrWrapper shared(str_ptr);
    // This should forward to shared_ptr's operator->, giving us string*
    // Then we can access string memfns
    assert(shared->length() == 5);
    assert(shared->substr(0, 2) == "He");

    // Test 3: unique_ptr forwarding
    test::UniquePtrWrapper unique(std::make_unique<std::string>("World"));
    assert(unique->length() == 5);
    assert(unique->at(0) == 'W');

    // Test 4: Non-pointer type (addressof fallback)
    test::StringWrapper str("Testing");
    assert(str->length() == 7);
    assert(str->substr(0, 4) == "Test");

    return 0;
}
)";

        auto result = tester.compile_and_run(description, test_code);

        if (not result.success) {
            std::cerr << "Arrow operator forwarding test failed:\n"
                << result.output << "\n";
        }
        CHECK(result.success);
    }

    TEST_CASE("Dereference operator forwards correctly for pointer types")
    {
        CompilationTester tester;

        auto description = R"(
# Test dereference operator forwarding with different pointer types

[IntPtrWrapper]
kind=struct
namespace=test
name=IntPtrWrapper
description=strong int*; @, no-constexpr

[SharedPtrWrapper]
kind=struct
namespace=test
name=SharedPtrWrapper
description=strong std::shared_ptr<std::string>; @, no-constexpr

[UniquePtrWrapper]
kind=struct
namespace=test
name=UniquePtrWrapper
description=strong std::unique_ptr<std::string>; @, no-constexpr

[StringWrapper]
kind=struct
namespace=test
name=StringWrapper
description=strong std::string; @, no-constexpr
)";

        auto test_code = R"(
#include <memory>
#include <cassert>
#include <string>

int main() {
    // Test 1: Raw pointer dereferencing
    int value = 42;
    test::IntPtrWrapper ptr(&value);
    // *ptr should dereference the pointer, giving us an int&
    assert(*ptr == 42);
    *ptr = 100;
    assert(value == 100);

    // Test 2: shared_ptr dereferencing
    auto str_ptr = std::make_shared<std::string>("Hello");
    test::SharedPtrWrapper shared(str_ptr);
    // *shared should forward to shared_ptr's operator*, giving us string&
    assert(*shared == "Hello");
    *shared = "World";
    assert(*str_ptr == "World");

    // Test 3: unique_ptr dereferencing
    test::UniquePtrWrapper unique(std::make_unique<std::string>("Test"));
    assert(*unique == "Test");
    *unique += "ing";
    assert(*unique == "Testing");

    // Test 4: Non-pointer type (reference fallback)
    test::StringWrapper str("Original");
    // *str should return reference to the wrapped string
    assert(*str == "Original");
    *str = "Modified";
    assert(*str == "Modified");

    return 0;
}
)";

        auto result = tester.compile_and_run(description, test_code);

        if (not result.success) {
            std::cerr << "Dereference operator forwarding test failed:\n"
                << result.output << "\n";
        }
        CHECK(result.success);
    }

    TEST_CASE("Spaceship operator generates all comparisons in C++17")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=SpaceshipType
description=int; <=>, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <vector>
#include <algorithm>

int main() {
    test::SpaceshipType x{10};
    test::SpaceshipType y{20};
    test::SpaceshipType z{10};

    // Test all six comparison operators that should be generated
    assert((x == z) == true);
    assert((x != y) == true);
    assert((x < y) == true);
    assert((x <= y) == true);
    assert((x <= z) == true);
    assert((y > x) == true);
    assert((y >= x) == true);
    assert((x >= z) == true);

    // Test that type works with std::sort (requires < operator)
    std::vector<test::SpaceshipType> vec;
    vec.push_back(test::SpaceshipType{30});
    vec.push_back(test::SpaceshipType{10});
    vec.push_back(test::SpaceshipType{20});
    std::sort(vec.begin(), vec.end());

    assert(atlas_value_for(vec[0]) == 10);
    assert(atlas_value_for(vec[1]) == 20);
    assert(atlas_value_for(vec[2]) == 30);

    return 0;
}
)";
        // Test with C++17 to verify fallback path works
        auto result = tester.compile_and_run(description, test_code, "c++17");

        CHECK(result.success);
        if (not result.success) {
            INFO("Spaceship C++17 fallback test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Spaceship operator works in C++20 with native operator<=>")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=SpaceshipTypeCpp20
description=int; <=>, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <vector>
#include <algorithm>

int main() {
    test::SpaceshipTypeCpp20 x{10};
    test::SpaceshipTypeCpp20 y{20};
    test::SpaceshipTypeCpp20 z{10};

    // Test all comparison operators (synthesized from <=> in C++20)
    assert((x == z) == true);
    assert((x != y) == true);
    assert((x < y) == true);
    assert((x <= y) == true);
    assert((x <= z) == true);
    assert((y > x) == true);
    assert((y >= x) == true);
    assert((x >= z) == true);

    // Test that type works with std::sort
    std::vector<test::SpaceshipTypeCpp20> vec;
    vec.push_back(test::SpaceshipTypeCpp20{30});
    vec.push_back(test::SpaceshipTypeCpp20{10});
    vec.push_back(test::SpaceshipTypeCpp20{20});
    std::sort(vec.begin(), vec.end());

    assert(atlas_value_for(vec[0]) == 10);
    assert(atlas_value_for(vec[1]) == 20);
    assert(atlas_value_for(vec[2]) == 30);

    return 0;
}
)";
        // Test with C++20 to verify native spaceship path works
        auto result = tester.compile_and_run(description, test_code, "c++20");

        CHECK(result.success);
        if (not result.success) {
            INFO("Spaceship C++20 native test failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Memfn Forwarding: Runtime Behavior")
{
    TEST_CASE("Basic memfn forwarding works")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=StringWrapper
description=std::string; forward=size,empty,clear; ==, !=, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::StringWrapper str{"hello"};

    // Test size() forwarding
    assert(str.size() == 5);

    // Test empty() forwarding
    assert(!str.empty());

    // Test clear() forwarding - modifies the string
    str.clear();
    assert(str.empty());
    assert(str.size() == 0);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Basic memfn forwarding test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Memfn aliasing works")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=StringWithAlias
description=std::string; forward=size:length,empty:is_empty; ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::StringWithAlias str{"test"};

    // Test size aliased as length
    assert(str.length() == 4);

    // Test empty aliased as is_empty
    assert(!str.is_empty());

    test::StringWithAlias empty{""};
    assert(empty.is_empty());
    assert(empty.length() == 0);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Memfn aliasing test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Const-only forwarding works")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=StringConstOnly
description=std::string; forward=const,size,empty; ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::StringConstOnly const str{"hello"};

    // These should work on const object
    assert(str.size() == 5);
    assert(!str.empty());

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Const-only forwarding test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Container memfn forwarding works")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=IntVector
description=std::vector<int>; forward=push_back,pop_back,size,empty; ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::IntVector vec{std::vector<int>{}};

    assert(vec.empty());
    assert(vec.size() == 0);

    // Test push_back forwarding
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    assert(!vec.empty());
    assert(vec.size() == 3);

    // Test pop_back forwarding
    vec.pop_back();
    assert(vec.size() == 2);

    return 0;
}
)";
        auto test_code_with_includes = R"(
#include <vector>
)" + std::string(test_code);

        auto result = tester.compile_and_run(
            description,
            test_code_with_includes);

        CHECK(result.success);
        if (not result.success) {
            INFO("Container memfn forwarding test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Smart pointer memfn forwarding works")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=IntPtr
description=std::unique_ptr<int>; forward=get,reset; ->, @, bool, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <memory>

int main() {
    test::IntPtr ptr{std::make_unique<int>(42)};

    // Test get() forwarding
    assert(ptr.get() != nullptr);
    assert(*ptr.get() == 42);

    // Test arrow operator
    assert(*ptr == 42);

    // Test bool operator
    assert(static_cast<bool>(ptr));

    // Test reset() forwarding
    ptr.reset();
    assert(!static_cast<bool>(ptr));
    assert(ptr.get() == nullptr);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Smart pointer memfn forwarding test failed:");
            INFO(result.output);
        }
    }

    TEST_CASE(
        "C++23 deducing this forwarding works" *
        doctest::skip(
            not CompilationTester::is_cpp_standard_supported("c++23")))
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=StringCpp23
description=std::string; forward=size,empty; c++23; ==, no-constexpr
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::StringCpp23 str{"hello"};

    // Test that forwarded memfns work with C++23 deducing this
    assert(str.size() == 5);
    assert(!str.empty());

    // Test with const object
    test::StringCpp23 const const_str{"world"};
    assert(const_str.size() == 5);
    assert(!const_str.empty());

    return 0;
}
)";
        // Test with C++23 to verify deducing this path works
        auto result = tester.compile_and_run(description, test_code, "c++23");

        INFO("C++23 deducing this forwarding - result output:");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("Multiple forward lines work")
    {
        CompilationTester tester;

        auto description = R"([type]
kind=struct
namespace=test
name=MultiForward
description=std::string; ==, no-constexpr
forward=size,empty
forward=clear
)";

        auto test_code = R"(
#include <cassert>

int main() {
    test::MultiForward str{"test"};

    // Memfns from first forward= line
    assert(str.size() == 4);
    assert(!str.empty());

    // Memfn from second forward= line
    str.clear();
    assert(str.empty());

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Multiple forward lines test failed:");
            INFO(result.output);
        }
    }
}

TEST_SUITE("Compilation Tests: Drilling Behavior")
{
    TEST_CASE("Hash drills through nested strong types")
    {
        CompilationTester tester;

        // Inner wraps int (hashable), Outer wraps Inner
        // Hash should drill from Outer -> Inner -> int
        auto description = R"([type]
kind=struct
namespace=test
name=Inner
description=strong int; ==, hash, no-constexpr

[type]
kind=struct
namespace=test
name=Outer
description=strong test::Inner; ==, hash, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <unordered_set>

int main() {
    test::Inner inner1{42};
    test::Inner inner2{100};
    test::Outer outer1{inner1};
    test::Outer outer2{inner2};
    test::Outer outer3{inner1}; // same as outer1

    // Test that hash works - use in unordered_set
    std::unordered_set<test::Outer> set;
    set.insert(outer1);
    set.insert(outer2);
    set.insert(outer3); // duplicate of outer1

    assert(set.size() == 2);
    assert(set.count(outer1) == 1);
    assert(set.count(outer2) == 1);

    // Also verify Inner works directly
    std::unordered_set<test::Inner> inner_set;
    inner_set.insert(inner1);
    inner_set.insert(inner2);
    assert(inner_set.size() == 2);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Hash drilling through nested types failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Hash drills enum to underlying type")
    {
        CompilationTester tester;

        // First, write a header file defining the enum
        tester.write_temp_file(
            "color.hpp",
            "namespace test { enum class Color { Red, Green, Blue }; }\n");

        // Strong type wrapping an enum - hash should use underlying int
        // Use #"color.hpp" to include our custom header
        auto description = R"([type]
kind=struct
namespace=test
name=ColorWrapper
description=strong test::Color; ==, hash, no-constexpr, #"color.hpp"
)";

        auto test_code = R"(
#include <cassert>
#include <unordered_set>

int main() {
    test::ColorWrapper red{test::Color::Red};
    test::ColorWrapper green{test::Color::Green};
    test::ColorWrapper blue{test::Color::Blue};
    test::ColorWrapper red2{test::Color::Red}; // same as red

    std::unordered_set<test::ColorWrapper> set;
    set.insert(red);
    set.insert(green);
    set.insert(blue);
    set.insert(red2); // duplicate

    assert(set.size() == 3);
    assert(set.count(red) == 1);
    assert(set.count(green) == 1);
    assert(set.count(blue) == 1);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("Hash drilling enum to underlying type failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("OStream drills through nested strong types")
    {
        CompilationTester tester;

        // Inner wraps int (streamable), Outer wraps Inner
        // ostream should drill from Outer -> Inner -> int
        auto description = R"([type]
kind=struct
namespace=test
name=Inner
description=strong int; out, no-constexpr

[type]
kind=struct
namespace=test
name=Outer
description=strong test::Inner; out, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <sstream>

int main() {
    test::Inner inner{42};
    test::Outer outer{inner};

    std::ostringstream oss;
    oss << outer;

    // Should output "42" by drilling through both wrappers
    assert(oss.str() == "42");

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("OStream drilling through nested types failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("IStream drills through nested strong types")
    {
        CompilationTester tester;

        // Inner wraps int (streamable), Outer wraps Inner
        // istream should drill from Outer -> Inner -> int
        auto description = R"([type]
kind=struct
namespace=test
name=Inner
description=strong int; in, no-constexpr

[type]
kind=struct
namespace=test
name=Outer
description=strong test::Inner; in, no-constexpr
)";

        auto test_code = R"(
#include <cassert>
#include <sstream>

int main() {
    test::Outer outer{test::Inner{0}};

    std::istringstream iss{"42"};
    iss >> outer;

    // Should read "42" by drilling through both wrappers
    assert(atlas_value_for(atlas_value_for(outer)) == 42);

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code);

        CHECK(result.success);
        if (not result.success) {
            INFO("IStream drilling through nested types failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Formatter drills through nested strong types")
    {
        CompilationTester tester;

        // Inner wraps int (formattable), Outer wraps Inner
        // formatter should drill from Outer -> Inner -> int
        auto description = R"([type]
kind=struct
namespace=test
name=Inner
description=strong int; fmt, no-constexpr, c++20

[type]
kind=struct
namespace=test
name=Outer
description=strong test::Inner; fmt, no-constexpr, c++20
)";

        auto test_code = R"(
#include <cassert>
#include <format>
#include <string>

int main() {
    test::Inner inner{42};
    test::Outer outer{inner};

    // Should format as "42" by drilling through both wrappers
    std::string result = std::format("{}", outer);
    assert(result == "42");

    // Also test Inner directly
    std::string inner_result = std::format("{}", inner);
    assert(inner_result == "42");

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++20");

        CHECK(result.success);
        if (not result.success) {
            INFO("Formatter drilling through nested types failed:");
            INFO(result.output);
        }
    }

    TEST_CASE("Formatter drills enum to underlying type")
    {
        CompilationTester tester;

        // First, write a header file defining the enum
        tester.write_temp_file(
            "format_color.hpp",
            "namespace test { enum class Color { Red = 1, Green = 2, Blue = 3 }; }\n");

        // Strong type wrapping an enum - formatter should use underlying int
        auto description = R"([type]
kind=struct
namespace=test
name=ColorWrapper
description=strong test::Color; fmt, no-constexpr, c++20, #"format_color.hpp"
)";

        auto test_code = R"(
#include <cassert>
#include <format>
#include <string>

int main() {
    test::ColorWrapper red{test::Color::Red};
    test::ColorWrapper green{test::Color::Green};
    test::ColorWrapper blue{test::Color::Blue};

    // Should format as the underlying int values
    assert(std::format("{}", red) == "1");
    assert(std::format("{}", green) == "2");
    assert(std::format("{}", blue) == "3");

    return 0;
}
)";
        auto result = tester.compile_and_run(description, test_code, "c++20");

        CHECK(result.success);
        if (not result.success) {
            INFO("Formatter drilling enum to underlying type failed:");
            INFO(result.output);
        }
    }
}

} // anonymous namespace
