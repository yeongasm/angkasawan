#include "buffer.h"
#include "vulkan/vk_device.h"

namespace rhi
{
BufferView::BufferView() :
	m_buffer{},
	m_offset{},
	m_size{}
{}

BufferView::BufferView(
	Buffer& buffer,
	size_t offset,
	size_t size
) :
	m_buffer{ &buffer },
	m_offset{ offset },
	m_size{ size }
{}

BufferView::BufferView(BufferView&& rhs) noexcept
{
	*this = std::move(rhs);
}

BufferView& BufferView::operator=(BufferView&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_buffer = rhs.m_buffer;
		m_offset = rhs.m_offset;
		m_size = rhs.m_size;
		new (&rhs) BufferView{};
	}
	return *this;
}

auto BufferView::valid() const -> bool
{
	return m_buffer != nullptr;
}

auto BufferView::buffer() const -> Buffer&
{
	ASSERTION(m_buffer && "Buffer view is not referencing a buffer resource.");
	return *m_buffer;
}

auto BufferView::data() const -> void*
{
	ASSERTION(m_buffer->is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	void* address = nullptr;
	if (m_buffer->m_mapped_address)
	{
		address = static_cast<uint8*>(m_buffer->m_mapped_address) + m_offset;
	}
	return address;
}

auto BufferView::size() const -> size_t
{
	return m_size;
}

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
	m_owning_queue{ DeviceQueueType::None }
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
		m_offset = rhs.m_offset;
		m_owning_queue = rhs.m_owning_queue;
		Resource::operator=(std::move(rhs));
		new (&rhs) Buffer{};
	}
	return *this;
}

auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::make_view(MakeBufferViewInfo const& info) -> BufferView
{
	ASSERTION(is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	if (!is_host_visible())
	{
		return BufferView{};
	}
	ASSERTION(info.offset < m_info.size					&& "Requested offset for buffer view exceeded the capacity of the buffer.");
	ASSERTION(info.size < m_info.size					&& "Requested size for buffer view exceeded the capacity of the buffer.");
	ASSERTION(info.offset + info.size <= m_info.size	&& "Requested range for buffer view exceeded the capacity of the buffer.");
	return BufferView{ *this, info.offset, info.size };
}

auto Buffer::data() const -> void*
{
	ASSERTION(is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	return m_mapped_address;
}

auto Buffer::is_host_visible() const -> bool
{
	uint32 const usageMask = static_cast<uint32>(m_info.memoryUsage);
	uint32 const hostWritable = static_cast<uint32>(MemoryUsage::Host_Writable);
	uint32 const hostAccessible = static_cast<uint32>(MemoryUsage::Host_Accessible);

	return (usageMask & hostWritable) || (usageMask & hostAccessible);
}

}