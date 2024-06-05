# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

enable_testing()

project(AngkasaOne)
include(cmake/CPM.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#set(EXECUTABLE_DIR "${PROJECT_BINARY_DIR}/exe/")

get_filename_component(
    DEP_PATH
    "../deps"
    REALPATH 
    BASE_DIR "${CMAKE_BINARY_DIR}"
)
set(CPM_SOURCE_CACHE "${DEP_PATH}")
message("CPM_SOURCE_CACHE: ${CPM_SOURCE_CACHE}")

# Include sub-projects.
add_subdirectory(exe)
add_subdirectory(library)
#add_subdirectory(math)
add_subdirectory(core)
add_subdirectory(gpu)
#add_subdirectory(renderer)
add_subdirectory(sandbox)
add_subdirectory(thirdparty)
add_subdirectory(unit_tests)