include(${CMAKE_SOURCE_DIR}/Util.cmake)

set(
	source_list
	"os_platform.h"
	"os_shims.h"
)

add_library(OSHeader INTERFACE ${source_list})

#set_property(TARGET OSHeader PROPERTY FOLDER library)
target_compile_features(OSHeader INTERFACE cxx_std_20)

target_include_directories(OSHeader INTERFACE .)
assign_source_group(${source_list})