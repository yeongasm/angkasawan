#include <fstream>
#include "image_importer.h"
#include "stb_image.h"

namespace sandbox
{
ImageImporter::ImageImporter(std::filesystem::path path) :
	m_path{ path },
	m_data{},
	m_format{},
	m_width{},
	m_height{},
	m_channels{}
{
	open(path);
}

ImageImporter::~ImageImporter()
{
	close(true);
}

auto ImageImporter::open(std::filesystem::path const& path) -> bool
{
	if (is_open())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	m_path = std::filesystem::absolute(path);

	auto narrowPath = m_path.string();

	int32 width = 0;
	int32 height = 0;
	int32 channels = 0;

	uint8* data = stbi_load(narrowPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!data)
	{ 
		return false; 
	}

	size_t const imageSize = static_cast<size_t>(width * height * channels);
	m_data.resize(imageSize);

	lib::memcopy(m_data.data(), data, imageSize);

	m_width		= static_cast<uint32>(width);
	m_height	= static_cast<uint32>(height);
	m_channels	= static_cast<uint32>(channels);

	switch (channels)
	{
	case 1:
		m_format = rhi::Format::R8_Srgb;
		break;
	case 2:
		m_format = rhi::Format::R8G8_Srgb;
		break;
	case 3:
		m_format = rhi::Format::R8G8B8_Srgb;
		break;
	case 4:
		m_format = rhi::Format::R8G8B8A8_Srgb;
		break;
	default:
		break;
	}

	stbi_image_free(data);

	return true;
}

auto ImageImporter::is_open() const -> bool
{
	return std::cmp_not_equal(m_data.size(), 0);
}

auto ImageImporter::close(bool release) -> void
{
	if (release)
	{
		m_data.release();
	}
	else
	{
		m_data.clear();
	}
	m_path.~path();
}

auto ImageImporter::size_bytes() const -> size_t
{
	return m_data.size_bytes();
}

auto ImageImporter::width() const -> uint32
{
	return m_width;
}

auto ImageImporter::height() const -> uint32
{
	return m_height;
}

auto ImageImporter::channels() const -> uint32
{
	return m_channels;
}

auto ImageImporter::format() const -> rhi::Format
{
	return m_format;
}
}