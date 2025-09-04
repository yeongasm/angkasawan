#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Sampler::info() const -> SamplerInfo const&
{
	return m_info;
}

auto Sampler::valid() const -> bool
{
	auto&& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Sampler::from(Device& device, SamplerInfo&& info) -> handle_type
{
	auto&& vkdevice = to_device(device);
	uint64 const packed = vk::sampler_info_packed_uint64(info);

	for (auto&& [id, it] : vkdevice.gpuResourcePool.caches.sampler)
	{
		if (it->m_packedInfoBits == packed)
		{
			return handle_type{ vkdevice, id };
		}
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

	auto it = vkdevice.gpuResourcePool.stores.samplers.emplace();

	auto&& vksampler = *it;

	vksampler.handle = handle;
	vksampler.m_packedInfoBits = packed;
	vksampler.m_info = std::move(info);

	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::SamplerImpl>,
		.id 	= vkdevice.gpuResourcePool.idCounter.sampler++ % vkdevice.config().maxSamplers
	};

	auto id = std::bit_cast<uint64>(meta);

	// Cache the sampler's state permutation for obvious reasons...
	vkdevice.gpuResourcePool.caches.sampler.emplace(id, it);
	vkdevice.bind(vksampler, meta.id);
	vkdevice.begin_referencing(id);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksampler);
	}

	return handle_type{ vkdevice, id };
}

auto Sampler::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Sampler with an invalid id");
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(device);
	auto&& vksampler = *vkdevice.gpuResourcePool.caches.sampler[id];

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vksampler, id](vk::DeviceImpl& device) -> void
		{
			vkDestroySampler(device.device, vksampler.handle, nullptr);
			
			auto it = device.gpuResourcePool.caches.sampler[id];
			
			device.gpuResourcePool.caches.sampler.erase(id);
			device.gpuResourcePool.stores.samplers.erase(it);
		}
	);
}
}