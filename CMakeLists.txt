project(LogoDetectionPoC)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

find_package(OpenCV REQUIRED)

# Project Executable
add_executable (logodetection_poc LogoDetectionPoC.cpp)
target_link_libraries(logodetection_poc ${OpenCV_LIBS})
