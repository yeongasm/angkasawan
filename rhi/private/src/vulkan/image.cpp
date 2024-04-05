#include "image.h"
#include "vulkan/vk_device.h"

namespace rhi
{
Image::Image(
	ImageInfo&& info,
	APIContext* context,
	void* data,
	resource_type type_id
) :
	Resource{ context, data, type_id },
	m_info{ std::move(info) }
{
	m_context->setup_debug_name(*this);
}

Image::Image(Image&& rhs) noexcept
{
	*this = std::move(rhs);
}

Image& Image::operator=(Image&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		Resource::operator=(std::move(rhs));
		new (&rhs) Image{};
	}
	return *this;
}

auto Image::info() const -> ImageInfo const&
{
	return m_info;
}

template <>
struct ResourceDeleter<Image>
{
	auto operator()(Image& image) const -> void
	{
		APIContext* api = image.m_context;
		api->destroy_image(image);
	}
};
}