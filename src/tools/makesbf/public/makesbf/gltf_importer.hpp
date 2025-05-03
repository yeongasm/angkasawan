#pragma once
#ifndef MESHIFY_MODEL_IMPORTER_HPP
#define MESHIFY_MODEL_IMPORTER_HPP

#include <filesystem>
#include "render/render.hpp"
#include "lib/array.hpp"
#include "lib/string.hpp"

namespace makesbf
{
namespace gltf
{
struct ImageInfo
{
	std::wstring_view uri;
	uint8 const* data;
	size_t size;
	render::material::ImageType type;
};

/**
* \brief GLTF 2.0 stores metallic and roughness in the same texture.
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
	render::material::AlphaMode alphaMode = {};
	float32 alphaCutoff = 0.f;
	bool unlit = true;
};

enum class VertexAttribute
{
	Position,
	Normal,
	Tangent,
	Color,
	TexCoord,
	Max
};

enum class DataType
{
	Invalid,
	Scalar,
	Vec2,
	Vec3,
	Vec4,
	Mat2,
	Mat3,
	Mat4
};

struct AttributeInfo
{
	DataType type = DataType::Invalid;
	size_t numVertices = 0;
	size_t componentCountForType = 0;
	size_t totalSizeBytes = 0;
};

struct Attribute
{
	AttributeInfo info;
	struct cgltf_accessor* accessor;
};

struct MeshInfo
{
	Attribute attributes[5];
	uint32 numVertices;
	struct cgltf_accessor* indices;
	MaterialInfo* material;
	render::Topology topology;
};

class Mesh
{
public:
	auto num_vertices() const -> uint32;
	auto num_indices() const -> uint32;
	auto vertices_size_bytes() const -> size_t;
	auto indices_size_bytes(bool alwaysUint32 = true) const -> size_t;
	auto topology() const -> render::Topology;
	auto attribute_info(VertexAttribute attribute) const -> AttributeInfo;
	auto has_attribute(VertexAttribute attribute) const -> bool;
	auto read_uint_data(size_t index) const -> uint32;
	auto read_float_data(VertexAttribute attribute, size_t index, float32* out, size_t elementSize) const -> void;
	auto unpack_vertex_data(VertexAttribute attribute, std::span<float32> floatData) const -> void;

	template <std::integral T>
	auto unpack_index_data(std::span<T> data) const -> void
	{
		unpack_index_data_internal(data.data(), data.size(), sizeof(T));
	}

	auto material_info() const -> MaterialInfo const&;

private:
	friend class Importer;

	MeshInfo& m_data;

	Mesh(MeshInfo& meshInfo);

	auto unpack_index_data_internal(void* data, size_t count, size_t size) const -> void;
};

class Importer
{
public:
	Importer();
	Importer(std::filesystem::path const& path);

	~Importer();

	Importer(Importer&&) noexcept;
	Importer& operator=(Importer&&) noexcept;

	auto open(std::filesystem::path const& path) -> bool;
	auto decode() -> void;
	auto path() const -> std::string;
	auto close() -> void;
	auto num_meshes() const -> uint32;
	auto mesh_at(uint32 index) const -> std::optional<Mesh>;
	auto ok() const -> bool;
private:
	friend class GeometryCache;

	std::filesystem::path m_path;
	struct cgltf_data* m_cgltf_ptr;
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

		auto it = m_data.end();
		std::byte* ptr = it.data();
		auto data = reinterpret_cast<T*>(ptr);
		m_data.insert(m_data.end(), sizeof(T) * numElements, std::byte{});
		for (size_t i = 0; i < numElements; ++i)
		{
			new (data + i) T{};
		}
		return std::span{ data, numElements };
	}
};
}
}

#endif // !MESHIFY_MODEL_IMPORTER_HPP
