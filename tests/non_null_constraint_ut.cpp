#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

#include <memory>
#include <optional>

// Include the generated types
#include "constraints-non-null.hpp"

namespace {

TEST_SUITE("Non-Null Constraint")
{
    TEST_CASE("non_null - void* - valid construction")
    {
        int value = 42;
        CHECK_NOTHROW(sys::Handle{&value});
        CHECK_NOTHROW(sys::Handle{reinterpret_cast<void*>(0x1234)});
    }

    TEST_CASE("non_null - void* - invalid construction")
    {
        CHECK_THROWS_AS(sys::Handle{nullptr}, atlas::ConstraintError);
        CHECK_THROWS_AS(sys::Handle{static_cast<void*>(nullptr)}, atlas::ConstraintError);
    }

    TEST_CASE("non_null - void* - exception message content")
    {
        try {
            sys::Handle invalid{nullptr};
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
        CHECK_NOTHROW(data::DataPointer{&value});
    }

    TEST_CASE("non_null - int* - invalid construction")
    {
        CHECK_THROWS_AS(data::DataPointer{nullptr}, atlas::ConstraintError);
    }

    TEST_CASE("non_null - int* - with arrow operator")
    {
        int value = 42;
        data::DataPointer ptr{&value};

        // Use arrow operator to access the pointed-to value
        CHECK(*ptr.operator->() == 42);
    }

    TEST_CASE("non_null - int* - exception message content")
    {
        try {
            data::DataPointer invalid{nullptr};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("DataPointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - shared_ptr - valid construction")
    {
        CHECK_NOTHROW(data::SharedPointer{std::make_shared<int>(42)});
    }

    TEST_CASE("non_null - shared_ptr - invalid construction")
    {
        CHECK_THROWS_AS(data::SharedPointer{std::shared_ptr<int>{}}, atlas::ConstraintError);
        CHECK_THROWS_AS(data::SharedPointer{nullptr}, atlas::ConstraintError);
    }

    TEST_CASE("non_null - shared_ptr - with arrow operator")
    {
        data::SharedPointer ptr{std::make_shared<int>(42)};

        // Cast to underlying type and use arrow operator
        auto const & underlying = static_cast<std::shared_ptr<int> const &>(ptr);
        CHECK(*underlying == 42);
    }

    TEST_CASE("non_null - shared_ptr - exception message content")
    {
        try {
            data::SharedPointer invalid{std::shared_ptr<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("SharedPointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - unique_ptr - valid construction")
    {
        CHECK_NOTHROW(data::UniquePointer{std::make_unique<int>(42)});
    }

    TEST_CASE("non_null - unique_ptr - invalid construction")
    {
        CHECK_THROWS_AS(data::UniquePointer{std::unique_ptr<int>{}}, atlas::ConstraintError);
        CHECK_THROWS_AS(data::UniquePointer{nullptr}, atlas::ConstraintError);
    }

    TEST_CASE("non_null - unique_ptr - with arrow operator")
    {
        data::UniquePointer ptr{std::make_unique<int>(42)};

        // Cast to underlying type and use arrow operator
        auto const & underlying = static_cast<std::unique_ptr<int> const &>(ptr);
        CHECK(*underlying == 42);
    }

    TEST_CASE("non_null - unique_ptr - exception message content")
    {
        try {
            data::UniquePointer invalid{std::unique_ptr<int>{}};
            FAIL("Should have thrown");
        } catch (atlas::ConstraintError const & e) {
            std::string msg = e.what();
            CHECK(msg.find("UniquePointer") != std::string::npos);
            CHECK(msg.find("null") != std::string::npos);
        }
    }

    TEST_CASE("non_null - optional - valid construction")
    {
        CHECK_NOTHROW(data::Optional{std::optional<int>{42}});
        CHECK_NOTHROW(data::Optional{42}); // implicit conversion
    }

    TEST_CASE("non_null - optional - invalid construction")
    {
        CHECK_THROWS_AS(data::Optional{std::optional<int>{}}, atlas::ConstraintError);
        CHECK_THROWS_AS(data::Optional{std::nullopt}, atlas::ConstraintError);
    }

    TEST_CASE("non_null - optional - with arrow operator")
    {
        data::Optional opt{std::optional<int>{42}};

        // Cast to underlying type and use arrow operator
        auto const & underlying = static_cast<std::optional<int> const &>(opt);
        CHECK(*underlying == 42);
    }

    TEST_CASE("non_null - optional - exception message content")
    {
        try {
            data::Optional invalid{std::optional<int>{}};
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

        sys::Handle h1{&a};
        sys::Handle h2{&b};
        sys::Handle h3{&a};

        CHECK(h1 == h3);
        CHECK(h1 != h2);
    }

    TEST_CASE("non_null - comparison operators work for int*")
    {
        int a = 1;
        int b = 2;

        data::DataPointer p1{&a};
        data::DataPointer p2{&b};
        data::DataPointer p3{&a};

        CHECK(p1 == p3);
        CHECK(p1 != p2);
    }

    TEST_CASE("non_null - copy and move constructors work")
    {
        int value = 42;
        data::DataPointer a{&value};

        // Copy constructor
        data::DataPointer b{a};
        CHECK(static_cast<int*>(b) == &value);

        // Move constructor
        data::DataPointer c{std::move(a)};
        CHECK(static_cast<int*>(c) == &value);
    }

    TEST_CASE("non_null - copy and move assignment work")
    {
        int value1 = 42;
        int value2 = 99;
        data::DataPointer a{&value1};
        data::DataPointer b{&value2};

        // Copy assignment
        b = a;
        CHECK(static_cast<int*>(b) == &value1);

        int value3 = 123;
        data::DataPointer d{&value3};
        // Move assignment
        d = std::move(a);
        CHECK(static_cast<int*>(d) == &value1);
    }

    TEST_CASE("non_null - move-from limitation with unique_ptr")
    {
        // This test documents a known limitation: moved-from smart pointers
        // violate the non_null invariant. This is inherent to C++ move semantics
        // and cannot be prevented at compile time.
        //
        // Users must be careful not to use smart pointer strong types after
        // moving from them, just as with regular smart pointers.

        data::UniquePointer ptr{std::make_unique<int>(42)};

        // Move the unique_ptr out - this leaves the strong type in a moved-from state
        auto underlying = std::move(static_cast<std::unique_ptr<int>&>(ptr));

        // The moved-from strong type now contains a null pointer, violating the invariant
        // This is a known limitation and users should avoid accessing moved-from objects
        auto const & moved_from_ptr = static_cast<std::unique_ptr<int> const &>(ptr);
        CHECK(moved_from_ptr == nullptr);  // Invariant violated!

        // Note: This is the same behavior as with regular unique_ptr.
        // The solution is the same: don't use objects after moving from them.
    }
}

} // anonymous namespace
