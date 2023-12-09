#pragma once
#ifndef RHI_IMAGE_H
#define RHI_IMAGE_H

#include "resource.h"

namespace rhi
{
class Image : public Resource
{
public:
	RHI_API Image() = default;
	RHI_API ~Image() = default;

	RHI_API Image(Image&& rhs) noexcept;
	RHI_API Image& operator=(Image&& rhs) noexcept;

	RHI_API auto info() const -> ImageInfo const&;
	/**
	* \brief Makes the image accessible in shaders.
	*/
	RHI_API auto bind() -> BindingSlot<Image> const&;
	/**
	* \brief Removes image accessibility in shaders.
	*/
	RHI_API auto unbind() -> void;
	RHI_API auto binding() const -> BindingSlot<Image> const&;
	RHI_API auto owner() const -> DeviceQueueType;
private:
	friend struct APIContext;
	friend class Swapchain;
	friend class CommandBuffer;

	ImageInfo m_info;
	DeviceQueueType m_owning_queue;
	BindingSlot<Image> m_binding;

	Image(
		ImageInfo&& info,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_IMAGE_H
