include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME fmt
	GIT_TAG 10.1.1
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)
set_target_properties(fmt PROPERTIES FOLDER thirdparty)
target_compile_features(fmt PUBLIC cxx_std_20)

if (MSVC)
	target_compile_options(fmt PUBLIC /wd4541)
else()
	target_compile_options(fmt PUBLIC -Wno-rtti)
endif()


set(
	source_list
	"public/lib/.natvis"
	"public/lib/api.h"
	"public/lib/array.h"
	"public/lib/bitset.h"
	"public/lib/bit_mask.h"
	"public/lib/common.h"
	"public/lib/concepts.h"
	"public/lib/dag.h"
	"public/lib/function.h"
	"public/lib/handle.h"
	"public/lib/hash.h"
	"public/lib/linear_allocator.h"
	"public/lib/map.h"
	"public/lib/memory.h"
	"public/lib/named_type.h"
	"public/lib/paged_array.h"
	"public/lib/set.h"
	"public/lib/string.h"
	"public/lib/type.h"
	"public/lib/utility.h"
	"private/memory.cpp"
)

add_library(Library SHARED ${source_list})
target_compile_features(Library PUBLIC cxx_std_20)
target_compile_definitions(Library PRIVATE "LIBRARY_EXPORT")

target_sources(
	Library
	PRIVATE
	$<$<PLATFORM_ID:Windows>:${win_source_list}>
)

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_definitions(Library PUBLIC "MSVC_COMPILER")
	target_compile_options(Library PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	Library 
	PUBLIC "public"
)

target_link_libraries(Library PUBLIC fmt)

# add_custom_command(
#     TARGET 
# 		Library POST_BUILD
#     COMMAND 
# 		${CMAKE_COMMAND} -E copy
#         $<TARGET_FILE_NAME:Library>
#         ${EXECUTABLE_DIR}
# )
assign_source_group(${source_list} ${win_source_list})
warnings_as_errors(Library)