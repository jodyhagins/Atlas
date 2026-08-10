// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
// Borrow-safety tests: rvalue borrow guards and borrow classification.
//
// Covers the rvalue-borrow-safety spec:
// - Borrow classification traits (borrowed_type, is_borrowed, borrow_policy)
// - Borrowing conversions reject rvalue sources on every route:
//   static_cast, copy-initialization (implicit_cast), atlas::cast drilling,
//   atlas::cast with reference targets, and the from_atlas_cast constructor
// - Guards are inert where no borrow-from-owner exists
// - Detection idioms report false instead of true-then-error
// - forward= accessors and iterable stay faithful to the underlying type
// - borrowed_types= directive and third-party opt-ins
// - The guard matrix compiles at C++11/14/17/20 (the C++11/14 legs use a
//   hand-written string-backed stand-in; generated std::string-backed types
//   do not compile below C++17 for reasons predating the guards)
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "compilation_support.hpp"

#include "doctest.hpp"

namespace {

using namespace wjh::atlas::testing;
using namespace wjh::atlas::testing::compilation;

// A std::string-backed type with both conversion flavors, used all over.
constexpr char const * user_id_description = R"([type]
kind=struct
namespace=test
name=UserId
description=std::string; ==, cast<std::string_view>, implicit_cast<std::string_view>, cast<std::string>, no-constexpr
)";

// The no-conversion-clause shape from the spec: drilling must be blocked
// even though the type declares nothing about conversions.
constexpr char const * plain_description = R"([type]
kind=struct
namespace=test
name=Plain
description=std::string; ==, hash, no-constexpr
)";

// SFINAE probe for atlas::cast validity, usable in any generated-code TU.
constexpr char const * cast_probe = R"(
template <typename TargetT, typename SourceT, typename = void>
struct cast_ok : std::false_type {};
template <typename TargetT, typename SourceT>
struct cast_ok<TargetT, SourceT,
    std::void_t<decltype(atlas::cast<TargetT>(std::declval<SourceT>()))>>
: std::true_type {};
)";

TEST_SUITE("Borrow Safety: classification traits")
{
    TEST_CASE("Borrow-from-owner is the only guarded quadrant")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(plain_description, R"(
#include <span>
#include <string>
#include <string_view>
#include <vector>

// The guarded quadrant: target borrows, underlying owns
static_assert(atlas::borrow_policy<std::string_view, std::string>::value, "");
static_assert(atlas::borrow_policy<std::span<int const>, std::vector<int>>::value, "");

// Every other quadrant is unguarded
static_assert(!atlas::borrow_policy<std::string_view, std::string_view>::value, "");
static_assert(!atlas::borrow_policy<std::string, std::string>::value, "");
static_assert(!atlas::borrow_policy<int, double>::value, "");
static_assert(!atlas::borrow_policy<std::string, std::string_view>::value, "");

// Seeds: standard view types are borrowed, owners and pointers are not
static_assert(atlas::is_borrowed<std::string_view>::value, "");
static_assert(atlas::is_borrowed<std::wstring_view>::value, "");
static_assert(atlas::is_borrowed<std::span<int>>::value, "");
static_assert(atlas::is_borrowed<std::span<int, 4>>::value, "");
static_assert(!atlas::is_borrowed<std::string>::value, "");
static_assert(!atlas::is_borrowed<int>::value, "");
static_assert(!atlas::is_borrowed<char const *>::value, "");
static_assert(!atlas::is_borrowed<void *>::value, "");

// cv/ref-stripped before classification
static_assert(atlas::is_borrowed<std::string_view const &>::value, "");
static_assert(atlas::is_borrowed<std::string_view &&>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("Per-pair override works in both directions")
    {
        CompilationTester tester;

        // Relax: a guarded pair whose backing keeps storage alive can turn
        // its guard off; no other pair is affected.
        auto description = R"([type]
kind=struct
namespace=test
name=Handle
description=std::string; ==, cast<ext::Ref>, cast<std::string_view>, no-constexpr, #"ext_ref.hpp"
)";
        tester.write_temp_file("ext_ref.hpp", R"(
#pragma once
#include <string>
struct SafeRef { char const * p = nullptr; };
namespace ext {
struct Ref
{
    char const * p = nullptr;
    Ref() = default;
    Ref(std::string const & s) : p(s.c_str()) {}
};
}
)");
        auto result = tester.compile_and_run(description, R"(
#include <string_view>
#include <type_traits>
#include <utility>

// Tighten first, then relax, before any use of the pairs involved.
namespace atlas {
// ext::Ref borrows; classify it (normally borrowed_types= does this)
template <> struct borrowed_type<ext::Ref> : std::true_type {};
// ...but this user's std::string backing outlives every Ref, so relax
// the one pair.
template <> struct borrow_policy<ext::Ref, std::string> : std::false_type {};
}

test::Handle make() { return test::Handle{"hi"}; }

// Relaxed pair: conversion from a temporary is allowed again
static_assert(std::is_constructible<ext::Ref, test::Handle>::value, "");
// Other pairs are unaffected: string_view is still guarded
static_assert(!std::is_constructible<std::string_view, test::Handle>::value, "");
static_assert(std::is_constructible<std::string_view, test::Handle &>::value, "");

int main()
{
    ext::Ref r = static_cast<ext::Ref>(make());
    (void)r;
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("Per-pair tightening guards a pair nothing else classified")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=CStr
description=std::string; ==, cast<ext::CharPtr>, no-constexpr, #"char_ptr.hpp"
)";
        tester.write_temp_file("char_ptr.hpp", R"(
#pragma once
#include <string>
namespace ext {
struct CharPtr
{
    char const * p = nullptr;
    CharPtr() = default;
    CharPtr(std::string const & s) : p(s.c_str()) {}
};
}
)");
        auto result = tester.compile_and_run(description, R"(
#include <type_traits>
#include <utility>

// Not a range and not seeded, so nothing classifies ext::CharPtr
// automatically. Tighten the one pair; conversions from temporaries
// become deleted.
namespace atlas {
template <> struct borrow_policy<ext::CharPtr, std::string> : std::true_type {};
}

test::CStr make() { return test::CStr{"hi"}; }

static_assert(!std::is_constructible<ext::CharPtr, test::CStr>::value, "");
static_assert(std::is_constructible<ext::CharPtr, test::CStr &>::value, "");

int main()
{
    test::CStr s{"hello"};
    ext::CharPtr p = static_cast<ext::CharPtr>(s);
    return p.p != nullptr ? 0 : 1;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("Preamble is self-contained for a no-conversion-clause header")
    {
        CompilationTester tester;
        // The preamble itself must include what its seeds name; the header
        // is compiled alone.
        auto result = tester.compile_and_run(plain_description, R"(
static_assert(atlas::borrow_policy<std::string_view, std::string>::value,
    "seed visible without any user include");
int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: conversions reject rvalue sources")
{
    TEST_CASE("static_cast from a temporary is rejected")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
test::UserId make_id() { return test::UserId{"x"}; }
int main()
{
    auto sv = static_cast<std::string_view>(make_id());
    return static_cast<int>(sv.size());
}
)");
        INFO(result.output);
        CHECK_FALSE(result.success);
        CHECK(result.output.find("deleted") != std::string::npos);
    }

    TEST_CASE("static_cast from an xvalue is rejected")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
int main()
{
    test::UserId id{"x"};
    auto sv = static_cast<std::string_view>(std::move(id));
    return static_cast<int>(sv.size());
}
)");
        INFO(result.output);
        CHECK_FALSE(result.success);
        CHECK(result.output.find("deleted") != std::string::npos);
    }

    TEST_CASE("implicit conversion from a temporary is rejected")
    {
        CompilationTester tester;

        SUBCASE("copy-initialization")
        {
            auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
test::UserId make_id() { return test::UserId{"x"}; }
int main()
{
    std::string_view sv = make_id();
    return static_cast<int>(sv.size());
}
)");
            INFO(result.output);
            CHECK_FALSE(result.success);
        }

        SUBCASE("passing to a string_view parameter")
        {
            auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
test::UserId make_id() { return test::UserId{"x"}; }
std::size_t consume(std::string_view sv) { return sv.size(); }
int main()
{
    return static_cast<int>(consume(make_id()));
}
)");
            INFO(result.output);
            CHECK_FALSE(result.success);
        }
    }

    TEST_CASE("borrowing from an lvalue succeeds and views its storage")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(user_id_description, R"(
#include <cassert>
#include <string_view>
std::size_t consume(std::string_view sv) { return sv.size(); }
int main()
{
    test::UserId id{"hello"};
    std::string_view sv = static_cast<std::string_view>(id);
    assert(sv == "hello");
    assert(sv.data() == atlas::undress(id).data());
    std::string_view sv2 = id;
    assert(sv2.data() == sv.data());
    assert(consume(id) == 5);
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("guard is inert where no borrow-from-owner exists")
    {
        CompilationTester tester;

        SUBCASE("owning target from a temporary")
        {
            auto result = tester.compile_and_run(user_id_description, R"(
#include <cassert>
#include <string>
test::UserId make_id() { return test::UserId{"hello"}; }
int main()
{
    std::string s = static_cast<std::string>(make_id());
    assert(s == "hello");
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }

        SUBCASE("view-backed type converts from a temporary")
        {
            auto description = R"([type]
kind=struct
namespace=test
name=Token
description=std::string_view; ==, cast<std::string_view>, no-constexpr
)";
            auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <string_view>
static char const storage[] = "hello";
test::Token make() { return test::Token{std::string_view{storage}}; }
int main()
{
    // The view is copied out of the temporary; the viewed storage is
    // static, so nothing dangles.
    std::string_view sv = static_cast<std::string_view>(make());
    assert(sv == "hello");
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }

        SUBCASE("scalar-backed type converts from a temporary")
        {
            auto description = R"([type]
kind=struct
namespace=test
name=Ratio
description=double; ==, cast<double>, cast<float>
)";
            auto result = tester.compile_and_run(description, R"(
#include <cassert>
test::Ratio make() { return test::Ratio{0.5}; }
int main()
{
    double d = static_cast<double>(make());
    float f = static_cast<float>(make());
    assert(d == 0.5 && f == 0.5f);
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }
    }

    TEST_CASE("detection idioms report false rather than true-then-error")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
#include <type_traits>
#include <utility>

// Standard traits
static_assert(!std::is_constructible<std::string_view, test::UserId>::value, "");
static_assert(!std::is_convertible<test::UserId, std::string_view>::value, "");
static_assert(std::is_constructible<std::string_view, test::UserId &>::value, "");
static_assert(std::is_convertible<test::UserId &, std::string_view>::value, "");

// Atlas's own probe
static_assert(!atlas::atlas_detail::is_directly_castable<
    std::string_view, test::UserId>::value, "");
static_assert(atlas::atlas_detail::is_directly_castable<
    std::string_view, test::UserId &>::value, "");

// requires-expression, probed through a dependent context
template <typename T>
constexpr bool rvalue_borrows =
    requires(T t) { static_cast<std::string_view>(std::move(t)); };
static_assert(!rvalue_borrows<test::UserId>, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: cast drilling")
{
    TEST_CASE("drilling from a temporary is blocked without a conversion clause")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(plain_description, R"(
#include <string_view>
test::Plain make_plain() { return test::Plain{"x"}; }
int main()
{
    auto sv = atlas::cast<std::string_view>(make_plain());
    return static_cast<int>(sv.size());
}
)");
        INFO(result.output);
        CHECK_FALSE(result.success);
    }

    TEST_CASE("drilling from a temporary is blocked with a conversion clause")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(user_id_description, R"(
#include <string_view>
test::UserId make_id() { return test::UserId{"x"}; }
int main()
{
    auto sv = atlas::cast<std::string_view>(make_id());
    return static_cast<int>(sv.size());
}
)");
        INFO(result.output);
        CHECK_FALSE(result.success);
    }

    TEST_CASE("drilling from an lvalue still borrows")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(plain_description, R"(
#include <cassert>
#include <string_view>
int main()
{
    test::Plain p{"hello"};
    auto sv = atlas::cast<std::string_view>(p);
    assert(sv == "hello");
    assert(sv.data() == atlas::undress(p).data());
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("non-borrowing drilling is unaffected, from any value category")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Inner
description=int; ==

[type]
kind=struct
namespace=test
name=Middle
description=test::Inner; ==

[type]
kind=struct
namespace=test
name=Outer
description=test::Middle; ==

[type]
kind=struct
namespace=test
name=Owner
description=std::string; ==, no-constexpr
)";
        auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <string>
test::Owner make_owner() { return test::Owner{"hello"}; }
int main()
{
    // Owning target from an rvalue
    std::string s = atlas::cast<std::string>(make_owner());
    assert(s == "hello");

    // Multi-layer drill to int, all value categories
    test::Outer o{test::Middle{test::Inner{42}}};
    assert(atlas::cast<int>(o) == 42);
    assert(atlas::cast<int>(static_cast<test::Outer const &>(o)) == 42);
    assert(atlas::cast<int>(test::Outer{test::Middle{test::Inner{7}}}) == 7);
    assert(atlas::cast<int>(std::move(o)) == 42);
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("SFINAE matrix for drilling and detection")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(
            plain_description,
            std::string(cast_probe) + R"(
#include <string>
#include <string_view>

static_assert(!cast_ok<std::string_view, test::Plain>::value, "");
static_assert(!cast_ok<std::string_view, test::Plain &&>::value, "");
static_assert(cast_ok<std::string_view, test::Plain &>::value, "");
static_assert(cast_ok<std::string_view, test::Plain const &>::value, "");
static_assert(cast_ok<std::string, test::Plain>::value, "");
static_assert(cast_ok<std::string, test::Plain &>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: reference targets")
{
    TEST_CASE("reference target from a temporary is rejected; lvalue works")
    {
        CompilationTester tester;

        SUBCASE("rvalue rejected")
        {
            auto result = tester.compile_and_run(user_id_description, R"(
#include <string>
test::UserId make_id() { return test::UserId{"x"}; }
int main()
{
    std::string const & ref = atlas::cast<std::string const &>(make_id());
    return static_cast<int>(ref.size());
}
)");
            INFO(result.output);
            CHECK_FALSE(result.success);
        }

        SUBCASE("lvalue works, SFINAE matrix agrees")
        {
            auto result = tester.compile_and_run(
                user_id_description,
                std::string(cast_probe) + R"(
#include <cassert>
#include <string>

static_assert(!cast_ok<std::string const &, test::UserId>::value, "");
static_assert(!cast_ok<std::string &&, test::UserId>::value, "");
static_assert(cast_ok<std::string const &, test::UserId &>::value, "");
static_assert(cast_ok<std::string &, test::UserId &>::value, "");

int main()
{
    test::UserId id{"hello"};
    std::string const & ref = atlas::cast<std::string const &>(id);
    assert(&ref == &atlas::undress(id));
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }
    }

    TEST_CASE("cross-type construction: borrows rejected, moves allowed")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Source
description=std::string; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Token
description=std::string_view; ==, no-constexpr, from_atlas_cast<test::Source>

[type]
kind=struct
namespace=test
name=Owned
description=std::string; ==, no-constexpr, from_atlas_cast<test::Source>
)";
        auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

test::Source make_source() { return test::Source{"hello"}; }

// Storing a borrow of an expiring owner is rejected...
static_assert(!std::is_constructible<test::Token, test::Source>::value, "");
// ...but borrowing from an lvalue is fine
static_assert(std::is_constructible<test::Token, test::Source &>::value, "");
// ...and an owning construction from a temporary moves
static_assert(std::is_constructible<test::Owned, test::Source>::value, "");

int main()
{
    test::Source src{"hello"};
    test::Token token{src};
    assert(atlas::undress(token) == "hello");
    assert(atlas::undress(token).data() == atlas::undress(src).data());

    test::Owned owned{make_source()};
    assert(atlas::undress(owned) == "hello");

    test::Source victim{"world"};
    test::Owned moved{std::move(victim)};
    assert(atlas::undress(moved) == "world");
    assert(atlas::undress(victim).empty());
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: file-scoped cross-construction")
{
    TEST_CASE("auto_from_atlas_cast: same-file family, cross-file rejected, "
              "explicit list unions")
    {
        CompilationTester tester;
        auto family = R"(auto_from_atlas_cast=true

[type]
kind=struct
namespace=fam
name=Millis
description=std::int64_t; ==, #<cstdint>

[type]
kind=struct
namespace=fam
name=Seconds
description=std::int64_t; ==, #<cstdint>

[type]
kind=struct
namespace=fam
name=Timeout
description=std::int64_t; ==, from_atlas_cast<other::Foo>, #<cstdint>
)";
        auto outsiders = R"([type]
kind=struct
namespace=xfile
name=Count
description=std::int64_t; ==, #<cstdint>

[type]
kind=struct
namespace=other
name=Foo
description=std::int64_t; ==, #<cstdint>
)";
        auto result = tester.compile_and_run_with_two_type_headers(
            outsiders,
            family,
            R"(
#include <cassert>
#include <type_traits>

int main()
{
    // Same-file family constructs freely, in every direction
    fam::Millis m{fam::Seconds{5}};
    fam::Seconds s{fam::Timeout{9}};
    assert(atlas::undress(m) == 5 && atlas::undress(s) == 9);

    // A cross-file type with the same underlying is rejected...
    static_assert(!std::is_constructible<fam::Millis, xfile::Count>::value, "");
    static_assert(!std::is_constructible<xfile::Count, fam::Millis>::value, "");

    // ...unless named in an explicit allowlist, which unions with the
    // same-file family
    fam::Timeout t1{other::Foo{3}};
    fam::Timeout t2{fam::Millis{4}};
    assert(atlas::undress(t1) == 3 && atlas::undress(t2) == 4);
    static_assert(!std::is_constructible<fam::Millis, other::Foo>::value, "");

    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: accessors stay faithful")
{
    TEST_CASE("forwarded accessors and iterators work on temporaries")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Id
description=std::string; forward=const,c_str,data; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Vec
description=std::vector<int>; iterable, ==, no-constexpr, #<vector>
)";
        auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <cstdio>
#include <numeric>

test::Id make_id() { return test::Id{"hello"}; }
test::Vec make_v() { return test::Vec{std::vector<int>{1, 2, 3}}; }

int main()
{
    // Accessors on a temporary are accepted, same as the underlying type
    std::printf("%s\n", make_id().c_str());
    assert(*make_id().data() == 'h');
    assert(*make_v().begin() == 1);

    // Range-for over a temporary iterates the lifetime-extended value
    int sum = 0;
    for (auto x : make_v()) {
        sum += x;
    }
    assert(sum == 6);
    return 0;
}
)",
            "c++20",
            CompilationTester::is_asan_supported() ? "-fsanitize=address"
                                                   : "");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: declared borrowing types")
{
    TEST_CASE("borrowed_types= guards a third-party view on every route")
    {
        CompilationTester tester;
        auto description = R"(borrowed_types=ext::StringRef

[type]
kind=struct
namespace=test
name=Name
description=std::string; ==, cast<ext::StringRef>, implicit_cast<ext::StringRef>, no-constexpr, #"ext_ref.hpp"
)";
        tester.write_temp_file("ext_ref.hpp", R"(
#pragma once
#include <string>
namespace ext {
struct StringRef
{
    char const * ptr = nullptr;
    std::size_t len = 0;
    StringRef() = default;
    StringRef(std::string const & s) : ptr(s.data()), len(s.size()) {}
};
}
)");
        auto result = tester.compile_and_run(
            description,
            std::string(cast_probe) + R"(
#include <cassert>
#include <type_traits>
#include <utility>

static_assert(atlas::is_borrowed<ext::StringRef>::value, "");

// Direct conversion, copy-initialization, and atlas::cast all reject a
// temporary source...
static_assert(!std::is_constructible<ext::StringRef, test::Name>::value, "");
static_assert(!std::is_convertible<test::Name, ext::StringRef>::value, "");
static_assert(!cast_ok<ext::StringRef, test::Name>::value, "");

// ...and all still work from an lvalue
static_assert(std::is_constructible<ext::StringRef, test::Name &>::value, "");
static_assert(std::is_convertible<test::Name &, ext::StringRef>::value, "");
static_assert(cast_ok<ext::StringRef, test::Name &>::value, "");

int main()
{
    test::Name n{"hello"};
    ext::StringRef r = n;
    assert(r.len == 5);
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("borrowed_types= may name a type defined in the same file")
    {
        CompilationTester tester;

        // The specialization for a declared borrowing type is emitted at the
        // top of the header, so a same-file type needs a forward declaration
        // ahead of it; the header must compile on its own either way the
        // file orders its types.
        SUBCASE("defined before its user")
        {
            auto description = R"(borrowed_types=test::View

[type]
kind=struct
namespace=test
name=View
description=std::string_view; ==, cast<std::string_view>, no-constexpr

[type]
kind=struct
namespace=test
name=Outer
description=test::View; ==, cast<std::string_view>, no-constexpr
)";
            auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <string_view>

static_assert(atlas::is_borrowed<test::View>::value, "");

static char const storage[] = "hello";

int main()
{
    test::Outer o{test::View{std::string_view{storage}}};
    auto sv = static_cast<std::string_view>(o);
    assert(sv == "hello" && sv.data() == storage);
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }

        SUBCASE("defined after its user")
        {
            // The user references the later type through a from_atlas_cast
            // allowlist, the reference a generated header can defer; a field
            // of the not-yet-declared type would be invalid C++ regardless.
            auto description = R"(borrowed_types=test::LateView

[type]
kind=struct
namespace=test
name=Early
description=std::string_view; ==, no-constexpr, from_atlas_cast<test::LateView>

[type]
kind=struct
namespace=test
name=LateView
description=std::string_view; ==, no-constexpr
)";
            auto result = tester.compile_and_run(description, R"(
#include <cassert>
#include <string_view>

static_assert(atlas::is_borrowed<test::LateView>::value, "");

static char const storage[] = "hello";

int main()
{
    test::LateView v{std::string_view{storage}};
    test::Early e{v};
    assert(atlas::undress(e).data() == storage);
    return 0;
}
)");
            INFO(result.output);
            CHECK(result.success);
        }
    }

    TEST_CASE("duplicate declaration across two headers in one TU is benign")
    {
        CompilationTester tester;

        // Generate two headers, both declaring the same borrowed type.
        auto make_header = [&](std::string const & ns,
                               std::string const & name) {
            return "borrowed_types=ext::StringRef\n\n[type]\nkind=struct\n"
                   "namespace=" + ns + "\nname=" + name +
                   "\ndescription=std::string; ==, no-constexpr, "
                   "#\"ext_ref.hpp\"\n";
        };
        tester.write_temp_file("ext_ref.hpp", R"(
#pragma once
#include <string>
namespace ext {
struct StringRef
{
    char const * ptr = nullptr;
    StringRef() = default;
    StringRef(std::string const & s) : ptr(s.data()) {}
};
}
)");
        auto result = tester.compile_and_run_with_two_type_headers(
            make_header("aa", "First"),
            make_header("bb", "Second"),
            R"(
int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("two spellings of one type across two headers is benign")
    {
        CompilationTester tester;

        // Both headers declare the same borrowing type, but only one of them
        // writes the global qualifier.
        auto make_header = [&](std::string const & spelling,
                               std::string const & ns,
                               std::string const & name) {
            return "borrowed_types=" + spelling + "\n\n[type]\nkind=struct\n"
                   "namespace=" + ns + "\nname=" + name +
                   "\ndescription=std::string; ==, no-constexpr, "
                   "#\"ext_ref.hpp\"\n";
        };
        tester.write_temp_file("ext_ref.hpp", R"(
#pragma once
#include <string>
namespace ext {
struct StringRef
{
    char const * ptr = nullptr;
    StringRef() = default;
    StringRef(std::string const & s) : ptr(s.data()) {}
};
}
)");
        auto result = tester.compile_and_run_with_two_type_headers(
            make_header("ext::StringRef", "cc", "Third"),
            make_header("::ext::StringRef", "dd", "Fourth"),
            R"(
#include <type_traits>

// One specialization survives, and it still classifies the type
static_assert(atlas::is_borrowed<ext::StringRef>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: multi-layer chains")
{
    // Chains of atlas types over one root: the guards must classify a
    // wrapper by what it ultimately holds, so view-rooted chains stay inert
    // at every depth and owner-rooted chains stay guarded at every depth.
    constexpr char const * chain_description = R"([type]
kind=struct
namespace=test
name=Token
description=std::string_view; ==, cast<std::string_view>, no-constexpr

[type]
kind=struct
namespace=test
name=Outer
description=test::Token; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Middle
description=test::Token; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Deep
description=test::Middle; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Outer3
description=test::Token; ==, cast<std::string_view>, no-constexpr

[type]
kind=struct
namespace=test
name=Owner
description=std::string; ==, no-constexpr

[type]
kind=struct
namespace=test
name=OwnerWrap
description=test::Owner; ==, no-constexpr

[type]
kind=struct
namespace=test
name=Dest
description=std::string_view; ==, no-constexpr, from_atlas_cast<test::Outer>

[type]
kind=struct
namespace=test
name=DestOwn
description=std::string_view; ==, no-constexpr, from_atlas_cast<test::OwnerWrap>
)";

    TEST_CASE("classification recurses through atlas layers")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(chain_description, R"(
#include <string>
#include <string_view>

// A wrapper whose root is a view is itself a borrowing type, at any depth
static_assert(atlas::is_borrowed<test::Token>::value, "");
static_assert(atlas::is_borrowed<test::Outer>::value, "");
static_assert(atlas::is_borrowed<test::Deep>::value, "");

// A wrapper whose root owns is not
static_assert(!atlas::is_borrowed<test::Owner>::value, "");
static_assert(!atlas::is_borrowed<test::OwnerWrap>::value, "");

// So the policy is inert between a view target and a view-rooted layer...
static_assert(!atlas::borrow_policy<std::string_view, test::Token>::value, "");
// ...and guards an owner-rooted layer, same as the raw owner
static_assert(atlas::borrow_policy<std::string_view, test::Owner>::value, "");
static_assert(atlas::borrow_policy<test::Token, std::string>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("rvalue drilling through view-rooted chains succeeds")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(chain_description, R"(
#include <cassert>
#include <string_view>

static char const storage[] = "hello";

test::Outer make_outer()
{ return test::Outer{test::Token{std::string_view{storage}}}; }
test::Deep make_deep()
{ return test::Deep{test::Middle{test::Token{std::string_view{storage}}}}; }

int main()
{
    // The view is copied out layer by layer; the viewed storage is static,
    // so nothing dangles.
    auto sv2 = atlas::cast<std::string_view>(make_outer());
    assert(sv2 == "hello" && sv2.data() == storage);
    auto sv3 = atlas::cast<std::string_view>(make_deep());
    assert(sv3 == "hello" && sv3.data() == storage);
    return 0;
}
)",
            "c++20",
            CompilationTester::is_asan_supported() ? "-fsanitize=address"
                                                   : "");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("deleted guard is inert on a view-rooted declared conversion")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(chain_description, R"(
#include <cassert>
#include <string_view>

static char const storage[] = "hello";

test::Outer3 make_outer3()
{ return test::Outer3{test::Token{std::string_view{storage}}}; }

int main()
{
    auto sv = static_cast<std::string_view>(make_outer3());
    assert(sv == "hello" && sv.data() == storage);
    return 0;
}
)",
            "c++20",
            CompilationTester::is_asan_supported() ? "-fsanitize=address"
                                                   : "");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("owner-rooted chains stay guarded, in both directions")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(
            chain_description,
            std::string(cast_probe) + R"(
#include <string_view>
#include <utility>

// Drilling out of an expiring owner-rooted chain is still rejected...
static_assert(!cast_ok<std::string_view, test::OwnerWrap>::value, "");
static_assert(!cast_ok<std::string_view, test::OwnerWrap &&>::value, "");
// ...and still borrows from an lvalue
static_assert(cast_ok<std::string_view, test::OwnerWrap &>::value, "");

// Drilling an expiring owner INTO a wrapper-of-view would store a dangling
// view inside the wrapper; that is guarded too.
static_assert(!cast_ok<test::Token, test::Owner>::value, "");
static_assert(cast_ok<test::Token, test::Owner &>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("hand-written protocol adopters are not classified by recursion")
    {
        CompilationTester tester;

        // A hand-written type may adopt the atlas_value_type/atlas_value_for
        // protocol while OWNING storage and computing a view from it in
        // atlas_value_for. Classifying it as borrowed would disarm the
        // guards and let that view dangle, so the layer-by-layer recursion
        // is limited to Atlas-generated types (which derive from
        // strong_type_tag); protocol adopters keep the explicit
        // borrowed_type / enable_borrowed_range opt-in.
        auto description = R"([type]
kind=struct
namespace=test
name=Outer
description=ext::SelfRef; ==, no-constexpr, #"self_ref.hpp"

[type]
kind=struct
namespace=test
name=Dest
description=std::string_view; ==, no-constexpr, from_atlas_cast<test::Outer>
)";
        tester.write_temp_file("self_ref.hpp", R"(
#pragma once
#include <string>
#include <string_view>
namespace ext {
struct SelfRef
{
    std::string storage;                        // owns...
    using atlas_value_type = std::string_view;  // ...and yields a view
    SelfRef() = default;
    explicit SelfRef(std::string s) : storage(std::move(s)) {}
    friend std::string_view atlas_value_for(SelfRef const & s)
    { return std::string_view(s.storage); }
    bool operator == (SelfRef const &) const = default;
};
}
)");
        auto result = tester.compile_and_run(
            description,
            std::string(cast_probe) + R"(
#include <string_view>
#include <type_traits>

// Not auto-classified: it owns the storage its view points into
static_assert(!atlas::is_borrowed<ext::SelfRef>::value, "");
static_assert(!atlas::is_borrowed<test::Outer>::value, "");

// So every rvalue route stays guarded...
static_assert(!cast_ok<std::string_view, test::Outer>::value, "");
static_assert(!std::is_constructible<test::Dest, test::Outer>::value, "");
// ...and borrowing from an lvalue still works
static_assert(cast_ok<std::string_view, test::Outer &>::value, "");
static_assert(std::is_constructible<test::Dest, test::Outer &>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("allowlist construction follows the chain's root")
    {
        CompilationTester tester;
        auto result = tester.compile_and_run(chain_description, R"(
#include <cassert>
#include <string_view>
#include <type_traits>

static char const storage[] = "hello";

test::Outer make_outer()
{ return test::Outer{test::Token{std::string_view{storage}}}; }

// View-rooted source: the view is copied out of the temporary, so
// construction is allowed from any value category
static_assert(std::is_constructible<test::Dest, test::Outer>::value, "");

// Owner-rooted source: a temporary would leave the stored view dangling
static_assert(!std::is_constructible<test::DestOwn, test::OwnerWrap>::value, "");
static_assert(std::is_constructible<test::DestOwn, test::OwnerWrap &>::value, "");

int main()
{
    test::Dest d{make_outer()};
    assert(atlas::undress(d) == "hello");
    assert(atlas::undress(d).data() == storage);
    return 0;
}
)",
            "c++20",
            CompilationTester::is_asan_supported() ? "-fsanitize=address"
                                                   : "");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: third-party opt-in")
{
    TEST_CASE("enable_borrowed_range alone classifies a C++20 view")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Buf
description=std::string; ==, cast<my::View>, no-constexpr, #"my_view.hpp"
)";
        tester.write_temp_file("my_view.hpp", R"(
#pragma once
#include <ranges>
#include <string>
namespace my {
struct View
{
    char const * b = nullptr;
    char const * e = nullptr;
    View() = default;
    View(std::string const & s) : b(s.data()), e(s.data() + s.size()) {}
    char const * begin() const { return b; }
    char const * end() const { return e; }
};
}
template <>
inline constexpr bool std::ranges::enable_borrowed_range<my::View> = true;
)");
        auto result = tester.compile_and_run(description, R"(
#include <type_traits>

// Classified with no Atlas-specific declaration anywhere
static_assert(atlas::is_borrowed<my::View>::value, "");
static_assert(!std::is_constructible<my::View, test::Buf>::value, "");
static_assert(std::is_constructible<my::View, test::Buf &>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }

    TEST_CASE("double opt-in is not ambiguous")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Buf
description=std::string; ==, no-constexpr, #"my_view.hpp"
)";
        tester.write_temp_file("my_view.hpp", R"(
#pragma once
#include <ranges>
#include <string>
namespace my {
struct View
{
    char const * b = nullptr;
    View() = default;
    View(std::string const & s) : b(s.data()) {}
    char const * begin() const { return b; }
    char const * end() const { return b; }
};
}
template <>
inline constexpr bool std::ranges::enable_borrowed_range<my::View> = true;
)");
        auto result = tester.compile_and_run(description, R"(
#include <type_traits>

// Atlas seed on top of the ranges opt-in: a disjunction of expressions,
// not competing partial specializations, so both together are fine.
namespace atlas {
template <> struct borrowed_type<my::View> : std::true_type {};
}

static_assert(atlas::is_borrowed<my::View>::value, "");
static_assert(atlas::borrow_policy<my::View, std::string>::value, "");

int main() { return 0; }
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

TEST_SUITE("Borrow Safety: guard matrix across standards")
{
    // Generated std::string-backed types do not compile below C++17 for
    // reasons that predate the guards, so the C++11/14 legs verify the
    // trait machinery and the guard construct with a hand-written
    // string-backed stand-in that uses the same preamble and guard text.
    constexpr char const * standin_test_code = R"(
#include <string>
#include <type_traits>
#include <utility>

namespace ext {
struct Ref
{
    char const * p;
    Ref() : p(0) {}
    Ref(std::string const & s) : p(s.c_str()) {}
};
}

namespace atlas {
template <> struct borrowed_type<ext::Ref> : std::true_type {};
}

namespace test {
struct Standin
{
    std::string value;
    explicit Standin(char const * s) : value(s) {}

    // The exact guard text the generator emits
    template <
        typename BorrowTargetT,
        typename = atlas::atlas_detail::enable_if_t<
            atlas::borrow_policy<BorrowTargetT, std::string>::value>>
    operator BorrowTargetT () const && = delete;

    operator ext::Ref () const & { return ext::Ref(value); }
    explicit operator std::string () const & { return value; }
};
}

// User opt-in works below C++20, with no dependency on std::ranges
static_assert(atlas::is_borrowed<ext::Ref>::value, "opt-in");
static_assert(atlas::borrow_policy<ext::Ref, std::string>::value, "policy");
static_assert(!atlas::borrow_policy<std::string, std::string>::value, "own");

// The guard rejects rvalue borrows and leaves everything else alone
static_assert(!std::is_convertible<test::Standin, ext::Ref>::value, "guarded");
static_assert(std::is_convertible<test::Standin &, ext::Ref>::value, "lvalue");
static_assert(std::is_constructible<std::string, test::Standin>::value, "own");

int main()
{
    test::Standin s("hello");
    ext::Ref r = s;
    std::string owned = static_cast<std::string>(std::move(s));
    return (r.p != 0 && owned == "hello") ? 0 : 1;
}
)";

    TEST_CASE("C++11 and C++14: traits and guard via stand-in")
    {
        CompilationTester tester;
        // An int-backed generated type carries the preamble; the guard on
        // it must be inert for scalar underlyings at every standard.
        auto description = R"([type]
kind=struct
namespace=test
name=Count
description=int; ==, cast<long>
default_value=0
)";
        for (auto standard : {"c++11", "c++14"}) {
            if (not CompilationTester::is_cpp_standard_supported(standard)) {
                continue;
            }
            CAPTURE(standard);
            auto result = tester.compile_and_run(
                description,
                std::string(R"(
static_assert(!atlas::is_borrowed<int>::value, "int");
static_assert(!atlas::is_borrowed<std::string>::value, "string");
static_assert(!atlas::borrow_policy<long, int>::value, "scalar");
namespace test {
inline long as_long(Count const & c) { return static_cast<long>(c); }
inline long as_long_rv(Count && c) { return static_cast<long>(std::move(c)); }
}
)") + standin_test_code,
                standard);
            INFO(result.output);
            CHECK(result.success);
        }
    }

    TEST_CASE("C++17 and C++20: generated string-backed guard matrix")
    {
        CompilationTester tester;
        for (auto standard : {"c++17", "c++20"}) {
            if (not CompilationTester::is_cpp_standard_supported(standard)) {
                continue;
            }
            CAPTURE(standard);

            auto rejected = tester.compile_and_run(user_id_description, R"(
#include <string_view>
test::UserId make_id() { return test::UserId{"x"}; }
int main()
{
    std::string_view sv = static_cast<std::string_view>(make_id());
    return static_cast<int>(sv.size());
}
)",
                standard);
            INFO(rejected.output);
            CHECK_FALSE(rejected.success);

            auto accepted = tester.compile_and_run(user_id_description, R"(
#include <cassert>
#include <string_view>
test::UserId make_id() { return test::UserId{"hello"}; }
int main()
{
    test::UserId id{"hello"};
    std::string_view sv = static_cast<std::string_view>(id);
    std::string owned = static_cast<std::string>(make_id());
    assert(sv == "hello" && owned == "hello");
    return 0;
}
)",
                standard);
            INFO(accepted.output);
            CHECK(accepted.success);
        }
    }
}

TEST_SUITE("Borrow Safety: no interference with other operators")
{
    TEST_CASE("kitchen-sink type with guard present behaves normally")
    {
        CompilationTester tester;
        auto description = R"([type]
kind=struct
namespace=test
name=Sink
description=int; ==, <=>, bool, +, -, ++, u-, cast<long>
default_value=0

[type]
kind=struct
namespace=test
name=Bag
description=std::vector<int>; iterable, [], ==, no-constexpr, #<vector>
)";
        auto result = tester.compile_and_run(description, R"(
#include <cassert>
int main()
{
    test::Sink a{1};
    test::Sink b{2};
    assert(a < b && a != b && (a + b) == test::Sink{3});
    assert(static_cast<bool>(a));
    assert(static_cast<long>(-a) == -1);
    ++a;
    assert(a == b);

    test::Bag bag{std::vector<int>{5, 6}};
    assert(bag[0] == 5);
    int sum = 0;
    for (auto x : bag) { sum += x; }
    assert(sum == 11);
    return 0;
}
)");
        INFO(result.output);
        CHECK(result.success);
    }
}

} // anonymous namespace
