cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(ecs LANGUAGES CXX)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Sanitizers.cmake)


add_executable(test app/test.cpp)
target_include_directories(test PUBLIC include ${Boost_INCLUDE_DIRS})

