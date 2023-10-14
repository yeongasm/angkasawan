#include "rhi.h"



namespace rhi
{

extern std::pair<bool, ShaderInfo> compile_to_shader_binaries(ShaderCompileInfo const& compileInfo, lib::string* error);

class _Instance_
{
private:
	friend RHI_API _Instance_*	create_instance	();
	friend RHI_API void			destroy_instance();
	friend RHI_API Device&		create_device	(Instance, DeviceInitInfo const&);
	friend RHI_API void			destroy_device	(Device&);

	lib::map<lib::hash_string_view, Device> m_devices = {};
} *_instance = nullptr;

auto create_instance() -> Instance
{
	if (!_instance)
	{
		_instance = static_cast<Instance>(lib::allocate_memory({ .size = sizeof(_Instance_) }));
		new (_instance) _Instance_{};
	}
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

auto create_device(Instance instance, DeviceInitInfo const& info) -> Device&
{
	ASSERTION(instance && "Fatal Error! Invalid instance provided.");
	_Instance_& inst = *instance;
	auto& pair = inst.m_devices.emplace(info.name, Device{});
	if (!pair.second.initialize(info))
	{
		ASSERTION(false && "Fatal Error! Failed to initialize device.");
	}
	return pair.second;
}

auto destroy_device(Device& device) -> void
{
	_Instance_& instance = *_instance;
	auto pair = instance.m_devices.at(device.m_info.name);
	device.terminate();
	instance.m_devices.erase(pair->first);
}

auto compile_shader(ShaderCompileInfo const& info, lib::string* error) -> std::pair<bool, ShaderInfo>
{
	return compile_to_shader_binaries(info, error);
}

}