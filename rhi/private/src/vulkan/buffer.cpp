#include "buffer.h"
#include "vulkan/vk_device.h"

namespace rhi
{
Buffer::Buffer(
	BufferInfo&& info,
	void* mappedAddress,
	APIContext* context,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_mapped_address{ mappedAddress },
	m_offset{},
	m_owning_queue{ DeviceQueueType::None },
	m_binding{}
{
	m_context->setup_debug_name(*this);
}

Buffer::Buffer(Buffer&& rhs) noexcept
{
	*this = std::move(rhs);
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_mapped_address = std::move(rhs.m_mapped_address);
		m_offset = rhs.m_offset;
		m_owning_queue = rhs.m_owning_queue;
		m_binding = std::move(rhs.m_binding);
		Resource::operator=(std::move(rhs));
		new (&rhs) Buffer{};
	}
	return *this;
}

auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::data() const -> void*
{
	ASSERTION(is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	return m_mapped_address;
}

auto Buffer::offset() const -> size_t
{
	return m_offset;
}

auto Buffer::size() const -> size_t
{
	return m_info.size;
}

auto Buffer::write(void* data, size_t size) -> BufferWriteInfo
{
	bool const isWithinRange = (m_offset + size) <= this-> m_info.size;

	BufferWriteInfo info = { 
		.offset = m_offset, 
		.size = size
	};

	ASSERTION(is_host_visible() && "Cannot write to a non host visible buffer!");
	ASSERTION(isWithinRange && "Write will overflow out of the buffer.");

	if (is_host_visible() &&
		isWithinRange)
	{
		std::byte* pointer = reinterpret_cast<std::byte*>(this->data());
		pointer += m_offset;
		lib::memcopy(pointer, data, size);
		m_offset += size;
	}
	return info;
}

auto Buffer::flush() -> void
{
	m_offset = 0;
}

auto Buffer::is_host_visible() const -> bool
{
	uint32 const usageMask = static_cast<uint32>(m_info.memoryUsage);
	uint32 const hostWritable = static_cast<uint32>(MemoryUsage::Host_Writable);
	uint32 const hostAccessible = static_cast<uint32>(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

auto Buffer::owner() const -> DeviceQueueType
{
	return m_owning_queue;
}

auto Buffer::gpu_address() const -> uint64
{
	uint64 address = 0;
	if (valid())
	{
		address = as<vulkan::Buffer>().address;
	}
	return address;
}

}