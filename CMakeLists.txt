PROJECT(cars)
cmake_minimum_required(VERSION 2.6)
# Debug, Release, RelWithDebInfo and MinSizeRe
set(CMAKE_BUILD_TYPE RelWithDebInfo)
SET(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra") # add extra warnings

# includes cmake/FindSDL2.cmake
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# sudo apt-get install libsdl2-gfx-dev libsdl2-image-dev  libsdl2-mixer-dev  libsdl2-ttf-dev
ADD_EXECUTABLE(cars cars.cpp timer.h sdl_utils.h)
TARGET_LINK_LIBRARIES(cars ${SDL2_LIBRARY}
                            SDL2_gfx SDL2_image SDL2_mixer SDL2_ttf)

