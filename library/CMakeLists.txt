include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME fmt
	GIT_TAG 11.1.1
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)
set_target_properties(fmt PROPERTIES FOLDER thirdparty)
target_compile_features(fmt PUBLIC cxx_std_23)

if (MSVC)
	target_compile_options(fmt PUBLIC /wd4541 /wd4702)
else()
	target_compile_options(fmt PUBLIC -Wno-rtti)
endif()


set(
	source_list
	"public/lib/.natvis"
	"public/lib/api.h"
	"public/lib/array.hpp"
	"public/lib/bitset.hpp"
	"public/lib/bit_mask.hpp"
	"public/lib/common.hpp"
	"public/lib/concepts.hpp"
	"public/lib/dag.hpp"
	"public/lib/function.hpp"
	"public/lib/handle.hpp"
	"public/lib/hash.hpp"
	#"public/lib/linear_allocator.hpp"
	"public/lib/map.hpp"
	"public/lib/memory.hpp"
	"public/lib/named_type.hpp"
	"public/lib/paged_array.hpp"
	"public/lib/resource.hpp"
	"public/lib/set_once.hpp"
	"public/lib/set.hpp"
	"public/lib/string.hpp"
	"public/lib/type.hpp"
	"public/lib/utility.hpp"
	"private/memory.cpp"
)

add_library(lib SHARED ${source_list})
target_compile_features(lib PUBLIC cxx_std_23)
target_compile_definitions(lib PRIVATE "LIBRARY_SHARED_LIB" "LIBRARY_EXPORT")

target_sources(
	lib
	PRIVATE
	$<$<PLATFORM_ID:Windows>:${win_source_list}>
)

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_definitions(lib PUBLIC "MSVC_COMPILER")
	target_compile_options(lib PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	lib
	PUBLIC "public"
)

target_link_libraries(lib PUBLIC fmt)

# add_custom_command(
#     TARGET 
# 		Library POST_BUILD
#     COMMAND 
# 		${CMAKE_COMMAND} -E copy
#         $<TARGET_FILE_NAME:Library>
#         ${EXECUTABLE_DIR}
# )
assign_source_group(${source_list} ${win_source_list})
warnings_as_errors(lib)