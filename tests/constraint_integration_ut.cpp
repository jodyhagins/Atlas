#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "doctest.hpp"

// Include the generated types
#include "constraint_integration_types.hpp"

namespace {

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

// ======================================================================
// TASK 2: POSITIVE CONSTRAINT
// ======================================================================

TEST_SUITE("Positive Constraint")
{
    TEST_CASE("positive constraint - valid construction")
    {
        CHECK_NOTHROW(test::constraints::PositiveInt{1});
        CHECK_NOTHROW(test::constraints::PositiveInt{100});
        CHECK_NOTHROW(
            test::constraints::PositiveInt{std::numeric_limits<int>::max()});
    }

    TEST_CASE("positive constraint - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::PositiveInt{0},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::PositiveInt{-1},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::PositiveInt{-100},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::PositiveInt{std::numeric_limits<int>::min()},
            atlas::ConstraintError);
    }

    TEST_CASE("positive constraint - comparison operators work")
    {
        test::constraints::PositiveInt a{5};
        test::constraints::PositiveInt b{10};
        test::constraints::PositiveInt c{5};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("positive constraint - exception message content")
    {
        try {
            test::constraints::PositiveInt invalid{0};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("PositiveInt") != std::string::npos);
            CHECK(msg.find("positive") != std::string::npos);
        }
    }

    TEST_CASE("positive constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::constraints::PositiveChecked{1});
        CHECK_NOTHROW(test::constraints::PositiveChecked{100});
        CHECK_NOTHROW(test::constraints::PositiveChecked{255});
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::PositiveChecked{0},
            atlas::ConstraintError);
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - overflow and constraint")
    {
        test::constraints::PositiveChecked a{200};
        test::constraints::PositiveChecked b{100};

        // This should throw CheckedOverflowError (overflow happens first)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - constraint violation")
    {
        test::constraints::PositiveChecked a{5};
        test::constraints::PositiveChecked b{10};

        // This should throw CheckedUnderflowError first (underflow before
        // constraint check)
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("positive constraint with checked arithmetic - valid operations")
    {
        test::constraints::PositiveChecked a{100};
        test::constraints::PositiveChecked b{50};

        CHECK_NOTHROW(a + b); // 150 is positive and within range
        CHECK_NOTHROW(a - b); // 50 is positive

        auto result = a - b;
        CHECK(atlas::undress(result) == 50);
    }

    TEST_CASE("positive constraint - copy and move don't re-check")
    {
        test::constraints::PositiveInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::PositiveInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::PositiveInt c{std::move(a)});

        test::constraints::PositiveInt d{1};
        test::constraints::PositiveInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("positive constraint with saturating - valid construction")
    {
        CHECK_NOTHROW(test::constraints::PositiveSaturating{1});
        CHECK_NOTHROW(test::constraints::PositiveSaturating{100});
        CHECK_NOTHROW(test::constraints::PositiveSaturating{255});
    }

    TEST_CASE("positive constraint with saturating - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::PositiveSaturating{0},
            atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with saturating - underflow to zero throws")
    {
        test::constraints::PositiveSaturating a{5};
        test::constraints::PositiveSaturating b{10};

        // Saturating subtraction: 5 - 10 saturates to 0, which violates
        // positive constraint
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with saturating - valid subtraction")
    {
        test::constraints::PositiveSaturating a{100};
        test::constraints::PositiveSaturating b{50};

        // 100 - 50 = 50, which is positive and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(atlas::undress(result) == 50);
    }

    TEST_CASE("positive constraint with saturating - valid addition")
    {
        test::constraints::PositiveSaturating a{100};
        test::constraints::PositiveSaturating b{50};

        // 100 + 50 = 150, which is positive and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 150);
    }

    TEST_CASE("positive constraint with saturating - overflow stays positive")
    {
        test::constraints::PositiveSaturating a{200};
        test::constraints::PositiveSaturating b{100};

        // Saturating addition: 200 + 100 saturates to 255, which is still
        // positive
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 255);
    }

    TEST_CASE("positive constraint with wrapping - valid construction")
    {
        CHECK_NOTHROW(test::constraints::PositiveWrapping{1});
        CHECK_NOTHROW(test::constraints::PositiveWrapping{100});
        CHECK_NOTHROW(test::constraints::PositiveWrapping{255});
    }

    TEST_CASE("positive constraint with wrapping - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::PositiveWrapping{0},
            atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with wrapping - underflow to zero throws")
    {
        test::constraints::PositiveWrapping a{5};
        test::constraints::PositiveWrapping b{10};

        // Wrapping subtraction: 5 - 10 wraps to 251 (5 - 10 + 256 = 251)
        // 251 is positive, so this should succeed
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(atlas::undress(result) == 251);
    }

    TEST_CASE("positive constraint with wrapping - wraps to zero throws")
    {
        test::constraints::PositiveWrapping a{10};
        test::constraints::PositiveWrapping b{10};

        // Wrapping subtraction: 10 - 10 = 0, which violates positive constraint
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with wrapping - valid subtraction")
    {
        test::constraints::PositiveWrapping a{100};
        test::constraints::PositiveWrapping b{50};

        // 100 - 50 = 50, which is positive and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(atlas::undress(result) == 50);
    }

    TEST_CASE("positive constraint with wrapping - valid addition")
    {
        test::constraints::PositiveWrapping a{100};
        test::constraints::PositiveWrapping b{50};

        // 100 + 50 = 150, which is positive and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 150);
    }

    TEST_CASE("positive constraint with wrapping - overflow wraps around")
    {
        test::constraints::PositiveWrapping a{200};
        test::constraints::PositiveWrapping b{100};

        // Wrapping addition: 200 + 100 = 300, wraps to 44 (300 - 256 = 44)
        // 44 is positive, so this should succeed
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 44);
    }
}

// ======================================================================
// TASK 3: NON-NEGATIVE CONSTRAINT
// ======================================================================

TEST_SUITE("Non-Negative Constraint")
{
    TEST_CASE("non_negative constraint - valid construction")
    {
        CHECK_NOTHROW(test::constraints::NonNegativeInt{0}); // Zero is OK!
        CHECK_NOTHROW(test::constraints::NonNegativeInt{1});
        CHECK_NOTHROW(test::constraints::NonNegativeInt{100});
        CHECK_NOTHROW(
            test::constraints::NonNegativeInt{std::numeric_limits<int>::max()});
    }

    TEST_CASE("non_negative constraint - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::NonNegativeInt{-1},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::NonNegativeInt{-100},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::NonNegativeInt{std::numeric_limits<int>::min()},
            atlas::ConstraintError);
    }

    TEST_CASE("non_negative constraint - comparison operators work")
    {
        test::constraints::NonNegativeInt a{0};
        test::constraints::NonNegativeInt b{5};
        test::constraints::NonNegativeInt c{0};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("non_negative constraint - arithmetic producing negative")
    {
        test::constraints::NonNegativeInt a{5};
        test::constraints::NonNegativeInt b{10};
        CHECK_THROWS_AS(a - b, atlas::ConstraintError); // -5 is negative
    }

    TEST_CASE("non_negative constraint - arithmetic producing zero is OK")
    {
        test::constraints::NonNegativeInt a{5};
        test::constraints::NonNegativeInt b{5};
        CHECK_NOTHROW(a - b); // 0 is non-negative
        auto result = a - b;
        CHECK(atlas::undress(result) == 0);
    }

    TEST_CASE("non_negative constraint - exception message")
    {
        try {
            test::constraints::NonNegativeInt invalid{-1};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonNegativeInt") != std::string::npos);
            CHECK(msg.find("non-negative") != std::string::npos);
        }
    }

    TEST_CASE("non_negative constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::constraints::NonNegativeChecked{0}); // Zero is OK!
        CHECK_NOTHROW(test::constraints::NonNegativeChecked{1});
        CHECK_NOTHROW(test::constraints::NonNegativeChecked{100});
        CHECK_NOTHROW(test::constraints::NonNegativeChecked{255});
    }

    TEST_CASE("non_negative constraint with checked arithmetic - overflow")
    {
        test::constraints::NonNegativeChecked a{200};
        test::constraints::NonNegativeChecked b{100};

        // This should throw CheckedOverflowError (overflow happens first)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("non_negative constraint with checked arithmetic - underflow")
    {
        test::constraints::NonNegativeChecked a{5};
        test::constraints::NonNegativeChecked b{10};

        // This should throw CheckedUnderflowError (underflow before constraint
        // check)
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE(
        "non_negative constraint with checked arithmetic - valid operations")
    {
        test::constraints::NonNegativeChecked a{100};
        test::constraints::NonNegativeChecked b{50};

        CHECK_NOTHROW(a + b); // 150 is non-negative and within range
        CHECK_NOTHROW(a - b); // 50 is non-negative

        auto result = a - b;
        CHECK(atlas::undress(result) == 50);
    }

    TEST_CASE(
        "non_negative constraint with checked arithmetic - zero result is OK")
    {
        test::constraints::NonNegativeChecked a{50};
        test::constraints::NonNegativeChecked b{50};

        CHECK_NOTHROW(a - b); // 0 is non-negative
        auto result = a - b;
        CHECK(atlas::undress(result) == 0);
    }

    TEST_CASE("non_negative constraint - copy and move don't re-check")
    {
        test::constraints::NonNegativeInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::NonNegativeInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::NonNegativeInt c{std::move(a)});

        test::constraints::NonNegativeInt d{1};
        test::constraints::NonNegativeInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("non_negative constraint with saturating - valid construction")
    {
        CHECK_NOTHROW(
            test::constraints::NonNegativeSaturating{0}); // Zero is OK!
        CHECK_NOTHROW(test::constraints::NonNegativeSaturating{1});
        CHECK_NOTHROW(test::constraints::NonNegativeSaturating{100});
        CHECK_NOTHROW(test::constraints::NonNegativeSaturating{255});
    }

    TEST_CASE(
        "non_negative constraint with saturating - underflow to zero is OK")
    {
        test::constraints::NonNegativeSaturating a{5};
        test::constraints::NonNegativeSaturating b{10};

        // Saturating subtraction: 5 - 10 saturates to 0, which is non-negative
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(atlas::undress(result) == 0);
    }

    TEST_CASE("non_negative constraint with saturating - valid subtraction")
    {
        test::constraints::NonNegativeSaturating a{100};
        test::constraints::NonNegativeSaturating b{50};

        // 100 - 50 = 50, which is non-negative and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(atlas::undress(result) == 50);
    }

    TEST_CASE("non_negative constraint with saturating - valid addition")
    {
        test::constraints::NonNegativeSaturating a{100};
        test::constraints::NonNegativeSaturating b{50};

        // 100 + 50 = 150, which is non-negative and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 150);
    }

    TEST_CASE(
        "non_negative constraint with saturating - overflow stays non-negative")
    {
        test::constraints::NonNegativeSaturating a{200};
        test::constraints::NonNegativeSaturating b{100};

        // Saturating addition: 200 + 100 saturates to 255, which is
        // non-negative
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(atlas::undress(result) == 255);
    }
}

// ======================================================================
// TASK 4: NON-ZERO CONSTRAINT
// ======================================================================

TEST_SUITE("Non-Zero Constraint")
{
    TEST_CASE("non_zero constraint - valid construction")
    {
        CHECK_NOTHROW(test::constraints::NonZeroInt{1});
        CHECK_NOTHROW(test::constraints::NonZeroInt{-1}); // Negative OK
        CHECK_NOTHROW(test::constraints::NonZeroInt{100});
        CHECK_NOTHROW(test::constraints::NonZeroInt{-100}); // Negative OK
        CHECK_NOTHROW(
            test::constraints::NonZeroInt{std::numeric_limits<int>::max()});
        CHECK_NOTHROW(
            test::constraints::NonZeroInt{std::numeric_limits<int>::min()});
    }

    TEST_CASE("non_zero constraint - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::NonZeroInt{0},
            atlas::ConstraintError); // Only zero fails
    }

    TEST_CASE("non_zero constraint - arithmetic producing zero")
    {
        test::constraints::NonZeroInt a{5};
        test::constraints::NonZeroInt b{5};

        CHECK_THROWS_AS(a - b, atlas::ConstraintError); // 0 is invalid

        test::constraints::NonZeroInt c{-3};
        test::constraints::NonZeroInt d{3};
        CHECK_THROWS_AS(c + d, atlas::ConstraintError); // 0 is invalid
    }

    TEST_CASE("non_zero constraint - arithmetic producing non-zero")
    {
        test::constraints::NonZeroInt a{5};
        test::constraints::NonZeroInt b{3};

        CHECK_NOTHROW(a + b); // 8
        CHECK_NOTHROW(a - b); // 2
        CHECK_NOTHROW(a * b); // 15

        // Negative results are OK
        CHECK_NOTHROW(b - a); // -2
    }

    TEST_CASE("non_zero constraint - safe division use case")
    {
        // Non-zero constraint makes division safe - no division by zero
        // possible
        test::constraints::NonZeroInt divisor{5};

        int numerator = 20;
        int result = numerator /
            atlas::undress(divisor); // Safe - divisor can't be zero
        CHECK(result == 4);
    }

    TEST_CASE("non_zero constraint - exception message")
    {
        try {
            test::constraints::NonZeroInt invalid{0};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonZeroInt") != std::string::npos);
            CHECK(msg.find("non-zero") != std::string::npos);
        }
    }

    TEST_CASE("non_zero constraint - comparison operators work")
    {
        test::constraints::NonZeroInt a{1};
        test::constraints::NonZeroInt b{5};
        test::constraints::NonZeroInt c{1};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("non_zero constraint - negative values work in comparisons")
    {
        test::constraints::NonZeroInt neg{-5};
        test::constraints::NonZeroInt pos{5};

        CHECK(neg < pos);
        CHECK(pos > neg);
        CHECK(neg != pos);
    }

    TEST_CASE("non_zero constraint - copy and move don't re-check")
    {
        test::constraints::NonZeroInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::NonZeroInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::NonZeroInt c{std::move(a)});

        test::constraints::NonZeroInt d{1};
        test::constraints::NonZeroInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("non_zero constraint - denominator use case")
    {
        // Denominator type for safe division
        test::constraints::Denominator denom{4};

        int value = 20;
        int quotient = value / atlas::undress(denom);
        CHECK(quotient == 5);

        // Can multiply to scale
        auto scaled = denom * test::constraints::Denominator{3};
        CHECK(atlas::undress(scaled) == 12);

        // Can divide to reduce
        auto reduced = denom / test::constraints::Denominator{2};
        CHECK(atlas::undress(reduced) == 2);
    }

    TEST_CASE("non_zero constraint - edge case with 1 and -1")
    {
        // Identity elements for multiplication
        CHECK_NOTHROW(test::constraints::NonZeroInt{1});
        CHECK_NOTHROW(test::constraints::NonZeroInt{-1});

        test::constraints::NonZeroInt one{1};
        test::constraints::NonZeroInt neg_one{-1};

        // Multiplication preserves non-zero
        auto result = one * neg_one;
        CHECK(atlas::undress(result) == -1);
    }

    TEST_CASE("non_zero constraint - unsigned wraparound to zero is caught")
    {
        // For unsigned char, 16 * 16 = 256 = 0 (mod 256)
        // This tests that wraparound to zero is caught by the constraint
        test::constraints::NonZeroUChar a{16};
        test::constraints::NonZeroUChar b{16};

        CHECK_THROWS_AS(a * b, atlas::ConstraintError);
    }
}

// ======================================================================
// TASK 5: BOUNDED CONSTRAINT
// ======================================================================

TEST_SUITE("Bounded Constraint")
{
    TEST_CASE("bounded constraint - integer - valid construction")
    {
        CHECK_NOTHROW(test::constraints::Percentage{0}); // Min boundary
        CHECK_NOTHROW(test::constraints::Percentage{50}); // Middle
        CHECK_NOTHROW(test::constraints::Percentage{100}); // Max boundary
    }

    TEST_CASE("bounded constraint - integer - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::Percentage{-1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::constraints::Percentage{101},
            atlas::ConstraintError); // Above max
        CHECK_THROWS_AS(
            test::constraints::Percentage{-100},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::Percentage{200},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - float - valid construction")
    {
        CHECK_NOTHROW(test::constraints::Temperature{-273.15}); // Absolute zero
        CHECK_NOTHROW(test::constraints::Temperature{0.0}); // Freezing
        CHECK_NOTHROW(test::constraints::Temperature{100.0}); // Boiling
        CHECK_NOTHROW(test::constraints::Temperature{1e7}); // Sun's core
    }

    TEST_CASE("bounded constraint - float - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::Temperature{-274.0},
            atlas::ConstraintError); // Below abs zero
        CHECK_THROWS_AS(
            test::constraints::Temperature{1e8},
            atlas::ConstraintError); // Hotter than sun
    }

    TEST_CASE("bounded constraint - construction with out-of-bounds value")
    {
        // Direct construction with out-of-bounds value should throw
        CHECK_THROWS_AS(
            test::constraints::Percentage{110},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::Percentage{-10},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - comparison operators work")
    {
        test::constraints::Percentage a{50};
        test::constraints::Percentage b{75};
        test::constraints::Percentage c{50};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("bounded constraint - narrow bounds (single value)")
    {
        // Only 42 is valid
        CHECK_NOTHROW(test::constraints::FortyTwo{42});
        CHECK_THROWS_AS(
            test::constraints::FortyTwo{41},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::FortyTwo{43},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - exception message shows value and bounds")
    {
        try {
            test::constraints::Percentage invalid{101};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("Percentage") != std::string::npos);
            CHECK(msg.find("101") != std::string::npos); // Actual value
            CHECK(msg.find("0") != std::string::npos); // Min bound
            CHECK(msg.find("100") != std::string::npos); // Max bound
        }
    }

    TEST_CASE("bounded constraint - floating point boundary precision")
    {
        // Test exact boundary values work
        CHECK_NOTHROW(test::constraints::Temperature{-273.15}); // Exact min
        CHECK_NOTHROW(test::constraints::Temperature{1e7}); // Exact max

        // Values very close but outside should fail
        CHECK_THROWS_AS(
            test::constraints::Temperature{-273.150001},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::constraints::BoundedChecked{0});
        CHECK_NOTHROW(test::constraints::BoundedChecked{50});
        CHECK_NOTHROW(test::constraints::BoundedChecked{100});
    }

    TEST_CASE(
        "bounded constraint with checked arithmetic - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::BoundedChecked{101},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint with checked arithmetic - overflow throws "
              "before constraint")
    {
        test::constraints::BoundedChecked a{60};
        test::constraints::BoundedChecked b{50};

        // 60 + 50 = 110, which is within uint8_t range, but violates bounded
        // constraint However, the result needs to be checked for constraint
        // violation The actual behavior depends on whether overflow happens
        // first or constraint check For uint8_t with checked mode, 60 + 50 =
        // 110 is within type range but exceeds bound This should throw
        // ConstraintError after addition
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint with checked arithmetic - valid operations")
    {
        test::constraints::BoundedChecked a{60};
        test::constraints::BoundedChecked b{30};

        CHECK_NOTHROW(a + b); // 90 is in [0,100]
        CHECK_NOTHROW(a - b); // 30 is in [0,100]

        auto add_result = a + b;
        CHECK(atlas::undress(add_result) == 90);

        auto sub_result = a - b;
        CHECK(atlas::undress(sub_result) == 30);
    }

    TEST_CASE("bounded constraint with checked arithmetic - underflow")
    {
        test::constraints::BoundedChecked a{10};
        test::constraints::BoundedChecked b{20};

        // 10 - 20 would underflow for unsigned type
        // Checked mode should throw CheckedUnderflowError
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("bounded constraint - copy and move don't re-check")
    {
        test::constraints::Percentage a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::Percentage b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::Percentage c{std::move(a)});

        test::constraints::Percentage d{10};
        test::constraints::Percentage e{20};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("bounded constraint - temperature arithmetic")
    {
        test::constraints::Temperature a{100.0}; // Boiling water
        test::constraints::Temperature b{50.0};

        CHECK_NOTHROW(a + b); // 150.0 is valid
        CHECK_NOTHROW(a - b); // 50.0 is valid
        CHECK_NOTHROW(a * b); // 5000.0 is valid
        CHECK_NOTHROW(a / b); // 2.0 is valid

        auto add_result = a + b;
        CHECK(atlas::undress(add_result) == 150.0);
    }

    TEST_CASE("bounded constraint - string - valid construction")
    {
        CHECK_NOTHROW(test::constraints::BoundedString{"A"}); // Min boundary
        CHECK_NOTHROW(test::constraints::BoundedString{"AA"}); // Middle
        CHECK_NOTHROW(test::constraints::BoundedString{"AAA"}); // Middle
        CHECK_NOTHROW(test::constraints::BoundedString{"AAAA"}); // Max boundary
    }

    TEST_CASE("bounded constraint - string - invalid construction")
    {
        // Below min (lexicographically less than "A")
        CHECK_THROWS_AS(
            test::constraints::BoundedString{""},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"0"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"9"},
            atlas::ConstraintError);

        // Above max (lexicographically greater than "AAAA")
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"AAAAA"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"AAAB"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"B"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"Z"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - string - boundary values")
    {
        // Exact boundaries should work
        CHECK_NOTHROW(test::constraints::BoundedString{"A"});
        CHECK_NOTHROW(test::constraints::BoundedString{"AAAA"});

        // Just outside boundaries should fail
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"0"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedString{"AAAAA"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - string - comparison operators work")
    {
        test::constraints::BoundedString a{"AA"};
        test::constraints::BoundedString b{"AAA"};
        test::constraints::BoundedString c{"AA"};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= c);
        CHECK(a >= c);
    }

    TEST_CASE("bounded constraint - string - exception message shows value and "
              "bounds")
    {
        try {
            test::constraints::BoundedString invalid{"ZZZZZ"};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("BoundedString") != std::string::npos);
            CHECK(msg.find("ZZZZZ") != std::string::npos); // Actual value
            CHECK(
                msg.find("A") !=
                std::string::npos); // Min bound (will appear in bounds message)
            CHECK(msg.find("AAAA") != std::string::npos); // Max bound
        }
    }

    TEST_CASE("bounded constraint - string - copy and move don't re-check")
    {
        test::constraints::BoundedString a{"AAA"};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::BoundedString b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::BoundedString c{std::move(a)});

        test::constraints::BoundedString d{"A"};
        test::constraints::BoundedString e{"AA"};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }
}

// ======================================================================
// TASK 5.5: BOUNDED_RANGE CONSTRAINT (Half-Open)
// ======================================================================

TEST_SUITE("Bounded Range Constraint (Half-Open)")
{
    TEST_CASE("bounded_range constraint - integer - valid construction")
    {
        CHECK_NOTHROW(test::constraints::HalfOpenPercentage{
            0}); // Min boundary (inclusive)
        CHECK_NOTHROW(test::constraints::HalfOpenPercentage{50}); // Middle
        CHECK_NOTHROW(
            test::constraints::HalfOpenPercentage{99}); // Just below max
    }

    TEST_CASE("bounded_range constraint - integer - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary is EXCLUDED in half-open range
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{100},
            atlas::ConstraintError); // Max boundary excluded!
    }

    TEST_CASE("bounded_range constraint - integer - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{-1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{100},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{101},
            atlas::ConstraintError); // Above max
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{-100},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{200},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - float - valid construction")
    {
        CHECK_NOTHROW(test::constraints::CelsiusRange{0.0}); // Min (inclusive)
        CHECK_NOTHROW(test::constraints::CelsiusRange{50.0}); // Middle
        CHECK_NOTHROW(test::constraints::CelsiusRange{99.99}); // Just below max
    }

    TEST_CASE("bounded_range constraint - float - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary is EXCLUDED
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{100.0},
            atlas::ConstraintError); // Exactly at max
    }

    TEST_CASE("bounded_range constraint - float - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{-0.1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{100.0},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{100.1},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE(
        "bounded_range constraint - construction with out-of-bounds value")
    {
        // Direct construction with out-of-bounds value should throw
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{100},
            atlas::ConstraintError); // Max excluded
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{110},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::HalfOpenPercentage{-10},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - comparison operators work")
    {
        test::constraints::HalfOpenPercentage a{50};
        test::constraints::HalfOpenPercentage b{75};
        test::constraints::HalfOpenPercentage c{50};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("bounded_range constraint - narrow half-open range")
    {
        // Half-open [42, 44) allows only 42 and 43
        CHECK_NOTHROW(test::constraints::TinyRange{42}); // Min (inclusive)
        CHECK_NOTHROW(test::constraints::TinyRange{43}); // Middle value
        CHECK_THROWS_AS(
            test::constraints::TinyRange{41},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::constraints::TinyRange{44},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::constraints::TinyRange{45},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE("bounded_range constraint - negative bounds half-open range")
    {
        // Half-open [-10, 10) allows -10 to 9
        CHECK_NOTHROW(test::constraints::NegativeRange{-10}); // Min (inclusive)
        CHECK_NOTHROW(test::constraints::NegativeRange{0}); // Middle
        CHECK_NOTHROW(test::constraints::NegativeRange{9}); // Just below max
        CHECK_THROWS_AS(
            test::constraints::NegativeRange{-11},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::constraints::NegativeRange{10},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::constraints::NegativeRange{11},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE(
        "bounded_range constraint - exception message shows value and bounds")
    {
        try {
            test::constraints::HalfOpenPercentage invalid{100};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("HalfOpenPercentage") != std::string::npos);
            CHECK(msg.find("100") != std::string::npos); // Actual value
            CHECK(msg.find("0") != std::string::npos); // Min bound
            // Check for half-open range notation [0, 100)
            CHECK(msg.find("[0, 100)") != std::string::npos);
        }
    }

    TEST_CASE("bounded_range constraint - floating point boundary precision")
    {
        // Test exact boundary values
        CHECK_NOTHROW(
            test::constraints::CelsiusRange{0.0}); // Exact min (inclusive)
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{100.0},
            atlas::ConstraintError); // Exact max (excluded!)

        // Values very close but outside should fail
        CHECK_THROWS_AS(
            test::constraints::CelsiusRange{-0.000001},
            atlas::ConstraintError);

        // Value just below max should succeed
        CHECK_NOTHROW(test::constraints::CelsiusRange{99.999999});
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::constraints::BoundedRangeChecked{0}); // Min
        CHECK_NOTHROW(test::constraints::BoundedRangeChecked{50}); // Middle
        CHECK_NOTHROW(
            test::constraints::BoundedRangeChecked{99}); // Just below max
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - max excluded")
    {
        // Max is excluded in half-open range
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeChecked{100},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - invalid "
              "construction")
    {
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeChecked{100},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeChecked{101},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - overflow "
              "throws before constraint")
    {
        test::constraints::BoundedRangeChecked a{60};
        test::constraints::BoundedRangeChecked b{50};

        // 60 + 50 = 110, which is within uint8_t range, but violates
        // bounded_range constraint This should throw ConstraintError after
        // addition
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE(
        "bounded_range constraint with checked arithmetic - valid operations")
    {
        test::constraints::BoundedRangeChecked a{60};
        test::constraints::BoundedRangeChecked b{30};

        CHECK_NOTHROW(a + b); // 90 is in [0,100)
        CHECK_NOTHROW(a - b); // 30 is in [0,100)

        auto add_result = a + b;
        CHECK(atlas::undress(add_result) == 90);

        auto sub_result = a - b;
        CHECK(atlas::undress(sub_result) == 30);
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - underflow")
    {
        test::constraints::BoundedRangeChecked a{10};
        test::constraints::BoundedRangeChecked b{20};

        // 10 - 20 would underflow for unsigned type
        // Checked mode should throw CheckedUnderflowError
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE(
        "bounded_range constraint with checked arithmetic - result at boundary")
    {
        test::constraints::BoundedRangeChecked a{50};
        test::constraints::BoundedRangeChecked b{50};

        // 50 + 50 = 100, which is at the max boundary (excluded in half-open
        // range!)
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - copy and move don't re-check")
    {
        test::constraints::HalfOpenPercentage a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::HalfOpenPercentage b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::HalfOpenPercentage c{std::move(a)});

        test::constraints::HalfOpenPercentage d{10};
        test::constraints::HalfOpenPercentage e{20};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("bounded_range constraint - celsius range arithmetic")
    {
        test::constraints::CelsiusRange a{50.0};
        test::constraints::CelsiusRange b{25.0};

        CHECK_NOTHROW(a + b); // 75.0 is valid [0, 100)
        CHECK_NOTHROW(a - b); // 25.0 is valid
        CHECK_NOTHROW(a / b); // 2.0 is valid

        auto add_result = a + b;
        CHECK(atlas::undress(add_result) == 75.0);

        // Multiplication that exceeds bounds should throw
        CHECK_THROWS_AS(
            a * b,
            atlas::ConstraintError); // 1250.0 exceeds [0, 100)
    }

    TEST_CASE("bounded_range constraint - celsius range - result at max "
              "boundary throws")
    {
        test::constraints::CelsiusRange a{50.0};
        test::constraints::CelsiusRange b{50.0};

        // 50.0 + 50.0 = 100.0, which is at max (excluded!)
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - valid construction")
    {
        CHECK_NOTHROW(test::constraints::BoundedRangeString{
            "A"}); // Min boundary (inclusive)
        CHECK_NOTHROW(test::constraints::BoundedRangeString{"AA"}); // Middle
        CHECK_NOTHROW(test::constraints::BoundedRangeString{"AAA"}); // Middle
    }

    TEST_CASE("bounded_range constraint - string - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary "AAAA" is EXCLUDED
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAA"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - invalid construction")
    {
        // Below min (lexicographically less than "A")
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{""},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"0"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"9"},
            atlas::ConstraintError);

        // At or above max (lexicographically >= "AAAA")
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAA"},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAAA"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAB"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"B"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"Z"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - boundary values")
    {
        // Min boundary should work (inclusive)
        CHECK_NOTHROW(test::constraints::BoundedRangeString{"A"});

        // Max boundary should fail (excluded in half-open range!)
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAA"},
            atlas::ConstraintError);

        // Just below max should work
        CHECK_NOTHROW(test::constraints::BoundedRangeString{"AAA"});

        // Just outside min should fail
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"0"},
            atlas::ConstraintError);

        // Just above max should fail
        CHECK_THROWS_AS(
            test::constraints::BoundedRangeString{"AAAAA"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - comparison operators work")
    {
        test::constraints::BoundedRangeString a{"AA"};
        test::constraints::BoundedRangeString b{"AAA"};
        test::constraints::BoundedRangeString c{"AA"};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= c);
        CHECK(a >= c);
    }

    TEST_CASE("bounded_range constraint - string - exception message shows "
              "value and bounds")
    {
        try {
            test::constraints::BoundedRangeString invalid{"ZZZZZ"};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("BoundedRangeString") != std::string::npos);
            CHECK(msg.find("ZZZZZ") != std::string::npos); // Actual value
            CHECK(
                msg.find("A") !=
                std::string::npos); // Min bound (will appear in bounds message)
            // Check for half-open range notation [A, AAAA)
            CHECK(msg.find("[") != std::string::npos);
            CHECK(msg.find(")") != std::string::npos);
        }
    }

    TEST_CASE(
        "bounded_range constraint - string - copy and move don't re-check")
    {
        test::constraints::BoundedRangeString a{"AAA"};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::constraints::BoundedRangeString b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::constraints::BoundedRangeString c{std::move(a)});

        test::constraints::BoundedRangeString d{"A"};
        test::constraints::BoundedRangeString e{"AA"};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }
}

// ======================================================================
// TASK 6: NON-EMPTY CONSTRAINT
// ======================================================================

TEST_SUITE("Non-Empty Constraint")
{
    TEST_CASE("non_empty - string - valid construction")
    {
        CHECK_NOTHROW(test::constraints::Username{"alice"});
        CHECK_NOTHROW(test::constraints::Username{std::string("bob")});
        CHECK_NOTHROW(test::constraints::Username{"x"}); // Single char OK
    }

    TEST_CASE("non_empty - string - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::Username{""},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::Username{std::string()},
            atlas::ConstraintError);
    }

    TEST_CASE("non_empty - string - exception message content")
    {
        try {
            test::constraints::Username invalid{""};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("Username") != std::string::npos);
            CHECK(msg.find("empty") != std::string::npos);
        }
    }

    TEST_CASE("non_empty - vector - valid construction")
    {
        CHECK_NOTHROW(test::constraints::NonEmptyVector{std::vector<int>{1}});
        CHECK_NOTHROW(
            test::constraints::NonEmptyVector{std::vector<int>{1, 2, 3}});
    }

    TEST_CASE("non_empty - vector - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::NonEmptyVector{std::vector<int>{}},
            atlas::ConstraintError);
    }

    TEST_CASE("non_empty - vector - exception message content")
    {
        try {
            test::constraints::NonEmptyVector invalid{std::vector<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonEmptyVector") != std::string::npos);
            CHECK(msg.find("empty") != std::string::npos);
        }
    }

    TEST_CASE("non_empty - copy and move constructors work")
    {
        test::constraints::Username a{"alice"};

        // Copy constructor
        test::constraints::Username b{a};
        CHECK(atlas::undress(b) == "alice");

        // Move constructor
        test::constraints::Username c{std::move(a)};
        CHECK(atlas::undress(c) == "alice");
    }

    TEST_CASE("non_empty - copy and move assignment work")
    {
        test::constraints::Username a{"alice"};
        test::constraints::Username b{"bob"};

        // Copy assignment
        b = a;
        CHECK(atlas::undress(b) == "alice");

        test::constraints::Username d{"dave"};
        // Move assignment
        d = std::move(a);
        CHECK(atlas::undress(d) == "alice");
    }

    TEST_CASE("non_empty - comparison operators work")
    {
        test::constraints::Username a{"alice"};
        test::constraints::Username b{"bob"};
        test::constraints::Username c{"alice"};

        CHECK(a == c);
        CHECK(a != b);
    }

    TEST_CASE("non_empty - vector comparison operators work")
    {
        test::constraints::NonEmptyVector a{std::vector<int>{1, 2, 3}};
        test::constraints::NonEmptyVector b{std::vector<int>{4, 5, 6}};
        test::constraints::NonEmptyVector c{std::vector<int>{1, 2, 3}};

        CHECK(a == c);
        CHECK(a != b);
    }

    TEST_CASE("non_empty - forwarded member functions: size and empty")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // size() should work
        CHECK(v.size() == 3);

        // empty() should return false for non-empty vector
        CHECK(not v.empty());
    }

    TEST_CASE("non_empty - forwarded member functions: push_back maintains "
              "constraint")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1}};

        // push_back should work and not violate constraint
        CHECK_NOTHROW(v.push_back(2));
        CHECK(v.size() == 2);

        CHECK_NOTHROW(v.push_back(3));
        CHECK(v.size() == 3);
    }

    TEST_CASE("non_empty - pop_back on multi-element vector is safe")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // pop_back is safe when it doesn't violate the constraint
        CHECK_NOTHROW(v.pop_back());
        CHECK(v.size() == 2);

        CHECK_NOTHROW(v.pop_back());
        CHECK(v.size() == 1);
    }

    TEST_CASE(
        "non_empty - pop_back on single-element vector violates constraint")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1}};

        // pop_back will execute, then throw because constraint is violated
        CHECK_THROWS_AS(v.pop_back(), atlas::ConstraintError);

        // IMPORTANT: The operation executed before the exception was thrown
        // The vector is now empty (in an invalid state per our constraint)
        // This demonstrates the post-condition checking limitation
        // We must access the underlying value directly since empty() also
        // checks constraints
        CHECK(atlas::undress(v).empty());
    }

    TEST_CASE("non_empty - clear violates constraint")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // clear will execute, then throw because constraint is violated
        CHECK_THROWS_AS(v.clear(), atlas::ConstraintError);

        // IMPORTANT: The operation executed before the exception was thrown
        // The vector is now empty (in an invalid state per our constraint)
        // This is an inherent limitation of post-condition constraint checking
        // We must access the underlying value directly since empty() also
        // checks constraints
        CHECK(atlas::undress(v).empty());
    }

    TEST_CASE("non_empty - exception message for constraint violation after "
              "operation")
    {
        test::constraints::NonEmptyVector v{std::vector<int>{1}};

        try {
            v.pop_back();
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonEmptyVector") != std::string::npos);
            CHECK(msg.find("pop_back") != std::string::npos);
            CHECK(msg.find("violates constraint") != std::string::npos);
        }
    }
}

// ======================================================================
// TASK 7: NON-NULL CONSTRAINT
// ======================================================================

TEST_SUITE("Non-Null Constraint")
{
    TEST_CASE("non_null - void* - valid construction")
    {
        int value = 42;
        CHECK_NOTHROW(test::constraints::Handle{&value});
        CHECK_NOTHROW(
            test::constraints::Handle{reinterpret_cast<void *>(0x1234)});
    }

    TEST_CASE("non_null - void* - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::Handle{nullptr},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::Handle{static_cast<void *>(nullptr)},
            atlas::ConstraintError);
    }

    TEST_CASE("non_null - void* - exception message content")
    {
        try {
            test::constraints::Handle invalid{nullptr};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("Handle") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - int* - valid construction")
    {
        int value = 42;
        CHECK_NOTHROW(test::constraints::DataPointer{&value});
    }

    TEST_CASE("non_null - int* - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::DataPointer{nullptr},
            atlas::ConstraintError);
    }

    TEST_CASE("non_null - int* - with arrow operator")
    {
        int value = 42;
        test::constraints::DataPointer ptr{&value};

        // Use arrow operator to access the pointed-to value
        CHECK(*ptr.operator -> () == 42);
    }

    TEST_CASE("non_null - int* - exception message content")
    {
        try {
            test::constraints::DataPointer invalid{nullptr};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("DataPointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - shared_ptr - valid construction")
    {
        CHECK_NOTHROW(
            test::constraints::SharedPointer{std::make_shared<int>(42)});
    }

    TEST_CASE("non_null - shared_ptr - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::SharedPointer{std::shared_ptr<int>{}},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::SharedPointer{nullptr},
            atlas::ConstraintError);
    }

    TEST_CASE("non_null - shared_ptr - exception message content")
    {
        try {
            test::constraints::SharedPointer invalid{std::shared_ptr<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("SharedPointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - unique_ptr - valid construction")
    {
        CHECK_NOTHROW(
            test::constraints::UniquePointer{std::make_unique<int>(42)});
    }

    TEST_CASE("non_null - unique_ptr - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::UniquePointer{std::unique_ptr<int>{}},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::UniquePointer{nullptr},
            atlas::ConstraintError);
    }

    TEST_CASE("non_null - unique_ptr - exception message content")
    {
        try {
            test::constraints::UniquePointer invalid{std::unique_ptr<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("UniquePointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - optional - valid construction")
    {
        CHECK_NOTHROW(test::constraints::Optional{std::optional<int>{42}});
        CHECK_NOTHROW(test::constraints::Optional{42}); // implicit conversion
    }

    TEST_CASE("non_null - optional - invalid construction")
    {
        CHECK_THROWS_AS(
            test::constraints::Optional{std::optional<int>{}},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::constraints::Optional{std::nullopt},
            atlas::ConstraintError);
    }

    TEST_CASE("non_null - optional - exception message content")
    {
        try {
            test::constraints::Optional invalid{std::optional<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("Optional") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - comparison operators work for void*")
    {
        int a = 1;
        int b = 2;

        test::constraints::Handle h1{&a};
        test::constraints::Handle h2{&b};
        test::constraints::Handle h3{&a};

        CHECK(h1 == h3);
        CHECK(h1 != h2);
    }

    TEST_CASE("non_null - comparison operators work for int*")
    {
        int a = 1;
        int b = 2;

        test::constraints::DataPointer p1{&a};
        test::constraints::DataPointer p2{&b};
        test::constraints::DataPointer p3{&a};

        CHECK(p1 == p3);
        CHECK(p1 != p2);
    }

    TEST_CASE("non_null - copy and move constructors work")
    {
        int value = 42;
        test::constraints::DataPointer a{&value};

        // Copy constructor
        test::constraints::DataPointer b{a};
        CHECK(atlas::undress(b) == &value);

        // Move constructor
        test::constraints::DataPointer c{std::move(a)};
        CHECK(atlas::undress(c) == &value);
    }

    TEST_CASE("non_null - copy and move assignment work")
    {
        int value1 = 42;
        int value2 = 99;
        test::constraints::DataPointer a{&value1};
        test::constraints::DataPointer b{&value2};

        // Copy assignment
        b = a;
        CHECK(atlas::undress(b) == &value1);

        int value3 = 123;
        test::constraints::DataPointer d{&value3};
        // Move assignment
        d = std::move(a);
        CHECK(atlas::undress(d) == &value1);
    }

    TEST_CASE("non_null - move-from limitation with unique_ptr")
    {
        // This test documents a known limitation: moved-from smart pointers
        // violate the non_null invariant. This is inherent to C++ move
        // semantics and cannot be prevented at compile time.
        //
        // Users must be careful not to use smart pointer strong types after
        // moving from them, just as with regular smart pointers.

        test::constraints::UniquePointer ptr{std::make_unique<int>(42)};

        // Move the unique_ptr out - this leaves the strong type in a moved-from
        // state
        auto underlying = std::move(atlas::undress(ptr));

        // The moved-from strong type now contains a null pointer, violating the
        // invariant This is a known limitation and users should avoid accessing
        // moved-from objects
        auto const & moved_from_ptr = atlas::undress(ptr);
        CHECK(moved_from_ptr == nullptr); // Invariant violated!

        // Note: This is the same behavior as with regular unique_ptr.
        // The solution is the same: don't use objects after moving from them.
    }
}


} // anonymous namespace
