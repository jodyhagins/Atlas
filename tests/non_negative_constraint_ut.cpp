#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <limits>

// Include the generated types
#include "constraints-non-negative.hpp"

namespace {

TEST_SUITE("Non-Negative Constraint")
{
    TEST_CASE("non_negative constraint - valid construction")
    {
        CHECK_NOTHROW(test::NonNegativeInt{0}); // Zero is OK!
        CHECK_NOTHROW(test::NonNegativeInt{1});
        CHECK_NOTHROW(test::NonNegativeInt{100});
        CHECK_NOTHROW(test::NonNegativeInt{std::numeric_limits<int>::max()});
    }

    TEST_CASE("non_negative constraint - invalid construction")
    {
        CHECK_THROWS_AS(test::NonNegativeInt{-1}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::NonNegativeInt{-100}, atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::NonNegativeInt{std::numeric_limits<int>::min()},
            atlas::ConstraintError);
    }

    TEST_CASE("non_negative constraint - comparison operators work")
    {
        test::NonNegativeInt a{0};
        test::NonNegativeInt b{5};
        test::NonNegativeInt c{0};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("non_negative constraint - arithmetic producing negative")
    {
        test::NonNegativeInt a{5};
        test::NonNegativeInt b{10};
        CHECK_THROWS_AS(a - b, atlas::ConstraintError); // -5 is negative
    }

    TEST_CASE("non_negative constraint - arithmetic producing zero is OK")
    {
        test::NonNegativeInt a{5};
        test::NonNegativeInt b{5};
        CHECK_NOTHROW(a - b); // 0 is non-negative
        auto result = a - b;
        CHECK(static_cast<int>(result) == 0);
    }

    TEST_CASE("non_negative constraint - exception message")
    {
        try {
            test::NonNegativeInt invalid{-1};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonNegativeInt") != std::string::npos);
            CHECK(msg.find("non-negative") != std::string::npos);
        }
    }

    TEST_CASE("non_negative constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::NonNegativeChecked{0}); // Zero is OK!
        CHECK_NOTHROW(test::NonNegativeChecked{1});
        CHECK_NOTHROW(test::NonNegativeChecked{100});
        CHECK_NOTHROW(test::NonNegativeChecked{255});
    }

    TEST_CASE("non_negative constraint with checked arithmetic - overflow")
    {
        test::NonNegativeChecked a{200};
        test::NonNegativeChecked b{100};

        // This should throw CheckedOverflowError (overflow happens first)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE("non_negative constraint with checked arithmetic - underflow")
    {
        test::NonNegativeChecked a{5};
        test::NonNegativeChecked b{10};

        // This should throw CheckedUnderflowError (underflow before constraint
        // check)
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE(
        "non_negative constraint with checked arithmetic - valid operations")
    {
        test::NonNegativeChecked a{100};
        test::NonNegativeChecked b{50};

        CHECK_NOTHROW(a + b); // 150 is non-negative and within range
        CHECK_NOTHROW(a - b); // 50 is non-negative

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 50);
    }

    TEST_CASE(
        "non_negative constraint with checked arithmetic - zero result is OK")
    {
        test::NonNegativeChecked a{50};
        test::NonNegativeChecked b{50};

        CHECK_NOTHROW(a - b); // 0 is non-negative
        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 0);
    }

    TEST_CASE("non_negative constraint - copy and move don't re-check")
    {
        test::NonNegativeInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::NonNegativeInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::NonNegativeInt c{std::move(a)});

        test::NonNegativeInt d{1};
        test::NonNegativeInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("non_negative constraint with saturating - valid construction")
    {
        CHECK_NOTHROW(test::NonNegativeSaturating{0}); // Zero is OK!
        CHECK_NOTHROW(test::NonNegativeSaturating{1});
        CHECK_NOTHROW(test::NonNegativeSaturating{100});
        CHECK_NOTHROW(test::NonNegativeSaturating{255});
    }

    TEST_CASE(
        "non_negative constraint with saturating - underflow to zero is OK")
    {
        test::NonNegativeSaturating a{5};
        test::NonNegativeSaturating b{10};

        // Saturating subtraction: 5 - 10 saturates to 0, which is non-negative
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 0);
    }

    TEST_CASE("non_negative constraint with saturating - valid subtraction")
    {
        test::NonNegativeSaturating a{100};
        test::NonNegativeSaturating b{50};

        // 100 - 50 = 50, which is non-negative and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 50);
    }

    TEST_CASE("non_negative constraint with saturating - valid addition")
    {
        test::NonNegativeSaturating a{100};
        test::NonNegativeSaturating b{50};

        // 100 + 50 = 150, which is non-negative and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 150);
    }

    TEST_CASE(
        "non_negative constraint with saturating - overflow stays non-negative")
    {
        test::NonNegativeSaturating a{200};
        test::NonNegativeSaturating b{100};

        // Saturating addition: 200 + 100 saturates to 255, which is
        // non-negative
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 255);
    }
}

} // anonymous namespace
