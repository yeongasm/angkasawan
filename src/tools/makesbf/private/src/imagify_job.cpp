#include "core.serialization/write_stream.hpp"
#include "makesbf.hpp"
#include "imagify_job.hpp"
#include "image_importer.hpp"
#include "render/material.hpp"

namespace makesbf
{
ImagifyJob::ImagifyJob(MakeSbf& tool, ImagifyJobInfo const& info) :
	m_tool{ tool },
	m_info{ info }
{}

auto ImagifyJob::execute() -> std::optional<std::string_view>
{
	ImageImporter importer{ m_info.input };

	if (!importer.is_open())
	{
		return "Could not open image file.";
	}

	core::sbf::WriteStream stream{ m_info.output };

	stream.write(core::sbf::SbfFileHeader{});

	size_t offsetBytes = sizeof(core::sbf::SbfFileHeader);

	auto const& imageInfo = importer.image_info();
	render::SbfImageViewHeader header{};

	header.imageInfo.dimension = imageInfo.dimension;
	header.imageInfo.format = imageInfo.format;
	header.imageInfo.type = imageInfo.type;
	header.imageInfo.mipLevels = std::min(m_info.mipLevels, imageInfo.mipLevel);

	offsetBytes += sizeof(render::SbfImageViewHeader);

	for (uint32 i = 0; i < header.imageInfo.mipLevels; ++i)
	{
		auto data = importer.data(i);

		uint32 const thisSectionSizeBytes = static_cast<uint32>(sizeof(render::SbfImageMipViewHeader) + data.size_bytes());

		header.descriptor.sizeBytes += thisSectionSizeBytes;

		header.imageInfo.mipInfos[i].offset = offsetBytes;
		header.imageInfo.mipInfos[i].size	= data.size_bytes();

		offsetBytes += thisSectionSizeBytes;
	}

	stream.write(header);

	for (uint32 i = 0; i < header.imageInfo.mipLevels; ++i)
	{
		auto data = importer.data(i);

		render::SbfImageMipViewHeader mipHeader{};

		mipHeader.descriptor.sizeBytes = static_cast<uint32>(data.size_bytes());
		mipHeader.level = i;

		stream.write(mipHeader);
		stream.write(data);
	}

	return std::nullopt;
}

auto ImagifyJob::job_info() const -> ImagifyJobInfo const&
{
	return m_info;
}
}