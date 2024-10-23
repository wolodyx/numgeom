#include "gtest/gtest.h"

#include <string.h>

#include <filesystem>
#include <fstream>

bool HasFlag(char** argv, const char* name);

namespace testing {
    bool GTEST_FLAG(write_output) = (false); //< "Write results to file"
}


int main(int argc, char** argv)
{
    GTEST_FLAG_SET(write_output, HasFlag(argv,"--write-output"));

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


bool HasFlag(char** argv, const char* name)
{
    char** args = argv + 1;
    while(*args != nullptr)
    {
        const char* arg = *args;
        if(strcmp(arg,name) == 0)
            return true;
        ++args;
    }
    return false;
}

