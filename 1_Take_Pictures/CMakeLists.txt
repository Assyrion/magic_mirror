#####################################
cmake_minimum_required (VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/local/lib/cmake/)
project (take_pic)
add_definitions(-std=c++14)
find_package(raspicam REQUIRED)
find_package(OpenCV)
IF  ( OpenCV_FOUND AND raspicam_CV_FOUND)
    MESSAGE(STATUS "COMPILING")
    add_executable (take_pic src/take_pic.cpp)
    target_link_libraries (take_pic ${raspicam_CV_LIBS} ${raspicam_LIBS} ${OpenCV_LIBS})
ELSE()
        MESSAGE(FATAL_ERROR "OPENCV NOT FOUND")
ENDIF()
#####################################
