	# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

enable_testing()

project(AngkasaOne)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXECUTABLE_DIR "${PROJECT_BINARY_DIR}/exe/")

# Include sub-projects.
add_subdirectory(exe)
add_subdirectory(foundation)
add_subdirectory(core)
add_subdirectory(renderer)
add_subdirectory(unit_tests)
add_subdirectory(os_header)
add_subdirectory(sandbox)