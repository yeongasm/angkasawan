#pragma once
#ifndef RHI_BUFFER_H
#define RHI_BUFFER_H

#include "resource.h"

namespace rhi
{
class Buffer;

struct BufferViewInfo
{
	size_t offset = 0;
	size_t size = std::numeric_limits<size_t>::max();
};

struct BufferWriteInfo
{
	size_t offset;
	size_t size;
};

class BufferView
{
public:
	RHI_API BufferView();
	RHI_API ~BufferView() = default;

	RHI_API BufferView(BufferView&& rhs) noexcept;
	RHI_API BufferView& operator=(BufferView&& rhs) noexcept;

	RHI_API auto valid() const -> bool;
	RHI_API auto buffer() const -> Buffer&;
	RHI_API auto make_view(BufferViewInfo const& info = {}) const -> BufferView;
	RHI_API auto data() const -> void*;
	RHI_API auto offset() const -> size_t;
	RHI_API auto offset_from_buffer() const -> size_t;
	RHI_API auto size() const -> size_t;
	RHI_API auto write(void* data, size_t size) -> BufferWriteInfo;
	RHI_API auto flush() -> void;
	RHI_API auto owner() const -> DeviceQueueType;
	/**
	* \brief Makes the buffer accessible in shaders.
	*/
	RHI_API auto bind() -> BindingSlot<Buffer> const&;
	/**
	* \brief Removes buffer accessibility in shaders.
	*/
	RHI_API auto unbind() -> void;
	RHI_API auto binding() const -> BindingSlot<Buffer> const&;
	/**
	* \brief Returns a GPU side address of this buffer.
	*/
	RHI_API auto gpu_address() const -> uint64;
private:
	friend class CommandBuffer;
	friend class Buffer;

	Buffer* m_buffer;
	size_t m_buffer_offset;
	size_t m_current_offset;
	size_t m_size;
	DeviceQueueType m_owning_queue;
	BindingSlot<Buffer> m_binding;

	BufferView(
		Buffer& buffer,
		size_t offset,
		size_t size
	);

	BufferView(BufferView const&) = delete;
	BufferView& operator=(BufferView const&) = delete;
};

class Buffer : public Resource
{
public:
	RHI_API Buffer() = default;
	RHI_API ~Buffer() = default;

	RHI_API Buffer(Buffer&& rhs) noexcept;
	RHI_API Buffer& operator=(Buffer&& rhs) noexcept;

	RHI_API explicit operator BufferView();

	RHI_API auto info() const -> BufferInfo const&;
	RHI_API auto make_view(BufferViewInfo const& info = {}) -> BufferView;
	/**
	* \brief Returns the CPU side address when the buffer is host visible. 
	* If the buffer is device local, calling this function returns nullptr instead.
	*/
	RHI_API auto data() const -> void*;
	/**
	* \brief Returns the current offset in the buffer if it is host visible.
	* If the buffer is device local, calling this function returns 0 instead.
	*/
	RHI_API auto offset() const -> size_t;
	RHI_API auto size() const -> size_t;
	/**
	* \brief Writes data supplied into the buffer if it is host visible.
	* If the buffer is device local, calling this function does nothing.
	*/
	RHI_API auto write(void* data, size_t size) -> BufferWriteInfo;
	/**
	* \brief Flushes the internal offset pointer in the buffer to 0.
	* Has no significant meaning if the buffer is device local.
	*/
	RHI_API auto flush() -> void;
	RHI_API auto is_host_visible() const -> bool;
	RHI_API auto owner() const -> DeviceQueueType;
	/**
	* \brief Makes the buffer accessible in shaders.
	*/
	RHI_API auto bind() -> BindingSlot<Buffer> const&;
	/**
	* \brief Removes buffer accessibility in shaders.
	*/
	RHI_API auto unbind() -> void;
	RHI_API auto binding() const -> BindingSlot<Buffer> const&;
	/**
	* \brief Returns a GPU side address of this buffer.
	*/
	RHI_API auto gpu_address() const -> uint64;
private:
	friend struct APIContext;
	friend class CommandBuffer;
	friend class BufferView;

	BufferInfo m_info = {};
	void* m_mapped_address = {};
	size_t m_offset = {};
	DeviceQueueType m_owning_queue = {};
	BindingSlot<Buffer> m_binding = {};

	Buffer(
		BufferInfo&& info,
		void* mappedAddress,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_BUFFER_H
