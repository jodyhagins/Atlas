#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <limits>

// Include the generated types
#include "constraints-non-zero.hpp"

namespace {

TEST_SUITE("Non-Zero Constraint")
{
    TEST_CASE("non_zero constraint - valid construction")
    {
        CHECK_NOTHROW(test::NonZeroInt{1});
        CHECK_NOTHROW(test::NonZeroInt{-1}); // Negative OK
        CHECK_NOTHROW(test::NonZeroInt{100});
        CHECK_NOTHROW(test::NonZeroInt{-100}); // Negative OK
        CHECK_NOTHROW(test::NonZeroInt{std::numeric_limits<int>::max()});
        CHECK_NOTHROW(test::NonZeroInt{std::numeric_limits<int>::min()});
    }

    TEST_CASE("non_zero constraint - invalid construction")
    {
        CHECK_THROWS_AS(
            test::NonZeroInt{0},
            atlas::ConstraintError); // Only zero fails
    }

    TEST_CASE("non_zero constraint - arithmetic producing zero")
    {
        test::NonZeroInt a{5};
        test::NonZeroInt b{5};

        CHECK_THROWS_AS(a - b, atlas::ConstraintError); // 0 is invalid

        test::NonZeroInt c{-3};
        test::NonZeroInt d{3};
        CHECK_THROWS_AS(c + d, atlas::ConstraintError); // 0 is invalid
    }

    TEST_CASE("non_zero constraint - arithmetic producing non-zero")
    {
        test::NonZeroInt a{5};
        test::NonZeroInt b{3};

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
        test::NonZeroInt divisor{5};

        int numerator = 20;
        int result = numerator /
            static_cast<int>(divisor); // Safe - divisor can't be zero
        CHECK(result == 4);
    }

    TEST_CASE("non_zero constraint - exception message")
    {
        try {
            test::NonZeroInt invalid{0};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonZeroInt") != std::string::npos);
            CHECK(msg.find("non-zero") != std::string::npos);
        }
    }

    TEST_CASE("non_zero constraint - comparison operators work")
    {
        test::NonZeroInt a{1};
        test::NonZeroInt b{5};
        test::NonZeroInt c{1};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("non_zero constraint - negative values work in comparisons")
    {
        test::NonZeroInt neg{-5};
        test::NonZeroInt pos{5};

        CHECK(neg < pos);
        CHECK(pos > neg);
        CHECK(neg != pos);
    }

    TEST_CASE("non_zero constraint - copy and move don't re-check")
    {
        test::NonZeroInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::NonZeroInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::NonZeroInt c{std::move(a)});

        test::NonZeroInt d{1};
        test::NonZeroInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("non_zero constraint - denominator use case")
    {
        // Denominator type for safe division
        math::Denominator denom{4};

        int value = 20;
        int quotient = value / static_cast<int>(denom);
        CHECK(quotient == 5);

        // Can multiply to scale
        auto scaled = denom * math::Denominator{3};
        CHECK(static_cast<int>(scaled) == 12);

        // Can divide to reduce
        auto reduced = denom / math::Denominator{2};
        CHECK(static_cast<int>(reduced) == 2);
    }

    TEST_CASE("non_zero constraint - edge case with 1 and -1")
    {
        // Identity elements for multiplication
        CHECK_NOTHROW(test::NonZeroInt{1});
        CHECK_NOTHROW(test::NonZeroInt{-1});

        test::NonZeroInt one{1};
        test::NonZeroInt neg_one{-1};

        // Multiplication preserves non-zero
        auto result = one * neg_one;
        CHECK(static_cast<int>(result) == -1);
    }

    TEST_CASE("non_zero constraint - unsigned wraparound to zero is caught")
    {
        // For unsigned char, 16 * 16 = 256 = 0 (mod 256)
        // This tests that wraparound to zero is caught by the constraint
        test::NonZeroUChar a{16};
        test::NonZeroUChar b{16};

        CHECK_THROWS_AS(a * b, atlas::ConstraintError);
    }
}

} // anonymous namespace
