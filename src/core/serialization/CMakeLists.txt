include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	serialization_header_files
    "public/core.serialization/buffer.hpp"
    "public/core.serialization/file.hpp"
    "public/core.serialization/sbf_header.hpp"
    "public/core.serialization/read_stream.hpp"
    "public/core.serialization/write_stream.hpp"
)

set(
	serialization_source_files
    "private/src/buffer.cpp"
	"private/src/read_stream.cpp"
)
	
set(
	windows_serialization_source_files
	"private/src/windows/file.cpp"
	"private/src/windows/write_stream.cpp"
)

add_library(core.serialization STATIC ${serialization_header_files} ${serialization_source_files} ${windows_serialization_source_files})

warnings_as_errors(core.serialization)

target_compile_features(core.serialization PUBLIC cxx_std_23)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(core.serialization PRIVATE /permissive-)
else()
	target_compile_options(core.serialization PRIVATE -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	core.serialization
	PUBLIC "public"
	PRIVATE "public/core.serialization"
	PRIVATE "private/include"
)

target_link_libraries(core.serialization PUBLIC lib core.platform)
set_target_properties(core.serialization PROPERTIES FOLDER core)

assign_source_group(${serialization_header_files} ${serialization_source_files} ${windows_serialization_source_files})