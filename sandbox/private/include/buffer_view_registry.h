#pragma once
#ifndef SANDBOX_BUFFER_VIEW_REGISTRY_H
#define SANDBOX_BUFFER_VIEW_REGISTRY_H

#include "resource_cache.h"

namespace sandbox
{
struct BufferViewInfo
{
	buffer_handle buffer;
	size_t offset;
	size_t size;
};

class BufferView
{
public:
	BufferView();
	~BufferView() = default;
	
	BufferView(BufferView&& rhs) noexcept;
	BufferView& operator=(BufferView&& rhs) noexcept;
	
	auto valid() const -> bool;
	auto buffer() const -> rhi::Buffer&;
	auto data() const -> void*;
	auto offset() const -> size_t;
	auto offset_from_buffer() const -> size_t;
	auto size() const -> size_t;
	auto write(void* data, size_t size) -> rhi::BufferWriteInfo;
	auto flush() -> void;
	auto owner() const -> rhi::DeviceQueueType;
	auto gpu_address() const -> uint64;
private:
	friend class BufferViewRegistry;

	lib::ref<rhi::Buffer> m_buffer;
	/**
	* \brief BufferView's offset in the main buffer.
	*/
	size_t m_buffer_offset;
	/**
	* \brief BufferView's current offset.
	*/
	size_t m_current_offset;
	size_t m_size;
	rhi::DeviceQueueType m_owning_queue;

	BufferView(BufferView const&) = delete;
	BufferView& operator=(BufferView const&) = delete;
};

class BufferViewRegistry
{
public:
	BufferViewRegistry(ResourceCache& resourceCache);
	~BufferViewRegistry() = default;
	
	auto create_buffer_view(BufferViewInfo const& info) -> std::pair<buffer_handle, lib::ref<BufferView>>;
	auto get_buffer_view(buffer_handle handle) -> lib::ref<BufferView>;
	auto get_source_handle(buffer_handle handle) -> buffer_handle;
	auto destroy_buffer_view(buffer_handle handle) -> void;

	auto resource_cache() const -> ResourceCache&;
private:
	static constexpr size_t MAX_BUFFER_VIEW_PER_BUFFER = 32;

	struct BufferViewSlot
	{
		std::array<BufferView, MAX_BUFFER_VIEW_PER_BUFFER> slots;
		uint32 count = 0;
		uint32 index = 0;
	};

	struct FreeSlot
	{
		std::array<size_t, MAX_BUFFER_VIEW_PER_BUFFER> slots;
		uint32 count = 0;
	};

	using BufferViewMap		= lib::map<uint32, BufferViewSlot>;
	using FreeSlotMap		= lib::map<uint32, FreeSlot>;

	ResourceCache& m_resource_cache;
	BufferViewMap m_registry;
	FreeSlotMap m_free_slots;

	auto validate_buffer_view_info(BufferViewInfo const& info, rhi::Buffer& buffer) -> bool;
};

}

#endif // !SANDBOX_BUFFER_VIEW_REGISTRY_H
