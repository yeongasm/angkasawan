include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME simdjson
	GIT_TAG v2.2.2
	GIT_REPOSITORY https://github.com/simdjson/simdjson.git
	GIT_SHALLOW TRUE
)
set(CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(simdjson PUBLIC cxx_std_20)
set_target_properties(simdjson PROPERTIES FOLDER thirdparty)

if (MSVC)
	target_compile_options(simdjson PUBLIC /wd4245 /wd4100 /wd4459 /wd4530 /wd4706)
else()
	target_compile_options(simdjson PUBLIC -Wno-everything)
endif()

set(
	header_list
	"public/inl/core/application.h"
	"public/inl/core/core_minimal.h"
	"public/inl/core/cursor.h"
	"public/inl/core/engine.h"
	"public/inl/core/engine_api.h"
	"public/inl/core/event_queue.h"
	"public/inl/core/file_watcher.h"
	"public/inl/core/io.h"
	"public/inl/core/io_enums.h"
	"public/inl/core/stat.h"
	"public/inl/core/windowing.h"
	"private/inl/platform_header.h"
)

set(
	source_list
	"private/src/engine.cpp"
	"private/src/io.cpp"
	"private/src/windowing.cpp"
)

set(
	win_source_list
	"private/src/win/cursor.cpp"
	"private/src/win/event_queue.cpp"
	"private/src/win/file_watcher.cpp"
	"private/src/win/stat.cpp"
	"private/src/win/windowing.cpp"
)

add_library(Core SHARED ${header_list} ${source_list} ${module_impl_units})
target_compile_features(Core PUBLIC cxx_std_20)
target_compile_definitions(Core PRIVATE "CORE_EXPORT")
target_compile_definitions(Core PRIVATE SIMDJSON_EXCEPTIONS=OFF $<$<CONFIG:Debug>:SIMDJSON_DEVELOPMENT_CHECKS=1>)
target_compile_definitions(Core PUBLIC $<$<PLATFORM_ID:Windows>:PLATFORM_OS_WINDOWS> $<$<PLATFORM_ID:Darwin>:PLATFORM_OS_APPLE> $<$<PLATFORM_ID:Linux>:PLATFORM_OS_LINUX>)
target_compile_definitions(Core PUBLIC "UNICODE" "_UNICODE")

target_sources(
	Core 
	PRIVATE
	$<$<PLATFORM_ID:Windows>:${win_source_list} ${win_module_impl_units}>
)

target_sources(
	Core
	PUBLIC
		FILE_SET CXX_MODULES FILES
		${module_interface_units}
)

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(Core PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	Core
	PUBLIC "public/inl"
	PRIVATE "public/inl/core"
	PRIVATE "private/inl"
)
target_link_libraries(Core PUBLIC Library simdjson)

assign_source_group(${header_list} ${source_list} ${module_interface_units} ${module_impl_units} $<$<PLATFORM_ID:Windows>:${win_source_list} ${win_module_impl_units}>)
warnings_as_errors(Core)