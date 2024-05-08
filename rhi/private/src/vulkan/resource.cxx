module;

#include <atomic>

module forge;

namespace frg
{
auto RefCountedResource::reference() -> void
{
	m_refCount.fetch_add(1, std::memory_order_relaxed);
}

auto RefCountedResource::dereference() -> void
{
	m_refCount.fetch_sub(1, std::memory_order_relaxed);
}

auto RefCountedResource::ref_count() const -> uint64
{
	return m_refCount.load(std::memory_order_acquire);
}

RefCountedDeviceResource::RefCountedDeviceResource(Device& device) :
	RefCountedResource{},
	m_device{ &device }
{}

auto RefCountedDeviceResource::device() const -> Device&
{
	return *m_device;
}

}