#include "buffer.h"
#include "vulkan/vk_device.h"

namespace rhi
{
BufferView::BufferView() :
	m_buffer{},
	m_buffer_offset{},
	m_current_offset{},
	m_size{},
	m_owning_queue{ DeviceQueueType::None },
	m_binding{}
{}

BufferView::BufferView(
	Buffer& buffer,
	size_t offset,
	size_t size
) :
	m_buffer{ &buffer },
	m_buffer_offset{ offset },
	m_current_offset{},
	m_size{ size },
	m_owning_queue{ DeviceQueueType::None },
	m_binding{}
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
		m_binding = std::move(rhs.m_binding);
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

auto BufferView::make_view(BufferViewInfo const& info) const -> BufferView
{
	return m_buffer->make_view({ .offset = m_buffer_offset + info.offset, .size = info.size });
}

auto BufferView::data() const -> void*
{
	ASSERTION(m_buffer->is_host_visible() && "Memory for this buffer is GPU exclusive and is not accessible by the host.");
	void* address = nullptr;
	if (m_buffer->m_mapped_address)
	{
		address = static_cast<uint8*>(m_buffer->m_mapped_address) + m_buffer_offset;
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

auto BufferView::write(void* data, size_t size) -> BufferWriteInfo
{
	bool const isWithinRange = (m_current_offset + size) <= m_size;

	BufferWriteInfo info = {
		.offset = m_buffer_offset + m_current_offset,
		.size = size 
	};

	ASSERTION(m_buffer->is_host_visible() && "Cannot write to a non host visible buffer!");
	ASSERTION(isWithinRange && "Write will overflow out of the buffer.");

	if (m_buffer->is_host_visible() &&
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

auto BufferView::owner() const -> DeviceQueueType
{
	return m_owning_queue;
}

auto BufferView::bind() -> BindingSlot<Buffer> const&
{
	if (m_binding.slot == BindingSlot<Buffer>::INVALID)
	{
		uint32 index = std::numeric_limits<uint32>::max();
		APIContext* apiContext = m_buffer->m_context;

		if (apiContext->descriptorCache.bdaFreeSlots.size())
		{
			index = apiContext->descriptorCache.bdaFreeSlots.back();
			apiContext->descriptorCache.bdaFreeSlots.pop_back();
		}
		else
		{
			ASSERTION(
				(apiContext->descriptorCache.bdaBindingSlot + 1) < apiContext->config.maxBuffers
				&& "Maximum bindable buffer count reached. Slot will rotate back to the 0th index."
			);
			index = apiContext->descriptorCache.bdaBindingSlot++;
			if (apiContext->descriptorCache.bdaBindingSlot >= apiContext->config.maxBuffers)
			{
				apiContext->descriptorCache.bdaBindingSlot = 0;
			}
		}

		VkDeviceAddress const address = m_buffer->as<vulkan::Buffer>().address + m_buffer_offset;
		apiContext->descriptorCache.bdaHostAddress[index] = address;

		m_binding.slot = index;
	}
	return m_binding;
}

auto BufferView::unbind() -> void
{
	if (m_binding.slot != BindingSlot<Buffer>::INVALID)
	{
		APIContext* apiContext = m_buffer->m_context;
		apiContext->descriptorCache.bdaFreeSlots.emplace_back(m_binding.slot);
		m_binding.slot = BindingSlot<Buffer>::INVALID;
	}
}

auto BufferView::gpu_address() const -> uint64
{
	uint64 address = 0;
	if (valid())
	{
		address = m_buffer->as<vulkan::Buffer>().address + m_buffer_offset;
	}
	return address;
}

auto BufferView::binding() const -> BindingSlot<Buffer> const&
{
	return m_binding;
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

Buffer::operator BufferView()
{
	return make_view();
}

auto Buffer::info() const -> BufferInfo const&
{
	return m_info;
}

auto Buffer::make_view(BufferViewInfo const& info) -> BufferView
{
	ASSERTION(info.offset < m_info.size	&& "Requested offset for buffer view exceeded the capacity of the buffer.");

	size_t size = info.size;

	if (info.size == std::numeric_limits<size_t>::max())
	{
		size = m_info.size;
	}

	ASSERTION(info.offset + size <= m_info.size		&& "Requested range for buffer view exceeded the capacity of the buffer.");
	ASSERTION(size <= m_info.size					&& "Requested size for buffer view exceeded the capacity of the buffer.");

	return BufferView{ *this, info.offset, size };
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

auto Buffer::bind() -> BindingSlot<Buffer> const&
{
	if (m_binding.slot == BindingSlot<Buffer>::INVALID)
	{
		uint32 index = std::numeric_limits<uint32>::max();

		if (m_context->descriptorCache.bdaFreeSlots.size())
		{
			index = m_context->descriptorCache.bdaFreeSlots.back();
			m_context->descriptorCache.bdaFreeSlots.pop_back();
		}
		else
		{
			ASSERTION(
				(m_context->descriptorCache.bdaBindingSlot + 1) < m_context->config.maxBuffers
				&& "Maximum bindable buffer count reached. Slot will rotate back to the 0th index."
			);
			index = m_context->descriptorCache.bdaBindingSlot++;
			if (m_context->descriptorCache.bdaBindingSlot >= m_context->config.maxBuffers)
			{
				m_context->descriptorCache.bdaBindingSlot = 0;
			}
		}
		VkDeviceAddress const address = as<vulkan::Buffer>().address;
		m_context->descriptorCache.bdaHostAddress[index] = address;

		m_binding.slot = index;
	}
	return m_binding;
}

auto Buffer::unbind() -> void
{
	if (m_binding.slot != BindingSlot<Buffer>::INVALID)
	{
		m_context->descriptorCache.bdaFreeSlots.emplace_back(m_binding.slot);
		m_binding.slot = BindingSlot<Buffer>::INVALID;
	}
}

auto Buffer::binding() const->BindingSlot<Buffer> const&
{
	return m_binding;
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