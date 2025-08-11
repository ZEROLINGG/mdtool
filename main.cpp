// main.cpp
#include <iostream>
#include "cli.h"
#include "other/coding.h"


int main(const int argc, char* argv[]) {
    // coding_test();
    const auto [options, funcPtr, success] = cli_work(argc, argv);
    if (!success) {
        return 1;
    }

    try {
        funcPtr(options);
    } catch (const std::exception& e) {
        std::cout << "Ö´ÐÐ´íÎó: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cout << "Ö´ÐÐ´íÎó: Î´ÖªÒì³£" << std::endl;
        return 2;
    }



    return 0;
}