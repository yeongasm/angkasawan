#pragma once
#ifndef RHI_BUFFER_H
#define RHI_BUFFER_H

#include "resource.h"

namespace rhi
{
class Buffer;
class BufferView
{
public:
	RHI_API BufferView();
	RHI_API ~BufferView() = default;

	RHI_API auto valid() const -> bool;
	RHI_API auto buffer() const -> Buffer&;
	RHI_API auto data() const -> void*;
	RHI_API auto size() const -> size_t;
private:
	friend class Buffer;

	Buffer* m_buffer;
	size_t const m_offset;
	size_t const m_size;

	BufferView(
		Buffer& buffer,
		size_t offset,
		size_t size
	);
};

struct MakeBufferViewInfo
{
	size_t offset = 0;
	size_t size = std::numeric_limits<size_t>::max();
};

class Buffer : public Resource
{
public:
	RHI_API Buffer() = default;
	RHI_API ~Buffer() = default;

	RHI_API Buffer(Buffer&& rhs) noexcept;
	RHI_API Buffer& operator=(Buffer&& rhs) noexcept;

	RHI_API auto info() const -> BufferInfo const&;
	RHI_API auto make_view(MakeBufferViewInfo const& info = {}) -> BufferView;
	RHI_API auto data() const -> void*;
private:
	friend struct APIContext;
	friend class CommandBuffer;
	friend class BufferView;

	BufferInfo m_info;
	void* m_device_local_address;
	size_t m_offset;
	DeviceQueueType m_owning_queue;

	Buffer(
		BufferInfo&& info,
		APIContext* context,
		void* bufferAddress,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_BUFFER_H
