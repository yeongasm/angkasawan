include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	demo_header_files
	"public/demo/sandbox.hpp"
	"private/include/camera.hpp"
	"private/include/applet.hpp"
	"private/include/model_demo_app.hpp"
)

set(
	demo_source_files
	"private/src/camera.cpp"
	"private/src/sandbox.cpp"
	"private/src/model_demo_app.cpp"
	"main.cpp"
)

CPMAddPackage(
	NAME glm
	GIT_TAG 0.9.9.8
	GIT_REPOSITORY https://github.com/g-truc/glm
)

if (NOT MSVC)
	target_compile_options(glm INTERFACE -Wno-gnu-anonymous-struct)
endif()

add_executable(sandbox.demo ${demo_header_files} ${demo_source_files})

warnings_as_errors(sandbox.demo)

if (ENABLE_ASAN_AND_UBSAN)
	enable_asan_and_ubsan(sandbox.demo)
endif()

target_compile_features(sandbox.demo PUBLIC cxx_std_23)
target_compile_definitions(sandbox.demo PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:RELEASE> $<$<CONFIG:RelWithDebInfo>:RELEASE_WDEBUG>)

set_target_properties(
	sandbox.demo
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_link_options(sandbox.demo PRIVATE /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup)
	target_compile_options(sandbox.demo PRIVATE /permissive-)
else()
	target_compile_options(sandbox.demo PRIVATE -fno-rtti -fno-exceptions -Wno-nested-anon-types -Wno-missing-designated-field-initializers -Wno-missing-field-initializers)
endif()

target_include_directories(
	sandbox.demo
	PUBLIC "public"
	PRIVATE "public/demo"
	PRIVATE "private/include"
)

target_link_libraries(sandbox.demo PRIVATE glm glaze::glaze)
target_link_libraries(sandbox.demo PUBLIC core.platform core.serialization core.cmdline render)
target_compile_definitions(sandbox.demo PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

set_target_properties(sandbox.demo PROPERTIES FOLDER sandbox)

add_custom_target(
	data
	${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data
)

assign_source_group(${demo_header_files} ${demo_source_files})