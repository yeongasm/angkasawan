include(${CMAKE_SOURCE_DIR}/CPM.cmake)

CPMAddPackage(
	NAME googletest
	GIT_TAG release-1.12.1
	GIT_REPOSITORY https://github.com/google/googletest.git
)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQURIED ON)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

set_target_properties(gtest PROPERTIES FOLDER thirdparty)
set_target_properties(gtest_main PROPERTIES FOLDER thirdparty)
set_target_properties(gmock PROPERTIES FOLDER thirdparty)
set_target_properties(gmock_main PROPERTIES FOLDER thirdparty)

add_subdirectory(library)