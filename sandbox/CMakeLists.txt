include(${CMAKE_SOURCE_DIR}/Util.cmake)

set(
	sandbox_header_files
	"public/sandbox/sandbox.h"
	"public/sandbox/sandbox_api.h"
	"public/sandbox/application_window.h"
)

set(
	sandbox_source_files
	"module.cpp"
	"private/src/sandbox.cpp"
	"private/src/application_window.cpp"
)

add_library(Sandbox SHARED ${sandbox_header_files} ${sandbox_source_files})
target_compile_definitions(Sandbox PRIVATE "SANDBOX_EXPORT")

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

set_target_properties(
	Sandbox
	PROPERTIES 
		CXX_STANDARD 20 
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		LINKER_LANGUAGE CXX
)
target_include_directories(
	Sandbox 
	PUBLIC "public"
	PRIVATE "public/sandbox"
	PRIVATE "private/include"
)
target_link_libraries(Sandbox PRIVATE Core Renderer)

add_custom_command(
    TARGET 
		Sandbox POST_BUILD
    COMMAND 
		${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Sandbox>
        ${EXECUTABLE_DIR}
	COMMAND_EXPAND_LISTS
)

add_custom_target(
	Data
	${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_DIR}data
)

#target_compile_definitions(Sandbox PRIVATE $<$<CONFIG:Debug>:"DATA_DIR=${CMAKE_SOURCE_DIR}/data">)

#message(STATUS "CMAKE_ROOT=" ${CMAKE_ROOT})

#add_custom_command(
#	TARGET
#		Sandbox POST_BUILD
#	COMMAND
#		${CMAKE_COMMAND} -E create_symlink
#		${EXECUTABLE_DIR}/data
#		${CMAKE_SOURCE_DIR}/data
#)

assign_source_group(${source_list})
warnings_as_errors(Sandbox)