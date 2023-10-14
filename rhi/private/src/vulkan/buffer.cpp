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
	void* address = nullptr;
	if (m_buffer->m_device_local_address)
	{
		address = static_cast<uint8*>(m_buffer->m_device_local_address) + m_offset;
	}
	return address;
}

auto BufferView::size() const -> size_t
{
	return m_size;
}

Buffer::Buffer(
	BufferInfo&& info,
	APIContext* context,
	void* bufferAddress,
	void* data,
	resource_type typeId
) :
	Resource{ context, data, typeId },
	m_info{ std::move(info) },
	m_device_local_address{ bufferAddress },
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
		m_device_local_address = rhs.m_device_local_address;
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
	if (m_info.locality == MemoryLocality::Gpu)
	{
		return BufferView{};
	}
	ASSERTION(info.offset < m_info.size				&& "Requested offset for buffer view exceeded the capacity of the buffer.");
	ASSERTION(info.size < m_info.size				&& "Requested size for buffer view exceeded the capacity of the buffer.");
	ASSERTION(info.offset + info.size < m_info.size	&& "Requested range for buffer view exceeded the capacity of the buffer.");
	return BufferView{ *this, info.offset, info.size };
}

auto Buffer::data() const -> void*
{
	ASSERTION(m_info.locality != MemoryLocality::Gpu && "Buffer with a GPU memory locality can not have their address be accessed.");
	return m_device_local_address;
}

}