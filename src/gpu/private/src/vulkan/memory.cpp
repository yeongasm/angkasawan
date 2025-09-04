#include "vulkan/vkgpu.hpp"

namespace gpu
{
MemoryBlock::MemoryBlock(bool aliased) :
	m_aliased{ aliased }
{}

auto MemoryBlock::info() const -> MemoryBlockInfo const&
{
	return m_info;
}

auto MemoryBlock::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
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

auto MemoryBlock::from(Device& device, MemoryBlockAllocateInfo&& info) -> handle_type
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

	auto it = vkdevice.gpuResourcePool.stores.memoryBlocks.emplace(true);
	
	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::MemoryBlockImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.memoryBlock.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vkmemory = *it;

	vkmemory.handle = handle;
	vkmemory.allocationInfo = std::move(allocInfo);
	vkmemory.m_info.name = std::move(info.name);
	vkmemory.m_info.usage = info.memoryRequirement.usage;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkmemory);
	}

	return handle_type{ vkdevice, id };
}

auto MemoryBlock::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a MemoryBlock with an invalid id.");
	
	auto&& vkdevice = to_device(device);
	auto&& vkmemoryblock = *vkdevice.gpuResourcePool.caches.memoryBlock[id];
	
	// Memory blocks that are not aliased are owned by the resource that references it and deallocation is done by the resource itself.
	if (!vkmemoryblock.aliased())
	{
		return;                                                             
	}

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkmemoryblock, id](vk::DeviceImpl& device) -> void
		{
			vmaFreeMemory(device.allocator, vkmemoryblock.handle);

			auto it = device.gpuResourcePool.caches.memoryBlock[id];

			device.gpuResourcePool.caches.memoryBlock.erase(id);
			device.gpuResourcePool.stores.memoryBlocks.erase(it);
		}
	);
}

namespace vk
{
MemoryBlockImpl::MemoryBlockImpl(bool aliased) :
	MemoryBlock{ aliased }
{}
}
}