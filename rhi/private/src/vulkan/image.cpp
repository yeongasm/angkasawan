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
	m_info{ std::move(info) },
	m_owning_queue{ DeviceQueueType::None }
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
		m_owning_queue = rhs.m_owning_queue;
		Resource::operator=(std::move(rhs));
		new (&rhs) Image{};
	}
	return *this;
}

auto Image::info() const -> ImageInfo const&
{
	return m_info;
}

}