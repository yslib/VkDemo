cmake_minimum_required(VERSION 3.12)


file(GLOB_RECURSE SRC *.cpp)
add_executable(test_spirv)
target_sources(test_spirv PRIVATE ${SRC} "../src/SPIRVConverter.cpp")
target_include_directories(test_spirv PRIVATE "../include")

enable_testing()
find_package(GTest CONFIG REQUIRED)
target_link_libraries(test_spirv GTest::gtest_main GTest::gtest GTest::gmock GTest::gmock_main SPIRV)

include(GoogleTest)
gtest_add_tests(test_spirv "" AUTO)
