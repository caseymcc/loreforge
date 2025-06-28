#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

TEST(LoreForgeTest, CppFile) {
    std::string output = exec("./loreforge ../../tests/cpp_test_files/sample.cpp");
    ASSERT_NE(output.find("MyClass"), std::string::npos);
    ASSERT_NE(output.find("myMethod"), std::string::npos);
    ASSERT_NE(output.find("myFunction"), std::string::npos);
}

TEST(LoreForgeTest, PythonFile) {
    std::string output = exec("./loreforge ../../tests/python_test_files/sample.py");
    ASSERT_NE(output.find("MyClass"), std::string::npos);
    ASSERT_NE(output.find("my_method"), std::string::npos);
    ASSERT_NE(output.find("my_function"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}