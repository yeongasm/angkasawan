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
	"engine.h"
	"engine_api.h"
	"engine.cpp"
	"core_minimal.h"
	"windowing/windowing.h"
	"windowing/windowing.cpp"
	"utility/bootstrapper.h"
	"utility/bootstrapper.cpp"
	"time/system_clock.h"
	"time/win/win_clock.h"
	"time/win/win_clock.cpp"
	"shared/engine_common.h"
	"platform_interface/platform_common.h"
	"platform_interface/platform_interface.h"
	"platform_interface/win/win_interface.h"
	"platform_interface/win/win_interface.cpp"
	"module/module_manager.h"
	"module/module_manager.cpp"
	"input/io.h"
	"input/io.cpp"
)

add_library(Core SHARED ${source_list})
target_compile_features(Core PUBLIC cxx_std_20)
target_compile_definitions(Core PRIVATE "CORE_EXPORT")
target_compile_definitions(Core PRIVATE SIMDJSON_EXCEPTIONS=OFF $<$<CONFIG:Debug>:SIMDJSON_DEVELOPMENT_CHECKS=1>)
target_compile_definitions(Core PUBLIC "UNICODE" "_UNICODE")

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(Core PUBLIC ".")
target_link_libraries(Core PUBLIC Library Math OSHeader simdjson)

# no longer needed as Executable copies all runtime dll dependencies to it's directory.
#add_custom_command(
#    TARGET 
#		Core POST_BUILD
#    COMMAND 
#		${CMAKE_COMMAND} -E copy
#        "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:Core>"
#        "${EXECUTABLE_DIR}"
#) 

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