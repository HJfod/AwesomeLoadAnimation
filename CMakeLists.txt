cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(AwesomeLoadAnim VERSION 0.1.0 LANGUAGES C CXX)

set(SOURCE_FILES src/LoadingLayer.cpp sdk/include/implicitEntry.cpp)
set(BINARY_NAME AwesomeLoadAnim)
include(sdk/Geode.cmake)

target_link_libraries(${PROJECT_NAME} gdiplus uxtheme)
