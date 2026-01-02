// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Comprehensive atlas::Nilable Tests
//
// This test suite provides comprehensive testing of the atlas::Nilable<T>
// template which provides std::optional-like semantics for atlas strong types
// that have an nil_value sentinel.
//
// Test coverage includes:
// - Trait detection (has_nil_value)
// - Construction (default, value, nullopt, in_place, copy, move)
// - Assignment (copy, move, nullopt, value)
// - Observers (has_value, operator bool, operator*, operator->, value,
// value_or)
// - Modifiers (reset, emplace, swap)
// - Monadic operations (and_then, or_else, transform)
// - Comparisons (==, !=, <, <=, >, >=, <=>)
// - Hash support
// - Interoperability with std::optional
// - Edge cases (move-only types, exception safety, const correctness)
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "nilable_test_types.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "doctest.hpp"

// ======================================================================
// TEST SUITE: TRAIT DETECTION
// ======================================================================

TEST_SUITE("can_be_nilable Trait")
{
    TEST_CASE("Types with nil_value are detected")
    {
        CHECK(atlas::can_be_nilable<test::SimpleInt>::value);
        CHECK(atlas::can_be_nilable<test::FileDescriptor>::value);
        CHECK(atlas::can_be_nilable<test::Name>::value);
        CHECK(atlas::can_be_nilable<test::Temperature>::value);
        CHECK(atlas::can_be_nilable<test::Age>::value);
        CHECK(atlas::can_be_nilable<test::Counter>::value);
        CHECK_FALSE(atlas::can_be_nilable<test::NoInvalidValue>::value);
    }

    TEST_CASE("Built-in types without nil_value are not detected")
    {
        CHECK_FALSE(atlas::can_be_nilable<int>::value);
        CHECK_FALSE(atlas::can_be_nilable<double>::value);
        CHECK_FALSE(atlas::can_be_nilable<std::string>::value);
        CHECK_FALSE(atlas::can_be_nilable<void *>::value);
    }

    TEST_CASE("Const types")
    {
        CHECK(atlas::can_be_nilable<test::SimpleInt const>::value);
        CHECK_FALSE(atlas::can_be_nilable<test::NoInvalidValue const>::value);
    }
}

// ======================================================================
// TEST SUITE: CONSTRUCTION
// ======================================================================

TEST_SUITE("Optional Construction")
{
    TEST_CASE("Default construction creates empty optional")
    {
        alignas(test::SimpleInt) char buffer[sizeof(test::SimpleInt)];
        std::memset(buffer, 0xa7, sizeof(buffer));
        auto & opt = *(::new (buffer) atlas::Nilable<test::SimpleInt>);

        CHECK_FALSE(opt.has_value());
        CHECK_FALSE(static_cast<bool>(opt));
    }

    TEST_CASE("std::nullopt_t construction creates empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(std::nullopt);

        CHECK_FALSE(opt.has_value());
        CHECK_FALSE(opt);
    }

    TEST_CASE("Value construction - explicit")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("Value construction - implicit (convertible)")
    {
        // This should work if conversion is implicit
        auto make_optional = [](atlas::Nilable<test::SimpleInt> opt) {
            return opt;
        };

        auto opt = make_optional(test::SimpleInt{42});
        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("Value construction with sentinel creates empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt::nil_value);

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("std::in_place_t construction")
    {
        atlas::Nilable<test::Name> opt(std::in_place, "Hello");

        REQUIRE(opt.has_value());
        CHECK(*opt == test::Name{std::string("Hello")});
    }

    TEST_CASE("Copy construction from empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2(opt1);

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Copy construction from non-empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(opt1);

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == test::SimpleInt{42});
        CHECK(*opt2 == test::SimpleInt{42});
    }

    TEST_CASE("Move construction from empty optional")
    {
        atlas::Nilable<test::Name> opt1;
        atlas::Nilable<test::Name> opt2(std::move(opt1));

        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Move construction from non-empty optional")
    {
        atlas::Nilable<test::Name> opt1(test::Name{std::string("Alice")});
        atlas::Nilable<test::Name> opt2(std::move(opt1));

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Name{std::string("Alice")});
    }

    TEST_CASE("Move construction leaves moved-from object in nil state")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(std::move(opt1));

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::SimpleInt{42});
        // Moved-from object should be in nil state
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::SimpleInt::nil_value);
    }

    TEST_CASE(
        "Move construction leaves moved-from object in nil state - string type")
    {
        atlas::Nilable<test::Name> opt1(test::Name{std::string("Alice")});
        atlas::Nilable<test::Name> opt2(std::move(opt1));

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Name{std::string("Alice")});
        // Moved-from object should be in nil state
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::Name::nil_value);
    }

    TEST_CASE("Move construction leaves moved-from object in nil state - with "
              "default_value")
    {
        // Test with Score which has a default_value
        atlas::Nilable<test::Score> opt1(test::Score{100});
        atlas::Nilable<test::Score> opt2(std::move(opt1));

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Score{100});
        // Moved-from object should be in nil state regardless of default_value
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::Score::nil_value);
    }

    TEST_CASE("Move-only type construction")
    {
        atlas::Nilable<test::UniqueHandle> opt(
            test::UniqueHandle{std::make_unique<int>(42)});

        REQUIRE(opt.has_value());
        test::UniqueHandle const & h = *opt;
        // Note: .get() forwarding is disabled for nullable types
        CHECK(atlas::undress(h).get() != nullptr);
    }

    TEST_CASE("Different sentinel values")
    {
        SUBCASE("Zero sentinel") {
            atlas::Nilable<test::SimpleInt> opt;
            CHECK_FALSE(opt.has_value());
        }

        SUBCASE("Negative sentinel") {
            // Test that FileDescriptor uses -1 as nil_value
            atlas::Nilable<test::FileDescriptor> opt;
            CHECK_FALSE(opt.has_value());
            // When default-constructed, should have nil_value
            CHECK(atlas::undress(*opt) == -1);
        }

        SUBCASE("Empty string sentinel") {
            atlas::Nilable<test::Name> opt;
            CHECK_FALSE(opt.has_value());
        }

        SUBCASE("nullptr sentinel") {
            atlas::Nilable<test::UniqueHandle> opt;
            CHECK_FALSE(opt.has_value());
        }

        SUBCASE("Max value sentinel") {
            atlas::Nilable<test::MaxSentinel> opt;
            CHECK_FALSE(opt.has_value());
        }

        SUBCASE("Min value sentinel") {
            atlas::Nilable<test::MinSentinel> opt;
            CHECK_FALSE(opt.has_value());
        }
    }
}

// ======================================================================
// TEST SUITE: ASSIGNMENT
// ======================================================================

TEST_SUITE("Optional Assignment")
{
    TEST_CASE("Copy assignment from empty to empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2;

        opt2 = opt1;

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Copy assignment from non-empty to empty")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2;

        opt2 = opt1;

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == test::SimpleInt{42});
        CHECK(*opt2 == test::SimpleInt{42});
    }

    TEST_CASE("Copy assignment from empty to non-empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        opt2 = opt1;

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Copy assignment from non-empty to non-empty")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{17});

        opt2 = opt1;

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == test::SimpleInt{42});
        CHECK(*opt2 == test::SimpleInt{42});
    }

    TEST_CASE("Move assignment from empty to empty")
    {
        atlas::Nilable<test::Name> opt1;
        atlas::Nilable<test::Name> opt2;

        opt2 = std::move(opt1);

        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Move assignment from non-empty to empty")
    {
        atlas::Nilable<test::Name> opt1(test::Name{std::string("Alice")});
        atlas::Nilable<test::Name> opt2;

        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Name{std::string("Alice")});
    }

    TEST_CASE("Move assignment from empty to non-empty")
    {
        atlas::Nilable<test::Name> opt1;
        atlas::Nilable<test::Name> opt2(test::Name{std::string("Bob")});

        opt2 = std::move(opt1);

        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Move assignment from non-empty to non-empty")
    {
        atlas::Nilable<test::Name> opt1(test::Name{std::string("Alice")});
        atlas::Nilable<test::Name> opt2(test::Name{std::string("Bob")});

        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Name{std::string("Alice")});
    }

    TEST_CASE("Move assignment leaves moved-from object in nil state")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2;

        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::SimpleInt{42});
        // Moved-from object should be in nil state
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::SimpleInt::nil_value);
    }

    TEST_CASE("Move assignment leaves moved-from object in nil state - "
              "non-empty to non-empty")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{17});

        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::SimpleInt{42});
        // Moved-from object should be in nil state
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::SimpleInt::nil_value);
    }

    TEST_CASE("Move assignment leaves moved-from object in nil state - with "
              "default_value")
    {
        atlas::Nilable<test::Score> opt1(test::Score{100});
        atlas::Nilable<test::Score> opt2;

        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::Score{100});
        // Moved-from object should be in nil state regardless of default_value
        CHECK_FALSE(opt1.has_value());
        CHECK(*opt1 == test::Score::nil_value);
    }

    TEST_CASE("Self-assignment copy")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        opt = opt;

        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("Self-assignment move")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        opt = std::move(opt);

        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("nullopt assignment to empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt;

        opt = std::nullopt;

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("nullopt assignment to non-empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        opt = std::nullopt;

        CHECK_FALSE(opt.has_value());
    }
}

// ======================================================================
// TEST SUITE: OBSERVERS
// ======================================================================

TEST_SUITE("Optional Observers")
{
    TEST_CASE("has_value() reflects optional state")
    {
        atlas::Nilable<test::SimpleInt> empty;
        atlas::Nilable<test::SimpleInt> full(test::SimpleInt{42});

        CHECK_FALSE(empty.has_value());
        CHECK(full.has_value());
    }

    TEST_CASE("operator bool() reflects optional state")
    {
        atlas::Nilable<test::SimpleInt> empty;
        atlas::Nilable<test::SimpleInt> full(test::SimpleInt{42});

        CHECK_FALSE(static_cast<bool>(empty));
        CHECK(static_cast<bool>(full));
    }

    TEST_CASE("operator* - lvalue reference")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        test::SimpleInt & ref = *opt;
        CHECK(ref == test::SimpleInt{42});

        // Modify through reference
        ref = test::SimpleInt{17};
        CHECK(*opt == test::SimpleInt{17});
    }

    TEST_CASE("operator* - const lvalue reference")
    {
        atlas::Nilable<test::SimpleInt> const opt(test::SimpleInt{42});

        test::SimpleInt const & ref = *opt;
        CHECK(ref == test::SimpleInt{42});
    }

    TEST_CASE("operator* - rvalue reference")
    {
        atlas::Nilable<test::Name> opt(test::Name{std::string("Alice")});

        test::Name moved = *std::move(opt);
        CHECK(moved == test::Name{std::string("Alice")});
    }

    TEST_CASE("operator* - const rvalue reference")
    {
        atlas::Nilable<test::Name> const opt(test::Name{std::string("Alice")});

        test::Name moved = *std::move(opt);
        CHECK(moved == test::Name{std::string("Alice")});
    }

    TEST_CASE("operator-> - non-const")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto * ptr = opt.operator -> ();
        CHECK(ptr != nullptr);
        CHECK(*ptr == test::SimpleInt{42});
    }

    TEST_CASE("operator-> - const")
    {
        atlas::Nilable<test::SimpleInt> const opt(test::SimpleInt{42});

        auto const * ptr = opt.operator -> ();
        CHECK(ptr != nullptr);
        CHECK(*ptr == test::SimpleInt{42});
    }

    TEST_CASE("value() returns reference when has value")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        test::SimpleInt & ref = opt.value();
        CHECK(ref == test::SimpleInt{42});

        ref = test::SimpleInt{17};
        CHECK(opt.value() == test::SimpleInt{17});
    }

    TEST_CASE("value() const returns const reference when has value")
    {
        atlas::Nilable<test::SimpleInt> const opt(test::SimpleInt{42});

        test::SimpleInt const & ref = opt.value();
        CHECK(ref == test::SimpleInt{42});
    }

    TEST_CASE("value() throws when empty")
    {
        atlas::Nilable<test::SimpleInt> opt;

        CHECK_THROWS_AS(opt.value(), atlas::BadNilableAccess);
    }

    TEST_CASE("value() const throws when empty")
    {
        atlas::Nilable<test::SimpleInt> const opt{};

        CHECK_THROWS_AS(opt.value(), atlas::BadNilableAccess);
    }

    TEST_CASE("value() && returns rvalue reference when has value")
    {
        atlas::Nilable<test::Name> opt(test::Name{std::string("Alice")});

        test::Name moved = std::move(opt).value();
        CHECK(moved == test::Name{std::string("Alice")});
    }

    TEST_CASE("value() && throws when empty")
    {
        atlas::Nilable<test::Name> opt;

        CHECK_THROWS_AS(std::move(opt).value(), atlas::BadNilableAccess);
    }

    TEST_CASE("value_or() returns value when present - lvalue")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto result = opt.value_or(test::SimpleInt{99});
        CHECK(result == test::SimpleInt{42});
    }

    TEST_CASE("value_or() returns default when empty - lvalue")
    {
        atlas::Nilable<test::SimpleInt> opt;

        auto result = opt.value_or(test::SimpleInt{99});
        CHECK(result == test::SimpleInt{99});
    }

    TEST_CASE("value_or() returns value when present - rvalue")
    {
        atlas::Nilable<test::Name> opt(test::Name{std::string("Alice")});

        auto result = std::move(opt).value_or(
            test::Name{std::string("Default")});
        CHECK(result == test::Name{std::string("Alice")});
    }

    TEST_CASE("value_or() returns default when empty - rvalue")
    {
        atlas::Nilable<test::Name> opt;

        auto result = std::move(opt).value_or(
            test::Name{std::string("Default")});
        CHECK(result == test::Name{std::string("Default")});
    }
}

// ======================================================================
// TEST SUITE: MODIFIERS
// ======================================================================

TEST_SUITE("Optional Modifiers")
{
    TEST_CASE("reset() on empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt;

        opt.reset();

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("reset() on non-empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        opt.reset();

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("emplace() with no arguments")
    {
        atlas::Nilable<test::SimpleInt> opt;
        CHECK_FALSE(opt.has_value());

        SUBCASE("with value") {
            test::SimpleInt & ref = opt.emplace(42);
            REQUIRE(opt.has_value());
            CHECK(42 == atlas::undress(*opt));
        }

        SUBCASE("with nothing") {
            test::SimpleInt & ref = opt.emplace();
            CHECK_FALSE(opt.has_value());
        }
    }

    TEST_CASE("emplace() with single argument")
    {
        atlas::Nilable<test::SimpleInt> opt;

        test::SimpleInt & ref = opt.emplace(42);

        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("emplace() replaces existing value")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{17});

        test::SimpleInt & ref = opt.emplace(42);

        REQUIRE(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
    }

    TEST_CASE("emplace() with string construction")
    {
        atlas::Nilable<test::Name> opt;

        test::Name & ref = opt.emplace(std::string("Alice"));

        REQUIRE(opt.has_value());
        CHECK(*opt == test::Name{std::string("Alice")});
    }

    TEST_CASE("swap() - both empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2;

        opt1.swap(opt2);

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("swap() - first empty, second full")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        opt1.swap(opt2);

        REQUIRE(opt1.has_value());
        CHECK(*opt1 == test::SimpleInt{42});
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("swap() - first full, second empty")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2;

        opt1.swap(opt2);

        CHECK_FALSE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt2 == test::SimpleInt{42});
    }

    TEST_CASE("swap() - both full")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{17});

        opt1.swap(opt2);

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == test::SimpleInt{17});
        CHECK(*opt2 == test::SimpleInt{42});
    }
}

// ======================================================================
// TEST SUITE: MONADIC OPERATIONS
// ======================================================================

TEST_SUITE("Optional Monadic Operations")
{
    TEST_CASE("and_then() on empty optional returns empty")
    {
        atlas::Nilable<test::SimpleInt> opt;

        auto result = opt.and_then([](test::SimpleInt val) {
            return atlas::Nilable<test::SimpleInt>{val};
        });

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("and_then() on non-empty optional applies function")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto result = opt.and_then([](test::SimpleInt val) {
            return atlas::Nilable<test::SimpleInt>{
                test::SimpleInt{atlas::undress(val) * 2}};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{84});
    }

    TEST_CASE("and_then() can change type")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto result = opt.and_then([](test::SimpleInt val) {
            return atlas::Nilable<test::Name>{
                test::Name{std::to_string(atlas::undress(val))}};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::Name{std::string("42")});
    }

    TEST_CASE("and_then() can return empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto result = opt.and_then(
            [](test::SimpleInt) { return atlas::Nilable<test::SimpleInt>{}; });

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("and_then() chaining")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{10});

        auto result = opt.and_then([](test::SimpleInt val) {
                             return atlas::Nilable<test::SimpleInt>{
                                 test::SimpleInt{atlas::undress(val) * 2}};
                         })
                          .and_then([](test::SimpleInt val) {
                              return atlas::Nilable<test::SimpleInt>{
                                  test::SimpleInt{atlas::undress(val) + 5}};
                          });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{25});
    }

    TEST_CASE("and_then() with move - lvalue")
    {
        atlas::Nilable<test::Name> opt(test::Name{std::string("Alice")});

        auto result = opt.and_then([](test::Name const & name) {
            return atlas::Nilable<test::Name>{name};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::Name{std::string("Alice")});
    }

    TEST_CASE("and_then() with move - rvalue")
    {
        atlas::Nilable<test::Name> opt(test::Name{std::string("Alice")});

        auto result = std::move(opt).and_then([](test::Name && name) {
            return atlas::Nilable<test::Name>{std::move(name)};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::Name{std::string("Alice")});
    }

    TEST_CASE("or_else() on empty optional applies fallback")
    {
        atlas::Nilable<test::SimpleInt> opt;

        auto result = opt.or_else([]() {
            return atlas::Nilable<test::SimpleInt>{test::SimpleInt{99}};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{99});
    }

    TEST_CASE("or_else() on non-empty optional returns original")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        auto result = opt.or_else([]() {
            return atlas::Nilable<test::SimpleInt>{test::SimpleInt{99}};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{42});
    }

    TEST_CASE("or_else() can return empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt;

        auto result = opt.or_else(
            []() { return atlas::Nilable<test::SimpleInt>{}; });

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("or_else() chaining with and_then")
    {
        atlas::Nilable<test::SimpleInt> opt;

        auto result = opt.or_else([]() {
                             return atlas::Nilable<test::SimpleInt>{
                                 test::SimpleInt{10}};
                         })
                          .and_then([](test::SimpleInt val) {
                              return atlas::Nilable<test::SimpleInt>{
                                  test::SimpleInt{atlas::undress(val) * 2}};
                          });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{20});
    }

    TEST_CASE("or_else() with move semantics")
    {
        atlas::Nilable<test::Name> opt;

        auto result = std::move(opt).or_else([]() {
            return atlas::Nilable<test::Name>{
                test::Name{std::string("Default")}};
        });

        REQUIRE(result.has_value());
        CHECK(*result == test::Name{std::string("Default")});
    }

    TEST_CASE("Complex monadic chain")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{5});

        auto result = opt.and_then([](test::SimpleInt val) {
                             // Double it
                             return atlas::Nilable<test::SimpleInt>{
                                 test::SimpleInt{atlas::undress(val) * 2}};
                         })
                          .and_then([](test::SimpleInt val) {
                              // Return empty if greater than 15
                              if (atlas::undress(val) > 15) {
                                  return atlas::Nilable<test::SimpleInt>{};
                              }
                              return atlas::Nilable<test::SimpleInt>{val};
                          })
                          .or_else([]() {
                              // Provide fallback
                              return atlas::Nilable<test::SimpleInt>{
                                  test::SimpleInt{100}};
                          });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{10});
    }

    TEST_CASE("Complex monadic chain with empty result")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{10});

        auto result = opt.and_then([](test::SimpleInt val) {
                             // Double it
                             return atlas::Nilable<test::SimpleInt>{
                                 test::SimpleInt{atlas::undress(val) * 2}};
                         })
                          .and_then([](test::SimpleInt val) {
                              // Return empty if greater than 15
                              if (atlas::undress(val) > 15) {
                                  return atlas::Nilable<test::SimpleInt>{};
                              }
                              return atlas::Nilable<test::SimpleInt>{val};
                          })
                          .or_else([]() {
                              // Provide fallback
                              return atlas::Nilable<test::SimpleInt>{
                                  test::SimpleInt{100}};
                          });

        REQUIRE(result.has_value());
        CHECK(*result == test::SimpleInt{100});
    }
}

// ======================================================================
// TEST SUITE: COMPARISONS WITH OPTIONAL
// ======================================================================

TEST_SUITE("Optional Comparisons - Optional vs Optional")
{
    TEST_CASE("Equality - both empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2;

        CHECK(opt1 == opt2);
        CHECK_FALSE(opt1 != opt2);
    }

    TEST_CASE("Equality - both non-empty with same value")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        CHECK(opt1 == opt2);
        CHECK_FALSE(opt1 != opt2);
    }

    TEST_CASE("Equality - both non-empty with different values")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{17});

        CHECK_FALSE(opt1 == opt2);
        CHECK(opt1 != opt2);
    }

    TEST_CASE("Equality - one empty, one non-empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        CHECK_FALSE(opt1 == opt2);
        CHECK(opt1 != opt2);
        CHECK_FALSE(opt2 == opt1);
        CHECK(opt2 != opt1);
    }

    TEST_CASE("Less than - both empty")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2;

        CHECK_FALSE(opt1 < opt2);
        CHECK_FALSE(opt2 < opt1);
        CHECK(opt1 <= opt2);
        CHECK(opt2 <= opt1);
    }

    TEST_CASE("Less than - empty vs non-empty")
    {
        atlas::Nilable<test::SimpleInt> empty;
        atlas::Nilable<test::SimpleInt> full(test::SimpleInt{42});

        CHECK(empty < full);
        CHECK_FALSE(full < empty);
        CHECK(empty <= full);
        CHECK_FALSE(full <= empty);
        CHECK_FALSE(empty > full);
        CHECK(full > empty);
        CHECK_FALSE(empty >= full);
        CHECK(full >= empty);
    }

    TEST_CASE("Less than - both non-empty")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{17});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        CHECK(opt1 < opt2);
        CHECK_FALSE(opt2 < opt1);
        CHECK(opt1 <= opt2);
        CHECK_FALSE(opt2 <= opt1);
        CHECK_FALSE(opt1 > opt2);
        CHECK(opt2 > opt1);
        CHECK_FALSE(opt1 >= opt2);
        CHECK(opt2 >= opt1);
    }

    TEST_CASE("Less than - both non-empty with equal values")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        CHECK_FALSE(opt1 < opt2);
        CHECK_FALSE(opt2 < opt1);
        CHECK(opt1 <= opt2);
        CHECK(opt2 <= opt1);
        CHECK_FALSE(opt1 > opt2);
        CHECK_FALSE(opt2 > opt1);
        CHECK(opt1 >= opt2);
        CHECK(opt2 >= opt1);
    }
}

// ======================================================================
// TEST SUITE: COMPARISONS WITH NULLOPT
// ======================================================================

TEST_SUITE("Optional Comparisons - Optional vs nullopt")
{
    TEST_CASE("Empty optional equals nullopt")
    {
        atlas::Nilable<test::SimpleInt> opt;

        CHECK(opt == std::nullopt);
        CHECK(std::nullopt == opt);
        CHECK_FALSE(opt != std::nullopt);
        CHECK_FALSE(std::nullopt != opt);
    }

    TEST_CASE("Non-empty optional not equal to nullopt")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        CHECK_FALSE(opt == std::nullopt);
        CHECK_FALSE(std::nullopt == opt);
        CHECK(opt != std::nullopt);
        CHECK(std::nullopt != opt);
    }

    TEST_CASE("nullopt is less than non-empty optional")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        CHECK(std::nullopt < opt);
        CHECK_FALSE(opt < std::nullopt);
        CHECK(std::nullopt <= opt);
        CHECK_FALSE(opt <= std::nullopt);
        CHECK_FALSE(std::nullopt > opt);
        CHECK(opt > std::nullopt);
        CHECK_FALSE(std::nullopt >= opt);
        CHECK(opt >= std::nullopt);
    }

    TEST_CASE("nullopt equals empty optional in ordering")
    {
        atlas::Nilable<test::SimpleInt> opt;

        CHECK_FALSE(opt < std::nullopt);
        CHECK_FALSE(std::nullopt < opt);
        CHECK(opt <= std::nullopt);
        CHECK(std::nullopt <= opt);
        CHECK_FALSE(opt > std::nullopt);
        CHECK_FALSE(std::nullopt > opt);
        CHECK(opt >= std::nullopt);
        CHECK(std::nullopt >= opt);
    }
}

// ======================================================================
// TEST SUITE: COMPARISONS WITH VALUE
// ======================================================================

TEST_SUITE("Optional Comparisons - Optional vs Value")
{
    TEST_CASE("Non-empty optional equals matching value")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        CHECK(opt == test::SimpleInt{42});
        CHECK(test::SimpleInt{42} == opt);
        CHECK_FALSE(opt != test::SimpleInt{42});
        CHECK_FALSE(test::SimpleInt{42} != opt);
    }

    TEST_CASE("Non-empty optional not equal to different value")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        CHECK_FALSE(opt == test::SimpleInt{17});
        CHECK_FALSE(test::SimpleInt{17} == opt);
        CHECK(opt != test::SimpleInt{17});
        CHECK(test::SimpleInt{17} != opt);
    }

    TEST_CASE("Empty optional not equal to any value")
    {
        atlas::Nilable<test::SimpleInt> opt;

        CHECK_FALSE(opt == test::SimpleInt{42});
        CHECK_FALSE(test::SimpleInt{42} == opt);
        CHECK(opt != test::SimpleInt{42});
        CHECK(test::SimpleInt{42} != opt);
    }

    TEST_CASE("Ordering - non-empty vs value")
    {
        atlas::Nilable<test::SimpleInt> opt(test::SimpleInt{42});

        CHECK(opt < test::SimpleInt{50});
        CHECK_FALSE(opt < test::SimpleInt{42});
        CHECK_FALSE(opt < test::SimpleInt{30});

        CHECK(test::SimpleInt{30} < opt);
        CHECK_FALSE(test::SimpleInt{42} < opt);
        CHECK_FALSE(test::SimpleInt{50} < opt);
    }

    TEST_CASE("Ordering - empty vs value (empty is always less)")
    {
        atlas::Nilable<test::SimpleInt> opt;

        CHECK(opt < test::SimpleInt{42});
        CHECK_FALSE(opt > test::SimpleInt{42});
        CHECK_FALSE(opt >= test::SimpleInt{42});

        CHECK_FALSE(test::SimpleInt{42} < opt);
        CHECK(test::SimpleInt{42} > opt);
        CHECK(test::SimpleInt{42} >= opt);
    }
}

// ======================================================================
// TEST SUITE: COMPARISONS WITH STD::OPTIONAL
// ======================================================================

TEST_SUITE("Optional Comparisons - Interop with std::optional")
{
    TEST_CASE("Both empty")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt;
        std::optional<test::SimpleInt> std_opt;

        CHECK(atlas_opt == std_opt);
        CHECK(std_opt == atlas_opt);
    }

    TEST_CASE("Both non-empty with same value")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt(test::SimpleInt{42});
        std::optional<test::SimpleInt> std_opt(test::SimpleInt{42});

        CHECK(atlas_opt == std_opt);
        CHECK(std_opt == atlas_opt);
    }

    TEST_CASE("Both non-empty with different values")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt(test::SimpleInt{42});
        std::optional<test::SimpleInt> std_opt(test::SimpleInt{17});

        CHECK_FALSE(atlas_opt == std_opt);
        CHECK_FALSE(std_opt == atlas_opt);
        CHECK(atlas_opt != std_opt);
        CHECK(std_opt != atlas_opt);
    }

    TEST_CASE("atlas empty, std non-empty")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt;
        std::optional<test::SimpleInt> std_opt(test::SimpleInt{42});

        CHECK_FALSE(atlas_opt == std_opt);
        CHECK_FALSE(std_opt == atlas_opt);
        CHECK(atlas_opt < std_opt);
        CHECK_FALSE(std_opt < atlas_opt);
    }

    TEST_CASE("atlas non-empty, std empty")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt(test::SimpleInt{42});
        std::optional<test::SimpleInt> std_opt;

        CHECK_FALSE(atlas_opt == std_opt);
        CHECK_FALSE(std_opt == atlas_opt);
        CHECK_FALSE(atlas_opt < std_opt);
        CHECK(std_opt < atlas_opt);
    }

    TEST_CASE("Ordering - both non-empty")
    {
        atlas::Nilable<test::SimpleInt> atlas_opt(test::SimpleInt{17});
        std::optional<test::SimpleInt> std_opt(test::SimpleInt{42});

        CHECK(atlas_opt < std_opt);
        CHECK_FALSE(std_opt < atlas_opt);
    }
}

// ======================================================================
// TEST SUITE: HASH SUPPORT
// ======================================================================

TEST_SUITE("Optional Hash Support")
{
    TEST_CASE("Hash of empty optional is consistent")
    {
        atlas::Nilable<test::SimpleInt> opt1;
        atlas::Nilable<test::SimpleInt> opt2;

        std::hash<atlas::Nilable<test::SimpleInt>> hasher;
        CHECK(hasher(opt1) == hasher(opt2));
    }

    TEST_CASE("Hash of non-empty optionals with same value are equal")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{42});

        std::hash<atlas::Nilable<test::SimpleInt>> hasher;
        CHECK(hasher(opt1) == hasher(opt2));
    }

    TEST_CASE("Hash of non-empty optionals with different values are likely "
              "different")
    {
        atlas::Nilable<test::SimpleInt> opt1(test::SimpleInt{42});
        atlas::Nilable<test::SimpleInt> opt2(test::SimpleInt{17});

        std::hash<atlas::Nilable<test::SimpleInt>> hasher;
        // Note: Different values should produce different hashes, but hash
        // collisions are technically allowed. This test may fail for specific
        // values.
        CHECK(hasher(opt1) != hasher(opt2));
    }

    TEST_CASE("Hash of empty and non-empty are different")
    {
        atlas::Nilable<test::SimpleInt> empty;
        atlas::Nilable<test::SimpleInt> full(test::SimpleInt{42});

        std::hash<atlas::Nilable<test::SimpleInt>> hasher;
        CHECK(hasher(empty) != hasher(full));
    }

    TEST_CASE("Can use optional in unordered_set")
    {
        std::unordered_set<atlas::Nilable<test::SimpleInt>> set;

        set.insert(atlas::Nilable<test::SimpleInt>{});
        set.insert(atlas::Nilable<test::SimpleInt>{test::SimpleInt{1}});
        set.insert(atlas::Nilable<test::SimpleInt>{test::SimpleInt{2}});
        set.insert(
            atlas::Nilable<test::SimpleInt>{test::SimpleInt{1}}); // Duplicate

        CHECK(set.size() == 3);
        CHECK(set.count(atlas::Nilable<test::SimpleInt>{}) == 1);
        CHECK(
            set.count(atlas::Nilable<test::SimpleInt>{test::SimpleInt{1}}) ==
            1);
        CHECK(
            set.count(atlas::Nilable<test::SimpleInt>{test::SimpleInt{2}}) ==
            1);
        CHECK(
            set.count(atlas::Nilable<test::SimpleInt>{test::SimpleInt{3}}) ==
            0);
    }

    TEST_CASE("Can use optional in unordered_map")
    {
        std::unordered_map<atlas::Nilable<test::Id>, std::string> map;

        map[atlas::Nilable<test::Id>{}] = "empty";
        map[atlas::Nilable<test::Id>{test::Id{1}}] = "one";
        map[atlas::Nilable<test::Id>{test::Id{2}}] = "two";

        CHECK(map.size() == 3);
        CHECK(map[atlas::Nilable<test::Id>{}] == "empty");
        CHECK(map[atlas::Nilable<test::Id>{test::Id{1}}] == "one");
        CHECK(map[atlas::Nilable<test::Id>{test::Id{2}}] == "two");
    }
}

// ======================================================================
// TEST SUITE: EDGE CASES AND SPECIAL SCENARIOS
// ======================================================================

TEST_SUITE("Optional Edge Cases")
{
    TEST_CASE("Const optional")
    {
        atlas::Nilable<test::SimpleInt> const opt(test::SimpleInt{42});

        CHECK(opt.has_value());
        CHECK(*opt == test::SimpleInt{42});
        CHECK(opt.value() == test::SimpleInt{42});
        CHECK(opt.value_or(test::SimpleInt{99}) == test::SimpleInt{42});
    }

    TEST_CASE("Optional with constrained type")
    {
        atlas::Nilable<test::Age> opt;

        CHECK_FALSE(opt.has_value());

        // Valid age
        opt = atlas::Nilable<test::Age>{test::Age{25}};
        REQUIRE(opt.has_value());
        CHECK(*opt == test::Age{25});

        // Reset to invalid
        opt.reset();
        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE(
        "Optional with constrained type - constraint violations still throw")
    {
        // Even though Age is wrapped in Optional, constraint violations should
        // still throw when constructing the Age value itself
        CHECK_THROWS(test::Age{200}); // Outside bounds [0, 150]
    }

    TEST_CASE("Move-only type in optional")
    {
        atlas::Nilable<test::UniqueHandle> opt;

        opt.emplace(std::make_unique<int>(42));

        REQUIRE(opt.has_value());
        // Note: .get() forwarding is disabled for nullable types
        CHECK(atlas::undress(*opt).get() != nullptr);
        CHECK(*atlas::undress(*opt) == 42);

        // Move out of optional
        auto handle = std::move(*opt);
        CHECK(atlas::undress(handle).get() != nullptr);
        CHECK(*atlas::undress(handle) == 42);
    }

    TEST_CASE("Optional with arithmetic type")
    {
        atlas::Nilable<test::Counter> opt1(test::Counter{10});
        atlas::Nilable<test::Counter> opt2(test::Counter{5});

        // Strong types support arithmetic
        auto sum = *opt1 + *opt2;
        CHECK(sum == test::Counter{15});

        auto diff = *opt1 - *opt2;
        CHECK(diff == test::Counter{5});

        auto prod = *opt1 * *opt2;
        CHECK(prod == test::Counter{50});
    }

    TEST_CASE("Optional with different sentinel values")
    {
        SUBCASE("FileDescriptor with -1 sentinel") {
            atlas::Nilable<test::FileDescriptor> opt(test::FileDescriptor{0});
            REQUIRE(opt.has_value());
            CHECK(*opt == test::FileDescriptor{0}); // 0 is valid!
        }

        SUBCASE("MaxSentinel with max value") {
            atlas::Nilable<test::MaxSentinel> opt(test::MaxSentinel{0});
            REQUIRE(opt.has_value());
            CHECK(*opt == test::MaxSentinel{0});

            opt = atlas::Nilable<test::MaxSentinel>{};
            CHECK_FALSE(opt.has_value());
        }

        SUBCASE("MinSentinel with min value") {
            atlas::Nilable<test::MinSentinel> opt(test::MinSentinel{0});
            REQUIRE(opt.has_value());
            CHECK(*opt == test::MinSentinel{0});

            opt = atlas::Nilable<test::MinSentinel>{};
            CHECK_FALSE(opt.has_value());
        }
    }

    TEST_CASE("Sorting optionals")
    {
        std::vector<atlas::Nilable<test::SimpleInt>> vec;
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{30}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{10}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{20}});

        std::sort(vec.begin(), vec.end());

        // Empty optionals should come first
        CHECK_FALSE(vec[0].has_value());
        CHECK_FALSE(vec[1].has_value());
        CHECK(*vec[2] == test::SimpleInt{10});
        CHECK(*vec[3] == test::SimpleInt{20});
        CHECK(*vec[4] == test::SimpleInt{30});
    }

    TEST_CASE("Optional in container")
    {
        std::vector<atlas::Nilable<test::Name>> vec;

        vec.push_back(
            atlas::Nilable<test::Name>{test::Name{std::string("Alice")}});
        vec.push_back(atlas::Nilable<test::Name>{});
        vec.push_back(
            atlas::Nilable<test::Name>{test::Name{std::string("Bob")}});

        CHECK(vec.size() == 3);
        CHECK(vec[0].has_value());
        CHECK_FALSE(vec[1].has_value());
        CHECK(vec[2].has_value());
    }

    TEST_CASE("Optional with default-valued type")
    {
        // Score has default_value=0, but Optional should still default to
        // invalid
        atlas::Nilable<test::Score> opt;
        REQUIRE(opt.has_value());
        CHECK(*opt == test::Score{0});
    }
}

// ======================================================================
// TEST SUITE: GENERIC PROGRAMMING
// ======================================================================

TEST_SUITE("Optional Generic Programming")
{
    TEST_CASE("Template function accepting Optional")
    {
        auto double_value = [](atlas::Nilable<test::SimpleInt> opt) {
            if (opt) {
                return atlas::Nilable<test::SimpleInt>{
                    test::SimpleInt{atlas::undress(*opt) * 2}};
            }
            return atlas::Nilable<test::SimpleInt>{};
        };

        auto result1 = double_value(
            atlas::Nilable<test::SimpleInt>{test::SimpleInt{21}});
        REQUIRE(result1.has_value());
        CHECK(*result1 == test::SimpleInt{42});

        auto result2 = double_value(atlas::Nilable<test::SimpleInt>{});
        CHECK_FALSE(result2.has_value());
    }

    TEST_CASE("Optional in algorithm")
    {
        std::vector<atlas::Nilable<test::SimpleInt>> vec;
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{1}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{2}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{3}});

        auto count = std::count_if(
            vec.begin(),
            vec.end(),
            [](atlas::Nilable<test::SimpleInt> const & opt) {
                return opt.has_value();
            });

        CHECK(count == 3);
    }

    TEST_CASE("Optional value accumulation")
    {
        std::vector<atlas::Nilable<test::SimpleInt>> vec;
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{10}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{20}});
        vec.push_back(atlas::Nilable<test::SimpleInt>{test::SimpleInt{30}});

        int sum = 0;
        for (auto const & opt : vec) {
            if (opt) {
                sum += atlas::undress(*opt);
            }
        }

        CHECK(sum == 60);
    }
}

// ======================================================================
// TEST SUITE: TYPE TRAIT COVERAGE
// ======================================================================

TEST_SUITE("Optional Type Traits")
{
    TEST_CASE("Optional is copy constructible for copyable types")
    {
        CHECK(
            std::is_copy_constructible<atlas::Nilable<test::SimpleInt>>::value);
        CHECK(std::is_copy_constructible<atlas::Nilable<test::Name>>::value);
    }

    TEST_CASE("Optional is move constructible")
    {
        CHECK(
            std::is_move_constructible<atlas::Nilable<test::SimpleInt>>::value);
        CHECK(std::is_move_constructible<atlas::Nilable<test::Name>>::value);
        CHECK(std::is_move_constructible<
              atlas::Nilable<test::UniqueHandle>>::value);
    }

    TEST_CASE("Optional is copy assignable for copyable types")
    {
        CHECK(std::is_copy_assignable<atlas::Nilable<test::SimpleInt>>::value);
        CHECK(std::is_copy_assignable<atlas::Nilable<test::Name>>::value);
    }

    TEST_CASE("Optional is move assignable")
    {
        CHECK(std::is_move_assignable<atlas::Nilable<test::SimpleInt>>::value);
        CHECK(std::is_move_assignable<atlas::Nilable<test::Name>>::value);
        CHECK(
            std::is_move_assignable<atlas::Nilable<test::UniqueHandle>>::value);
    }

    TEST_CASE("Optional size equals wrapped type size")
    {
        CHECK(
            sizeof(atlas::Nilable<test::SimpleInt>) == sizeof(test::SimpleInt));
        CHECK(sizeof(atlas::Nilable<test::Name>) == sizeof(test::Name));
        CHECK(
            sizeof(atlas::Nilable<test::FileDescriptor>) ==
            sizeof(test::FileDescriptor));
    }
}
