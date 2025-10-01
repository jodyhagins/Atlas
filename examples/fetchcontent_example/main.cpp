// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "Price.hpp"
#include "UserId.hpp"

#include <iostream>

int
main()
{
    // Create a UserId
    example::UserId userId{42};
    example::UserId anotherUserId{100};

    // Test comparison operators
    if (userId == anotherUserId) {
        std::cout << "User IDs are equal\n";
    } else {
        std::cout << "User IDs are different\n";
    }

    // Create a Price
    example::Price price1{19.99};
    example::Price price2{5.00};

    // Test arithmetic operators
    example::Price total = price1 + price2;
    std::cout << "Total price: " << total << "\n";

    example::Price discount = price1 - price2;
    std::cout << "Price after discount: " << discount << "\n";

    // Test comparison
    if (price1 > price2) {
        std::cout << "price1 is greater than price2\n";
    }

    std::cout << "FetchContent integration test passed!\n";
    return 0;
}
