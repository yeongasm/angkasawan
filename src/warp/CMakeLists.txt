include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	warp_header_files
    "public/warp/warp.hpp"
    "public/warp/detail/allocator_support.hpp"
    "public/warp/detail/awaiter.hpp"
    "public/warp/detail/awaiter_traits.hpp"
    "public/warp/detail/common.hpp"
    "public/warp/detail/handle.hpp"
    "public/warp/detail/promise_base.hpp"
    "public/warp/detail/promise_type.hpp"
    "public/warp/detail/queue.hpp"
    "public/warp/detail/result_container.hpp"
    "public/warp/detail/scheduler.hpp"
    "public/warp/detail/static_thread_pool.hpp"
    "public/warp/detail/sync_wait.hpp"
    "public/warp/detail/task.hpp"
    "public/warp/detail/thread.hpp"
    "public/warp/detail/when_all.hpp"
    "public/warp/detail/when_any.hpp"
    "public/warp/detail/work_stealing_deque.hpp"
    "public/warp/detail/work_stealing_scheduler.hpp"
)

set(
	warp_source_files
    "private/src/windows/thread.cpp"
    "private/src/static_thread_pool.cpp"
)

set(
	warp_test_files
	"private/src/test/main.cpp"
)

add_library(warp STATIC ${warp_header_files} ${warp_source_files})

warnings_as_errors(warp)

target_compile_features(warp PUBLIC cxx_std_23)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(warp PRIVATE /permissive-)
else()
	target_compile_options(warp PRIVATE -fno-rtti -fno-exceptions)
endif()

target_include_directories(
	warp
	PUBLIC "public"
	PRIVATE "public/warp/detail"
	PRIVATE "private/include"
)

target_link_libraries(warp PUBLIC lib core.platform)
set_target_properties(warp PROPERTIES FOLDER core)

assign_source_group(${warp_header_files} ${warp_source_files} ${warp_test_files})

add_executable(warp.test ${warp_test_files})
warnings_as_errors(warp.test)

target_compile_features(warp.test PUBLIC cxx_std_23)

if (MSVC)
	target_link_options(warp.test PRIVATE /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup)
	target_compile_options(warp.test PRIVATE /permissive- /wd4559 /wd4324 /wd4456)
else()
	target_compile_options(warp.test PRIVATE -fno-rtti -fno-exceptions -Wno-nested-anon-types -Wno-missing-designated-field-initializers -Wno-missing-field-initializers)
endif()

target_link_libraries(warp.test PRIVATE warp lib)