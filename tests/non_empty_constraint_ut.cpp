#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <string>
#include <vector>

// Include the generated types
#include "constraints-non-empty.hpp"

namespace {

TEST_SUITE("Non-Empty Constraint")
{
    TEST_CASE("non_empty - string - valid construction")
    {
        CHECK_NOTHROW(test::Username{"alice"});
        CHECK_NOTHROW(test::Username{std::string("bob")});
        CHECK_NOTHROW(test::Username{"x"}); // Single char OK
    }

    TEST_CASE("non_empty - string - invalid construction")
    {
        CHECK_THROWS_AS(test::Username{""}, atlas::ConstraintError);
        CHECK_THROWS_AS(test::Username{std::string()}, atlas::ConstraintError);
    }

    TEST_CASE("non_empty - string - exception message content")
    {
        try {
            test::Username invalid{""};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("Username") != std::string::npos);
            CHECK(msg.find("empty") != std::string::npos);
        }
    }

    TEST_CASE("non_empty - vector - valid construction")
    {
        CHECK_NOTHROW(test::NonEmptyVector{std::vector<int>{1}});
        CHECK_NOTHROW(test::NonEmptyVector{std::vector<int>{1, 2, 3}});
    }

    TEST_CASE("non_empty - vector - invalid construction")
    {
        CHECK_THROWS_AS(
            test::NonEmptyVector{std::vector<int>{}},
            atlas::ConstraintError);
    }

    TEST_CASE("non_empty - vector - exception message content")
    {
        try {
            test::NonEmptyVector invalid{std::vector<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("NonEmptyVector") != std::string::npos);
            CHECK(msg.find("empty") != std::string::npos);
        }
    }

    TEST_CASE("non_empty - copy and move constructors work")
    {
        test::Username a{"alice"};

        // Copy constructor
        test::Username b{a};
        CHECK(static_cast<std::string>(b) == "alice");

        // Move constructor
        test::Username c{std::move(a)};
        CHECK(static_cast<std::string>(c) == "alice");
    }

    TEST_CASE("non_empty - copy and move assignment work")
    {
        test::Username a{"alice"};
        test::Username b{"bob"};

        // Copy assignment
        b = a;
        CHECK(static_cast<std::string>(b) == "alice");

        test::Username d{"dave"};
        // Move assignment
        d = std::move(a);
        CHECK(static_cast<std::string>(d) == "alice");
    }

    TEST_CASE("non_empty - comparison operators work")
    {
        test::Username a{"alice"};
        test::Username b{"bob"};
        test::Username c{"alice"};

        CHECK(a == c);
        CHECK(a != b);
    }

    TEST_CASE("non_empty - vector comparison operators work")
    {
        test::NonEmptyVector a{std::vector<int>{1, 2, 3}};
        test::NonEmptyVector b{std::vector<int>{4, 5, 6}};
        test::NonEmptyVector c{std::vector<int>{1, 2, 3}};

        CHECK(a == c);
        CHECK(a != b);
    }

    TEST_CASE("non_empty - forwarded member functions: size and empty")
    {
        test::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // size() should work
        CHECK(v.size() == 3);

        // empty() should return false for non-empty vector
        CHECK(not v.empty());
    }

    TEST_CASE("non_empty - forwarded member functions: push_back maintains "
              "constraint")
    {
        test::NonEmptyVector v{std::vector<int>{1}};

        // push_back should work and not violate constraint
        CHECK_NOTHROW(v.push_back(2));
        CHECK(v.size() == 2);

        CHECK_NOTHROW(v.push_back(3));
        CHECK(v.size() == 3);
    }

    TEST_CASE("non_empty - pop_back on multi-element vector is safe")
    {
        test::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // pop_back is safe when it doesn't violate the constraint
        CHECK_NOTHROW(v.pop_back());
        CHECK(v.size() == 2);

        CHECK_NOTHROW(v.pop_back());
        CHECK(v.size() == 1);
    }

    TEST_CASE(
        "non_empty - pop_back on single-element vector violates constraint")
    {
        test::NonEmptyVector v{std::vector<int>{1}};

        // pop_back will execute, then throw because constraint is violated
        CHECK_THROWS_AS(v.pop_back(), atlas::ConstraintError);

        // IMPORTANT: The operation executed before the exception was thrown
        // The vector is now empty (in an invalid state per our constraint)
        // This demonstrates the post-condition checking limitation
        // We must access the underlying value directly since empty() also
        // checks constraints
        CHECK(static_cast<std::vector<int> const &>(v).empty());
    }

    TEST_CASE("non_empty - clear violates constraint")
    {
        test::NonEmptyVector v{std::vector<int>{1, 2, 3}};

        // clear will execute, then throw because constraint is violated
        CHECK_THROWS_AS(v.clear(), atlas::ConstraintError);

        // IMPORTANT: The operation executed before the exception was thrown
        // The vector is now empty (in an invalid state per our constraint)
        // This is an inherent limitation of post-condition constraint checking
        // We must access the underlying value directly since empty() also
        // checks constraints
        CHECK(static_cast<std::vector<int> const &>(v).empty());
    }

    TEST_CASE("non_empty - exception message for constraint violation after "
              "operation")
    {
        test::NonEmptyVector v{std::vector<int>{1}};

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

} // anonymous namespace
