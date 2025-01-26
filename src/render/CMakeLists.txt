include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)

set(
	render_header_files
    "public/render/async_device.hpp"
    "public/render/gpu_ptr.hpp"
    "public/render/command_queue.hpp"
    "public/render/material.hpp"
    "public/render/mesh.hpp"
    "public/render/render.hpp"
    "public/render/upload_heap.hpp"
    "public/render/work_graph.hpp"
)

set(
	render_source_files
	"private/src/async_device.cpp"
	"private/src/command_queue.cpp"
	"private/src/material.cpp"
	"private/src/mesh.cpp"
	"private/src/upload_heap.cpp"
	"private/src/work_graph.cpp"
)

add_library(render STATIC ${render_header_files} ${render_source_files})

warnings_as_errors(render)

target_compile_features(render PUBLIC cxx_std_23)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(render PRIVATE /permissive-)
else()
	target_compile_options(render PRIVATE -fno-rtti -fno-exceptions -Wno-missing-designated-field-initializers)
endif()

target_include_directories(
	render
	PUBLIC "public"
	PRIVATE "public/render"
	PRIVATE "private/include"
)

target_link_libraries(render PUBLIC core.platform gpu core.serialization)

assign_source_group(${render_header_files} ${render_source_files})