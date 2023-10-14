# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

enable_testing()

project(AngkasaOne)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXECUTABLE_DIR "${PROJECT_BINARY_DIR}/exe/")

# Include sub-projects.
add_subdirectory(exe)
add_subdirectory(library)
add_subdirectory(math)
add_subdirectory(core)
add_subdirectory(rhi)
#add_subdirectory(renderer)
add_subdirectory(sandbox)
add_subdirectory(unit_tests)