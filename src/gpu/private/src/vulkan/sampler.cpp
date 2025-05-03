#include "vulkan/vkgpu.hpp"

namespace gpu
{
Sampler::Sampler(Device& device) :
	DeviceResource{ device },
	m_info{},
	m_packedInfoBits{}
{}

auto Sampler::info() const -> SamplerInfo const&
{
	return m_info;
}

auto Sampler::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Sampler::info_packed() const -> uint64
{
	return m_packedInfoBits;
}

auto Sampler::bind(SamplerBindInfo const& info) const -> SamplerBindInfo
{
	auto&& self = to_impl(*this);
	auto&& vkdevice = to_device(self.device());

	uint32 index = info.index;

	index = index % vkdevice.config().maxSamplers;

	VkDescriptorImageInfo descriptorSamplerInfo{
		.sampler = self.handle,
		.imageView = VK_NULL_HANDLE,
		.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkWriteDescriptorSet writeDescriptorSetImage{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = vkdevice.descriptorCache.descriptorSet,
		.dstBinding = SAMPLER_BINDING,
		.dstArrayElement = index,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo = &descriptorSamplerInfo
	};

	vkUpdateDescriptorSets(vkdevice.device, 1, &writeDescriptorSetImage, 0, nullptr);

	return SamplerBindInfo{ .index = index };
}

auto Sampler::from(Device& device, SamplerInfo&& info) -> Resource<Sampler>
{
	auto&& vkdevice = to_device(device);
	uint64 const packed = vk::sampler_info_packed_uint64(info);

	if (vkdevice.gpuResourcePool.samplerCache.contains(packed))
	{
		auto cachedSampler = vkdevice.gpuResourcePool.samplerCache[packed];

		cachedSampler->reference();

		return Resource<Sampler>{ *cachedSampler };
	}

	VkSamplerCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = vk::translate_texel_filter(info.magFilter),
		.minFilter = vk::translate_texel_filter(info.minFilter),
		.mipmapMode = vk::translate_mipmap_mode(info.mipmapMode),
		.addressModeU = vk::translate_sampler_address_mode(info.addressModeU),
		.addressModeV = vk::translate_sampler_address_mode(info.addressModeV),
		.addressModeW = vk::translate_sampler_address_mode(info.addressModeW),
		.mipLodBias = info.mipLodBias,
		.anisotropyEnable = (info.maxAnisotropy > 0.f) ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = info.maxAnisotropy,
		.compareEnable = (info.compareOp != CompareOp::Never) ? VK_TRUE : VK_FALSE,
		.compareOp = vk::translate_compare_op(info.compareOp),
		.minLod = info.minLod,
		.maxLod = info.maxLod,
		.borderColor = vk::translate_border_color(info.borderColor),
		.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
	};

	VkSampler handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSampler(vkdevice.device, &createInfo, nullptr, &handle))

	auto it = vkdevice.gpuResourcePool.samplers.emplace(vkdevice);

	auto&& vksampler = *it;

	if (info.name.size())
	{
		info.name.format("<sampler>:{}", info.name.c_str());
	}

	vksampler.handle = handle;
	vksampler.m_packedInfoBits = packed;
	vksampler.m_info = std::move(info);

	// Cache the sampler's state permutation for obvious reasons...
	vkdevice.gpuResourcePool.samplerCache[packed] = &vksampler;

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksampler);
	}

	return Resource<Sampler>{ vksampler };
}

auto Sampler::destroy(Sampler& resource) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vksampler = to_impl(resource);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, &vksampler, vk::ResourceType::Sampler);
}

namespace vk
{
SamplerImpl::SamplerImpl(DeviceImpl& device) :
	Sampler{ device }
{}
}
}