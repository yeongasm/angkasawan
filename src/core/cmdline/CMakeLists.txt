include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)

set(
	header_list
	"public/core.cmdline/cmdline.hpp"
)

set(
	source_list
	"private/src/cmdline.cpp"
)

add_library(core.cmdline STATIC ${header_list} ${source_list})

warnings_as_errors(core.cmdline)

if (MSVC)
	target_compile_options(core.cmdline PUBLIC /wd4530)
endif()

target_compile_features(core.cmdline PUBLIC cxx_std_23)

if (MSVC)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(core.cmdline PRIVATE /permissive- /GR-)
else()
	target_compile_options(core.cmdline PRIVATE -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	core.cmdline
	PUBLIC "public/"
	PRIVATE "public/core.cmdline"
	PRIVATE "private/include"
)
target_link_libraries(core.cmdline PRIVATE lib)
set_target_properties(core.cmdline PROPERTIES FOLDER core)
assign_source_group(${header_list} ${source_list})