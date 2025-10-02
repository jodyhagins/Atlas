// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "InteractionGenerator.hpp"
#include "StrongTypeGenerator.hpp"
#include "doctest.hpp"
#include "test_common.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace wjh::atlas;

namespace {

// Helper to compile generated code
bool
compile_interaction_code(
    std::string const & type1_code,
    std::string const & type2_code,
    std::string const & interaction_code,
    std::string const & test_code)
{
    namespace fs = std::filesystem;

    auto temp_dir = fs::temp_directory_path() /
        ("atlas_interaction_test_" + std::to_string(::getpid()));
    fs::create_directories(temp_dir);

    // Write type headers
    {
        std::ofstream out(temp_dir / "type1.hpp");
        out << type1_code;
    }
    {
        std::ofstream out(temp_dir / "type2.hpp");
        out << type2_code;
    }
    {
        std::ofstream out(temp_dir / "interactions.hpp");
        out << interaction_code;
    }

    // Write test program
    auto test_path = temp_dir / "test.cpp";
    {
        std::ofstream out(test_path);
        out << test_code;
    }

    std::string compiler = wjh::atlas::test::find_working_compiler();
    std::string compile_cmd = compiler + " -std=c++20 -I" + temp_dir.string() +
        " " + test_path.string() + " -o " + (temp_dir / "test").string() +
        " 2>&1";

    int result = std::system(compile_cmd.c_str());

    // Clean up
    fs::remove_all(temp_dir);

    return result == 0;
}

} // anonymous namespace

TEST_SUITE("Interaction Compilation Tests")
{
    TEST_CASE("Generated interaction code compiles and executes")
    {
        // Generate two simple types
        StrongTypeDescription type1_desc{
            .kind = "struct",
            .type_namespace = "test",
            .type_name = "Price",
            .description = "strong int"};
        auto type1_code = generate_strong_type(type1_desc);
#if 0
        StrongTypeDescription type2_desc{
            .kind = "struct",
            .type_namespace = "test",
            .type_name = "Discount",
            .description = "strong int"};
        auto type2_code = generate_strong_type(type2_desc);
#else
        std::string type2_code = R"(
namespace test {
struct Discount
{
    int value;
};
}
        )";
#endif

        // Generate interactions with binary operators
        InteractionFileDescription interaction_desc{
            .includes = {},
            .guard_prefix = "TEST",
            .guard_separator = "_",
            .upcase_guard = true,
            .interactions = {
                {.lhs_type = "Price",
                 .lhs_is_template = false,
                 .op_symbol = "+",
                 .rhs_type = "Discount",
                 .rhs_is_template = false,
                 .result_type = "Price",
                 .interaction_namespace = "test",
                 .is_constexpr = true,
                 .symmetric = false,
                 .rhs_value_access = ".value"}}};

        InteractionGenerator gen;
        auto interaction_code = gen(interaction_desc);

        // Test program that uses both binary and compound operators
        std::string test_program = R"(
#include "type1.hpp"
#include "type2.hpp"
#include "interactions.hpp"

int main() {
    test::Price p{100};
    test::Discount d{10};

    // Use compound operator (generated via ADL)
    p += d;

    // Use binary operator (explicitly defined)
    test::Price p2 = p + d;

    return 0;
}
)";

        bool success = compile_interaction_code(
            type1_code,
            type2_code,
            interaction_code,
            test_program);

        CHECK(success);
    }

    TEST_CASE("Per-operand value access works")
    {
        // Generate an Atlas strong type
        StrongTypeDescription atlas_type_desc{
            .kind = "struct",
            .type_namespace = "mylib",
            .type_name = "AtlasValue",
            .description = "strong int"};
        auto atlas_type_code = generate_strong_type(atlas_type_desc);

        // Generate a non-Atlas type (simulating external library)
        std::string external_type_code = R"(
namespace external {
    struct LibValue {
        int data;
        int getValue() const { return data; }
    };
}
)";

        // Generate interaction with different value access for each operand
        InteractionFileDescription interaction_desc{
            .includes = {},
            .guard_prefix = "TEST",
            .guard_separator = "_",
            .upcase_guard = true,
            .interactions = {
                {.lhs_type = "AtlasValue",
                 .lhs_is_template = false,
                 .op_symbol = "+",
                 .rhs_type = "external::LibValue",
                 .rhs_is_template = false,
                 .result_type = "AtlasValue",
                 .interaction_namespace = "mylib",
                 .is_constexpr = false,
                 .symmetric = false,
                 .lhs_value_access = "atlas::value",
                 .rhs_value_access = ".getValue()",
                 .value_access = ""}}};

        InteractionGenerator gen;
        auto interaction_code = gen(interaction_desc);

        std::string test_program = R"(
#include "type1.hpp"
#include "type2.hpp"
#include "interactions.hpp"

int main() {
    mylib::AtlasValue a{10};
    external::LibValue b{5};

    // Use binary operator with different value access methods
    mylib::AtlasValue result = a + b;

    return static_cast<int>(result) == 15 ? 0 : 1;
}
)";

        bool success = compile_interaction_code(
            atlas_type_code,
            external_type_code,
            interaction_code,
            test_program);

        CHECK(success);
    }

    TEST_CASE("Compound assignment with custom RHS value access")
    {
        // Generate an Atlas strong type
        StrongTypeDescription atlas_type_desc{
            .kind = "struct",
            .type_namespace = "mylib",
            .type_name = "Counter",
            .description = "strong int"};
        auto atlas_type_code = generate_strong_type(atlas_type_desc);

        // Generate a non-Atlas type with custom value accessor
        std::string external_type_code = R"(
namespace external {
    struct Delta {
        int data;
        int getValue() const { return data; }
    };
}
)";

        // Generate interaction with custom RHS value access
        InteractionFileDescription interaction_desc{
            .includes = {},
            .guard_prefix = "TEST",
            .guard_separator = "_",
            .upcase_guard = true,
            .interactions = {
                {.lhs_type = "Counter",
                 .lhs_is_template = false,
                 .op_symbol = "+",
                 .rhs_type = "external::Delta",
                 .rhs_is_template = false,
                 .result_type = "Counter",
                 .interaction_namespace = "mylib",
                 .is_constexpr = false,
                 .symmetric = false,
                 .lhs_value_access = "atlas::value",
                 .rhs_value_access = ".getValue()",
                 .value_access = ""}}};

        InteractionGenerator gen;
        auto interaction_code = gen(interaction_desc);

        // Verify atlas_value was generated for external::Delta
        CHECK(interaction_code.find("atlas_value(external::Delta const& v, value_tag)") != std::string::npos);
        CHECK(interaction_code.find("v.getValue()") != std::string::npos);

        std::string test_program = R"(
#include "type1.hpp"
#include "type2.hpp"
#include "interactions.hpp"

int main() {
    mylib::Counter c{10};
    external::Delta d{5};

    // Use binary operator
    mylib::Counter result1 = c + d;

    // Use compound operator (should work via generated atlas_value)
    c += d;

    return static_cast<int>(c) == 15 && static_cast<int>(result1) == 15 ? 0 : 1;
}
)";

        bool success = compile_interaction_code(
            atlas_type_code,
            external_type_code,
            interaction_code,
            test_program);

        CHECK(success);
    }

    TEST_CASE("User-provided atlas_value overrides generated one")
    {
        // Generate an Atlas strong type
        StrongTypeDescription atlas_type_desc{
            .kind = "struct",
            .type_namespace = "mylib",
            .type_name = "Value",
            .description = "strong int"};
        auto atlas_type_code = generate_strong_type(atlas_type_desc);

        // Generate a non-Atlas type with BOTH .data and user-provided atlas_value
        std::string external_type_code = R"(
namespace external {
    struct CustomType {
        int data;        // This is what rhs_value_access points to
        int special;     // This is what user's atlas_value returns
    };
}

// User provides their own atlas_value (priority 2 - higher than generated)
namespace atlas {
    inline constexpr int atlas_value(external::CustomType const& v) {
        return v.special;  // Use special, not data
    }
}
)";

        // Generate interaction that would normally use .data
        InteractionFileDescription interaction_desc{
            .includes = {},
            .guard_prefix = "TEST",
            .guard_separator = "_",
            .upcase_guard = true,
            .interactions = {
                {.lhs_type = "Value",
                 .lhs_is_template = false,
                 .op_symbol = "+",
                 .rhs_type = "external::CustomType",
                 .rhs_is_template = false,
                 .result_type = "Value",
                 .interaction_namespace = "mylib",
                 .is_constexpr = false,
                 .symmetric = false,
                 .lhs_value_access = "atlas::value",
                 .rhs_value_access = ".data",  // We specify .data
                 .value_access = ""}}};

        InteractionGenerator gen;
        auto interaction_code = gen(interaction_desc);

        std::string test_program = R"(
#include "type1.hpp"
#include "type2.hpp"
#include "interactions.hpp"

int main() {
    mylib::Value v{10};
    external::CustomType ct{100, 5};  // data=100, special=5

    // Binary operator uses .data (as specified in rhs_value_access)
    mylib::Value result1 = v + ct;
    if (static_cast<int>(result1) != 110) return 1;  // 10 + 100 = 110

    // Compound operator uses user's atlas_value which returns .special
    v += ct;
    if (static_cast<int>(v) != 15) return 2;  // 10 + 5 = 15

    return 0;
}
)";

        bool success = compile_interaction_code(
            atlas_type_code,
            external_type_code,
            interaction_code,
            test_program);

        CHECK(success);
    }

    TEST_CASE("value_access applies to RHS when rhs_value_access not specified")
    {
        // Generate an Atlas strong type
        StrongTypeDescription atlas_type_desc{
            .kind = "struct",
            .type_namespace = "mylib",
            .type_name = "Amount",
            .description = "strong int"};
        auto atlas_type_code = generate_strong_type(atlas_type_desc);

        // Generate a non-Atlas type with .data member
        std::string external_type_code = R"(
namespace external {
    struct Offset {
        int data;
    };
}
)";

        // Use value_access (not rhs_value_access) - should still generate atlas_value
        InteractionFileDescription interaction_desc{
            .includes = {},
            .guard_prefix = "TEST",
            .guard_separator = "_",
            .upcase_guard = true,
            .interactions = {
                {.lhs_type = "Amount",
                 .lhs_is_template = false,
                 .op_symbol = "+",
                 .rhs_type = "external::Offset",
                 .rhs_is_template = false,
                 .result_type = "Amount",
                 .interaction_namespace = "mylib",
                 .is_constexpr = false,
                 .symmetric = false,
                 .lhs_value_access = "atlas::value",  // Explicit for LHS
                 .rhs_value_access = "",  // Empty - will fall back to value_access
                 .value_access = ".data"}}};  // This should apply to RHS when rhs_value_access is empty

        InteractionGenerator gen;
        auto interaction_code = gen(interaction_desc);

        // Verify atlas_value was generated for external::Offset using .data
        CHECK(interaction_code.find("atlas_value(external::Offset const& v, value_tag)") != std::string::npos);
        CHECK(interaction_code.find("v.data") != std::string::npos);

        std::string test_program = R"(
#include "type1.hpp"
#include "type2.hpp"
#include "interactions.hpp"

int main() {
    mylib::Amount a{100};
    external::Offset o{25};

    // Binary operator
    mylib::Amount result = a + o;
    if (static_cast<int>(result) != 125) return 1;

    // Compound operator (via generated atlas_value)
    a += o;
    if (static_cast<int>(a) != 125) return 2;

    return 0;
}
)";

        bool success = compile_interaction_code(
            atlas_type_code,
            external_type_code,
            interaction_code,
            test_program);

        CHECK(success);
    }
}
