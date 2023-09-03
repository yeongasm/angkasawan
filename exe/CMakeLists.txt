include(${CMAKE_SOURCE_DIR}/Util.cmake)

# Add source to this project's executable.
set(
	source_list
	"main.cpp"
	"angkasawan.h"
	"angkasawan.cpp"
)

add_executable(Executable ${source_list})
target_compile_features(Executable PUBLIC cxx_std_20)

#set_target_properties(
#	Executable 
#	PROPERTIES 
#		CXX_STANDARD 20 
#		CXX_STANDARD_REQUIRED ON
#		CXX_EXTENSIONS OFF
#		LINKER_LANGUAGE CXX
#		UNITY_BUILD ON
#)

#for testing ...
target_link_libraries(Executable PRIVATE Core)
target_link_libraries(Executable PRIVATE OSHeader)

# Disable RTTI and exceptions.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

	#temporary solution until we have a virtual filesystem.
	#set_target_properties(
	#	Executable 
	#	PROPERTIES 
	#		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	#)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

add_custom_command(
	TARGET 
		Executable POST_BUILD
	COMMAND 
		${CMAKE_COMMAND} -E copy 
		$<TARGET_RUNTIME_DLLS:Executable> 
		$<TARGET_FILE_DIR:Executable>
	COMMAND_EXPAND_LISTS
)

add_custom_command(
	TARGET 
		Executable POST_BUILD
	COMMAND 
		${CMAKE_COMMAND} -E copy 
		${PROJECT_SOURCE_DIR}"/.bootstrap"
		$<TARGET_FILE_DIR:Executable>
	COMMAND_EXPAND_LISTS
)

#message(STATUS "cmake_working_dir=" ${CMAKE_SOURCE_DIR})

#assign_source_group(${source_list})
warnings_as_errors(Executable)