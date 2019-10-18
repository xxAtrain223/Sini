cmake_minimum_required(VERSION 3.1)
project(sini LANGUAGES CXX)

option(${PROJECT_NAME}_ENABLE_TESTING "Enable Testing for ${PROJECT_NAME}" ON)

add_subdirectory(ext)

find_package(Boost 1.67 REQUIRED)

file(GLOB_RECURSE ${PROJECT_NAME}_HEADERS "include/*.hpp")

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_link_libraries(${PROJECT_NAME} INTERFACE Boost::headers)

if (${PROJECT_NAME}_ENABLE_TESTING)
    file(GLOB_RECURSE ${PROJECT_NAME}_TEST_SOURCES "test/*.[ch]pp")

    add_executable(${PROJECT_NAME}Test ${${PROJECT_NAME}_TEST_SOURCES})
    set_target_properties(${PROJECT_NAME}Test PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED YES)
    target_link_libraries(${PROJECT_NAME}Test ${PROJECT_NAME} gtest gtest_main)
    target_compile_options(${PROJECT_NAME}Test PRIVATE -g -O0 --coverage)
    set_target_properties(${PROJECT_NAME}Test PROPERTIES LINK_FLAGS "--coverage")

    cmake_policy(SET CMP0057 NEW)
    include(GoogleTest)

    enable_testing()
    gtest_add_tests(${PROJECT_NAME}Test "" ${${PROJECT_NAME}_TEST_SOURCES})
endif()