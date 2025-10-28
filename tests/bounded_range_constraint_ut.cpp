#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <string>

// Include the generated types
#include "constraints-bounded_range.hpp"

namespace {

TEST_SUITE("Bounded Range Constraint (Half-Open)")
{
    TEST_CASE("bounded_range constraint - integer - valid construction")
    {
        CHECK_NOTHROW(test::HalfOpenPercentage{0}); // Min boundary (inclusive)
        CHECK_NOTHROW(test::HalfOpenPercentage{50}); // Middle
        CHECK_NOTHROW(test::HalfOpenPercentage{99}); // Just below max
    }

    TEST_CASE("bounded_range constraint - integer - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary is EXCLUDED in half-open range
        CHECK_THROWS_AS(
            test::HalfOpenPercentage{100},
            atlas::ConstraintError); // Max boundary excluded!
    }

    TEST_CASE("bounded_range constraint - integer - invalid construction")
    {
        CHECK_THROWS_AS(
            test::HalfOpenPercentage{-1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::HalfOpenPercentage{100},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            test::HalfOpenPercentage{101},
            atlas::ConstraintError); // Above max
        CHECK_THROWS_AS(test::HalfOpenPercentage{-100}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::HalfOpenPercentage{200}, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - float - valid construction")
    {
        CHECK_NOTHROW(physics::CelsiusRange{0.0}); // Min (inclusive)
        CHECK_NOTHROW(physics::CelsiusRange{50.0}); // Middle
        CHECK_NOTHROW(physics::CelsiusRange{99.99}); // Just below max
    }

    TEST_CASE("bounded_range constraint - float - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary is EXCLUDED
        CHECK_THROWS_AS(
            physics::CelsiusRange{100.0},
            atlas::ConstraintError); // Exactly at max
    }

    TEST_CASE("bounded_range constraint - float - invalid construction")
    {
        CHECK_THROWS_AS(
            physics::CelsiusRange{-0.1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            physics::CelsiusRange{100.0},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            physics::CelsiusRange{100.1},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE(
        "bounded_range constraint - construction with out-of-bounds value")
    {
        // Direct construction with out-of-bounds value should throw
        CHECK_THROWS_AS(
            test::HalfOpenPercentage{100},
            atlas::ConstraintError); // Max excluded
        CHECK_THROWS_AS(test::HalfOpenPercentage{110}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::HalfOpenPercentage{-10}, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - comparison operators work")
    {
        test::HalfOpenPercentage a{50};
        test::HalfOpenPercentage b{75};
        test::HalfOpenPercentage c{50};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("bounded_range constraint - narrow half-open range")
    {
        // Half-open [42, 44) allows only 42 and 43
        CHECK_NOTHROW(test::TinyRange{42}); // Min (inclusive)
        CHECK_NOTHROW(test::TinyRange{43}); // Middle value
        CHECK_THROWS_AS(
            test::TinyRange{41},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::TinyRange{44},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::TinyRange{45},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE("bounded_range constraint - negative bounds half-open range")
    {
        // Half-open [-10, 10) allows -10 to 9
        CHECK_NOTHROW(test::NegativeRange{-10}); // Min (inclusive)
        CHECK_NOTHROW(test::NegativeRange{0}); // Middle
        CHECK_NOTHROW(test::NegativeRange{9}); // Just below max
        CHECK_THROWS_AS(
            test::NegativeRange{-11},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::NegativeRange{10},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::NegativeRange{11},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE(
        "bounded_range constraint - exception message shows value and bounds")
    {
        try {
            test::HalfOpenPercentage invalid{100};
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
        CHECK_NOTHROW(physics::CelsiusRange{0.0}); // Exact min (inclusive)
        CHECK_THROWS_AS(
            physics::CelsiusRange{100.0},
            atlas::ConstraintError); // Exact max (excluded!)

        // Values very close but outside should fail
        CHECK_THROWS_AS(
            physics::CelsiusRange{-0.000001},
            atlas::ConstraintError);

        // Value just below max should succeed
        CHECK_NOTHROW(physics::CelsiusRange{99.999999});
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::BoundedRangeChecked{0}); // Min
        CHECK_NOTHROW(test::BoundedRangeChecked{50}); // Middle
        CHECK_NOTHROW(test::BoundedRangeChecked{99}); // Just below max
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - max excluded")
    {
        // Max is excluded in half-open range
        CHECK_THROWS_AS(test::BoundedRangeChecked{100}, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - invalid "
              "construction")
    {
        CHECK_THROWS_AS(
            test::BoundedRangeChecked{100},
            atlas::ConstraintError); // At max (excluded)
        CHECK_THROWS_AS(
            test::BoundedRangeChecked{101},
            atlas::ConstraintError); // Above max
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - overflow "
              "throws before constraint")
    {
        test::BoundedRangeChecked a{60};
        test::BoundedRangeChecked b{50};

        // 60 + 50 = 110, which is within uint8_t range, but violates
        // bounded_range constraint This should throw ConstraintError after
        // addition
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE(
        "bounded_range constraint with checked arithmetic - valid operations")
    {
        test::BoundedRangeChecked a{60};
        test::BoundedRangeChecked b{30};

        CHECK_NOTHROW(a + b); // 90 is in [0,100)
        CHECK_NOTHROW(a - b); // 30 is in [0,100)

        auto add_result = a + b;
        CHECK(static_cast<uint8_t>(add_result) == 90);

        auto sub_result = a - b;
        CHECK(static_cast<uint8_t>(sub_result) == 30);
    }

    TEST_CASE("bounded_range constraint with checked arithmetic - underflow")
    {
        test::BoundedRangeChecked a{10};
        test::BoundedRangeChecked b{20};

        // 10 - 20 would underflow for unsigned type
        // Checked mode should throw CheckedUnderflowError
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE(
        "bounded_range constraint with checked arithmetic - result at boundary")
    {
        test::BoundedRangeChecked a{50};
        test::BoundedRangeChecked b{50};

        // 50 + 50 = 100, which is at the max boundary (excluded in half-open
        // range!)
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - copy and move don't re-check")
    {
        test::HalfOpenPercentage a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::HalfOpenPercentage b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::HalfOpenPercentage c{std::move(a)});

        test::HalfOpenPercentage d{10};
        test::HalfOpenPercentage e{20};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("bounded_range constraint - celsius range arithmetic")
    {
        physics::CelsiusRange a{50.0};
        physics::CelsiusRange b{25.0};

        CHECK_NOTHROW(a + b); // 75.0 is valid [0, 100)
        CHECK_NOTHROW(a - b); // 25.0 is valid
        CHECK_NOTHROW(a / b); // 2.0 is valid

        auto add_result = a + b;
        CHECK(static_cast<double>(add_result) == 75.0);

        // Multiplication that exceeds bounds should throw
        CHECK_THROWS_AS(
            a * b,
            atlas::ConstraintError); // 1250.0 exceeds [0, 100)
    }

    TEST_CASE("bounded_range constraint - celsius range - result at max "
              "boundary throws")
    {
        physics::CelsiusRange a{50.0};
        physics::CelsiusRange b{50.0};

        // 50.0 + 50.0 = 100.0, which is at max (excluded!)
        CHECK_THROWS_AS(a + b, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - valid construction")
    {
        CHECK_NOTHROW(
            test::BoundedRangeString{"A"}); // Min boundary (inclusive)
        CHECK_NOTHROW(test::BoundedRangeString{"AA"}); // Middle
        CHECK_NOTHROW(test::BoundedRangeString{"AAA"}); // Middle
    }

    TEST_CASE("bounded_range constraint - string - max boundary excluded")
    {
        // KEY DIFFERENCE: Max boundary "AAAA" is EXCLUDED
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAA"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - invalid construction")
    {
        // Below min (lexicographically less than "A")
        CHECK_THROWS_AS(test::BoundedRangeString{""}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedRangeString{"0"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedRangeString{"9"}, atlas::ConstraintError);

        // At or above max (lexicographically >= "AAAA")
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAA"},
            atlas::ConstraintError); // At max (excluded!)
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAAA"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAB"},
            atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedRangeString{"B"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedRangeString{"Z"}, atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - boundary values")
    {
        // Min boundary should work (inclusive)
        CHECK_NOTHROW(test::BoundedRangeString{"A"});

        // Max boundary should fail (excluded in half-open range!)
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAA"},
            atlas::ConstraintError);

        // Just below max should work
        CHECK_NOTHROW(test::BoundedRangeString{"AAA"});

        // Just outside min should fail
        CHECK_THROWS_AS(test::BoundedRangeString{"0"}, atlas::ConstraintError);

        // Just above max should fail
        CHECK_THROWS_AS(
            test::BoundedRangeString{"AAAAA"},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded_range constraint - string - comparison operators work")
    {
        test::BoundedRangeString a{"AA"};
        test::BoundedRangeString b{"AAA"};
        test::BoundedRangeString c{"AA"};

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
            test::BoundedRangeString invalid{"ZZZZZ"};
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
        test::BoundedRangeString a{"AAA"};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::BoundedRangeString b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::BoundedRangeString c{std::move(a)});

        test::BoundedRangeString d{"A"};
        test::BoundedRangeString e{"AA"};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }
}

} // anonymous namespace
