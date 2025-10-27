#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <limits>

// Include the generated types
#include "constraints-positive.hpp"

namespace {

TEST_SUITE("Positive Constraint")
{
    TEST_CASE("positive constraint - valid construction")
    {
        CHECK_NOTHROW(test::PositiveInt{1});
        CHECK_NOTHROW(test::PositiveInt{100});
        CHECK_NOTHROW(test::PositiveInt{std::numeric_limits<int>::max()});
    }

    TEST_CASE("positive constraint - invalid construction")
    {
        CHECK_THROWS_AS(test::PositiveInt{0}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::PositiveInt{-1}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::PositiveInt{-100}, atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::PositiveInt{std::numeric_limits<int>::min()},
            atlas::ConstraintError);
    }

    TEST_CASE("positive constraint - comparison operators work")
    {
        test::PositiveInt a{5};
        test::PositiveInt b{10};
        test::PositiveInt c{5};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("positive constraint - exception message content")
    {
        try {
            test::PositiveInt invalid{0};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("PositiveInt") != std::string::npos);
            CHECK(msg.find("positive") != std::string::npos);
        }
    }

    TEST_CASE("positive constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::PositiveChecked{1});
        CHECK_NOTHROW(test::PositiveChecked{100});
        CHECK_NOTHROW(test::PositiveChecked{255});
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - invalid construction")
    {
        CHECK_THROWS_AS(test::PositiveChecked{0}, atlas::ConstraintError);
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - overflow and constraint")
    {
        test::PositiveChecked a{200};
        test::PositiveChecked b{100};

        // This should throw CheckedOverflowError (overflow happens first)
        CHECK_THROWS_AS(a + b, atlas::CheckedOverflowError);
    }

    TEST_CASE(
        "positive constraint with checked arithmetic - constraint violation")
    {
        test::PositiveChecked a{5};
        test::PositiveChecked b{10};

        // This should throw CheckedUnderflowError first (underflow before
        // constraint check)
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("positive constraint with checked arithmetic - valid operations")
    {
        test::PositiveChecked a{100};
        test::PositiveChecked b{50};

        CHECK_NOTHROW(a + b); // 150 is positive and within range
        CHECK_NOTHROW(a - b); // 50 is positive

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 50);
    }

    TEST_CASE("positive constraint - copy and move don't re-check")
    {
        test::PositiveInt a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::PositiveInt b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::PositiveInt c{std::move(a)});

        test::PositiveInt d{1};
        test::PositiveInt e{2};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("positive constraint with saturating - valid construction")
    {
        CHECK_NOTHROW(test::PositiveSaturating{1});
        CHECK_NOTHROW(test::PositiveSaturating{100});
        CHECK_NOTHROW(test::PositiveSaturating{255});
    }

    TEST_CASE("positive constraint with saturating - invalid construction")
    {
        CHECK_THROWS_AS(test::PositiveSaturating{0}, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with saturating - underflow to zero throws")
    {
        test::PositiveSaturating a{5};
        test::PositiveSaturating b{10};

        // Saturating subtraction: 5 - 10 saturates to 0, which violates
        // positive constraint
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with saturating - valid subtraction")
    {
        test::PositiveSaturating a{100};
        test::PositiveSaturating b{50};

        // 100 - 50 = 50, which is positive and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 50);
    }

    TEST_CASE("positive constraint with saturating - valid addition")
    {
        test::PositiveSaturating a{100};
        test::PositiveSaturating b{50};

        // 100 + 50 = 150, which is positive and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 150);
    }

    TEST_CASE("positive constraint with saturating - overflow stays positive")
    {
        test::PositiveSaturating a{200};
        test::PositiveSaturating b{100};

        // Saturating addition: 200 + 100 saturates to 255, which is still
        // positive
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 255);
    }

    TEST_CASE("positive constraint with wrapping - valid construction")
    {
        CHECK_NOTHROW(test::PositiveWrapping{1});
        CHECK_NOTHROW(test::PositiveWrapping{100});
        CHECK_NOTHROW(test::PositiveWrapping{255});
    }

    TEST_CASE("positive constraint with wrapping - invalid construction")
    {
        CHECK_THROWS_AS(test::PositiveWrapping{0}, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with wrapping - underflow to zero throws")
    {
        test::PositiveWrapping a{5};
        test::PositiveWrapping b{10};

        // Wrapping subtraction: 5 - 10 wraps to 251 (5 - 10 + 256 = 251)
        // 251 is positive, so this should succeed
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 251);
    }

    TEST_CASE("positive constraint with wrapping - wraps to zero throws")
    {
        test::PositiveWrapping a{10};
        test::PositiveWrapping b{10};

        // Wrapping subtraction: 10 - 10 = 0, which violates positive constraint
        CHECK_THROWS_AS(a - b, atlas::ConstraintError);
    }

    TEST_CASE("positive constraint with wrapping - valid subtraction")
    {
        test::PositiveWrapping a{100};
        test::PositiveWrapping b{50};

        // 100 - 50 = 50, which is positive and valid
        CHECK_NOTHROW(a - b);

        auto result = a - b;
        CHECK(static_cast<uint8_t>(result) == 50);
    }

    TEST_CASE("positive constraint with wrapping - valid addition")
    {
        test::PositiveWrapping a{100};
        test::PositiveWrapping b{50};

        // 100 + 50 = 150, which is positive and valid
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 150);
    }

    TEST_CASE("positive constraint with wrapping - overflow wraps around")
    {
        test::PositiveWrapping a{200};
        test::PositiveWrapping b{100};

        // Wrapping addition: 200 + 100 = 300, wraps to 44 (300 - 256 = 44)
        // 44 is positive, so this should succeed
        CHECK_NOTHROW(a + b);

        auto result = a + b;
        CHECK(static_cast<uint8_t>(result) == 44);
    }
}

} // anonymous namespace
