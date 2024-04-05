include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)

# Add source to this project's executable.
set(
	source_list
	"main.cpp"
	"angkasawan.h"
	"angkasawan.cpp"
)

add_executable(Executable ${source_list})
target_compile_features(Executable PUBLIC cxx_std_20)

target_link_libraries(Executable PRIVATE Core Sandbox)
set_target_properties(
	Executable
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
)
# Disable RTTI and exceptions.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	set(SUBSYSTEM_LINKER_OPTIONS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	target_compile_options(Executable PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

# add_custom_command(
# 	TARGET 
# 		Executable POST_BUILD
# 	COMMAND 
# 		${CMAKE_COMMAND} -E copy 
# 		$<TARGET_RUNTIME_DLLS:Executable> 
# 		$<TARGET_FILE_DIR:Executable>
# 	COMMAND_EXPAND_LISTS
# )

#add_custom_command(
#	TARGET 
#		Executable POST_BUILD
#	COMMAND 
#		${CMAKE_COMMAND} -E copy 
#		${PROJECT_SOURCE_DIR}"/.bootstrap"
#		$<TARGET_FILE_DIR:Executable>
#	COMMAND_EXPAND_LISTS
#)

assign_source_group(${source_list})
warnings_as_errors(Executable)