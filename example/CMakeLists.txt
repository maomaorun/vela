cmake_minimum_required(VERSION 3.16)

project(vela-example VERSION 1.0.0)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -pipe -m64 -msse4")
set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_BUILD_TYPE DEBUG)

set(PROJECT_SRC ${PROJECT_SOURCE_DIR})
set(PROJECT_DEP ${PROJECT_SOURCE_DIR}/../dep)

include_directories(${PROJECT_SRC}/.. ${PROJECT_DEP})
link_directories(${PROJECT_DEP}/lib)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath=./lib")

add_executable(${PROJECT_NAME} ${PROJECT_SRC}/reactor-demo.cc)
target_link_libraries(${PROJECT_NAME} pthread)
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_DEP}/lib ${PROJECT_SOURCE_DIR}/lib)