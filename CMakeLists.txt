PROJECT(cars)
cmake_minimum_required(VERSION 2.6)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE OFF)

# includes cmake/FindSDL2.cmake
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
FIND_PACKAGE(OpenCV REQUIRED )

ADD_EXECUTABLE(cars cars.cpp
                    image_contour.h voronoi.h timer.h debug.h)
TARGET_LINK_LIBRARIES(cars ${OpenCV_LIBS} ${SDL2_LIBRARY} SDL2_image)

