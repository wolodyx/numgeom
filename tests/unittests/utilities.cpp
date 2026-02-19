#include "utilities.h"

#include <gtest/gtest.h>

#include "configure.h"


namespace {;

//! ���� � ��������� �������� � ��������� �������.
std::filesystem::path GetTestDataDir()
{
    return TEST_DATA_DIR;
}

std::filesystem::path GetExternalTestDataDir()
{
    using namespace std::filesystem;
#ifdef EXTERNAL_TEST_DATA_DIR
    return EXTERNAL_TEST_DATA_DIR;
#else
    return path();
#endif
}
}


std::filesystem::path TestData(const std::string& fileName)
{
    return GetTestDataDir()/fileName;
}


std::string GetTestName()
{
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = test_info->name();
    std::replace(testName.begin(), testName.end(), '/', '_');
    return std::string(test_info->test_suite_name()) + '-' + testName;
}
