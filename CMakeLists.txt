cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(ecs LANGUAGES CXX)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS -pthread)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Sanitizers.cmake)

add_executable(test app/test.cpp)
target_include_directories(test PUBLIC include ${Boost_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(pong app/pong.cpp)

target_include_directories(pong PUBLIC include ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
target_link_libraries(pong ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})
