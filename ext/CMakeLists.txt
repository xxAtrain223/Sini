cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(ext)

if (NOT TARGET gtest AND EmbGen_ENABLE_TESTING)
    set(gtest_force_shared_crt TRUE CACHE BOOL "")
    add_subdirectory(googletest)
endif()