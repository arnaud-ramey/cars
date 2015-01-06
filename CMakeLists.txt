PROJECT(cars)
cmake_minimum_required(VERSION 2.6)
#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_VERBOSE_MAKEFILE OFF)
FIND_PACKAGE(OpenCV REQUIRED )

ADD_EXECUTABLE(cars cars.cpp
                    image_contour.h voronoi.h timer.h debug.h)
TARGET_LINK_LIBRARIES(cars ${OpenCV_LIBS})

