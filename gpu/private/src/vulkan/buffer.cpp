#include "vulkan/vkgpu.hpp"

namespace gpu
{
Buffer::Buffer(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
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
		lib::memcopy(pointer, data, size);
	}
}

auto Buffer::clear() const -> void
{
	if (is_host_visible())
	{
		auto const& self = to_impl(*this);
		auto const& memoryBlock = to_impl(*self.allocationBlock);

		lib::memzero(memoryBlock.allocationInfo.pMappedData, m_info.size);
	}
}

auto Buffer::is_host_visible() const -> bool
{
	uint32 const usageMask = static_cast<uint32>(m_info.memoryUsage);
	uint32 const hostWritable = static_cast<uint32>(MemoryUsage::Host_Writable);
	uint32 const hostAccessible = static_cast<uint32>(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

auto Buffer::is_transient() const -> bool
{
	auto const& self = to_impl(*this);

	return self.allocationBlock->aliased();
}

auto Buffer::gpu_address() const -> DeviceAddress
{
	auto const& self = to_impl(*this);

	return DeviceAddress{ self.address };
}

auto Buffer::bind(BufferBindInfo const& info) const -> BufferBindInfo
{
	auto&& self = to_impl(*this);
	auto&& vkdevice = to_device(self.device());

	uint32 index = info.index;

	if (info.offset < self.m_info.size && self.m_info.size <= info.size)
	{
		index = index % vkdevice.config().maxBuffers;

		VkDeviceAddress const address = self.address + info.offset;

		vkdevice.descriptorCache.bdaHostAddress[index] = address;
	}

	return BufferBindInfo{ .offset = info.offset, .size = info.size, .index = index };
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

auto Buffer::from(Device& device, BufferInfo&& info, Resource<MemoryBlock> memoryBlock) -> Resource<Buffer>
{
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

		auto&& [memoryBlockId, vkmemoryblock] = vkdevice.gpuResourcePool.memoryBlocks.emplace(vkdevice, false);

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) Resource<MemoryBlock>{ memoryBlockId.to_uint64(), vkmemoryblock };
	}

	auto&& [id, vkbuffer] = vkdevice.gpuResourcePool.buffers.emplace(vkdevice);

	VkBufferDeviceAddressInfo addressInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = handle
	};

	vkbuffer.address = vkGetBufferDeviceAddress(vkdevice.device, &addressInfo);

	if (!info.name.empty())
	{
		info.name.format("<buffer>:{}", info.name.c_str());
	}

	vkbuffer.handle = handle;
	vkbuffer.allocationBlock = memoryBlock;
	vkbuffer.m_info = std::move(info);

	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		vkdevice.setup_debug_name(vkbuffer);
	}

	return Resource<Buffer>{ id.to_uint64(), vkbuffer };
}

auto Buffer::destroy(Buffer& resource, uint64 id) -> void
{
	/*
	* At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	*/
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vkbuffer = to_impl(resource);

	/*
	* Release ownership of it's memory allocation.
	*/
	vkbuffer.allocationBlock.destroy();

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Buffer);
}

namespace vk
{
BufferImpl::BufferImpl(DeviceImpl& device) :
	Buffer{ device }
{}
}
}