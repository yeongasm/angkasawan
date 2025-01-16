include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

set(
	gpu_rhi_header_files
	"public/gpu/common.hpp"
	"public/gpu/constants.hpp"
	"public/gpu/gpu.hpp"
	"public/gpu/gpu_api.hpp"
	"public/gpu/util/shader_compiler.hpp"
)

set(
	gpu_rhi_source_files
	"private/src/common.cpp"
)

add_library(gpu STATIC ${gpu_rhi_header_files} ${gpu_rhi_source_files})	

target_compile_features(gpu PUBLIC cxx_std_23)

warnings_as_errors(gpu)

target_compile_definitions(gpu PRIVATE "GPU_EXPORT")
target_compile_definitions(gpu PRIVATE $<$<PLATFORM_ID:Windows>:PLATFORM_OS_WINDOWS> $<$<PLATFORM_ID:Darwin>:PLATFORM_OS_APPLE> $<$<PLATFORM_ID:Linux>:PLATFORM_OS_LINUX>)

target_link_libraries(gpu PUBLIC lib)
assign_source_group(${gpu_rhi_header_files} ${gpu_rhi_source_files})

target_include_directories(
	gpu 
	PUBLIC "public"
	PRIVATE "public/gpu"
	PRIVATE "private/include"
)

if (USE_VULKAN)
	include(FindVulkan)

	set(
		vulkan_header_files
		"private/include/vulkan/vk.h"
		"private/include/vulkan/vkgpu.hpp"
	)

	set(
		vulkan_source_files
		"private/src/vulkan/buffer.cpp"
		"private/src/vulkan/command_buffer.cpp"
		"private/src/vulkan/command_pool.cpp"
		"private/src/vulkan/device.cpp"
		"private/src/vulkan/image.cpp"
		"private/src/vulkan/memory.cpp"
		"private/src/vulkan/pipeline.cpp"
		"private/src/vulkan/shader.cpp"
		"private/src/vulkan/sampler.cpp"
		"private/src/vulkan/swapchain.cpp"
		"private/src/vulkan/sync.cpp"
		"private/src/vulkan/vk.cpp"
		"private/src/util/spirv_shader_compiler.cpp"
	)

	target_sources(
		gpu
		PRIVATE
		${vulkan_header_files}
		${vulkan_source_files}
	)

	CPMAddPackage(
		NAME volk
		GIT_TAG sdk-1.3.224.1
		GIT_REPOSITORY https://github.com/zeux/volk.git
	)
	set_target_properties(volk PROPERTIES FOLDER thirdparty)

	set(VMA_STATIC_VULKAN_FUNCTIONS OFF)
	set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON)

	CPMAddPackage(
		NAME vma
		GIT_TAG v3.0.1
		GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
	)
	set_target_properties(VulkanMemoryAllocator PROPERTIES FOLDER thirdparty)

	if (NOT MSVC)
		target_compile_options(VulkanMemoryAllocator PRIVATE -Wno-unused-variable -Wno-missing-field-initializers)
	endif()

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

	CPMAddPackage(
		NAME spirv-reflect-static
		GIT_TAG sdk-1.3.224.1
		GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
	)
	set_target_properties(spirv-reflect-static PROPERTIES FOLDER thirdparty)
	
	if (MSVC)
		target_compile_options(VulkanMemoryAllocator PUBLIC /wd4701 /wd4703)
	endif()

	target_link_libraries(gpu PRIVATE volk_headers VulkanMemoryAllocator shaderc spirv-reflect-static)
	target_compile_definitions(gpu PRIVATE $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>)
	assign_source_group(${vulkan_header_files} ${vulkan_source_files})
endif()


if (MSVC)
	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	target_compile_options(gpu PRIVATE /permissive- /wd4530 /wd4530)
else()
	target_compile_options(
		gpu 
		PRIVATE 
		-fno-rtti 
		-fno-exceptions 
		-Wno-missing-field-initializers 
		-Wno-nullability-extension 
		-Wno-nullability-completeness 
		-Wno-language-extension-token
	)
endif()