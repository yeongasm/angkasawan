#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto MemoryBlock::info() const -> MemoryBlockInfo const&
{
	return __self().info;
}

auto MemoryBlock::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto MemoryBlock::aliased() const -> bool
{
	return __self().aliased;
}

auto MemoryBlock::size() const -> size_t
{
	return __self().allocationInfo.size;
}

auto MemoryBlock::from(Device& device, MemoryBlockAllocateInfo&& info) -> MemoryBlock
{
	if (std::cmp_equal(info.memoryRequirement.size, 0) || std::cmp_equal(info.memoryRequirement.alignment, 0))
	{
		return {};
	}

	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	VmaAllocationCreateFlags allocationFlags = {};

	allocationFlags |= translate_memory_usage(info.memoryRequirement.usage);

	if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
		(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
	{
		allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VkMemoryRequirements const memReq{
		.size = info.memoryRequirement.size,
		.alignment = info.memoryRequirement.alignment,
		.memoryTypeBits = info.memoryRequirement.memoryTypeBits
	};

	VmaAllocationCreateInfo const allocCreateInfo{
		.flags = allocationFlags,
		.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};

	VmaAllocation handle = VK_NULL_HANDLE;
	VmaAllocationInfo allocInfo = {};

	CHECK_OP(vmaAllocateMemory(vkdevice.allocator, &memReq, &allocCreateInfo, &handle, &allocInfo))

	auto&& vkmemoryblock = *vkdevice.gpuResourcePool.stores.memoryBlocks.emplace();

	vkmemoryblock.handle = handle;
	vkmemoryblock.allocationInfo = std::move(allocInfo);
	vkmemoryblock.info.name = std::move(info.name);
	vkmemoryblock.info.usage = info.memoryRequirement.usage;
	vkmemoryblock.aliased = true;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkmemoryblock);
	}

	return MemoryBlock{ &vkmemoryblock, &vkdevice };
}

auto MemoryBlock::zombify(Device& dvc, ref_counted_base& resource) -> void
{	
	// Memory blocks that are not aliased are owned by the resource that references it and deallocation is done by the resource itself.
	auto&& actualDevice = static_cast<DeviceImpl&>(dvc);
	auto&& memoryBlock = static_cast<MemoryBlockImpl&>(resource);

	if (!memoryBlock.aliased)
	{
		return;                                                             
	}

	std::lock_guard const lock{ actualDevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = actualDevice.cpu_timeline();

	actualDevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&memoryBlock](DeviceImpl& device) -> void
		{
			vmaFreeMemory(device.allocator, memoryBlock.handle);

			auto it = device.gpuResourcePool.stores.memoryBlocks.get_iterator(&memoryBlock);
			device.gpuResourcePool.stores.memoryBlocks.erase(it);
		}
	);
};
}