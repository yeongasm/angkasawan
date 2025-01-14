include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)

set(
	source_list
	"math/common.h"
	"math/vector.h"
	"math/matrix.h"
	"math/quaternion.h"
)

add_library(Math INTERFACE ${source_list})
target_compile_features(Math INTERFACE cxx_std_23)

target_include_directories(
	Math 
	INTERFACE .
	INTERFACE "math"
)