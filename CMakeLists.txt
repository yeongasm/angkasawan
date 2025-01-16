# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

enable_testing()

project(AngkasaOne)
include(CheckIPOSupported)
include(cmake/CPM.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
    DEP_PATH
    "../deps"
    REALPATH 
    BASE_DIR "${CMAKE_BINARY_DIR}"
)
set(CPM_SOURCE_CACHE "${DEP_PATH}")

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

#if (WIN32 AND ENABLE_ASAN_AND_UBSAN)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
#endif()

message("CPM_SOURCE_CACHE: ${CPM_SOURCE_CACHE}")

check_ipo_supported(RESULT result OUTPUT output)

if (result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "IPO is not supported: ${output}")
endif()

add_subdirectory(src)