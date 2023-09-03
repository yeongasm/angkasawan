include(${CMAKE_SOURCE_DIR}/Util.cmake)

set(
	source_list
	"math.h"
	"vector.h"
	"matrix.h"
	"quaternion.h"
)

add_library(Math INTERFACE ${source_list})
target_compile_features(Math INTERFACE cxx_std_20)

target_include_directories(Math INTERFACE .)