include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	makesbf_header_files
	"public/makesbf/image_importer.hpp"
	"public/makesbf/gltf_importer.hpp"
	"public/makesbf/makesbf.hpp"
	"public/makesbf/meshify_job.hpp"
	"public/makesbf/imagify_job.hpp"
)

set(
	makesbf_source_files
	"private/src/image_importer.cpp"
	"private/src/gltf_importer.cpp"
	"private/src/meshify_job.cpp"
	"private/src/imagify_job.cpp"
	"private/src/makesbf.cpp"
	"main.cpp"
)

add_executable(makesbf ${makesbf_header_files} ${makesbf_source_files})
target_compile_features(makesbf PUBLIC cxx_std_23)
target_compile_definitions(makesbf PRIVATE $<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Release>:RELEASE> $<$<CONFIG:RelWithDebInfo>:RELEASE_WDEBUG>)

set_target_properties(
	makesbf
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}
)

if (MSVC)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(makesbf PRIVATE /permissive- /GR-)
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	makesbf
	PUBLIC "public"
	PRIVATE "public/makesbf"
	PRIVATE "private/include"
)

target_link_libraries(makesbf PRIVATE ktx cgltf render glaze::glaze core.cmdline)
set_target_properties(makesbf PROPERTIES FOLDER tools)

assign_source_group(${makesbf_header_files} ${makesbf_source_files})
warnings_as_errors(makesbf)