#pragma once
#ifndef SANDBOX_MODEL_IMPORTER_H
#define SANDBOX_MODEL_IMPORTER_H

#include <filesystem>
#include "lib/array.h"
#include "lib/string.h"

namespace sandbox
{
enum class GltfImageType
{
	BaseColor,
	Metallic_Roughness,
	Normal,
	Occlusion,
	Emissive
};

enum class CgltfAlphaMode
{
	Opaque,
	Mask,
	Blend
};

class GltfImporter
{
public:
	struct ImageInfo
	{
		std::string_view path;
		GltfImageType type;
		size_t size;
	};

	/**
	* \brief GTLF 2.0 stores metallic and roughness in the same texture.
	* \brief - Roughness is in the green channel. 
	* \brief - Metalness is in the blue channel.
	*/
	struct MaterialInfo
	{
		std::string_view name;
		ImageInfo* imageInfos[5] = {};
		float32 baseColorFactor[4] = { 1.f, 1.f, 1.f, 1.f };
		float32 metallicFactor = 0.f;
		float32 roughnessFactor = 0.f;
		float32 emmissiveFactor[3] = {};
		float32 emmissiveStrength = 0.f;
		CgltfAlphaMode alphaMode = {};
		float32 alphaCutoff = 0.f;
		bool unlit = true;
	};

	struct MeshInfo
	{
		enum class Topology
		{
			Points,
			Lines,
			Line_Strip,
			Triangles,
			Triangle_Strip,
			Triangle_Fan
		};
		std::span<float32> data[5] = {};
		std::span<uint32> indices;
		MaterialInfo* material;
		Topology topology;
	};

	using value_type		= MeshInfo;
	using difference_type	= size_t;
	using pointer			= MeshInfo*;
	using const_pointer		= MeshInfo const*;

	using iterator			= lib::array_iterator<GltfImporter>;
	using const_iterator	= lib::array_const_iterator<GltfImporter>;

	GltfImporter();
	GltfImporter(std::filesystem::path path);

	~GltfImporter();

	GltfImporter(GltfImporter&&) noexcept;
	GltfImporter& operator=(GltfImporter&&) noexcept;

	auto open(std::filesystem::path path) -> bool;
	auto is_open() const -> bool;
	auto close() -> void;
	auto num_meshes() const -> size_t;
	auto vertex_data_size_bytes() const -> size_t;
	auto index_data_size_bytes() const -> size_t;
	auto materials() const -> std::span<const MaterialInfo> const;
	auto ok() const -> bool;
	auto begin() -> iterator;
	auto end() -> iterator;
	auto begin() const -> const_iterator;
	auto end() const -> const_iterator;
	auto cbegin() const -> const_iterator;
	auto cend() const -> const_iterator;
private:
	std::filesystem::path m_path;
	lib::array<std::byte> m_data;
	std::span<MeshInfo> m_meshes;
	std::span<ImageInfo> m_images;
	std::span<MaterialInfo> m_materials;

	template <typename T>
	auto store_in_buffer(size_t numElements) -> std::span<T>
	{
		if (!numElements)
		{
			return {};
		}

		auto data = reinterpret_cast<T*>(&(*m_data.end()));
		m_data.insert(m_data.end(), sizeof(T) * numElements, std::byte{});
		for (size_t i = 0; i < numElements; ++i)
		{
			new (data + i) T{};
		}
		return std::span{ data, numElements };
	}
};
}

#endif // !SANDBOX_MODEL_IMPORTER_H
