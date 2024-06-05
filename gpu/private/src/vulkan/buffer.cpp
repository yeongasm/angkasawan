#include "vulkan/vkgpu.h"

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
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Buffer::data() const -> void*
{
	auto&& self = to_impl(*this);

	if (is_host_visible())
	{
		return self.allocationInfo.pMappedData;
	}

	return nullptr;
}

auto Buffer::size() const -> size_t
{
	auto&& self = to_impl(*this);

	return self.allocationInfo.size;
}

auto Buffer::write(void const* data, size_t size, size_t offset) const -> void
{
	auto&& self = to_impl(*this);

	bool const isWithinRange = (offset + size) <= self.allocationInfo.size;

	if (is_host_visible() && isWithinRange)
	{
		std::byte* pointer = static_cast<std::byte*>(self.allocationInfo.pMappedData);
		pointer += offset;
		lib::memcopy(pointer, data, size);
	}
}

auto Buffer::clear() const -> void
{
	if (is_host_visible())
	{
		auto&& self = to_impl(*this);

		lib::memzero(self.allocationInfo.pMappedData, self.allocationInfo.size);
	}
}

auto Buffer::is_host_visible() const -> bool
{
	uint32 const usageMask = static_cast<uint32>(m_info.memoryUsage);
	uint32 const hostWritable = static_cast<uint32>(MemoryUsage::Host_Writable);
	uint32 const hostAccessible = static_cast<uint32>(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

auto Buffer::gpu_address() const -> uint64
{
	auto const& self = to_impl(*this);

	return self.address;
}

auto Buffer::bind(BufferBindInfo const& info) const -> BufferBindInfo
{
	auto&& self = to_impl(*this);
	auto&& vkdevice = to_device(self.device());

	uint32 index = info.index;

	if (info.offset < self.allocationInfo.size && self.allocationInfo.size <= info.size)
	{
		index = index % vkdevice.config().maxBuffers;

		VkDeviceAddress const address = self.address + info.offset;

		vkdevice.descriptorCache.bdaHostAddress[index] = address;
	}

	return BufferBindInfo{ .offset = info.offset, .size = info.size, .index = index };
}

auto Buffer::from(Device& device, BufferInfo&& info) -> Resource<Buffer>
{
	auto&& vkdevice = to_device(device);

	uint32 queueFamilyIndices[] = {
		vkdevice.mainQueue.familyIndex,
		vkdevice.computeQueue.familyIndex,
		vkdevice.transferQueue.familyIndex
	};

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | vk::translate_buffer_usage_flags(info.bufferUsage),
		.sharingMode = vk::translate_sharing_mode(info.sharingMode)
	};

	if ((bufferInfo.sharingMode & VK_SHARING_MODE_CONCURRENT) != 0)
	{
		bufferInfo.queueFamilyIndexCount = 3u;
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

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

	VkBuffer handle = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocationInfo = {};

	VkResult result = vmaCreateBuffer(vkdevice.allocator, &bufferInfo, &allocInfo, &handle, &allocation, &allocationInfo);

	if (result != VK_SUCCESS)
	{
		return null_resource;
	}

	auto&& [id, vkbuffer] = vkdevice.gpuResourcePool.buffers.emplace(vkdevice);

	VkBufferDeviceAddressInfo addressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = handle
	};

	vkbuffer.address = vkGetBufferDeviceAddress(vkdevice.device, &addressInfo);

	if (info.name.size())
	{
		info.name.format("<buffer>:{}", info.name.c_str());
	}

	vkbuffer.handle = handle;
	vkbuffer.allocation = allocation;
	vkbuffer.allocationInfo = std::move(allocationInfo);
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