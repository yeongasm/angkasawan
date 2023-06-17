include(${CMAKE_SOURCE_DIR}/Util.cmake)
include(FetchContent)

FetchContent_Declare(
	fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 9.0.0
)
FetchContent_MakeAvailable(fmt)
set_target_properties(fmt PROPERTIES FOLDER thirdparty)

set(
	source_list
	"foundation_api.h"
	"common.h"
	".natvis"
	"types/strong_type.h"
	"types/handle.h"
	"memory/memory.h"
	"memory/memory.cpp"
	"stream/fstream.h"
	"stream/flags/stream_flags.h"
	"stream/win/win_fstream.h"
	"stream/win/win_fstream.cpp"
	"math/math.h"
	"math/matrix.h"
	"math/quaternion.h"
	"math/vector.h"
	"graphs/dag.h"
	"containers/array.h"
	"containers/bitset.h"
	"containers/hash.h"
	"containers/map.h"
	"containers/set.h"
	"containers/string.h"
	"containers/utility.h"
	"allocator/linear_allocator.h"
	"allocator/stack_allocator.h"
)

add_library(Foundation SHARED ${source_list})
target_compile_definitions(Foundation PRIVATE "FOUNDATION_EXPORT")

# Disable RTTI and exceptions.
# It's 2022 and CMake doesn't have built in properties for this, pathetic.
if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

set_target_properties(
	Foundation 
	PROPERTIES 
		CXX_STANDARD 20 
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		LINKER_LANGUAGE CXX
)

target_include_directories(Foundation PUBLIC ".")

target_link_libraries(Foundation PUBLIC fmt)
target_link_libraries(Foundation PRIVATE OSHeader)

#add_custom_command(
#    TARGET 
#		Foundation POST_BUILD
#    COMMAND 
#		${CMAKE_COMMAND} -E copy
#        "${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_NAME:Foundation>"
#        "${EXECUTABLE_DIR}"
#)

add_custom_command(
    TARGET 
		Foundation POST_BUILD
    COMMAND 
		${CMAKE_COMMAND} -E copy
        $<TARGET_FILE_NAME:Foundation>
        ${EXECUTABLE_DIR}
)

message(STATUS "EXECUTABLE_DIR=" ${EXECUTABLE_DIR})

#assign_source_group(${source_list})
warnings_as_errors(Foundation)