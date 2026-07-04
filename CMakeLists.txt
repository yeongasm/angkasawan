# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")

enable_testing()

file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_SOURCE_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)

project(AngkasaOne)
include(CheckIPOSupported)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
    DEP_PATH
    "../deps"
    REALPATH 
    BASE_DIR "${CMAKE_BINARY_DIR}"
)
set(CPM_SOURCE_CACHE "${DEP_PATH}")

set(CMAKE_VERBOSE_MAKEFILE ON)

if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

message("CPM_SOURCE_CACHE: ${CPM_SOURCE_CACHE}")
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

check_ipo_supported(RESULT result OUTPUT output)

if (result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(WARNING "IPO is not supported: ${output}")
endif()

if (PROJECT_IS_TOP_LEVEL AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/build/compile_commands.json
    )
endif()

add_subdirectory(src)

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")