// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#include "DomainTypes.hpp"

#include <string>

#include <iostream>

int
main()
{
    std::cout << "Atlas CMake Helpers - File-based Types Example\n";
    std::cout << "===============================================\n\n";

    // All types defined in types.txt configuration file
    domain::CustomerId customer{12345};
    domain::OrderId order1{100};
    domain::OrderId order2{200};

    std::cout << "Customer and Order IDs:\n";
    std::cout << "  customer: " << customer.value << "\n";
    std::cout << "  order1: " << order1.value << "\n";
    std::cout << "  order2: " << order2.value << "\n";
    std::cout << "  order1 == order2: " << (order1 == order2 ? "true" : "false")
        << "\n";
    std::cout << "\n";

    // Money with arithmetic operations
    domain::Money price{99.99};
    domain::Money tax{8.50};
    domain::Money total = price + tax;

    std::cout << "Money calculations:\n";
    std::cout << "  price: " << price << "\n";
    std::cout << "  tax: " << tax << "\n";
    std::cout << "  total: " << total << "\n";
    std::cout << "  price * 2: " << (price * 2.0) << "\n";
    std::cout << "\n";

    // Quantity with increment/decrement
    domain::Quantity qty{10};
    std::cout << "Quantity operations:\n";
    std::cout << "  initial: " << qty << "\n";
    ++qty;
    std::cout << "  after ++: " << qty << "\n";
    qty = qty + domain::Quantity{5};
    std::cout << "  after +5: " << qty << "\n";
    qty = qty - domain::Quantity{3};
    std::cout << "  after -3: " << qty << "\n";
    std::cout << "\n";

    // EmailAddress (string-based)
    domain::EmailAddress email1{std::string("alice@example.com")};
    domain::EmailAddress email2{std::string("bob@example.com")};
    domain::EmailAddress email3{std::string("alice@example.com")};

    std::cout << "Email addresses:\n";
    std::cout << "  email1 == email2: " << (email1 == email2 ? "true" : "false")
        << "\n";
    std::cout << "  email1 == email3: " << (email1 == email3 ? "true" : "false")
        << "\n";
    std::cout << "  email1 <=> email2: " << static_cast<int>(email1 <=> email2)
        << "\n";

    std::cout << "\nAll file-based type examples completed successfully!\n";
    return 0;
}
