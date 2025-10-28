#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <string>

// Include the generated types
#include "constraints-bounded.hpp"

namespace {

TEST_SUITE("Bounded Constraint")
{
    TEST_CASE("bounded constraint - integer - valid construction")
    {
        CHECK_NOTHROW(test::Percentage{0}); // Min boundary
        CHECK_NOTHROW(test::Percentage{50}); // Middle
        CHECK_NOTHROW(test::Percentage{100}); // Max boundary
    }

    TEST_CASE("bounded constraint - integer - invalid construction")
    {
        CHECK_THROWS_AS(
            test::Percentage{-1},
            atlas::ConstraintError); // Below min
        CHECK_THROWS_AS(
            test::Percentage{101},
            atlas::ConstraintError); // Above max
        CHECK_THROWS_AS(test::Percentage{-100}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::Percentage{200}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - float - valid construction")
    {
        CHECK_NOTHROW(physics::Temperature{-273.15}); // Absolute zero
        CHECK_NOTHROW(physics::Temperature{0.0}); // Freezing
        CHECK_NOTHROW(physics::Temperature{100.0}); // Boiling
        CHECK_NOTHROW(physics::Temperature{1e7}); // Sun's core
    }

    TEST_CASE("bounded constraint - float - invalid construction")
    {
        CHECK_THROWS_AS(
            physics::Temperature{-274.0},
            atlas::ConstraintError); // Below abs zero
        CHECK_THROWS_AS(
            physics::Temperature{1e8},
            atlas::ConstraintError); // Hotter than sun
    }

    TEST_CASE("bounded constraint - construction with out-of-bounds value")
    {
        // Direct construction with out-of-bounds value should throw
        CHECK_THROWS_AS(test::Percentage{110}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::Percentage{-10}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - comparison operators work")
    {
        test::Percentage a{50};
        test::Percentage b{75};
        test::Percentage c{50};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("bounded constraint - narrow bounds (single value)")
    {
        // Only 42 is valid
        CHECK_NOTHROW(test::FortyTwo{42});
        CHECK_THROWS_AS(test::FortyTwo{41}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::FortyTwo{43}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - exception message shows value and bounds")
    {
        try {
            test::Percentage invalid{101};
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
        CHECK_NOTHROW(physics::Temperature{-273.15}); // Exact min
        CHECK_NOTHROW(physics::Temperature{1e7}); // Exact max

        // Values very close but outside should fail
        CHECK_THROWS_AS(
            physics::Temperature{-273.150001},
            atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint with checked arithmetic - valid values")
    {
        CHECK_NOTHROW(test::BoundedChecked{0});
        CHECK_NOTHROW(test::BoundedChecked{50});
        CHECK_NOTHROW(test::BoundedChecked{100});
    }

    TEST_CASE(
        "bounded constraint with checked arithmetic - invalid construction")
    {
        CHECK_THROWS_AS(test::BoundedChecked{101}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint with checked arithmetic - overflow throws "
              "before constraint")
    {
        test::BoundedChecked a{60};
        test::BoundedChecked b{50};

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
        test::BoundedChecked a{60};
        test::BoundedChecked b{30};

        CHECK_NOTHROW(a + b); // 90 is in [0,100]
        CHECK_NOTHROW(a - b); // 30 is in [0,100]

        auto add_result = a + b;
        CHECK(static_cast<uint8_t>(add_result) == 90);

        auto sub_result = a - b;
        CHECK(static_cast<uint8_t>(sub_result) == 30);
    }

    TEST_CASE("bounded constraint with checked arithmetic - underflow")
    {
        test::BoundedChecked a{10};
        test::BoundedChecked b{20};

        // 10 - 20 would underflow for unsigned type
        // Checked mode should throw CheckedUnderflowError
        CHECK_THROWS_AS(a - b, atlas::CheckedUnderflowError);
    }

    TEST_CASE("bounded constraint - copy and move don't re-check")
    {
        test::Percentage a{42};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::Percentage b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::Percentage c{std::move(a)});

        test::Percentage d{10};
        test::Percentage e{20};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }

    TEST_CASE("bounded constraint - temperature arithmetic")
    {
        physics::Temperature a{100.0}; // Boiling water
        physics::Temperature b{50.0};

        CHECK_NOTHROW(a + b); // 150.0 is valid
        CHECK_NOTHROW(a - b); // 50.0 is valid
        CHECK_NOTHROW(a * b); // 5000.0 is valid
        CHECK_NOTHROW(a / b); // 2.0 is valid

        auto add_result = a + b;
        CHECK(static_cast<double>(add_result) == 150.0);
    }

    TEST_CASE("bounded constraint - string - valid construction")
    {
        CHECK_NOTHROW(test::BoundedString{"A"}); // Min boundary
        CHECK_NOTHROW(test::BoundedString{"AA"}); // Middle
        CHECK_NOTHROW(test::BoundedString{"AAA"}); // Middle
        CHECK_NOTHROW(test::BoundedString{"AAAA"}); // Max boundary
    }

    TEST_CASE("bounded constraint - string - invalid construction")
    {
        // Below min (lexicographically less than "A")
        CHECK_THROWS_AS(test::BoundedString{""}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"0"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"9"}, atlas::ConstraintError);

        // Above max (lexicographically greater than "AAAA")
        CHECK_THROWS_AS(test::BoundedString{"AAAAA"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"AAAB"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"B"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"Z"}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - string - boundary values")
    {
        // Exact boundaries should work
        CHECK_NOTHROW(test::BoundedString{"A"});
        CHECK_NOTHROW(test::BoundedString{"AAAA"});

        // Just outside boundaries should fail
        CHECK_THROWS_AS(test::BoundedString{"0"}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::BoundedString{"AAAAA"}, atlas::ConstraintError);
    }

    TEST_CASE("bounded constraint - string - comparison operators work")
    {
        test::BoundedString a{"AA"};
        test::BoundedString b{"AAA"};
        test::BoundedString c{"AA"};

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
            test::BoundedString invalid{"ZZZZZ"};
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
        test::BoundedString a{"AAA"};

        // Copy constructor should not re-check
        CHECK_NOTHROW(test::BoundedString b{a});

        // Move constructor should not re-check
        CHECK_NOTHROW(test::BoundedString c{std::move(a)});

        test::BoundedString d{"A"};
        test::BoundedString e{"AA"};

        // Copy assignment should not re-check
        CHECK_NOTHROW(d = e);

        // Move assignment should not re-check
        CHECK_NOTHROW(d = std::move(e));
    }
}

} // anonymous namespace
