#####################################
cmake_minimum_required (VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/local/lib/cmake/)
project (training)
add_definitions(-std=c++11)
find_package(OpenCV REQUIRED)
IF  ( OpenCV_FOUND)
    MESSAGE(STATUS "COMPILING")
    add_executable (training src/training.cpp)
    target_link_libraries (training ${OpenCV_LIBS})
ELSE()
        MESSAGE(FATAL_ERROR "OPENCV NOT FOUND")
ENDIF()
#####################################
