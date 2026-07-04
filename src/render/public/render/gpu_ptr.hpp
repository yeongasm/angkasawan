#pragma once
#ifndef RENDER_BUFFER_HPP
#define RENDER_BUFFER_HPP

#include "upload_heap.hpp"

namespace render
{
namespace detail
{
template <typename T>
struct DeviceLocalDataStorage
{
    T data;
    lib::ref<UploadHeap> uploadHeap;
};

template <typename T, size_t N>
struct DeviceLocalDataStorage<T[N]>
{
    T data[N];
    lib::ref<UploadHeap> uploadHeap;
};

template <typename T>
requires (
    std::is_standard_layout_v<T>  
    && !std::is_pointer_v<T>
    && !std::is_reference_v<T>
)
struct GpuDataContainer : public lib::non_copyable
{
    gpu::Buffer storage;
    std::unique_ptr<detail::DeviceLocalDataStorage<T>> localData;

    GpuDataContainer() = default;

    GpuDataContainer(gpu::Buffer& buffer) : 
        storage{ buffer },
        localData{}
    {}

    GpuDataContainer(gpu::Buffer& buffer, std::unique_ptr<detail::DeviceLocalDataStorage<T>> data) :
        storage{ buffer },
        localData{ std::move(data) }
    {}

    explicit operator bool() const
    {
        return storage.valid();
    }

    /**
    * Returns the underlying storage resources info. 
    */
    auto buffer_info() const -> gpu::BufferInfo const&
    {
        return storage.info();
    }
};
}

struct GpuPtrInfo
{
    std::string name;
    gpu::BufferUsage bufferUsage;
	gpu::SharingMode sharingMode = gpu::SharingMode::Concurrent;
};

template <typename T>
class GpuPtr : protected detail::GpuDataContainer<T>
{
private:

    using super = detail::GpuDataContainer<T>;

    GpuPtr(gpu::Buffer& buffer) : 
        super{ buffer }
    {}

    GpuPtr(gpu::Buffer& buffer, std::unique_ptr<detail::DeviceLocalDataStorage<T>> data) :
        super{ buffer, std::move(data) }
    {}

public:

    using value_type        = T;
    using reference         = value_type&;
    using const_reference   = value_type const&;
    using pointer           = value_type*;
    using const_pointer     = value_type const*;

    GpuPtr() = default;

    /**
    * Returns the address of the object in the GPU.
    */
    auto address() const -> gpu::device_address
    {
        return super::storage.gpu_address();
    }

    template <typename Self>
    auto operator->(this Self&& self) -> auto
    {
        using enum gpu::MemoryUsage;

        auto memoryUsage = self.storage.info().memoryUsage;

        if (((memoryUsage & Host_Writable) == None) && 
            ((memoryUsage & Host_Transferable) == None))
        {
            return &std::forward<std::remove_reference_t<Self>>(self).localData->data;
        }
        return std::forward_like<std::remove_reference_t<Self>>(static_cast<pointer>(self.storage.data()));
    }

    /**
    * Commit changes made and sends it to the GPU.
    * No-op if GpuPtr points to memory that is host visible.
    */
    auto commit(gpu::DeviceQueue srcQueue = gpu::DeviceQueue::None, gpu::DeviceQueue dstQueue = gpu::DeviceQueue::Main) const -> std::optional<upload_id>
    {
        using enum gpu::MemoryUsage;

        auto memoryUsage = super::storage.info().memoryUsage;
        // If the buffer is device local only, we need to do a transfer op.
        // Ideally this should be removed once all motherboards enable ReBAR by default.
        if (((memoryUsage & Host_Writable) == None) && 
            ((memoryUsage & Host_Transferable) == None))
        {
            return super::localData->uploadHeap->upload_data_to_buffer({
                .dst        = super::storage,
                .data       = &super::localData->data,
                .size       = sizeof(value_type),
                .srcQueue   = srcQueue,
                .dstQueue   = dstQueue
            });
        }

        // If the buffer is host visible, do nothing.

        return std::nullopt;
    }

	auto resource() -> gpu::Buffer { return super::storage; }

    template <typename... Args>
    requires (std::is_constructible_v<value_type, Args...>)
    static auto from(gpu::Device& device, GpuPtrInfo&& info, Args&&... args) -> GpuPtr<value_type>
    {
        using enum gpu::MemoryUsage;

        auto buffer = gpu::Buffer::from(
            device, 
            {
                .name = std::move(info.name),
                .size = sizeof(value_type),
                .bufferUsage = info.bufferUsage,
                .memoryUsage = Best_Fit | Host_Writable | Host_Transferable,
                .sharingMode = info.sharingMode
            }
        );

        if (buffer.is_host_visible())
        {
            pointer ptr = static_cast<pointer>(buffer.data());

            std::construct_at(ptr, std::forward<Args>(args)...);
        }

        return GpuPtr<value_type>{ buffer };
    };

    template <typename... Args>
    requires (std::is_constructible_v<value_type, Args...>)
    static auto from(UploadHeap& uploadHeap, GpuPtrInfo&& info, Args&&... args) -> GpuPtr<value_type>
    {
        using enum gpu::MemoryUsage;
        using enum gpu::BufferUsage;

        auto busage = info.bufferUsage;

        if (std::cmp_equal(std::to_underlying(busage) & std::to_underlying(Transfer_Dst), 0))
        {
            busage |= Transfer_Dst;
        }

        auto buffer = gpu::Buffer::from(
            uploadHeap.device(),
            {
                .name = std::move(info.name),
                .size = sizeof(value_type),
                .bufferUsage = busage,
                .memoryUsage = Best_Fit,
                .sharingMode = info.sharingMode
            }
        );

        GpuPtr<value_type> gpuPtr{ buffer, std::make_unique<detail::DeviceLocalDataStorage<value_type>>(value_type{ std::forward<Args>(args)... }, uploadHeap) };

        if constexpr (std::cmp_not_equal(sizeof...(Args), 0))
        {
            gpuPtr.commit();
        }

        return gpuPtr;
    }
};

template <typename T, size_t N>
class GpuPtr<T[N]> : protected detail::GpuDataContainer<T[N]>
{
private:

    using type = T[N];
    using super = detail::GpuDataContainer<type>;

    GpuPtr(gpu::Buffer& storage) :
        super{ storage }
    {}

    GpuPtr(gpu::Buffer& storage, std::unique_ptr<detail::DeviceLocalDataStorage<type>> data) :
        super{ storage, std::move(data) }
    {}

public:

    using value_type        = std::remove_pointer_t<std::decay_t<type>>;
    using reference         = value_type&;
    using const_reference   = value_type const&;
    using pointer           = value_type*;
    using const_pointer     = value_type const*;

    GpuPtr() = default;

    /**
    * Returns the address of the object in the GPU.
    */
    auto address_at(size_t i) const -> gpu::device_address
    {
        ASSERTION(std::cmp_less(i, N) && "Index being accessed exceeds the amount of data the pointer is holding.");
        return super::storage.gpu_address() + (i * sizeof(value_type));
    }

    template <typename Self>
    auto operator[](this Self&& self, size_t i) -> decltype(auto)
    {
        ASSERTION(std::cmp_less(i, N) && "Index being accessed exceeds the amount of data the pointer is holding.");

        using enum gpu::MemoryUsage;

        auto memoryUsage = self.storage.info().memoryUsage;

        if (((memoryUsage & Host_Writable) == None) && 
            ((memoryUsage & Host_Transferable) == None))
        {
            return std::forward<Self>(self).localData->data[i];
        }
        return std::forward_like<Self>(*(static_cast<pointer>(self.storage.data()) + i));      
    }

    auto size() const -> size_t { return N; }

    /**
    * Commit changes made and sends it to the GPU. No-op if GpuPtr points to memory that is host visible.
    * 
    * For a GpuPtr that's holding a reference to C-style arrays, it is best to call commit() as soon as change is made to some element from an index.
    * 
    * This class can't possibly know changes made at multiple indexes and to keep the class lightweight, it was decided not to keep track via holding an array of indices.
    */
    auto commit(size_t i, gpu::DeviceQueue srcQueue = gpu::DeviceQueue::None, gpu::DeviceQueue dstQueue = gpu::DeviceQueue::Main) const -> std::optional<upload_id>
    {
        using enum gpu::MemoryUsage;

        auto memoryUsage = super::storage.info().memoryUsage;
        // If the buffer is device local only, we need to do a transfer op.
        // Ideally this should be removed once all motherboards enable ReBAR by default.
        if (((memoryUsage & Host_Writable) == None) && 
            ((memoryUsage & Host_Transferable) == None))
        {
            return super::localData->uploadHeap->upload_data_to_buffer({
                .dst        = super::storage,
                .dstOffset  = i * sizeof(value_type),
                .data       = &super::localData->data[i],
                .size       = sizeof(value_type),
                .srcQueue   = srcQueue,
                .dstQueue   = dstQueue
            });
        }

        // If the buffer is host visible, do nothing.

        return std::nullopt;
    }

	auto resource() -> gpu::Buffer { return super::storage; }

    template <typename... Args>
    requires 
    (
        (... && std::is_convertible_v<Args, T>)
        && std::is_constructible_v<type, Args...>
    )
    static auto from(gpu::Device& device, GpuPtrInfo&& info, Args&&... args) -> GpuPtr<type>
    {
        using enum gpu::MemoryUsage;

        auto buffer = gpu::Buffer::from(
            device, 
            {
                .name = std::move(info.name),
                .size = sizeof(type),
                .bufferUsage = info.bufferUsage,
                .memoryUsage = Best_Fit | Host_Writable | Host_Transferable,
                .sharingMode = info.sharingMode
            }
        );

        if (buffer.is_host_visible())
        {
            pointer ptr = static_cast<pointer>(buffer.data());

            std::construct_at(ptr, std::forward<Args>(args)...);
        }

        return GpuPtr<type>{ buffer };
    };

    template <typename... Args>
    requires 
    (
        (... && std::is_convertible_v<Args, T>)
        && std::is_constructible_v<type, Args...>
    )
    static auto from(UploadHeap& uploadHeap, GpuPtrInfo&& info, Args&&... args) -> GpuPtr<type>
    {
        using enum gpu::MemoryUsage;
        using enum gpu::BufferUsage;

        auto busage = info.bufferUsage;

        if (std::cmp_equal(std::to_underlying(busage) & std::to_underlying(Transfer_Dst), 0))
        {
            busage |= Transfer_Dst;
        }

        auto buffer = gpu::Buffer::from(
            uploadHeap.device(),
            {
                .name = std::move(info.name),
                .size = sizeof(type),
                .bufferUsage = busage,
                .memoryUsage = Best_Fit,
                .sharingMode = info.sharingMode
            }
        );

        GpuPtr<type> gpuPtr{ buffer, std::unique_ptr<detail::DeviceLocalDataStorage<type>>{ new detail::DeviceLocalDataStorage<type>{ .data = { std::forward<Args>(args)... }, .uploadHeap = &uploadHeap } } };

        if constexpr (std::cmp_not_equal(sizeof...(Args), 0))
        {
            gpuPtr.commit();
        }

        return gpuPtr;
    }
};
}

#endif // !RENDER_BUFFER_HPP