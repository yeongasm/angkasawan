#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Buffer::info() const -> BufferInfo const&
{
	return __self().info;
}

auto Buffer::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Buffer::data() const -> void*
{
	auto const& memoryBlock = impl_of(__self().memoryBlock);

	if (is_host_visible())
	{
		return memoryBlock.allocationInfo.pMappedData;
	}

	return nullptr;
}

auto Buffer::size() const -> size_t
{
	auto const& self = __self();
	auto const& memoryBlock = impl_of(self.memoryBlock);

	return is_transient() ? self.info.size : memoryBlock.allocationInfo.size;
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
{}

auto Buffer::is_host_visible() const -> bool
{
	auto usageMask 		= std::to_underlying(__self().info.memoryUsage);
	auto hostWritable 	= std::to_underlying(MemoryUsage::Host_Writable);
	auto hostAccessible = std::to_underlying(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

auto Buffer::is_transient() const -> bool
{
	return __self().memoryBlock.aliased();
}

auto Buffer::gpu_address() const -> device_address_t
{
	return __self().address;
}

auto Buffer::id() const -> resource_id_t
{
	return __self().id;
}

auto Buffer::memory_requirement(Device& device, BufferInfo const& info) -> MemoryRequirementInfo
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	VkBufferCreateInfo bufferCreateInfo = get_buffer_create_info(info);

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

auto Buffer::from(Device& device, BufferInfo&& info, MemoryBlock memoryBlock) -> Buffer
{
	ASSERTION(info.memoryUsage != MemoryUsage::None && "MemoryUsage cannot be 'None'");
	ASSERTION(info.size != 80000 && "Allocation size is below threshold");
	
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	uint32 queueFamilyIndices[] = {
		vkdevice.mainQueue.familyIndex,
		vkdevice.computeQueue.familyIndex,
		vkdevice.transferQueue.familyIndex
	};

	VkBufferCreateInfo bufferInfo = get_buffer_create_info(info);

	if ((bufferInfo.sharingMode & VK_SHARING_MODE_CONCURRENT) != 0)
	{
		bufferInfo.queueFamilyIndexCount = 3u;
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	VkBuffer handle = VK_NULL_HANDLE;

	if (memoryBlock.valid())
	{
		auto const& memBlockImpl = impl_of(memoryBlock);
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
		auto allocationFlags = translate_memory_usage(info.memoryUsage);

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

		auto&& vkmemoryblock = *vkdevice.gpuResourcePool.stores.memoryBlocks.emplace();

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) MemoryBlock{ &vkmemoryblock, &vkdevice };
	}

	auto&& vkbuffer = *vkdevice.gpuResourcePool.stores.buffers.emplace();

	reflect::_ResourceMeta meta{
		.id = vkdevice.gpuResourcePool.idCounter.buffers++ % vkdevice.config().maxBuffers,
		.type = reflect::type_id_v<BufferImpl>
	};

	VkBufferDeviceAddressInfo addressInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT,
		.buffer = handle
	};

	vkbuffer.address = vkGetBufferDeviceAddress(vkdevice.device, &addressInfo);
	vkbuffer.handle = handle;
	vkbuffer.memoryBlock = memoryBlock;
	vkbuffer.info = std::move(info);
	vkbuffer.id = std::bit_cast<uint64>(meta);

	vkdevice.bind(vkbuffer, meta.id);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkbuffer);
	}

	return Buffer{ &vkbuffer, &vkdevice };
}

auto Buffer::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	BufferImpl& buffer = static_cast<BufferImpl&>(resource);

	bool const bufferIsTransient = buffer.memoryBlock.aliased();
	/*
	* Release ownership of it's memory allocation.
	*/
	buffer.memoryBlock.destroy();

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();
	
	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&buffer, bufferIsTransient](DeviceImpl& device) -> void
		{	
			if (!bufferIsTransient)
			{
				auto&& block = shared_base::impl_of(buffer.memoryBlock);

				vmaDestroyBuffer(device.allocator, buffer.handle, block.handle);

				auto it = device.gpuResourcePool.stores.memoryBlocks.get_iterator(&block);
				device.gpuResourcePool.stores.memoryBlocks.erase(it);
			}
			else
			{
				vkDestroyBuffer(device.device, buffer.handle, nullptr);
			}

			auto it = device.gpuResourcePool.stores.buffers.get_iterator(&buffer);
			device.gpuResourcePool.stores.buffers.erase(it);
		}
	);
}
}