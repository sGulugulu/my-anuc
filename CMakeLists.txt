cmake_minimum_required(VERSION 3.16)
project(anuc)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Debug")

file(GLOB_RECURSE LIB_SRC "lib/*.cpp")
file(GLOB_RECURSE GERATED "generated/*.cpp")

include_directories(lib/IR lib/ADT lib/Analysis lib/BackEnd third_party/antlr4-runtime)
link_directories(third_party/lib)

add_executable(a main.cpp ${LIB_SRC} ${GERATED})
target_link_libraries(a libantlr4-runtime.a)

#google test
set(GTest_DIR /usr/local/lib/cmake/GTest)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
enable_testing()

#测试ir
add_executable(irtest _unittest/IR/test.cc ${LIB_SRC})
target_link_libraries(irtest GTest::gtest GTest::gtest_main)
add_test(AllTest irtest)

