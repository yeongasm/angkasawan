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
	m_owning_queue{ DeviceQueueType::None },
	m_binding{}
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
		m_binding = std::move(rhs.m_binding);
		Resource::operator=(std::move(rhs));
		new (&rhs) Image{};
	}
	return *this;
}

auto Image::info() const -> ImageInfo const&
{
	return m_info;
}

auto Image::bind() -> BindingSlot<Image> const&
{
	if (m_binding.slot == BindingSlot<Image>::INVALID)
	{
		uint32 index = std::numeric_limits<uint32>::max();

		if (m_context->descriptorCache.imageFreeSlots.size())
		{
			index = m_context->descriptorCache.imageFreeSlots.back();
			m_context->descriptorCache.imageFreeSlots.pop_back();
		}
		else
		{
			ASSERTION(
				(m_context->descriptorCache.imageBindingSlot + 1) < m_context->config.maxImages
				&& "Maximum bindable image count reached. Slot will rotate back to the 0th index."
			);
			index = m_context->descriptorCache.imageBindingSlot++;
			if (m_context->descriptorCache.imageBindingSlot >= m_context->config.maxImages)
			{
				m_context->descriptorCache.imageBindingSlot = 0;
			}
		}
		m_binding.slot = index;
		vulkan::Image& image = as<vulkan::Image>();
		m_context->update_descriptor_set_image(image.imageView, m_info.imageUsage, m_binding.slot);
	}
	return m_binding;
}

auto Image::unbind() -> void
{
	if (m_binding.slot != BindingSlot<Image>::INVALID)
	{
		m_context->descriptorCache.imageFreeSlots.push_back(m_binding.slot);
		m_binding.slot = BindingSlot<Image>::INVALID;
	}
}

auto Image::binding() const -> BindingSlot<Image> const&
{
	return m_binding;
}

auto Image::owner() const -> DeviceQueueType
{
	return m_owning_queue;
}
}