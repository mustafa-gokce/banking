#include "Tools.h"
#include <random>

namespace Tools {

    std::string Tools::random_string(std::string::size_type length) {
        static auto &chars = "0123456789"
                             "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        thread_local static std::mt19937 rg{std::random_device{}()};
        thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chars) - 2);

        std::string s;

        s.reserve(length);

        while (length--)
            s += chars[pick(rg)];

        return s;
    }
} // Tools