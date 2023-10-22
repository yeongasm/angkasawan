#include "rhi.h"

namespace rhi
{

_Instance_* _instance = nullptr;

RHI_API auto _Instance_::create_device(DeviceInitInfo const& info) -> Device&
{
	auto& pair = m_devices.emplace(info.name, Device{});
	if (!pair.second.initialize(info))
	{
		ASSERTION(false && "Fatal Error! Failed to initialize device.");
	}
	return pair.second;
}

RHI_API auto _Instance_::destroy_device(Device& device) -> void
{
	auto pair = m_devices.at(device.m_info.name);
	device.terminate();
	m_devices.erase(pair->first);
}

auto create_instance() -> Instance
{
	if (!_instance)
	{
		_instance = static_cast<_Instance_*>(lib::allocate_memory({ .size = sizeof(_Instance_) }));
		new (_instance) _Instance_;
	}
	ASSERTION(_instance && "Fatal Error! Failed to create an RHI instance.");
	return _instance;
}

auto destroy_instance() -> void
{
	_Instance_& instance = *_instance;
	if (instance.m_devices.size())
	{
		ASSERTION(instance.m_devices.size() == 0 && "Fatal Error! Not all devices are freed.");
		// TODO(afiq):
		// Once an exception handler / assert system is implemented, use that to report the unfreed devices.
	}
	instance.m_devices.clear();
}

}