// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_8A2430C93F9248C19DDBE4C08F13238E
#define WJH_ATLAS_8A2430C93F9248C19DDBE4C08F13238E

#include "atlas/StrongTypeGenerator.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "rapidcheck.hpp"

namespace wjh::atlas::testing::generators {

// Generate valid C++ identifier (starts with letter, then alphanumeric)
inline auto
cppIdentifier()
-> ::rc::Gen<std::string>
{
    return ::rc::gen::apply(
        [](char first, std::string rest) {
            return std::string(1, first) + rest;
        },
        // First character: A-Z or a-z
        ::rc::gen::element(
            'A',
            'B',
            'C',
            'D',
            'E',
            'F',
            'G',
            'H',
            'I',
            'J',
            'K',
            'L',
            'M',
            'N',
            'O',
            'P',
            'Q',
            'R',
            'S',
            'T',
            'U',
            'V',
            'W',
            'X',
            'Y',
            'Z',
            'a',
            'b',
            'c',
            'd',
            'e',
            'f',
            'g',
            'h',
            'i',
            'j',
            'k',
            'l',
            'm',
            'n',
            'o',
            'p',
            'q',
            'r',
            's',
            't',
            'u',
            'v',
            'w',
            'x',
            'y',
            'z'),
        // Remaining characters: alphanumeric or underscore (0-20 chars)
        ::rc::gen::container<std::string>(::rc::gen::element(
            'a',
            'b',
            'c',
            'd',
            'e',
            'f',
            'g',
            'h',
            'i',
            'j',
            'k',
            'l',
            'm',
            'n',
            'o',
            'p',
            'q',
            'r',
            's',
            't',
            'u',
            'v',
            'w',
            'x',
            'y',
            'z',
            'A',
            'B',
            'C',
            'D',
            'E',
            'F',
            'G',
            'H',
            'I',
            'J',
            'K',
            'L',
            'M',
            'N',
            'O',
            'P',
            'Q',
            'R',
            'S',
            'T',
            'U',
            'V',
            'W',
            'X',
            'Y',
            'Z',
            '0',
            '1',
            '2',
            '3',
            '4',
            '5',
            '6',
            '7',
            '8',
            '9',
            '_')));
}

// Generate valid namespace (empty, single, or nested)
inline auto
cppNamespace()
-> ::rc::Gen<std::string>
{
    return ::rc::gen::oneOf(
        // No namespace
        ::rc::gen::just(std::string("")),

        // Single namespace
        cppIdentifier(),

        // Nested namespace (2-4 levels)
        ::rc::gen::apply(
            [](std::vector<std::string> const & parts) {
                std::string result;
                for (size_t i = 0; i < parts.size(); ++i) {
                    if (i > 0) {
                        result += "::";
                    }
                    result += parts[i];
                }
                return result;
            },
            ::rc::gen::container<std::vector<std::string>>(cppIdentifier())));
}

// Generate valid underlying type
inline auto
underlyingType()
-> ::rc::Gen<std::string>
{
    return ::rc::gen::element<std::string>(
        "int",
        "unsigned int",
        "long",
        "unsigned long",
        "int32_t",
        "int64_t",
        "uint32_t",
        "uint64_t",
        "double",
        "float",
        "bool",
        "std::string",
        "std::vector<int>",
        "std::optional<int>");
}

// Generate valid operator set
inline auto
operatorSet()
-> ::rc::Gen<std::vector<std::string>>
{
    return ::rc::gen::apply(
        [](std::vector<std::string> ops) {
            // Deduplicate
            std::sort(ops.begin(), ops.end());
            ops.erase(std::unique(ops.begin(), ops.end()), ops.end());
            return ops;
        },
        ::rc::gen::container<std::vector<std::string>>(
            ::rc::gen::element<std::string>(
                "+",
                "-",
                "*",
                "/",
                "%",
                "==",
                "!=",
                "<",
                "<=",
                ">",
                ">=",
                "<=>",
                "++",
                "--",
                "&",
                "|",
                "^",
                "<<",
                ">>",
                "@",
                "->",
                "[]",
                // Note: "in", "out", "hash", "fmt" are deprecated.
                // Use auto_istream, auto_ostream, auto_hash, auto_format
                // instead.
                "iterable",
                "assign",
                "bool")));
}

// Generate complete type description
inline auto
typeDescription()
-> ::rc::Gen<wjh::atlas::StrongTypeDescription>
{
    return ::rc::gen::apply(
        [](std::string kind,
           std::string type_namespace,
           std::string type_name,
           std::string utype,
           std::vector<std::string> ops) {
            std::string desc = "strong " + utype;
            if (not ops.empty()) {
                desc += "; ";
                for (size_t i = 0; i < ops.size(); ++i) {
                    if (i > 0) {
                        desc += ", ";
                    }
                    desc += ops[i];
                }
            }

            return wjh::atlas::StrongTypeDescription{
                .kind = kind,
                .type_namespace = type_namespace,
                .type_name = type_name,
                .description = desc};
        },
        ::rc::gen::element<std::string>("struct", "class"),
        cppNamespace(),
        cppIdentifier(),
        underlyingType(),
        operatorSet());
}

} // namespace wjh::atlas::testing::generators

#endif // WJH_ATLAS_8A2430C93F9248C19DDBE4C08F13238E
