#include "buffer_view_registry.h"

namespace sandbox
{
BufferView::BufferView() :
	m_buffer{},
	m_buffer_offset{},
	m_current_offset{},
	m_size{},
	m_owning_queue{ rhi::DeviceQueueType::None }
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
		m_buffer_offset = rhs.m_buffer_offset;
		m_current_offset = rhs.m_current_offset;
		m_size = rhs.m_size;
		m_owning_queue = rhs.m_owning_queue;
		new (&rhs) BufferView{};
	}
	return *this;
}

auto BufferView::valid() const -> bool
{
	return !m_buffer.is_null();
}

auto BufferView::buffer() const -> rhi::Buffer&
{
	ASSERTION(!m_buffer.is_null() && "Buffer view is not referencing a buffer resource.");
	return *m_buffer;
}

auto BufferView::data() const -> void*
{
	ASSERTION(!m_buffer.is_null() && "Buffer view is not referencing a buffer resource.");
	ASSERTION(m_buffer->is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	void* address = nullptr;
	if (m_buffer->is_host_visible())
	{
		address = static_cast<uint8*>(m_buffer->data()) + m_buffer_offset;
	}
	return address;
}

auto BufferView::offset() const -> size_t
{
	return m_current_offset;
}

auto BufferView::offset_from_buffer() const -> size_t
{
	return m_buffer_offset;
}

auto BufferView::size() const -> size_t
{
	return m_size;
}

auto BufferView::write(void* data, size_t size) -> rhi::BufferWriteInfo
{
	bool const isWithinRange = (m_current_offset + size) <= m_size;

	rhi::BufferWriteInfo info{
		.offset = m_buffer_offset + m_current_offset,
		.size = size 
	};

	ASSERTION(!m_buffer.is_null() && "Buffer view is not referencing a buffer resource.");
	ASSERTION(m_buffer->is_host_visible() && "Cannot write to a non host visible buffer!");
	ASSERTION(isWithinRange && "Write will overflow out of the buffer.");

	if (!m_buffer.is_null() &&
		m_buffer->is_host_visible() &&
		isWithinRange)
	{
		lib::memcopy(this->data(), data, size);
		m_current_offset += size;
	}

	return info;
}

auto BufferView::flush() -> void
{
	m_current_offset = 0;
}

auto BufferView::owner() const -> rhi::DeviceQueueType
{
	return m_owning_queue;
}

auto BufferView::gpu_address() const -> uint64
{
	uint64 address = 0;
	if (valid())
	{
		address = m_buffer->gpu_address() + m_buffer_offset;
	}
	return address;
}

BufferViewRegistry::BufferViewRegistry(ResourceCache& resourceCache) :
	m_resource_cache{ resourceCache },
	m_registry{},
	m_free_slots{}
{}

auto BufferViewRegistry::create_buffer_view(BufferViewInfo const& info) -> std::pair<buffer_handle, lib::ref<BufferView>>
{
	constexpr buffer_handle INVALID_HANDLE = buffer_handle::invalid_handle();

	if (!info.buffer.valid())
	{
		return std::pair{ INVALID_HANDLE, lib::ref<BufferView>{} };
	}

	resource_index const src{ info.buffer.get() };
	lib::ref<rhi::Buffer> buffer = m_resource_cache.get_buffer(info.buffer);

	if (buffer.is_null())
	{
		return std::pair{ INVALID_HANDLE, lib::ref<BufferView>{} };
	}

	if (!validate_buffer_view_info(info, *buffer))
	{
		return std::pair{ INVALID_HANDLE, lib::ref<BufferView>{} };
	}

	uint32 index = 0;
	BufferViewSlot& bufferViewSlot = m_registry[src._metadata.id];
	FreeSlot& freeSlot = m_free_slots[src._metadata.id];

	if (freeSlot.count)
	{
		index = freeSlot.slots[freeSlot.count--];
	}
	else
	{
		if (bufferViewSlot.count >= MAX_BUFFER_VIEW_PER_BUFFER)
		{
			return std::pair{ INVALID_HANDLE, lib::ref<BufferView>{} };
		}
		index = bufferViewSlot.index++;
	}

	BufferView& bufferView = bufferViewSlot.slots[index];

	bufferView.m_buffer = buffer;
	bufferView.m_buffer_offset = info.offset;
	bufferView.m_size = info.size;
	bufferView.m_owning_queue = rhi::DeviceQueueType::None;

	++bufferViewSlot.count;

	resource_index idx{ src._metadata.id, index };

	return std::pair{ buffer_handle{ idx._alias }, lib::ref<BufferView>{ bufferView } };
}

auto BufferViewRegistry::get_buffer_view(buffer_handle handle) -> lib::ref<BufferView>
{
	resource_index const idx{ handle.get() };

	auto bufferViewSlot = m_registry.at(idx._metadata.parent);

	if (bufferViewSlot.is_null())
	{
		return lib::ref<BufferView>{};
	}

	BufferView& bufferView = bufferViewSlot->second.slots[idx._metadata.id];

	return lib::ref<BufferView>{ bufferView };
}

auto BufferViewRegistry::get_source_handle(buffer_handle handle) -> buffer_handle
{
	buffer_handle src = handle;
	if (resource_index const idx{ handle.get() }; idx._metadata.parent)
	{
		src = buffer_handle{ resource_index{ 0, idx._metadata.parent }._alias };
	}
	return src;
}

auto BufferViewRegistry::destroy_buffer_view(buffer_handle handle) -> void
{
	resource_index const idx{ handle.get() };

	auto bufferViewSlot = m_registry.at(idx._metadata.parent);

	if (!bufferViewSlot.is_null())
	{
		FreeSlot& freeSlot = m_free_slots[idx._metadata.parent];
		freeSlot.slots[freeSlot.count++] = idx._metadata.id;

		--bufferViewSlot->second.count;
	}
}

auto BufferViewRegistry::resource_cache() const -> ResourceCache&
{
	return m_resource_cache;
}

auto BufferViewRegistry::validate_buffer_view_info(BufferViewInfo const& info, rhi::Buffer& buffer) -> bool
{
	return  (info.offset < buffer.size()) && 
			(info.offset + info.size <= buffer.size()) && 
			(info.size <= buffer.size());
}
}
