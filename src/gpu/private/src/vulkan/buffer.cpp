#include "constants.hpp"
#include "gpu.hpp"
#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Buffer::data() const -> void*
{
	auto const& self = to_impl(*this);
	auto const& memoryBlock = to_impl(*self.allocationBlock);

	if (is_host_visible())
	{
		return memoryBlock.allocationInfo.pMappedData;
	}

	return nullptr;
}

auto Buffer::size() const -> size_t
{
	auto const& self = to_impl(*this);
	auto const& memoryBlock = to_impl(*self.allocationBlock);

	return is_transient() ? m_info.size : memoryBlock.allocationInfo.size;
}

auto Buffer::write(void const* data, size_t size, size_t offset) const -> void
{
	bool const isWithinRange = (offset + size) <= Buffer::size();

	if (is_host_visible() && isWithinRange)
	{
		std::byte* pointer = static_cast<std::byte*>(Buffer::data());
		pointer += offset;
		std::memcpy(pointer, data, size);
	}
}

auto Buffer::clear() const -> void
{
	if (is_host_visible())
	{
		[[maybe_unused]] auto const& self = to_impl(*this);
		[[maybe_unused]] auto const& memoryBlock = to_impl(*self.allocationBlock);

		//lib::memzero(memoryBlock.allocationInfo.pMappedData, m_info.size);
	}
}

auto Buffer::is_host_visible() const -> bool
{
	auto usageMask 		= std::to_underlying(m_info.memoryUsage);
	auto hostWritable 	= std::to_underlying(MemoryUsage::Host_Writable);
	auto hostAccessible = std::to_underlying(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

auto Buffer::is_transient() const -> bool
{
	auto const& self = to_impl(*this);

	return (*self.allocationBlock).aliased();
}

auto Buffer::gpu_address() const -> DeviceAddress
{
	auto const& self = to_impl(*this);

	return DeviceAddress{ self.address };
}

auto Buffer::memory_requirement(Device& device, BufferInfo const& info) -> MemoryRequirementInfo
{
	auto&& vkdevice = to_device(device);

	VkBufferCreateInfo bufferCreateInfo = vk::get_buffer_create_info(info);

	VkDeviceBufferMemoryRequirements bufferMemReq{
		.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
		.pCreateInfo = &bufferCreateInfo
	};

	VkMemoryRequirements2 memReq{
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
	};

	vkGetDeviceBufferMemoryRequirements(vkdevice.device, &bufferMemReq, &memReq);

	return MemoryRequirementInfo{
		.size = memReq.memoryRequirements.size,
		.alignment = memReq.memoryRequirements.alignment,
		.memoryTypeBits = memReq.memoryRequirements.memoryTypeBits
	};
}

auto Buffer::from(Device& device, BufferInfo&& info, MemoryBlock::handle_type memoryBlock) -> handle_type
{
	ASSERTION(info.memoryUsage != MemoryUsage::None && "MemoryUsage cannot be 'None'");
	ASSERTION(info.size != 80000 && "Allocation size is below threshold");
	
	auto&& vkdevice = to_device(device);

	uint32 queueFamilyIndices[] = {
		vkdevice.mainQueue.familyIndex,
		vkdevice.computeQueue.familyIndex,
		vkdevice.transferQueue.familyIndex
	};

	VkBufferCreateInfo bufferInfo = vk::get_buffer_create_info(info);

	if ((bufferInfo.sharingMode & VK_SHARING_MODE_CONCURRENT) != 0)
	{
		bufferInfo.queueFamilyIndexCount = 3u;
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	VkBuffer handle = VK_NULL_HANDLE;

	if (memoryBlock.valid())
	{
		auto const& memBlockImpl = to_impl(*memoryBlock);
		MemoryRequirementInfo const memReq = Buffer::memory_requirement(device, info);

		if ((memReq.memoryTypeBits & memBlockImpl.allocationInfo.memoryType) != memReq.memoryTypeBits ||
			memReq.size > memBlockImpl.allocationInfo.size)
		{
			return {};
		}

		CHECK_OP(vkCreateBuffer(vkdevice.device, &bufferInfo, nullptr, &handle))

		vmaBindBufferMemory(vkdevice.allocator, memBlockImpl.handle, handle);
	}
	else
	{
		auto allocationFlags = vk::translate_memory_usage(info.memoryUsage);

		if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
			(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
		{
			allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VmaAllocationCreateInfo allocInfo{
			.flags = allocationFlags,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};

		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo = {};

		CHECK_OP(vmaCreateBuffer(vkdevice.allocator, &bufferInfo, &allocInfo, &handle, &allocation, &allocationInfo))

		auto it = vkdevice.gpuResourcePool.stores.memoryBlocks.emplace(false);

		vk::_ResourceMeta meta{
			.type = vk::detail::type_id_v<vk::MemoryBlockImpl>,
			.id = ++vkdevice.gpuResourcePool.idCounter.others
		};

		auto id = std::bit_cast<uint64>(meta);

		vkdevice.gpuResourcePool.caches.memoryBlock.emplace(id, it);
		vkdevice.begin_referencing(id);

		auto&& vkmemoryblock = *it;

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) MemoryBlock::handle_type{ vkdevice, id };
	}

	auto it = vkdevice.gpuResourcePool.stores.buffers.emplace();

	vk::_ResourceMeta meta{
		.type = vk::detail::type_id_v<vk::BufferImpl>,
		.id = vkdevice.gpuResourcePool.idCounter.buffers++ % vkdevice.config().maxBuffers
	};

	auto id = std::bit_cast<uint64>(meta);

	auto&& vkbuffer = *it;

	VkBufferDeviceAddressInfo addressInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = handle
	};

	vkbuffer.address = vkGetBufferDeviceAddress(vkdevice.device, &addressInfo);
	vkbuffer.handle = handle;
	vkbuffer.allocationBlock = memoryBlock;
	vkbuffer.m_info = std::move(info);

	vkdevice.gpuResourcePool.caches.buffer.emplace(id, it);
	vkdevice.bind(vkbuffer, meta.id);
	vkdevice.begin_referencing(id);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkbuffer);
	}

	return { vkdevice, id };
}

auto Buffer::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Buffer with an invalid id.");
	/*
	* At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	*/
	auto&& vkdevice = to_device(device);
	auto&& vkbuffer = *vkdevice.gpuResourcePool.caches.buffer[id];

	/*
	* Release ownership of it's memory allocation.
	*/
	vkbuffer.allocationBlock.destroy();

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();
	
	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkbuffer, id](vk::DeviceImpl& device) -> void
		{	
			if (!vkbuffer.is_transient())
			{
				vkbuffer.allocationBlock.destroy();
			}
			else
			{
				vkDestroyBuffer(device.device, vkbuffer.handle, nullptr);
			}

			auto it = device.gpuResourcePool.caches.buffer[id];

			device.gpuResourcePool.caches.buffer.erase(id);
			device.gpuResourcePool.stores.buffers.erase(it);
		}
	);
}
}