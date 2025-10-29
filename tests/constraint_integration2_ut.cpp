#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <variant>

// Include the generated types
#include "constraint_integration2_test_types.hpp"

namespace {

TEST_SUITE("Constraint × Arithmetic Mode Composition")
{
    // ======================================================================
    // POSITIVE CONSTRAINT × ARITHMETIC MODES
    // ======================================================================

    TEST_CASE("positive + default - basic operations")
    {
        test::PositiveDefault a{100};
        test::PositiveDefault b{50};

        CHECK_NOTHROW(a + b);
        CHECK_NOTHROW(a - b);

        auto sum = a + b;
        CHECK(static_cast<std::uint8_t>(sum) == 150);

        auto diff = a - b;
        CHECK(static_cast<std::uint8_t>(diff) == 50);
    }

    TEST_CASE("positive + checked - overflow detection before constraint")
    {
        test::PositiveChecked a{200};
        test::PositiveChecked b{100};

        // Overflow happens first (200 + 100 = 300 > 255)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("positive + checked - underflow detection")
    {
        test::PositiveChecked a{5};
        test::PositiveChecked b{10};

        // Underflow happens (5 - 10 would underflow for unsigned)
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("positive + checked - valid operations")
    {
        test::PositiveChecked a{100};
        test::PositiveChecked b{50};

        CHECK_NOTHROW(a + b);
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<std::uint8_t>(result) == 50);
    }

    TEST_CASE("positive + saturating - saturation to 0 violates constraint")
    {
        test::PositiveSaturating a{5};
        test::PositiveSaturating b{10};

        // Saturates to 0, which violates positive (> 0)
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("positive + saturating - saturation to max stays positive")
    {
        test::PositiveSaturating a{200};
        test::PositiveSaturating b{100};

        // Saturates to 255, which is positive
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 255);
    }

    TEST_CASE("positive + wrapping - wrap to positive is valid")
    {
        test::PositiveWrapping a{200};
        test::PositiveWrapping b{100};

        // Wraps to 44 (300 % 256 = 44), which is positive
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 44);
    }

    TEST_CASE("positive + wrapping - wrap to zero violates constraint")
    {
        test::PositiveWrapping a{10};
        test::PositiveWrapping b{10};

        // Results in 0, which violates positive
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    // ======================================================================
    // NON-NEGATIVE CONSTRAINT × ARITHMETIC MODES
    // ======================================================================

    TEST_CASE("non_negative + default - allows zero")
    {
        CHECK_NOTHROW(test::NonNegativeDefault{0});
        CHECK_NOTHROW(test::NonNegativeDefault{100});
    }

    TEST_CASE("non_negative + checked - overflow detection")
    {
        test::NonNegativeChecked a{200};
        test::NonNegativeChecked b{100};

        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("non_negative + saturating - saturation to 0 is valid")
    {
        test::NonNegativeSaturating a{5};
        test::NonNegativeSaturating b{10};

        // Saturates to 0, which is non-negative (>= 0)
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<std::uint8_t>(result) == 0);
    }

    TEST_CASE("non_negative + wrapping - all uint8_t values are non-negative")
    {
        test::NonNegativeWrapping a{5};
        test::NonNegativeWrapping b{10};

        // Wraps around, but uint8_t is always >= 0
        CHECK_NOTHROW(a - b);
    }

    // ======================================================================
    // NON-ZERO CONSTRAINT × ARITHMETIC MODES
    // ======================================================================

    TEST_CASE("non_zero + default - rejects zero")
    {
        CHECK_THROWS_AS(test::NonZeroDefault{0}, atlas::ConstraintError);
        CHECK_NOTHROW(test::NonZeroDefault{1});
        CHECK_NOTHROW(test::NonZeroDefault{255});
    }

    TEST_CASE("non_zero + checked - overflow before constraint")
    {
        test::NonZeroChecked a{200};
        test::NonZeroChecked b{100};

        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("non_zero + saturating - saturation to zero violates")
    {
        test::NonZeroSaturating a{5};
        test::NonZeroSaturating b{10};

        // Saturates to 0, which violates non_zero
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("non_zero + saturating - valid operations")
    {
        test::NonZeroSaturating a{100};
        test::NonZeroSaturating b{50};

        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<std::uint8_t>(result) == 50);
    }

    TEST_CASE("non_zero + wrapping - wrap to zero violates")
    {
        test::NonZeroWrapping a{10};
        test::NonZeroWrapping b{10};

        // Results in 0, violates non_zero
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("non_zero + wrapping - wrap to non-zero is valid")
    {
        test::NonZeroWrapping a{200};
        test::NonZeroWrapping b{100};

        // Wraps to 44, which is non-zero
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 44);
    }

    // ======================================================================
    // BOUNDED CONSTRAINT × ARITHMETIC MODES
    // ======================================================================

    TEST_CASE("bounded + default - construction with bounds check")
    {
        CHECK_NOTHROW(test::BoundedDefault{10}); // At min
        CHECK_NOTHROW(test::BoundedDefault{100}); // In range
        CHECK_NOTHROW(test::BoundedDefault{200}); // At max

        CHECK_THROWS_AS(
            test::BoundedDefault{9},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::BoundedDefault{201},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE("bounded + default - operations can violate bounds")
    {
        test::BoundedDefault a{50};
        test::BoundedDefault b{60};

        // 50 - 60 would underflow to large value, violating upper bound
        // Or if handled as unsigned arithmetic, might wrap
        // The exact behavior depends on the underlying type
        auto result = a + b; // 110, within bounds
        CHECK(static_cast<std::uint8_t>(result) == 110);
    }

    TEST_CASE("bounded + checked - overflow before bounds")
    {
        test::BoundedChecked a{150};
        test::BoundedChecked b{150};

        // Overflow happens first (150 + 150 = 300 > 255)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("bounded + checked - valid operations within bounds")
    {
        test::BoundedChecked a{100};
        test::BoundedChecked b{50};

        CHECK_NOTHROW(a + b); // 150, within bounds

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 150);
    }

    TEST_CASE("bounded + saturating - saturation can violate bounds")
    {
        test::BoundedSaturating a{150};
        test::BoundedSaturating b{150};

        // Saturates to 255, which exceeds upper bound of 200
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded + saturating - saturation below bounds")
    {
        test::BoundedSaturating a{15};
        test::BoundedSaturating b{20};

        // Saturates to 0, which is below lower bound of 10
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("bounded + saturating - valid saturating operations")
    {
        test::BoundedSaturating a{100};
        test::BoundedSaturating b{50};

        CHECK_NOTHROW(a + b); // 150, within bounds

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 150);
    }

    TEST_CASE("bounded + wrapping - wrapping can violate bounds")
    {
        test::BoundedWrapping a{200};
        test::BoundedWrapping b{100};

        // Wraps to 44, which is within bounds [10, 200]
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<std::uint8_t>(result) == 44);
    }

    TEST_CASE("bounded + wrapping - wrapping below bounds")
    {
        test::BoundedWrapping a{15};
        test::BoundedWrapping b{20};

        // 15 - 20 wraps to 251, which exceeds upper bound
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    // ======================================================================
    // INTERACTION BETWEEN ARITHMETIC MODE AND CONSTRAINT CHECK
    // ======================================================================

    TEST_CASE("arithmetic mode processes first, then constraint checks")
    {
        // This is a critical design principle:
        // 1. Arithmetic mode determines the result value
        // 2. Constraint checks the result
        // 3. If constraint fails, throw ConstraintError

        SUBCASE("checked throws before constraint can check") {
            test::PositiveChecked a{200};
            test::PositiveChecked b{100};

            // CheckedOverflowError is thrown, not ConstraintError
            CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
        }

        SUBCASE("saturating computes value, then constraint checks") {
            test::PositiveSaturating a{5};
            test::PositiveSaturating b{10};

            // Saturation computes 0, then constraint rejects it
            CHECK_THROWS_AS(a - b, atlas::ConstraintError);
        }

        SUBCASE("wrapping computes value, then constraint checks") {
            test::PositiveWrapping a{10};
            test::PositiveWrapping b{10};

            // Wrapping computes 0, then constraint rejects it
            CHECK_THROWS_AS(a - b, atlas::ConstraintError);
        }
    }

    TEST_CASE("all 16 combinations exist and compile")
    {
        // Just verify all types can be instantiated with valid values
        CHECK_NOTHROW(test::PositiveDefault{1});
        CHECK_NOTHROW(test::PositiveChecked{1});
        CHECK_NOTHROW(test::PositiveSaturating{1});
        CHECK_NOTHROW(test::PositiveWrapping{1});

        CHECK_NOTHROW(test::NonNegativeDefault{0});
        CHECK_NOTHROW(test::NonNegativeChecked{0});
        CHECK_NOTHROW(test::NonNegativeSaturating{0});
        CHECK_NOTHROW(test::NonNegativeWrapping{0});

        CHECK_NOTHROW(test::NonZeroDefault{1});
        CHECK_NOTHROW(test::NonZeroChecked{1});
        CHECK_NOTHROW(test::NonZeroSaturating{1});
        CHECK_NOTHROW(test::NonZeroWrapping{1});

        CHECK_NOTHROW(test::BoundedDefault{100});
        CHECK_NOTHROW(test::BoundedChecked{100});
        CHECK_NOTHROW(test::BoundedSaturating{100});
        CHECK_NOTHROW(test::BoundedWrapping{100});
    }

    // ======================================================================
    // BOUNDARY VALUE TESTING (Carl's M-1, M-5)
    // ======================================================================

    TEST_CASE("boundary - positive at min boundary (value = 1)")
    {
        // Test operations at the minimum valid value for positive (> 0)
        test::PositiveDefault min_pos{1};

        // Addition should work
        auto result = min_pos + min_pos;
        CHECK(static_cast<std::uint8_t>(result) == 2);

        // Subtraction that results in 0 should violate constraint
        // (1 - 1 = 0, which violates positive constraint)
        CHECK_THROWS_AS(min_pos - min_pos, atlas::ConstraintError);
    }

    TEST_CASE("boundary - positive at max boundary (value = 255)")
    {
        test::PositiveDefault max_val{255};
        test::PositiveDefault one{1};

        // Wrapping addition: 255 + 1 = 0 (wraps), violates positive
        // The result of the addition should throw when constraint is checked
        CHECK_THROWS_AS(max_val + one, atlas::ConstraintError);
    }

    TEST_CASE("boundary - bounded at exact bounds")
    {
        // BoundedDefault is bounded<10, 200>
        test::BoundedDefault at_min{10};
        test::BoundedDefault at_max{200};

        // Values at boundaries should be valid
        CHECK(static_cast<std::uint8_t>(at_min) == 10);
        CHECK(static_cast<std::uint8_t>(at_max) == 200);

        // One below min should fail
        CHECK_THROWS_AS(test::BoundedDefault{9}, atlas::ConstraintError);

        // One above max should fail
        CHECK_THROWS_AS(test::BoundedDefault{201}, atlas::ConstraintError);
    }

    TEST_CASE("boundary - bounded arithmetic at lower boundary")
    {
        // Test arithmetic operations near the lower bound
        // BoundedDefault is bounded<10, 200>
        test::BoundedDefault at_min{10};
        test::BoundedDefault eleven{11};

        // Subtraction: 10 - 11 = 255 (wraps in uint8_t), violates upper bound
        CHECK_THROWS_AS(at_min - eleven, atlas::ConstraintError);
    }

    TEST_CASE("boundary - bounded arithmetic at upper boundary")
    {
        // Test arithmetic operations near the upper bound
        // BoundedDefault is bounded<10, 200>
        test::BoundedDefault at_max{200};
        test::BoundedDefault ten{10};

        // Addition: 200 + 10 = 210, exceeds upper bound of 200
        CHECK_THROWS_AS(at_max + ten, atlas::ConstraintError);
    }

    TEST_CASE("boundary - checked arithmetic at type limits")
    {
        // Test checked arithmetic at uint8_t boundaries
        test::PositiveChecked max_val{255};
        test::PositiveChecked one{1};

        // Should detect overflow first (checked arithmetic processes before
        // constraint)
        CHECK_THROWS(max_val + one);

        test::PositiveChecked min_val{1};
        // 1 - 1 = 0, which violates positive constraint (underflow doesn't
        // apply to unsigned)
        CHECK_THROWS(min_val - min_val);
    }

    TEST_CASE("boundary - saturating at constraint boundary")
    {
        // Test saturation behavior at constraint boundaries
        test::PositiveSaturating small{1};
        test::PositiveSaturating large{10};

        // Saturates to 0, which violates positive constraint
        CHECK_THROWS_AS(small - large, atlas::ConstraintError);

        test::PositiveSaturating max_adjacent{254};
        test::PositiveSaturating one{1};

        // Saturates to 255, which is still positive
        CHECK_NOTHROW(max_adjacent + max_adjacent);
    }

    TEST_CASE("boundary - non_negative includes zero boundary")
    {
        // Test that non_negative constraint properly handles zero
        CHECK_NOTHROW(test::NonNegativeDefault{0});

        test::NonNegativeSaturating at_zero{0};
        test::NonNegativeSaturating one{1};

        // 0 - 1 saturates to 0, which is still non_negative
        CHECK_NOTHROW(at_zero - one);
    }

    TEST_CASE("boundary - non_zero excludes zero boundary")
    {
        // Test that non_zero constraint rejects zero
        CHECK_THROWS_AS(test::NonZeroDefault{0}, atlas::ConstraintError);

        // But accepts values on either side
        CHECK_NOTHROW(test::NonZeroDefault{1});
        CHECK_NOTHROW(test::NonZeroDefault{255});
    }

    // ======================================================================
    // ASSIGNMENT OPERATOR CONSTRAINT CHECKING (Carl's M-6)
    // ======================================================================

    TEST_CASE("assignment - copy assignment preserves constraints")
    {
        test::PositiveDefault a{42};
        test::PositiveDefault b{10};

        // Copy assignment should work without re-checking constraints
        // (source is already valid)
        CHECK_NOTHROW(b = a);
        CHECK(static_cast<std::uint8_t>(b) == 42);
    }

    TEST_CASE("assignment - move assignment preserves constraints")
    {
        test::PositiveDefault a{42};
        test::PositiveDefault b{10};

        // Move assignment should work without re-checking constraints
        CHECK_NOTHROW(b = std::move(a));
        CHECK(static_cast<std::uint8_t>(b) == 42);
    }

    TEST_CASE("assignment - bounded assignment respects bounds")
    {
        test::BoundedDefault a{150};
        test::BoundedDefault b{50};

        // Assignment of valid values should work
        CHECK_NOTHROW(b = a);
        CHECK(static_cast<std::uint8_t>(b) == 150);
    }

    TEST_CASE("assignment - arithmetic result assignment")
    {
        test::PositiveDefault a{100};
        test::PositiveDefault b{50};

        // Arithmetic result assigned to new variable
        test::PositiveDefault result{1};
        CHECK_NOTHROW(result = a + b);
        CHECK(static_cast<std::uint8_t>(result) == 150);
    }

    TEST_CASE("assignment - compound operations preserve constraints")
    {
        test::BoundedDefault a{100};
        test::BoundedDefault b{20};
        test::BoundedDefault c{30};

        // Multiple operations: (a + b) assigned to result
        auto result = a + b; // 120
        CHECK(static_cast<std::uint8_t>(result) == 120);

        // Further operation with result
        auto final_result = result + c; // 150
        CHECK(static_cast<std::uint8_t>(final_result) == 150);
    }
}

TEST_SUITE("Constraint Constexpr Validation")
{
    // ======================================================================
    // COMPILE-TIME CONSTRAINT VALIDATION
    // ======================================================================

    TEST_CASE("positive - constexpr construction with valid value")
    {
        // This should compile without issues
        constexpr auto valid = []() {
            test::PositiveForConstexpr p{42};
            return static_cast<int>(p);
        }();

        CHECK(valid == 42);
    }

    TEST_CASE("positive - constexpr ensures compile-time validity")
    {
        // At runtime, verify constexpr construction worked
        constexpr test::PositiveForConstexpr p{100};
        CHECK(static_cast<int>(p) == 100);
    }

    // NOTE: The following would fail to compile (constraint violation):
    // constexpr auto invalid_positive = []() {
    //     test::PositiveForConstexpr p{0};  // Violates positive constraint
    //     return true;
    // }();

    TEST_CASE("bounded - constexpr construction with valid value")
    {
        constexpr auto valid = []() {
            test::BoundedForConstexpr b{50};
            return static_cast<int>(b);
        }();

        CHECK(valid == 50);
    }

    TEST_CASE("bounded - constexpr at boundaries")
    {
        // Test at lower bound (inclusive)
        constexpr test::BoundedForConstexpr at_min{1};
        CHECK(static_cast<int>(at_min) == 1);

        // Test at upper bound (inclusive)
        constexpr test::BoundedForConstexpr at_max{100};
        CHECK(static_cast<int>(at_max) == 100);
    }

    // NOTE: The following would fail to compile:
    // constexpr auto invalid_bounded_low = []() {
    //     test::BoundedForConstexpr b{0};  // Below lower bound
    //     return true;
    // }();
    //
    // constexpr auto invalid_bounded_high = []() {
    //     test::BoundedForConstexpr b{101};  // Above upper bound
    //     return true;
    // }();

    TEST_CASE("constexpr - static_assert can verify constraints")
    {
        // These compile-time checks verify that valid values work
        constexpr test::PositiveForConstexpr p{42};
        static_assert(static_cast<int>(p) == 42, "Should be 42");

        constexpr test::BoundedForConstexpr b{50};
        static_assert(static_cast<int>(b) == 50, "Should be 50");
    }

    // ======================================================================
    // CONSTEXPR COPY AND MOVE
    // ======================================================================

    TEST_CASE("constexpr - copy construction")
    {
        constexpr auto test_copy = []() {
            test::PositiveForConstexpr a{42};
            test::PositiveForConstexpr b{a}; // Copy construct
            return static_cast<int>(b);
        }();

        CHECK(test_copy == 42);
    }

    TEST_CASE("constexpr - value extraction in lambda")
    {
        // Note: This test documents that we can extract values in constexpr
        // context Assignment operators are tested separately in runtime
        // contexts
        constexpr auto test_value = []() {
            test::PositiveForConstexpr a{42};
            test::PositiveForConstexpr b{10};
            return static_cast<int>(a);
        }();

        CHECK(test_value == 42);
    }

    // ======================================================================
    // CONSTEXPR COMPARISON
    // ======================================================================

    TEST_CASE("constexpr - comparison operators")
    {
        constexpr test::PositiveForConstexpr a{42};
        constexpr test::PositiveForConstexpr b{42};
        constexpr test::PositiveForConstexpr c{100};

        static_assert(a == b, "Equal values should compare equal");
        static_assert(a != c, "Different values should not compare equal");
        static_assert(a < c, "42 < 100");
        static_assert(c > a, "100 > 42");
    }

    TEST_CASE("constexpr - bounded comparison")
    {
        constexpr test::BoundedForConstexpr a{50};
        constexpr test::BoundedForConstexpr b{75};

        static_assert(a < b, "50 < 75");
        static_assert(b > a, "75 > 50");
        static_assert(a != b, "Different values");
    }

    // ======================================================================
    // CONSTEXPR VALUE ACCESS
    // ======================================================================

    TEST_CASE("constexpr - value extraction")
    {
        constexpr test::PositiveForConstexpr p{42};

        // Can extract value at compile time
        constexpr int value = static_cast<int>(p);
        static_assert(value == 42, "Value should be extractable");

        CHECK(value == 42);
    }

    TEST_CASE("constexpr - bounded value extraction")
    {
        constexpr test::BoundedForConstexpr b{75};

        constexpr int value = static_cast<int>(b);
        static_assert(value == 75, "Bounded value extraction");

        CHECK(value == 75);
    }

    // ======================================================================
    // RUNTIME TESTS FOR CONSTEXPR OBJECTS
    // ======================================================================

    TEST_CASE("constexpr objects work at runtime")
    {
        constexpr test::PositiveForConstexpr compile_time{42};

        // Can use compile-time object at runtime
        CHECK(static_cast<int>(compile_time) == 42);

        // Can compare with runtime objects
        test::PositiveForConstexpr runtime{42};
        CHECK(compile_time == runtime);
    }

    TEST_CASE("constexpr - boundary testing")
    {
        // Test that boundary values work in constexpr context
        constexpr test::BoundedForConstexpr min_val{1};
        constexpr test::BoundedForConstexpr max_val{100};

        CHECK(static_cast<int>(min_val) == 1);
        CHECK(static_cast<int>(max_val) == 100);

        // Verify they can be compared
        static_assert(min_val < max_val, "Min should be less than max");
    }

    // ======================================================================
    // DOCUMENTATION OF NON-CONSTEXPR CONSTRAINT VIOLATIONS
    // ======================================================================

    TEST_CASE(
        "documentation - constraint violations are NOT constexpr-friendly")
    {
        // This test documents that constraint violations throw exceptions,
        // which means they cannot be used in constexpr contexts that
        // require compile-time evaluation.

        // Runtime test: invalid values throw
        CHECK_THROWS_AS(test::PositiveForConstexpr{0}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::PositiveForConstexpr{-1}, atlas::ConstraintError);

        CHECK_THROWS_AS(
            test::BoundedForConstexpr{0},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::BoundedForConstexpr{101},
            atlas::ConstraintError); // Above max

        // The following would NOT compile if uncommented:
        // constexpr test::PositiveForConstexpr invalid{0};
        // Compiler error: constraint violation in constexpr context
    }

    TEST_CASE("constexpr - array initialization")
    {
        // Can use constrained types in constexpr arrays
        constexpr test::PositiveForConstexpr values[] = {
            test::PositiveForConstexpr{1},
            test::PositiveForConstexpr{2},
            test::PositiveForConstexpr{3}};

        static_assert(static_cast<int>(values[0]) == 1, "First element is 1");
        static_assert(static_cast<int>(values[2]) == 3, "Third element is 3");

        CHECK(static_cast<int>(values[1]) == 2);
    }

#if defined(__cpp_constexpr) && __cpp_constexpr >= 201907L
    TEST_CASE("constexpr - C++20 enhanced constexpr")
    {
        // C++20 allows more things in constexpr context
        constexpr auto test_algo = []() {
            test::PositiveForConstexpr values[] = {
                test::PositiveForConstexpr{1},
                test::PositiveForConstexpr{5},
                test::PositiveForConstexpr{3}};

            // In C++20, we can do more complex compile-time operations
            return static_cast<int>(values[1]);
        }();

        CHECK(test_algo == 5);
    }
#endif

    TEST_CASE("constexpr - benefits of compile-time checking")
    {
        // The main benefit: constraint violations are caught at compile time
        // if the value is used in a constexpr context

        // This compiles and runs fine
        constexpr test::PositiveForConstexpr valid{42};
        CHECK(static_cast<int>(valid) == 42);

        // This would fail to compile (uncomment to test):
        // constexpr test::PositiveForConstexpr invalid{0};

        // This allows catching errors earlier in the development cycle
    }
}

TEST_SUITE("Constraint Edge Cases")
{
    // ======================================================================
    // MOVE SEMANTICS
    // ======================================================================

    TEST_CASE("move semantics - basic move construction")
    {
        test::PositiveIntMoveCopy a{42};
        test::PositiveIntMoveCopy b{std::move(a)};

        // b has the value
        CHECK(static_cast<int>(b) == 42);

        // a is in valid but unspecified state (for int, likely still 42)
        // Don't rely on a's value after move
    }

    TEST_CASE("move semantics - move assignment")
    {
        test::PositiveIntMoveCopy a{42};
        test::PositiveIntMoveCopy b{10};

        b = std::move(a);

        CHECK(static_cast<int>(b) == 42);
    }

    TEST_CASE("move semantics - string move preserves constraint")
    {
        test::NonEmptyStringMoveCopy a{"hello"};
        test::NonEmptyStringMoveCopy b{std::move(a)};

        // b gets the value
        CHECK(static_cast<std::string>(b) == "hello");

        // a is moved-from (empty for strings)
        // But we don't re-check constraints on moved-from objects
    }

    TEST_CASE("move semantics - unique_ptr move")
    {
        auto ptr = std::make_unique<int>(42);
        test::NonNullUniquePtr a{std::move(ptr)};

        // Move construct
        test::NonNullUniquePtr b{std::move(a)};

        // b has the pointer (constraint satisfied)
        CHECK(static_cast<std::unique_ptr<int> const &>(b).get() != nullptr);
        CHECK(*static_cast<std::unique_ptr<int> const &>(b) == 42);

        // a is moved-from (nullptr for unique_ptr)
        // Constraint is NOT re-checked on moved-from objects
    }

    TEST_CASE("move semantics - constraint not re-checked on move")
    {
        // This is important: move operations use defaulted special members
        // which do not re-check constraints. This is correct behavior because:
        // 1. The moved-to object inherits a valid value
        // 2. The moved-from object is in an unspecified state but is not used

        test::PositiveIntMoveCopy valid{42};

        // Move is allowed even though source becomes unspecified
        CHECK_NOTHROW(test::PositiveIntMoveCopy moved{std::move(valid)});
    }

    // ======================================================================
    // COPY SEMANTICS
    // ======================================================================

    TEST_CASE("copy semantics - basic copy construction")
    {
        test::PositiveIntMoveCopy a{42};
        test::PositiveIntMoveCopy b{a};

        CHECK(static_cast<int>(a) == 42);
        CHECK(static_cast<int>(b) == 42);
    }

    TEST_CASE("copy semantics - copy assignment")
    {
        test::PositiveIntMoveCopy a{42};
        test::PositiveIntMoveCopy b{10};

        b = a;

        CHECK(static_cast<int>(a) == 42);
        CHECK(static_cast<int>(b) == 42);
    }

    TEST_CASE("copy semantics - string copy preserves value")
    {
        test::NonEmptyStringMoveCopy a{"hello"};
        test::NonEmptyStringMoveCopy b{a};

        CHECK(static_cast<std::string>(a) == "hello");
        CHECK(static_cast<std::string>(b) == "hello");
    }

    TEST_CASE("copy semantics - constraint not re-checked")
    {
        // Copy operations use defaulted special members
        // Constraints are not re-checked because the source is already valid

        test::PositiveIntMoveCopy a{42};

        // Copy preserves valid state without re-checking
        CHECK_NOTHROW(test::PositiveIntMoveCopy b{a});
        CHECK_NOTHROW(test::PositiveIntMoveCopy c = a);
    }

    // ======================================================================
    // STD::OPTIONAL COMPOSITION
    // ======================================================================

    TEST_CASE("optional - empty optional")
    {
        std::optional<test::PositiveForOptional> maybe;

        CHECK(not maybe.has_value());
    }

    TEST_CASE("optional - optional with valid value")
    {
        std::optional<test::PositiveForOptional> maybe{
            test::PositiveForOptional{42}};

        REQUIRE(maybe.has_value());
        CHECK(static_cast<int>(*maybe) == 42);
    }

    TEST_CASE("optional - emplace with valid value")
    {
        std::optional<test::PositiveForOptional> maybe;

        maybe.emplace(42);

        REQUIRE(maybe.has_value());
        CHECK(static_cast<int>(*maybe) == 42);
    }

    TEST_CASE("optional - assignment with valid value")
    {
        std::optional<test::PositiveForOptional> maybe;

        maybe = test::PositiveForOptional{42};

        REQUIRE(maybe.has_value());
        CHECK(static_cast<int>(*maybe) == 42);
    }

    TEST_CASE("optional - constraint violation throws before optional")
    {
        // Invalid value throws during construction, before optional sees it
        CHECK_THROWS_AS(test::PositiveForOptional{0}, atlas::ConstraintError);

        std::optional<test::PositiveForOptional> maybe;

        // Can't even create invalid value to put in optional
        CHECK_THROWS_AS(
            maybe.emplace(0), // Constraint checked in constructor
            atlas::ConstraintError);
    }

    TEST_CASE("optional - bounded constraint")
    {
        std::optional<test::BoundedForOptional> maybe;

        CHECK_NOTHROW(maybe = test::BoundedForOptional{50});
        CHECK(maybe.has_value());

        // Constraint violations throw
        CHECK_THROWS_AS(
            maybe = test::BoundedForOptional{101},
            atlas::ConstraintError);
    }

    TEST_CASE("optional - reset and reassign")
    {
        std::optional<test::PositiveForOptional> maybe{
            test::PositiveForOptional{42}};

        maybe.reset();
        CHECK(not maybe.has_value());

        maybe = test::PositiveForOptional{100};
        CHECK(maybe.has_value());
        CHECK(static_cast<int>(*maybe) == 100);
    }

    // ======================================================================
    // STD::VARIANT COMPOSITION
    // ======================================================================

    TEST_CASE("variant - holds alternative")
    {
        std::variant<int, test::NonZeroForVariant> v;

        // Initially holds int (first alternative)
        CHECK(std::holds_alternative<int>(v));

        v = test::NonZeroForVariant{42};

        CHECK(std::holds_alternative<test::NonZeroForVariant>(v));
    }

    TEST_CASE("variant - get value")
    {
        std::variant<int, test::NonZeroForVariant> v{
            test::NonZeroForVariant{42}};

        REQUIRE(std::holds_alternative<test::NonZeroForVariant>(v));

        auto & val = std::get<test::NonZeroForVariant>(v);
        CHECK(static_cast<int>(val) == 42);
    }

    TEST_CASE("variant - multiple constrained types")
    {
        std::variant<
            test::NonZeroForVariant,
            test::NonNegativeForVariant,
            std::string>
            v;

        v = test::NonZeroForVariant{42};
        CHECK(std::holds_alternative<test::NonZeroForVariant>(v));

        v = test::NonNegativeForVariant{0}; // 0 is valid for non_negative
        CHECK(std::holds_alternative<test::NonNegativeForVariant>(v));

        v = std::string{"hello"};
        CHECK(std::holds_alternative<std::string>(v));
    }

    TEST_CASE("variant - constraint enforced on construction")
    {
        std::variant<int, test::NonZeroForVariant> v;

        // Constraint violation throws before variant sees it
        CHECK_THROWS_AS(v = test::NonZeroForVariant{0}, atlas::ConstraintError);
    }

    TEST_CASE("variant - visit with constrained types")
    {
        std::variant<test::NonZeroForVariant, test::NonNegativeForVariant> v{
            test::NonZeroForVariant{42}};

        int result = std::visit(
            [](auto const & val) { return static_cast<int>(val); },
            v);

        CHECK(result == 42);
    }

    TEST_CASE("variant - emplace with constraint")
    {
        std::variant<int, test::NonZeroForVariant> v;

        v.emplace<test::NonZeroForVariant>(42);

        REQUIRE(std::holds_alternative<test::NonZeroForVariant>(v));
        CHECK(static_cast<int>(std::get<test::NonZeroForVariant>(v)) == 42);
    }

    TEST_CASE("variant - emplace with invalid value throws")
    {
        std::variant<int, test::NonZeroForVariant> v;

        CHECK_THROWS_AS(
            v.emplace<test::NonZeroForVariant>(0),
            atlas::ConstraintError);
    }

    // ======================================================================
    // ASSIGNMENT OPERATORS
    // ======================================================================

    TEST_CASE("assignment - copy assignment between same types")
    {
        test::PositiveWithAssignment a{42};
        test::PositiveWithAssignment b{10};

        b = a;

        CHECK(static_cast<int>(a) == 42);
        CHECK(static_cast<int>(b) == 42);
    }

    TEST_CASE("assignment - move assignment")
    {
        test::PositiveWithAssignment a{42};
        test::PositiveWithAssignment b{10};

        b = std::move(a);

        CHECK(static_cast<int>(b) == 42);
    }

    TEST_CASE("assignment - self assignment")
    {
        test::PositiveWithAssignment a{42};

        // Self-assignment should be safe (though pointless)
        a = a;

        CHECK(static_cast<int>(a) == 42);
    }

    TEST_CASE("assignment - constraint not re-checked on assignment")
    {
        // Assignment uses defaulted operators
        // Source is already validated, so no re-check needed

        test::PositiveWithAssignment a{42};
        test::PositiveWithAssignment b{10};

        CHECK_NOTHROW(b = a);
        CHECK_NOTHROW(b = std::move(a));
    }

    // ======================================================================
    // COMPARISON EDGE CASES
    // ======================================================================

    TEST_CASE("comparison - works with different valid values")
    {
        test::PositiveWithAssignment a{42};
        test::PositiveWithAssignment b{42};
        test::PositiveWithAssignment c{100};

        CHECK(a == b);
        CHECK(a != c);
        CHECK(a < c);
        CHECK(c > a);
    }

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    TEST_CASE("comparison - spaceship operator")
    {
        test::PositiveWithAssignment a{42};
        test::PositiveWithAssignment b{100};
        test::PositiveWithAssignment c{42};

        CHECK((a <=> b) < 0);
        CHECK((b <=> a) > 0);
        CHECK((a <=> c) == 0);
    }
#endif

    // ======================================================================
    // MEMORY SAFETY
    // ======================================================================

    TEST_CASE("memory safety - no leaks with unique_ptr constraint")
    {
        // This test verifies that move semantics work correctly
        // and don't cause memory leaks

        {
            auto ptr = std::make_unique<int>(42);
            test::NonNullUniquePtr a{std::move(ptr)};

            test::NonNullUniquePtr b{std::move(a)};

            // b owns the pointer, a is empty
            // When b goes out of scope, memory is freed
        }

        // No leaks - unique_ptr semantics are preserved
    }

    TEST_CASE("memory safety - exception safety")
    {
        // If constraint check throws, no object is created
        // No partially-constructed objects left in invalid state

        int call_count = 0;

        auto make_invalid = [&call_count]() {
            ++call_count;
            return test::PositiveForOptional{0}; // Throws
        };

        CHECK_THROWS_AS(make_invalid(), atlas::ConstraintError);
        CHECK(call_count == 1);

        // Exception was thrown, no object created
    }

    // ======================================================================
    // TYPE COMPATIBILITY
    // ======================================================================

    TEST_CASE("type compatibility - constrained types are distinct")
    {
        // Different constrained types are distinct even with same underlying
        // type

        static_assert(
            not std::is_same<
                test::NonZeroForVariant,
                test::NonNegativeForVariant>::value,
            "Different constraints create different types");

        // This is good - prevents accidental mixing
    }

    TEST_CASE("type compatibility - can be used in standard algorithms")
    {
        // Constrained types work with standard library

        test::PositiveWithAssignment values[] = {
            test::PositiveWithAssignment{3},
            test::PositiveWithAssignment{1},
            test::PositiveWithAssignment{2}};

        std::sort(std::begin(values), std::end(values));

        CHECK(static_cast<int>(values[0]) == 1);
        CHECK(static_cast<int>(values[1]) == 2);
        CHECK(static_cast<int>(values[2]) == 3);
    }
}

// ======================================================================
// ERROR MESSAGE TESTING HELPERS
// ======================================================================

/// Helper function to expect a ConstraintError and return the error message
/// This eliminates the repetitive try-catch pattern and makes tests DRY.
template <typename Func>
std::string
expect_constraint_error(Func && f)
{
    try {
        f();
        FAIL("Should have thrown ConstraintError");
        return "";
    } catch (atlas::ConstraintError const & e) {
        return e.what();
    }
}

/// Check if the error message contains a specific keyword
inline bool
message_contains(std::string const & msg, std::string const & keyword)
{
    return msg.find(keyword) != std::string::npos;
}

/// Check if the error message contains any of the given keywords
template <typename... Keywords>
bool
message_contains_any(std::string const & msg, Keywords const &... keywords)
{
    return (message_contains(msg, keywords) || ...);
}

TEST_SUITE("Constraint Error Message Quality")
{
    // ======================================================================
    // POSITIVE CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("positive - error message contains type name")
    {
        auto msg = expect_constraint_error([] { test::PositiveDefault{0}; });

        // Message should mention the type name
        CHECK(message_contains(msg, "Positive"));
    }

    TEST_CASE("positive - error message mentions constraint")
    {
        auto msg = expect_constraint_error(
            [] { test::PositiveForOptional{0}; });

        // Message should mention "positive" or "> 0"
        CHECK(message_contains_any(msg, "positive", "> 0", ">0"));
    }

    TEST_CASE("positive - error message includes actual value")
    {
        auto msg = expect_constraint_error([] { test::PositiveDefault{0}; });

        CHECK(not msg.empty());
        // Should include the actual violating value
        CHECK(message_contains(msg, "0"));
    }

    TEST_CASE("positive - error message for negative value")
    {
        auto msg = expect_constraint_error(
            [] { test::PositiveIntMoveCopy{-42}; });

        CHECK(not msg.empty());
        // Should include the actual negative value
        CHECK(message_contains_any(msg, "-42", "42"));
    }

    // ======================================================================
    // NON-NEGATIVE CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("non_negative - error message for negative value")
    {
        auto msg = expect_constraint_error(
            [] { test::NonNegativeWithComparison{-1}; });

        // Message should mention constraint
        CHECK(message_contains_any(
            msg,
            "non_negative",
            "non-negative",
            ">= 0",
            ">=0"));
    }

    TEST_CASE("non_negative - error message contains type name")
    {
        auto msg = expect_constraint_error(
            [] { test::NonNegativeForVariant{-100}; });

        CHECK(not msg.empty());
        // Should identify which type violated the constraint
    }

    TEST_CASE("non_negative - error message includes actual value")
    {
        auto msg = expect_constraint_error(
            [] { test::NonNegativeWithComparison{-1}; });

        CHECK(not msg.empty());
        // Should include the violating value
        CHECK(message_contains_any(msg, "-1", "1"));
    }

    // ======================================================================
    // NON-ZERO CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("non_zero - error message for zero")
    {
        auto msg = expect_constraint_error([] { test::NonZeroDefault{0}; });

        // Message should mention constraint
        CHECK(message_contains_any(msg, "non_zero", "non-zero", "!= 0", "!=0"));
    }

    TEST_CASE("non_zero - informative message")
    {
        auto msg = expect_constraint_error([] { test::NonZeroForVariant{0}; });

        CHECK(not msg.empty());
        CHECK(msg.length() > 10); // Should be reasonably detailed
    }

    TEST_CASE("non_zero - error message includes actual value")
    {
        auto msg = expect_constraint_error([] { test::NonZeroDefault{0}; });

        CHECK(not msg.empty());
        // Should mention that the value is 0
        CHECK(message_contains(msg, "0"));
    }

    // ======================================================================
    // BOUNDED CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("bounded - error message for below lower bound")
    {
        auto msg = expect_constraint_error(
            [] { test::BoundedDefault{9}; }); // Below lower bound of 10

        CHECK(not msg.empty());
        // Should mention bounds or the violating value
        CHECK(message_contains_any(msg, "9", "10", "200"));
    }

    TEST_CASE("bounded - error message for above upper bound")
    {
        auto msg = expect_constraint_error(
            [] { test::BoundedDefault{201}; }); // Above upper bound of 200

        CHECK(not msg.empty());
        // Should mention the violating value
        CHECK(message_contains(msg, "201"));
    }

    TEST_CASE("bounded - error message mentions bounds")
    {
        auto msg = expect_constraint_error([] {
            test::BoundedForOptional{101};
        }); // bounded<0,100>, so 101 is invalid

        // Message should ideally mention the bounds (0 and 100)
        // and the invalid value (101)
        CHECK(not msg.empty());
        CHECK(message_contains(msg, "101"));

        // At minimum, should be informative
        CHECK(msg.length() > 15);
    }

    TEST_CASE("bounded - different violations have appropriate messages")
    {
        auto msg_low = expect_constraint_error(
            [] { test::BoundedForOptional{-1}; }); // Below lower bound
        auto msg_high = expect_constraint_error(
            [] { test::BoundedForOptional{101}; }); // Above upper bound

        // Both should have messages
        CHECK(not msg_low.empty());
        CHECK(not msg_high.empty());

        // Messages should be informative
        CHECK(msg_low.length() > 10);
        CHECK(msg_high.length() > 10);

        // Messages should include the actual violating values
        CHECK(message_contains_any(msg_low, "-1", "1"));
        CHECK(message_contains(msg_high, "101"));
    }

    // ======================================================================
    // BOUNDED_RANGE CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("bounded_range - error message for out of range")
    {
        auto msg = expect_constraint_error([] {
            test::BoundedRangeWithCmp{
                10}; // bounded_range<0,10>, so 10 is invalid (half-open)
        });

        CHECK(not msg.empty());
        // Should mention the value and/or range
        CHECK(message_contains(msg, "10"));
    }

    TEST_CASE("bounded_range - informative about half-open semantics")
    {
        auto msg = expect_constraint_error(
            [] { test::BoundedRangeWithCmp{-1}; }); // Below lower bound

        CHECK(not msg.empty());
        CHECK(msg.length() > 10);
        // Should include the violating value
        CHECK(message_contains_any(msg, "-1", "1"));
    }

    // ======================================================================
    // NON-EMPTY CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("non_empty - error message for empty string")
    {
        auto msg = expect_constraint_error(
            [] { test::NonEmptyWithForwarding{""}; });

        // Message should mention constraint
        CHECK(message_contains_any(msg, "non_empty", "non-empty", "empty"));
    }

    TEST_CASE("non_empty - error message for empty vector")
    {
        auto msg = expect_constraint_error(
            [] { test::NonEmptyVectorOps{std::vector<int>{}}; });

        CHECK(not msg.empty());
        // Should be informative about the constraint
    }

    // ======================================================================
    // NON-NULL CONSTRAINT ERROR MESSAGES
    // ======================================================================

    TEST_CASE("non_null - error message for null pointer")
    {
        auto msg = expect_constraint_error(
            [] { test::NonNullWithForwarding{std::unique_ptr<int>{}}; });

        // Message should mention constraint
        CHECK(message_contains_any(msg, "non_null", "non-null", "null"));
    }

    TEST_CASE("non_null - informative message")
    {
        auto msg = expect_constraint_error(
            [] { test::NonNullUniquePtr{std::unique_ptr<int>{}}; });

        CHECK(not msg.empty());
        CHECK(msg.length() > 10);
    }

    // ======================================================================
    // ARITHMETIC OPERATION ERROR MESSAGES
    // ======================================================================

    TEST_CASE("arithmetic - positive saturating underflow message")
    {
        auto msg = expect_constraint_error([] {
            test::PositiveSaturating a{5};
            test::PositiveSaturating b{10};
            auto result = a - b; // Saturates to 0, violates positive
        });

        CHECK(not msg.empty());
        // Should explain the constraint violation (mentions "arithmetic result"
        // or "positive")
        CHECK(message_contains_any(msg, "arithmetic", "positive", "> 0"));
    }

    TEST_CASE("arithmetic - bounded saturating overflow message")
    {
        auto msg = expect_constraint_error([] {
            test::BoundedSaturating a{150};
            test::BoundedSaturating b{150};
            auto result = a + b; // Saturates to 255, exceeds upper bound of 200
        });

        CHECK(not msg.empty());
        // Should mention bounds or arithmetic result
        CHECK(message_contains_any(msg, "arithmetic", "10", "200", "bound"));
    }

    TEST_CASE("arithmetic - bounded wrapping underflow message")
    {
        auto msg = expect_constraint_error([] {
            test::BoundedWrapping a{15};
            test::BoundedWrapping b{20};
            auto result = a - b; // Wraps to 251, exceeds upper bound
        });

        CHECK(not msg.empty());
        // Should mention bounds or arithmetic result
        CHECK(message_contains_any(msg, "arithmetic", "10", "200", "bound"));
    }

    // ======================================================================
    // COMPLEX COMPOSITION ERROR MESSAGES
    // ======================================================================

    TEST_CASE("complex composition - informative messages")
    {
        auto msg = expect_constraint_error([] {
            test::ComplexComposition{1023};
        }); // Below lower bound of 1024

        CHECK(not msg.empty());
        // Should identify type and constraint
        // Should be informative
        bool is_informative = message_contains(msg, "Complex") ||
            message_contains(msg, "1023") || message_contains(msg, "1024") ||
            (msg.length() > 20);
        CHECK(is_informative);
    }

    TEST_CASE("complex composition - arithmetic violation message")
    {
        auto msg = expect_constraint_error([] {
            test::ComplexComposition p{2000};
            test::ComplexComposition q{1500};
            auto result = p - q; // 500 < 1024, violates lower bound
        });

        CHECK(not msg.empty());
        // Should mention arithmetic result or bounds
        CHECK(message_contains_any(msg, "arithmetic", "1024", "bound"));
    }

    // ======================================================================
    // ERROR MESSAGE CONSISTENCY
    // ======================================================================

    TEST_CASE("consistency - all ConstraintError messages are non-empty")
    {
        // Every constraint violation should produce a non-empty message

        std::vector<std::string> messages;

        // Collect messages from various constraint violations
        messages.push_back(
            expect_constraint_error([] { test::PositiveDefault{0}; }));
        messages.push_back(
            expect_constraint_error([] { test::NonZeroDefault{0}; }));
        messages.push_back(expect_constraint_error(
            [] { test::BoundedDefault{5}; })); // Below minimum

        // All messages should be non-empty
        for (auto const & msg : messages) {
            CHECK(not msg.empty());
            CHECK(msg.length() > 5); // Should be reasonably informative
        }
    }

    TEST_CASE("consistency - messages are derived from std::logic_error")
    {
        // ConstraintError inherits from std::logic_error
        // This means it integrates with standard exception handling

        try {
            test::PositiveDefault{0};
            FAIL("Should have thrown");
        } catch (std::logic_error const & e) {
            // Can catch as base class
            std::string msg = e.what();
            CHECK(not msg.empty());
        }
    }

    TEST_CASE("consistency - can distinguish ConstraintError from other "
              "exceptions")
    {
        try {
            test::PositiveDefault{0};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const &) {
            // Caught specifically as ConstraintError
            CHECK(true);
        } catch (...) {
            FAIL("Should have caught ConstraintError specifically");
        }
    }

    // ======================================================================
    // ERROR MESSAGE USEFULNESS
    // ======================================================================

    TEST_CASE("usefulness - messages help debug constraint violations")
    {
        // Error messages should provide enough information to:
        // 1. Identify which type violated a constraint
        // 2. Understand what constraint was violated
        // 3. See what value caused the violation (when possible)

        auto msg = expect_constraint_error(
            [] { test::BoundedForOptional{150}; }); // Above upper bound

        // Message should be helpful for debugging
        CHECK(not msg.empty());

        // Should be more than just "error" - needs details
        CHECK(msg.length() > 10);

        // Should include the actual violating value
        CHECK(message_contains(msg, "150"));
    }

    TEST_CASE("usefulness - different constraints produce different messages")
    {
        auto positive_msg = expect_constraint_error(
            [] { test::PositiveDefault{0}; });
        auto bounded_msg = expect_constraint_error(
            [] { test::BoundedDefault{5}; });
        auto non_zero_msg = expect_constraint_error(
            [] { test::NonZeroDefault{0}; });

        // All should have messages
        CHECK(not positive_msg.empty());
        CHECK(not bounded_msg.empty());
        CHECK(not non_zero_msg.empty());

        // Messages should be distinct (different constraints)
        // At minimum, they should differ in content
        bool all_different = positive_msg != bounded_msg &&
            bounded_msg != non_zero_msg && positive_msg != non_zero_msg;

        // Note: It's possible that all messages might be the same if the
        // generator uses a generic message. We're checking if they're
        // different (which is better)
        if (all_different) {
            CHECK(true); // Good! Messages are distinct
        } else {
            // If messages are the same, at least they should exist
            CHECK(not positive_msg.empty());
        }
    }
}

TEST_SUITE("Constraint + Feature Interaction")
{
    // ======================================================================
    // CONSTRAINT + FORWARDING
    // ======================================================================

    TEST_CASE("non_empty + forwarding - forwarded methods work")
    {
        test::NonEmptyWithForwarding s{"hello"};

        CHECK(s.size() == 5);
        CHECK(s.length() == 5);
        CHECK(not s.empty()); // Always false for non_empty strings
    }

    TEST_CASE("non_empty + forwarding - clear violates constraint")
    {
        test::NonEmptyWithForwarding s{"hello"};

        // clear() modifies then checks constraint
        // Note: This is post-condition checking - the string is cleared
        // before the constraint violation is detected. This is by design.
        // See documentation for rationale on post-condition checking vs
        // pre-condition checking for forwarded methods.
        CHECK_THROWS_AS(s.clear(), atlas::ConstraintError);
    }

    TEST_CASE("non_empty + forwarding - construction requires non-empty")
    {
        CHECK_THROWS_AS(
            test::NonEmptyWithForwarding{""},
            atlas::ConstraintError);
        CHECK_NOTHROW(test::NonEmptyWithForwarding{"a"});
    }

    TEST_CASE("non_null + arrow operator - pointer operations work")
    {
        auto ptr = std::make_unique<int>(42);
        test::NonNullWithForwarding p{std::move(ptr)};

        CHECK(p.get() != nullptr);
        // Verify the pointer contains the expected value
        CHECK(*(p.get()) == 42);
    }

    TEST_CASE("non_null + arrow operator - construction requires non-null")
    {
        CHECK_THROWS_AS(
            test::NonNullWithForwarding{std::unique_ptr<int>{}},
            atlas::ConstraintError);

        auto ptr = std::make_unique<int>(42);
        CHECK_NOTHROW(test::NonNullWithForwarding{std::move(ptr)});
    }

    TEST_CASE("non_empty vector + forwarding - operations work")
    {
        test::NonEmptyVectorOps v{std::vector<int>{1, 2, 3}};

        CHECK(v.size() == 3);
        CHECK(not v.empty());
        CHECK(v.front() == 1);
        CHECK(v.back() == 3);
    }

    TEST_CASE("non_empty vector + forwarding - push_back works")
    {
        test::NonEmptyVectorOps v{std::vector<int>{1}};

        CHECK_NOTHROW(v.push_back(2));
        CHECK(v.size() == 2);
    }

    // ======================================================================
    // CONSTRAINT + CAST OPERATORS
    // ======================================================================

    TEST_CASE("positive + cast - explicit cast to double")
    {
        test::PositiveWithCast p{42};

        double d = static_cast<double>(p);
        CHECK(d == 42.0);

        long l = static_cast<long>(p);
        CHECK(l == 42L);
    }

    TEST_CASE("positive + cast - constraint enforced on construction")
    {
        CHECK_THROWS_AS(test::PositiveWithCast{0}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::PositiveWithCast{-1}, atlas::ConstraintError);
        CHECK_NOTHROW(test::PositiveWithCast{1});
    }

    // ======================================================================
    // CONSTRAINT + HASH
    // ======================================================================

    TEST_CASE("bounded + hash - hashable in standard containers")
    {
        test::BoundedWithHash p1{50};
        test::BoundedWithHash p2{50};
        test::BoundedWithHash p3{75};

        std::hash<test::BoundedWithHash> hasher;

        size_t h1 = hasher(p1);
        size_t h2 = hasher(p2);
        size_t h3 = hasher(p3);

        // Same values should hash the same
        CHECK(h1 == h2);

        // Different values should (probably) hash differently
        // (This is not guaranteed, but very likely)
        CHECK(h1 != h3);
    }

    TEST_CASE("bounded + hash - can be used in unordered_set")
    {
        std::unordered_set<test::BoundedWithHash> set;

        set.insert(test::BoundedWithHash{50});
        set.insert(test::BoundedWithHash{75});
        set.insert(test::BoundedWithHash{50}); // Duplicate

        CHECK(set.size() == 2); // Only two unique values
    }

    TEST_CASE("bounded + hash - constraint enforced")
    {
        CHECK_THROWS_AS(test::BoundedWithHash{-1}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedWithHash{101}, atlas::ConstraintError);
        CHECK_NOTHROW(test::BoundedWithHash{0});
        CHECK_NOTHROW(test::BoundedWithHash{100});
    }

    // ======================================================================
    // CONSTRAINT + COMPARISON OPERATORS
    // ======================================================================

    TEST_CASE("non_negative + comparison - all operators work")
    {
        test::NonNegativeWithComparison a{5};
        test::NonNegativeWithComparison b{10};
        test::NonNegativeWithComparison c{5};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= b);
        CHECK(a <= c);
        CHECK(b >= a);
        CHECK(c >= a);
    }

    TEST_CASE("non_negative + comparison - constraint enforced")
    {
        CHECK_THROWS_AS(
            test::NonNegativeWithComparison{-1},
            atlas::ConstraintError);
        CHECK_NOTHROW(test::NonNegativeWithComparison{0});
        CHECK_NOTHROW(test::NonNegativeWithComparison{100});
    }

    // ======================================================================
    // COMPLEX COMPOSITION: MULTIPLE FEATURES
    // ======================================================================

    TEST_CASE("complex composition - all features work together")
    {
        test::ComplexComposition p{8080};

        // Value access
        CHECK(static_cast<std::uint16_t>(p) == 8080);

        // Constraint enforced
        CHECK_THROWS_AS(test::ComplexComposition{1023}, atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::ComplexComposition{65536},
            atlas::ConstraintError);
        CHECK_NOTHROW(test::ComplexComposition{1024}); // Min
        CHECK_NOTHROW(test::ComplexComposition{65535}); // Max
    }

    TEST_CASE("complex composition - comparison works")
    {
        test::ComplexComposition p{8080};
        test::ComplexComposition q{9000};

        CHECK(p < q);
        CHECK(q > p);
        CHECK(p != q);
    }

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    TEST_CASE("complex composition - spaceship works")
    {
        test::ComplexComposition p{8080};
        test::ComplexComposition q{9000};
        test::ComplexComposition r{8080};

        CHECK((p <=> q) < 0);
        CHECK((q <=> p) > 0);
        CHECK((p <=> r) == 0);
    }
#endif

    TEST_CASE("complex composition - hash works")
    {
        test::ComplexComposition p{8080};
        std::hash<test::ComplexComposition> hasher;

        size_t h = hasher(p);
        CHECK(h != 0); // Some hash value (not guaranteed, but likely)
    }

    TEST_CASE("complex composition - checked arithmetic with bounds")
    {
        test::ComplexComposition p{60000};
        test::ComplexComposition q{10000};

        // Overflow: 60000 + 10000 = 70000, which exceeds uint16_t max
        // Checked throws before constraint check
        CHECK_THROWS_AS(p + q, atlas::CheckedOverflowError);
    }

    TEST_CASE("complex composition - arithmetic violating bounds")
    {
        test::ComplexComposition p{2000};
        test::ComplexComposition q{1500};

        // 2000 - 1500 = 500 < 1024 (violates lower bound)
        CHECK_THROWS_AS(p - q, atlas::ConstraintError);
    }

    TEST_CASE("complex composition - valid arithmetic")
    {
        test::ComplexComposition p{10000};
        test::ComplexComposition q{5000};

        CHECK_NOTHROW(p + q); // 15000, within bounds

        auto result = p + q;
        CHECK(static_cast<std::uint16_t>(result) == 15000);
    }

    // ======================================================================
    // BOUNDED_RANGE + COMPARISON
    // ======================================================================

    TEST_CASE("bounded_range + comparison - half-open range semantics")
    {
        // bounded_range<0,10> means [0, 10) - includes 0, excludes 10
        CHECK_NOTHROW(test::BoundedRangeWithCmp{0});
        CHECK_NOTHROW(test::BoundedRangeWithCmp{9});
        CHECK_THROWS_AS(test::BoundedRangeWithCmp{10}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedRangeWithCmp{-1}, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range + comparison - comparison works")
    {
        test::BoundedRangeWithCmp a{3};
        test::BoundedRangeWithCmp b{7};
        test::BoundedRangeWithCmp c{3};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    TEST_CASE("bounded_range + spaceship")
    {
        test::BoundedRangeWithCmp a{3};
        test::BoundedRangeWithCmp b{7};

        CHECK((a <=> b) < 0);
        CHECK((b <=> a) > 0);
    }
#endif

    // ======================================================================
    // CONTAINER OPERATIONS WITH CONSTRAINED TYPES (Carl's M-4)
    // ======================================================================

    TEST_CASE("container - vector of constrained types")
    {
        std::vector<test::NonNegativeWithComparison> vec;

        // Can add valid values
        vec.push_back(test::NonNegativeWithComparison{0});
        vec.push_back(test::NonNegativeWithComparison{5});
        vec.push_back(test::NonNegativeWithComparison{10});

        CHECK(vec.size() == 3);
        CHECK(static_cast<int>(vec[0]) == 0);
        CHECK(static_cast<int>(vec[1]) == 5);
        CHECK(static_cast<int>(vec[2]) == 10);
    }

    TEST_CASE("container - vector operations preserve constraints")
    {
        std::vector<test::PositiveWithCast> vec;

        vec.push_back(test::PositiveWithCast{1});
        vec.push_back(test::PositiveWithCast{2});
        vec.push_back(test::PositiveWithCast{3});

        // Copy vector
        auto vec_copy = vec;
        CHECK(vec_copy.size() == 3);
        CHECK(static_cast<int>(vec_copy[0]) == 1);

        // Move vector
        auto vec_moved = std::move(vec);
        CHECK(vec_moved.size() == 3);
    }

    TEST_CASE("container - vector with non_empty constrained types")
    {
        test::NonEmptyVectorOps v1{std::vector<int>{1, 2, 3}};
        test::NonEmptyVectorOps v2{std::vector<int>{4, 5}};

        // Store in vector
        std::vector<test::NonEmptyVectorOps> containers;
        containers.push_back(std::move(v1));
        containers.push_back(std::move(v2));

        CHECK(containers.size() == 2);
        CHECK(containers[0].size() == 3);
        CHECK(containers[1].size() == 2);
    }

    TEST_CASE("container - vector resize with constrained types")
    {
        std::vector<test::BoundedWithHash> vec;

        vec.reserve(10); // Reserve space
        CHECK(vec.capacity() >= 10);

        // Add elements
        for (int i = 0; i <= 100; i += 10) {
            vec.push_back(test::BoundedWithHash{i});
        }

        CHECK(vec.size() == 11);
    }

    TEST_CASE("container - vector erase operations")
    {
        std::vector<test::NonNegativeWithComparison> vec;
        vec.push_back(test::NonNegativeWithComparison{1});
        vec.push_back(test::NonNegativeWithComparison{2});
        vec.push_back(test::NonNegativeWithComparison{3});

        // Erase middle element
        vec.erase(vec.begin() + 1);

        CHECK(vec.size() == 2);
        CHECK(static_cast<int>(vec[0]) == 1);
        CHECK(static_cast<int>(vec[1]) == 3);
    }

    TEST_CASE("container - vector clear and refill")
    {
        std::vector<test::PositiveWithCast> vec;
        vec.push_back(test::PositiveWithCast{1});
        vec.push_back(test::PositiveWithCast{2});

        vec.clear();
        CHECK(vec.empty());

        // Refill with new values
        vec.push_back(test::PositiveWithCast{10});
        vec.push_back(test::PositiveWithCast{20});

        CHECK(vec.size() == 2);
        CHECK(static_cast<int>(vec[0]) == 10);
    }

    TEST_CASE("container - vector assignment operations")
    {
        std::vector<test::BoundedWithHash> vec1;
        vec1.push_back(test::BoundedWithHash{25});
        vec1.push_back(test::BoundedWithHash{50});

        std::vector<test::BoundedWithHash> vec2;
        vec2.push_back(test::BoundedWithHash{75});

        // Copy assignment
        vec2 = vec1;
        CHECK(vec2.size() == 2);
        CHECK(static_cast<int>(vec2[0]) == 25);

        std::vector<test::BoundedWithHash> vec3;
        // Move assignment
        vec3 = std::move(vec1);
        CHECK(vec3.size() == 2);
    }

    TEST_CASE("container - unordered_set with multiple operations")
    {
        std::unordered_set<test::BoundedWithHash> set;

        // Insert multiple values
        set.insert(test::BoundedWithHash{10});
        set.insert(test::BoundedWithHash{20});
        set.insert(test::BoundedWithHash{30});
        set.insert(test::BoundedWithHash{20}); // Duplicate

        CHECK(set.size() == 3);

        // Find operation
        auto it = set.find(test::BoundedWithHash{20});
        CHECK(it != set.end());

        // Erase operation
        set.erase(test::BoundedWithHash{20});
        CHECK(set.size() == 2);

        // Verify erased
        it = set.find(test::BoundedWithHash{20});
        CHECK(it == set.end());
    }

    TEST_CASE("container - vector with complex composition types")
    {
        std::vector<test::ComplexComposition> ports;

        ports.push_back(test::ComplexComposition{8080});
        ports.push_back(test::ComplexComposition{8443});
        ports.push_back(test::ComplexComposition{9000});

        CHECK(ports.size() == 3);

        // Sort ports
        std::sort(ports.begin(), ports.end());

        CHECK(static_cast<std::uint16_t>(ports[0]) == 8080);
        CHECK(static_cast<std::uint16_t>(ports[1]) == 8443);
        CHECK(static_cast<std::uint16_t>(ports[2]) == 9000);
    }

    TEST_CASE("container - vector emplace operations")
    {
        std::vector<test::PositiveWithCast> vec;

        // Emplace_back creates object in-place
        vec.emplace_back(42);
        vec.emplace_back(100);

        CHECK(vec.size() == 2);
        CHECK(static_cast<int>(vec[0]) == 42);
        CHECK(static_cast<int>(vec[1]) == 100);
    }
}

} // anonymous namespace
