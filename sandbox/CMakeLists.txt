include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	sandbox_header_files
	"public/sandbox/sandbox.h"
	"public/sandbox/sandbox_api.h"
	"public/sandbox/graphics_processing_unit.h"
	"public/sandbox/command_queue.h"
	"public/sandbox/upload_heap.h"
	"private/include/camera.h"
	"private/include/geometry_cache.h"
	"private/include/image_importer.h"
	"private/include/material_cache.h"
	"private/include/model_importer.h"
	#"private/include/resource_cache.h"
	"private/include/sandbox_demo_application.h"
	"private/include/demo/model_demo_app.h"
	#"private/include/demo/triangle_demo_app.h"
)

set(
	sandbox_source_files
	"private/src/camera.cpp"
	"private/src/geometry_cache.cpp"
	"private/src/graphics_processing_unit.cpp"
	"private/src/image_importer.cpp"
	"private/src/model_importer.cpp"
	#"private/src/resource_cache.cpp"
	"private/src/sandbox.cpp"
	"private/src/sandbox.cpp"
	"private/src/command_queue.cpp"
	"private/src/upload_heap.cpp"
	"private/src/demo/model_demo_app.cpp"
	#"private/src/demo/triangle_demo_app.cpp"
)

CPMAddPackage(
	NAME glm
	GIT_TAG 0.9.9.8
	GIT_REPOSITORY https://github.com/g-truc/glm
)

add_library(Sandbox SHARED ${sandbox_header_files} ${sandbox_source_files} ${module_impl_units})
target_compile_features(Sandbox PUBLIC cxx_std_20)
target_compile_definitions(Sandbox PRIVATE "SANDBOX_EXPORT")

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(Sandbox PRIVATE /permissive-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	Sandbox 
	PUBLIC "public"
	PRIVATE "public/sandbox"
	PRIVATE "private/include"
)
target_link_libraries(Sandbox PRIVATE Core cgltf glm ktx imgui)
target_link_libraries(Sandbox PUBLIC gpu)
target_compile_definitions(Sandbox PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

add_custom_target(
	Data
	${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/data
)

assign_source_group(${sandbox_header_files} ${sandbox_source_files} ${module_interface_units} ${module_impl_units})
warnings_as_errors(Sandbox)