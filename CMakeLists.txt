cmake_minimum_required(VERSION 3.0.0)
  
project(ffmpeg-cpp CXX)
  
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
  
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE OFF)


if (CMAKE_SYSTEM MATCHES Linux)
#testing some warning flags. Be carefull: some can be DANGEROUS when badly used
   set(CMAKE_CXX_FLAGS "-g -DDEBUG -DLinux -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wsign-compare -fms-extensions -Wfloat-equal")
##    set(CMAKE_CXX_FLAGS "-Wall -Wextra -g -DDEBUG -DLinux")
endif()

file(GLOB_RECURSE INC sources/*.h)
file(GLOB_RECURSE SRC sources/*.cpp)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

add_library(${PROJECT_NAME} ${SRC}) 


target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/sources )


target_link_libraries(${PROJECT_NAME} PkgConfig::LIBAV )

