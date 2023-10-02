# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
enable_testing()
project ("Coroutine Tutorial")

# specify the C++ standard 
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(MSVC)
add_compile_options(/utf-8 /permissive-)
endif (MSVC)
add_subdirectory("hello_coroutine")
