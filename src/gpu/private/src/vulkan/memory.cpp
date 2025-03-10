#include "vulkan/vkgpu.hpp"

namespace gpu
{
MemoryBlock::MemoryBlock(Device& device, bool aliased) :
	DeviceResource{ device },
	m_aliased{ aliased }
{}

auto MemoryBlock::info() const -> MemoryBlockInfo const&
{
	return m_info;
}

auto MemoryBlock::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto MemoryBlock::aliased() const -> bool
{
	return m_aliased;
}

auto MemoryBlock::size() const -> size_t
{
	auto const& self = to_impl(*this);

	return self.allocationInfo.size;
}

auto MemoryBlock::from(Device& device, MemoryBlockAllocateInfo&& info) -> Resource<MemoryBlock>
{
	if (std::cmp_equal(info.memoryRequirement.size, 0) || std::cmp_equal(info.memoryRequirement.alignment, 0))
	{
		return {};
	}

	auto&& vkdevice = to_device(device);

	VmaAllocationCreateFlags allocationFlags = {};

	allocationFlags |= vk::translate_memory_usage(info.memoryRequirement.usage);

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

	auto it = vkdevice.gpuResourcePool.memoryBlocks.emplace(vkdevice, true);

	auto&& vkmemory = *it;

	if (!info.name.empty())
	{
		info.name.format("<memory>:{}", info.name.c_str());
	}

	vkmemory.handle = handle;
	vkmemory.allocationInfo = std::move(allocInfo);
	vkmemory.m_info.name = std::move(info.name);
	vkmemory.m_info.usage = info.memoryRequirement.usage;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkmemory);
	}

	return Resource<MemoryBlock>{ vkmemory, vk::to_id(it) };
}

auto MemoryBlock::destroy(MemoryBlock& resource, Id id) -> void
{
	// Memory blocks that are not aliased are owned by the resource that references it and deallocation is done by the resource itself.
	if (!resource.aliased())
	{
		return;
	}

	auto&& vkdevice = to_device(resource.m_device);
	auto&& vkmemoryblock = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkmemoryblock, id](vk::DeviceImpl& device) -> void
		{
			using iterator = typename lib::hive<vk::MemoryBlockImpl>::iterator;

			vmaFreeMemory(device.allocator, vkmemoryblock.handle);

			auto const it = vk::to_hive_it<iterator>(id);

			device.gpuResourcePool.memoryBlocks.erase(it);
		}
	);
}

namespace vk
{
MemoryBlockImpl::MemoryBlockImpl(DeviceImpl& device, bool aliased) :
	MemoryBlock{ device, aliased }
{}
}
}