include(${CMAKE_SOURCE_DIR}/Util.cmake)
include(FetchContent)

FetchContent_Declare(
	simdjson
	GIT_REPOSITORY https://github.com/simdjson/simdjson.git
	GIT_TAG  v2.2.2
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(simdjson)
set_target_properties(
	simdjson 
	PROPERTIES 
		FOLDER thirdparty
		CXX_STANDARD 20
)

if (MSVC)
	target_compile_options(simdjson PUBLIC /wd4245 /wd4100 /wd4459 /wd4530 /wd4706)
else()
	target_compile_options(simdjson PUBLIC -Wno-everything)
endif()

set(
	source_list
	"public/core/application.h"
	"public/core/engine.h"
	"public/core/engine_common.h"
	"public/core/io.h"
	"public/core/io_enums.h"
	"public/core/stat.h"
	"public/core/windowing.h"
	"public/core/core_minimal.h"
	"public/core/engine_api.h"
	"public/core/event_queue.h"
	"private/include/platform_header.h"
	"private/src/engine.cpp"
	"private/src/io.cpp"
	"private/src/windowing.cpp"
)

set(
	win_source_list
	"private/src/win/win_engine.cpp"
	"private/src/win/win_event_queue.cpp"
	"private/src/win/win_stat.cpp"
	"private/src/win/win_windowing.cpp"
)

add_library(Core SHARED ${source_list})
target_compile_features(Core PUBLIC cxx_std_20)
target_compile_definitions(Core PRIVATE "CORE_EXPORT")
target_compile_definitions(Core PRIVATE SIMDJSON_EXCEPTIONS=OFF $<$<CONFIG:Debug>:SIMDJSON_DEVELOPMENT_CHECKS=1>)
target_compile_definitions(Core PUBLIC $<$<PLATFORM_ID:Windows>:PLATFORM_OS_WINDOWS> $<$<PLATFORM_ID:Darwin>:PLATFORM_OS_APPLE> $<$<PLATFORM_ID:Linux>:PLATFORM_OS_LINUX>)
target_compile_definitions(Core PUBLIC "UNICODE" "_UNICODE")

target_sources(
	Core 
	PRIVATE
	$<$<PLATFORM_ID:Windows>:${win_source_list}>
)

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	Core 
	PUBLIC "public"
	PRIVATE "public/core"
	PRIVATE "private/include"
)
#target_link_libraries(Core PUBLIC Library Math simdjson)
target_link_libraries(Core PUBLIC Library simdjson)

add_custom_command(
	TARGET 
		Core POST_BUILD
	COMMAND 
		${CMAKE_COMMAND} -E copy 
		$<TARGET_FILE_NAME:Core> 
		${EXECUTABLE_DIR}
	COMMAND_EXPAND_LISTS
)

#assign_source_group(${source_list})
warnings_as_errors(Core)