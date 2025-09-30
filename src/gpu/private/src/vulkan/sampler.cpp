#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Sampler::info() const -> SamplerInfo const&
{
	return __self().info;
}

auto Sampler::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Sampler::id() const -> resource_id_t
{
	return __self().id;
}

auto Sampler::from(Device& device, SamplerInfo&& info) -> Sampler
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);
	uint64 const packed = sampler_info_packed_uint64(info);

	for (auto&& storedSampler : vkdevice.gpuResourcePool.stores.samplers)
	{
		if (storedSampler.packedInfoBits == packed)
		{
			return Sampler{ &storedSampler, &vkdevice };
		}
	}

	VkSamplerCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = translate_texel_filter(info.magFilter),
		.minFilter = translate_texel_filter(info.minFilter),
		.mipmapMode = translate_mipmap_mode(info.mipmapMode),
		.addressModeU = translate_sampler_address_mode(info.addressModeU),
		.addressModeV = translate_sampler_address_mode(info.addressModeV),
		.addressModeW = translate_sampler_address_mode(info.addressModeW),
		.mipLodBias = info.mipLodBias,
		.anisotropyEnable = (info.maxAnisotropy > 0.f) ? VK_TRUE : VK_FALSE,
		.maxAnisotropy = info.maxAnisotropy,
		.compareEnable = (info.compareOp != CompareOp::Never) ? VK_TRUE : VK_FALSE,
		.compareOp = translate_compare_op(info.compareOp),
		.minLod = info.minLod,
		.maxLod = info.maxLod,
		.borderColor = translate_border_color(info.borderColor),
		.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE
	};

	VkSampler handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSampler(vkdevice.device, &createInfo, nullptr, &handle))

	reflect::_ResourceMeta meta{
		.id 	= vkdevice.gpuResourcePool.idCounter.sampler++ % vkdevice.config().maxSamplers,
		.type 	= reflect::type_id_v<SamplerImpl>
	};

	auto&& vksampler = *vkdevice.gpuResourcePool.stores.samplers.emplace();
	
	vksampler.handle = handle;
	vksampler.info = std::move(info);
	vksampler.packedInfoBits = packed;
	vksampler.id = std::bit_cast<uint64>(meta);

	vkdevice.bind(vksampler, meta.id);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vksampler);
	}

	return Sampler{ &vksampler, &vkdevice };
}

auto Sampler::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	auto&& device = static_cast<DeviceImpl&>(dvc);
	auto&& sampler = static_cast<SamplerImpl&>(resource);
	
	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&sampler](DeviceImpl& device) -> void
		{
			vkDestroySampler(device.device, sampler.handle, nullptr);
			
			auto it = device.gpuResourcePool.stores.samplers.get_iterator(&sampler);
			device.gpuResourcePool.stores.samplers.erase(it);
		}
	);
}
}