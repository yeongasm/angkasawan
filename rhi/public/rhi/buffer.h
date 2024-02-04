#pragma once
#ifndef RHI_BUFFER_H
#define RHI_BUFFER_H

#include "resource.h"

namespace rhi
{
struct BufferWriteInfo
{
	size_t offset;
	size_t size;
};

class Buffer : public Resource
{
public:
	RHI_API Buffer() = default;
	RHI_API ~Buffer() = default;

	RHI_API Buffer(Buffer&& rhs) noexcept;
	RHI_API Buffer& operator=(Buffer&& rhs) noexcept;

	RHI_API auto info() const -> BufferInfo const&;
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
	/**
	* \brief Returns a GPU side address of this buffer.
	*/
	RHI_API auto gpu_address() const -> uint64;
private:
	friend struct APIContext;
	friend class CommandBuffer;

	BufferInfo m_info = {};
	void* m_mapped_address = {};
	size_t m_offset = {};

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
