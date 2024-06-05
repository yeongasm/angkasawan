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
private:
	friend struct APIContext;
	friend struct ResourceDeleter<Image>;
	friend class Swapchain;
	friend class CommandBuffer;

	ImageInfo m_info = {};

	Image(
		ImageInfo&& info,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_IMAGE_H
