include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME fmt
	GIT_TAG 11.1.1
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)
set_target_properties(fmt PROPERTIES FOLDER thirdparty)
target_compile_features(fmt PUBLIC cxx_std_23)

CPMAddPackage(
	NAME unordered_dense
	GIT_TAG v4.5.0
	GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
)

set_target_properties(unordered_dense PROPERTIES FOLDER thirdparty)
target_compile_features(unordered_dense INTERFACE cxx_std_23)

CPMAddPackage(
	NAME plf_colony
	GIT_TAG c4c1a2e7f4346faa5d16e89ba1b4fa8c3d976bd2
	GIT_REPOSITORY https://github.com/mattreecebentley/plf_colony.git
	DOWNLOAD_ONLY True
)

if (plf_colony_ADDED)
	add_library(plf_colony INTERFACE ${plf_colony_SOURCE_DIR}/plf_colony.h)
	target_include_directories(plf_colony INTERFACE ${plf_colony_SOURCE_DIR})
	target_compile_features(plf_colony INTERFACE cxx_std_23)
    set_target_properties(plf_colony PROPERTIES FOLDER thirdparty)

	if (NOT MSVC)
		target_compile_options(plf_colony INTERFACE -Wno-unused-parameter)
	endif()
endif()

if (MSVC)
	target_compile_options(fmt PUBLIC /wd4541 /wd4702)
else()
	target_compile_options(fmt PUBLIC -Wno-rtti -fno-exceptions)
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
	#"public/lib/paged_array.hpp"
	"public/lib/resource.hpp"
	"public/lib/set_once.hpp"
	"public/lib/set.hpp"
	"public/lib/string.hpp"
	"public/lib/type.hpp"
	"public/lib/utility.hpp"
	"private/memory.cpp"
)

add_library(lib STATIC ${source_list})

warnings_as_errors(lib)

get_target_property(LIB_TARGET_TYPE lib TYPE)

target_compile_features(lib PUBLIC cxx_std_23)
target_compile_definitions(lib PUBLIC $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:RELEASE> $<$<CONFIG:RelWithDebInfo>:RELEASE_WDEBUG> PRIVATE $<$<STREQUAL:${LIB_TARGET_TYPE},"SHARED_LIBRARY">:"LIBRARY_SHARED_LIB;LIBRARY_EXPORT">)

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
	target_compile_definitions(lib PUBLIC "MSVC")
	target_compile_options(lib PRIVATE /permissive-)
else()
	target_compile_options(lib PRIVATE -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	lib
	PUBLIC "public"
)

target_link_libraries(lib PUBLIC fmt plf_colony unordered_dense)
assign_source_group(${source_list} ${win_source_list})