// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#include "Handle.hpp"
#include "Price.hpp"
#include "UserId.hpp"

#include <iostream>

int
main()
{
    std::cout << "Atlas CMake Helpers - Basic Example\n";
    std::cout << "====================================\n\n";

    // UserId - using simplified atlas_add_type()
    example::UserId user1{42};
    example::UserId user2{42};
    example::UserId user3{99};

    std::cout << "UserId examples:\n";
    std::cout << "  user1 == user2: " << (user1 == user2 ? "true" : "false")
        << "\n";
    std::cout << "  user1 != user3: " << (user1 != user3 ? "true" : "false")
        << "\n";
    std::cout << "  user1 < user3: " << (user1 < user3 ? "true" : "false")
        << "\n";
    std::cout << "\n";

    // Price - using add_atlas_strong_type() with arithmetic
    example::Price price1{100.50};
    example::Price price2{50.25};

    std::cout << "Price examples:\n";
    std::cout << "  price1: " << price1 << "\n";
    std::cout << "  price2: " << price2 << "\n";
    std::cout << "  price1 + price2: " << (price1 + price2) << "\n";
    std::cout << "  price1 - price2: " << (price1 - price2) << "\n";
    std::cout << "  price1 > price2: " << (price1 > price2 ? "true" : "false")
        << "\n";
    std::cout << "\n";

    // Handle - generated without target integration
    example::Handle h1{12345};
    example::Handle h2{67890};
    example::Handle h3{0}; // Represents null

    std::cout << "Handle examples:\n";
    std::cout << "  h1: " << h1.value << "\n";
    std::cout << "  h2: " << h2.value << "\n";
    std::cout << "  h3 (null): " << h3.value << "\n";
    std::cout << "  h1 valid: " << (h1 ? "true" : "false") << "\n";
    std::cout << "  h3 valid: " << (h3 ? "true" : "false") << "\n";
    std::cout << "  h1 == h2: " << (h1 == h2 ? "true" : "false") << "\n";

    std::cout << "\nAll examples completed successfully!\n";
    return 0;
}
