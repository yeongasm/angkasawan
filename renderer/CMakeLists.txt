include(${CMAKE_SOURCE_DIR}/Util.cmake)
include(FetchContent)
include(FindVulkan) 

option(RENDERER_USE_VULKAN "Uses Vulkan as default API" ON)

set(
	renderer_header_files
	"public/renderer/render_graph.h"
	"public/renderer/renderer.h"
	"public/renderer/renderer_api.h"
	"public/renderer/rhi.h"
	"public/renderer/command_buffer.h"
	"public/renderer/task_queue.h"
	"public/renderer/task_controller.h"
	"public/renderer/device.h"
	"public/renderer/resource_store.h"
	"private/include/rhi/vulkan/vk.h"
	"private/include/rhi/vulkan/vk_device.h"
	"private/include/rhi/shader_compiler.h"
)

set(
	renderer_source_files
	"module.cpp"
	"private/src/rhi.cpp"
	"private/src/render_graph.cpp"
	"private/src/renderer.cpp"
	"private/src/resource_store.cpp"
	"private/src/task_queue.cpp"
	"private/src/task_controller.cpp"
	"private/src/rhi/vk.cpp"
	"private/src/rhi/vk_device.cpp"
	"private/src/rhi/vk_command_buffer.cpp"
	"private/src/rhi/shader_compiler.cpp"
)

add_library(Renderer SHARED ${renderer_header_files} ${renderer_source_files})
target_compile_features(Renderer PUBLIC cxx_std_20)
target_compile_definitions(Renderer PRIVATE "RENDERER_EXPORT" $<$<CONFIG:Debug>:"RENDERER_ENABLE_LOGGING">)

if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions)
endif()

#set_target_properties(
#	Renderer
#	PROPERTIES 
#		CXX_STANDARD 20 
#		CXX_STANDARD_REQUIRED ON
#		CXX_EXTENSIONS OFF
#		LINKER_LANGUAGE CXX
#)
target_include_directories(
	Renderer 
	PUBLIC "public"
	PRIVATE "public/renderer"
	PRIVATE "private/include"
)
target_link_libraries(Renderer PRIVATE Core)

if (RENDERER_USE_VULKAN)
	FetchContent_Declare(
		volk
		GIT_REPOSITORY https://github.com/zeux/volk.git
		GIT_TAG sdk-1.3.224.1
	)
	FetchContent_MakeAvailable(volk)
	set_target_properties(volk PROPERTIES FOLDER thirdparty)

	if (MSVC)
		target_compile_definitions(Renderer PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	endif()

	FetchContent_Declare(
		vma
		GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
		GIT_TAG v3.0.1
	)
	FetchContent_MakeAvailable(vma)
	set_target_properties(VulkanMemoryAllocator PROPERTIES FOLDER thirdparty)

	# import shaderc from the VulkanSDK.
	add_library(shaderc UNKNOWN IMPORTED)
	set_target_properties(
		shaderc 
		PROPERTIES
			IMPORTED_LOCATION "$ENV{VULKAN_SDK}/Lib/shaderc_shared.lib"
			INTERFACE_INCLUDE_DIRECTORIES "$ENV{VULKAN_SDK}/Include"
	)

	set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
	set(SPIRV_REFLECT_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)

	FetchContent_Declare(
		spirv-reflect-static
		GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
		GIT_TAG sdk-1.3.224.1
	)
	FetchContent_MakeAvailable(spirv-reflect-static)
	set_target_properties(spirv-reflect-static PROPERTIES FOLDER thirdparty)

	target_link_libraries(Renderer PRIVATE volk_headers VulkanMemoryAllocator shaderc spirv-reflect-static)
endif()

# since the renderer is not part of Executable's runtime dependency, we do a manual copy.
# optionally, we can make the renderer a dependency of Executable.
#add_custom_command(
#    TARGET 
#		Renderer POST_BUILD
#    COMMAND 
#		${CMAKE_COMMAND} -E copy
#        $<TARGET_FILE:Renderer>
#        $<TARGET_FILE_DIR:Executable>
#	COMMAND_EXPAND_LISTS
#)

add_custom_command(
    TARGET 
		Renderer POST_BUILD
    COMMAND 
		${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:Renderer>
        ${EXECUTABLE_DIR}
	COMMAND_EXPAND_LISTS
)

assign_source_group(${source_list})
warnings_as_errors(Renderer)