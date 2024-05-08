module;

#include <mutex>

#include "vulkan/vk.h"
#include "lib/memory.h"

module forge;

namespace frg
{
auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Buffer::data() const -> void*
{
	return m_impl.allocationInfo.pMappedData;
}

auto Buffer::size() const -> size_t
{
	return m_impl.allocationInfo.size;
}

auto Buffer::write(void const* data, size_t size, size_t offset) const -> void
{
	bool const isWithinRange = (offset + size) <= m_impl.allocationInfo.size;

	if (is_host_visible() &&
		isWithinRange)
	{
		std::byte* pointer = static_cast<std::byte*>(m_impl.allocationInfo.pMappedData);
		pointer += offset;
		lib::memcopy(pointer, data, size);
	}
}

auto Buffer::clear() const -> void
{
	if (is_host_visible())
	{
		lib::memzero(m_impl.allocationInfo.pMappedData, m_impl.allocationInfo.size);
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
	return m_impl.address;
}

auto Buffer::from(Device& device, BufferInfo&& info) -> Resource<Buffer>
{
	auto&& [index, buffer] = device.m_gpuResourcePool.buffers.emplace(device);

	VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = info.size,
		.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | api::translate_buffer_usage_flags(info.bufferUsage),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	auto allocationFlags = api::translate_memory_usage(info.memoryUsage);

	if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
		(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
	{
		allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationCreateInfo allocInfo{
		.flags = allocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};

	VmaAllocationInfo allocationInfo = {};
	VkResult result = vmaCreateBuffer(device.m_apiContext.allocator, &bufferInfo, &allocInfo, &buffer.m_impl.handle, &buffer.m_impl.allocation, &buffer.m_impl.allocationInfo);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.buffers.erase(index);

		return null_resource;
	}

	VkBufferDeviceAddressInfo addressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = buffer.m_impl.handle
	};
	buffer.m_impl.address = vkGetBufferDeviceAddress(device.m_apiContext.device, &addressInfo);

	info.name.format("<buffer>:{}", info.name.c_str());

	buffer.m_info = std::move(info);

	return Resource{ index, buffer };
}

auto Buffer::destroy(Buffer const& resource, id_type id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a refernece to itself and can be safely deleted.
	 */
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Buffer> });
}

Buffer::Buffer(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}

}