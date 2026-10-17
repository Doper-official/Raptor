#include "first_app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        rap::RaptorFirstApp app{};
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}