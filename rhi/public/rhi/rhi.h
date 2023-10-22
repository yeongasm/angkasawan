#pragma once
#ifndef RHI_RHI_H
#define RHI_RHI_H

#include "device.h"
#include "swapchain.h"
#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "pipeline.h"

namespace rhi
{

class _Instance_
{
public:
	_Instance_() = default;
	~_Instance_() = default;

	RHI_API auto create_device(DeviceInitInfo const& info) -> Device&;
	RHI_API auto destroy_device(Device& device) -> void;
private:
	friend RHI_API auto create_instance() -> _Instance_*;
	friend RHI_API auto destroy_instance() -> void;

	lib::map<lib::hash_string_view, Device> m_devices = {};

	_Instance_(_Instance_ const&)				= delete;
	_Instance_(_Instance_&&)					= delete;
	_Instance_& operator=(_Instance_ const&)	= delete;
	_Instance_& operator=(_Instance_&&)			= delete;
};

using Instance = _Instance_*;

RHI_API auto create_instance() -> Instance;
RHI_API auto destroy_instance() -> void;

}

#endif // !RHI_RHI_H
