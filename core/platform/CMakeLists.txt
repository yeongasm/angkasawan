include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)

set(
	header_list
	"public/inl/core.platform/application.hpp"
	"public/inl/core.platform/platform_api.h"
	"public/inl/core.platform/cursor.hpp"
	"public/inl/core.platform/event_queue.hpp"
	"public/inl/core.platform/file_watcher.hpp"
	"public/inl/core.platform/io_enums.hpp"
	"public/inl/core.platform/io.hpp"
	"public/inl/core.platform/platform_minimal.hpp"
	"public/inl/core.platform/performance_counter.hpp"
	"public/inl/core.platform/windowing.hpp"
	"private/inl/platform_header.h"
)

set(
	source_list
	"private/src/application.cpp"
	"private/src/io.cpp"
)

set(
	win_source_list
	"private/src/windows/cursor.cpp"
	"private/src/windows/event_queue.cpp"
	"private/src/windows/file_watcher.cpp"
	"private/src/windows/performance_counter.cpp"
	"private/src/windows/windowing.cpp"
)

add_library(core.platform STATIC ${header_list} ${source_list})
if (MSVC)
target_compile_options(core.platform PUBLIC /wd4530)
endif()
target_compile_features(core.platform PUBLIC cxx_std_20)
target_compile_definitions(core.platform PRIVATE "PLATFORM_EXPORT")
target_compile_definitions(core.platform PRIVATE SIMDJSON_EXCEPTIONS=OFF $<$<CONFIG:Debug>:SIMDJSON_DEVELOPMENT_CHECKS=1>)
target_compile_definitions(core.platform PUBLIC $<$<PLATFORM_ID:Windows>:PLATFORM_OS_WINDOWS> $<$<PLATFORM_ID:Darwin>:PLATFORM_OS_APPLE> $<$<PLATFORM_ID:Linux>:PLATFORM_OS_LINUX>)
target_compile_definitions(core.platform PUBLIC "UNICODE" "_UNICODE")

target_sources(
	core.platform 
	PRIVATE
	$<$<PLATFORM_ID:Windows>:${win_source_list}>
)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(core.platform PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	core.platform
	PUBLIC "public/inl"
	PRIVATE "public/inl/core.platform"
	PRIVATE "private/inl"
)
target_link_libraries(core.platform PUBLIC lib)
set_target_properties(core.platform PROPERTIES FOLDER core)
assign_source_group(${header_list} ${source_list} $<$<PLATFORM_ID:Windows>:${win_source_list}>)
warnings_as_errors(core.platform)