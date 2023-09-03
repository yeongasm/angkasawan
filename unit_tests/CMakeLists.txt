include(FetchContent)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.12.1
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQURIED ON)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_target_properties(gtest PROPERTIES FOLDER thirdparty)
set_target_properties(gtest_main PROPERTIES FOLDER thirdparty)
set_target_properties(gmock PROPERTIES FOLDER thirdparty)
set_target_properties(gmock_main PROPERTIES FOLDER thirdparty)

add_subdirectory(library)