#ifndef BANKING_TOOLS_H
#define BANKING_TOOLS_H

#include <string>

namespace Tools {

    class Tools {

    public:

        /*
         * Generates a random string of length characters.
         */
        static std::string random_string(std::string::size_type length);
    };

} // Tools

#endif //BANKING_TOOLS_H
