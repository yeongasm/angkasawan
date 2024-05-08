module;

#include <mutex>

#include "lib/string.h"
#include "vulkan/vk.h"

module forge;

namespace frg
{
auto Sampler::info() const -> SamplerInfo const&
{
	return m_info;
}

auto Sampler::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Sampler::info_packed() const -> uint64
{
	return m_packedInfoBits;
}

auto Sampler::from(Device& device, SamplerInfo&& info) -> Resource<Sampler>
{
	uint64 const packed = api::sampler_info_packed_uint64(info);

	if (device.m_gpuResourcePool.samplerPermutationCache.contains(packed))
	{
		auto const index = device.m_gpuResourcePool.samplerPermutationCache[packed];
		Sampler& cachedSampler = device.m_gpuResourcePool.samplers[index];

		cachedSampler.reference();

		return Resource{ index, cachedSampler };
	}

	auto&& [index, sampler] = device.m_gpuResourcePool.samplers.emplace(device);

	VkSamplerCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = api::translate_texel_filter(info.magFilter),
		.minFilter = api::translate_texel_filter(info.minFilter),
		.mipmapMode = api::translate_mipmap_mode(info.mipmapMode),
		.addressModeU = api::translate_sampler_address_mode(info.addressModeU),
		.addressModeV = api::translate_sampler_address_mode(info.addressModeV),
		.addressModeW = api::translate_sampler_address_mode(info.addressModeW),
		.mipLodBias = info.mipLodBias,
		.anisotropyEnable = (info.maxAnisotropy > 0.f) ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = info.maxAnisotropy,
		.compareEnable = (info.compareOp != CompareOp::Never) ? VK_TRUE : VK_FALSE,
		.compareOp = api::translate_compare_op(info.compareOp),
		.minLod = info.minLod,
		.maxLod = info.maxLod,
		.borderColor = api::translate_border_color(info.borderColor),
		.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
	};

	VkResult result = vkCreateSampler(device.m_apiContext.device, &createInfo, nullptr, &sampler.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.samplers.erase(index);

		return null_resource;
	}

	// Cache the sampler's state permutation for obvious reasons...
	device.m_gpuResourcePool.samplerPermutationCache[packed] = index;

	return Resource{ index, sampler };
}

auto Sampler::destroy(Sampler const& resource, id_type id) -> void
{
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.samplerPermutationCache.erase(resource.m_packedInfoBits);
	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Sampler> });
}

Sampler::Sampler(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{},
	m_packedInfoBits{}
{}

}