#include <utility>
#include <fstream>
#include <array>
#include <bitset>
#include "cgltf.h"
#include "lib/map.hpp"
#include "gltf_importer.hpp"

namespace makesbf
{
namespace gltf
{
static lib::map<cgltf_attribute_type, size_t> load_order = {
	{ cgltf_attribute_type_position,	0  },
	{ cgltf_attribute_type_normal,		1  },
	{ cgltf_attribute_type_tangent,		2  },
	{ cgltf_attribute_type_color,		3  },
	{ cgltf_attribute_type_texcoord,	4  }
};

struct CgltfDecodedData
{
	size_t numMeshes;
	size_t numImages;
	size_t numImageUriChars;
	size_t numMaterials;
	size_t numMaterialsWithName;
	size_t numMaterialNameChars;
};

auto translate_topology(cgltf_primitive_type topology) -> Topology
{
	switch (topology)
	{
	case cgltf_primitive_type_points:
		return Topology::Points;
	case cgltf_primitive_type_lines:
		return Topology::Lines;
	case cgltf_primitive_type_line_strip:
		return Topology::Line_Strip;
	case cgltf_primitive_type_triangle_strip:
		return Topology::Triangle_Strip;
	case cgltf_primitive_type_triangle_fan:
		return Topology::Triangle_Fan;
	case cgltf_primitive_type_triangles:
	default:
		return Topology::Triangles;
	}
}

auto translate_type(cgltf_type type) -> DataType
{
	switch (type)
	{
	case cgltf_type_scalar:
		return DataType::Scalar;
	case cgltf_type_vec2:
		return DataType::Vec2;
	case cgltf_type_vec3:
		return DataType::Vec3;
	case cgltf_type_vec4:
		return DataType::Vec4;
	case cgltf_type_mat2:
		return DataType::Mat2;
	case cgltf_type_mat3:
		return DataType::Mat3;
	case cgltf_type_mat4:
		return DataType::Mat4;
	case cgltf_type_invalid:
	default:
		return DataType::Invalid;
	}
}

auto translate_alpha_mode(cgltf_alpha_mode mode) -> AlphaMode
{
	switch (mode)
	{
	case cgltf_alpha_mode_mask:
		return AlphaMode::Mask;
	case cgltf_alpha_mode_blend:
		return AlphaMode::Blend;
	case cgltf_alpha_mode_opaque:
	default:
		return AlphaMode::Opaque;
	}
}

auto load_gltf_node(
	cgltf_node* node,
	std::span<MeshInfo>& meshInfos,
	std::span<MaterialInfo>& materialInfos,
	std::span<cgltf_primitive*>& primitiveSpan,
	size_t meshOffset = 0,
	size_t materialOffset = 0
) -> void
{
	// cgltf_attribute_type should be used to index into this array.
	// load ordering -> positions, normals, tangents, colors, texCoords, indices.
	if (node->mesh)
	{
		cgltf_mesh const* mesh = node->mesh;

		for (size_t i = 0; i < mesh->primitives_count; ++i)
		{
			cgltf_primitive& primitive = mesh->primitives[i];

			primitiveSpan[meshOffset] = &primitive;
			MeshInfo& meshInfo = meshInfos[meshOffset++];

			meshInfo.topology = translate_topology(primitive.type);

			if (primitive.material)
			{
				MaterialInfo& materialInfo = materialInfos[materialOffset++];
				meshInfo.material = &materialInfo;
			}
		}
	}

	for (size_t i = 0; i < node->children_count; ++i)
	{
		load_gltf_node(node->children[i], meshInfos, materialInfos, primitiveSpan, meshOffset, materialOffset);
	}
}

auto create_cgltf_data(std::filesystem::path const& path) -> cgltf_data*
{
	std::ifstream stream{ path, std::ios::in | std::ios::binary };
	if (!stream.good())
	{
		return nullptr;
	}

	std::ostringstream buf = {};
	buf << stream.rdbuf();
	auto json = buf.str();

	cgltf_data* data = nullptr;
	cgltf_options options = {};

	cgltf_result result = cgltf_parse(&options, json.data(), json.size() * sizeof(decltype(json)::value_type), &data);

	if (result != cgltf_result_success)
	{
		return nullptr;
	}

	if (cgltf_load_buffers(&options, data, path.generic_string().c_str()) != cgltf_result_success)
	{
		return nullptr;
	}

	if (cgltf_validate(data) != cgltf_result_success)
	{
		return nullptr;
	}

	return data;
}

auto decode_cgltf_data(cgltf_data* data) -> CgltfDecodedData
{
	CgltfDecodedData decoded{};
	// Get total primitives.
	for (size_t i = 0; i < data->meshes_count; ++i)
	{
		cgltf_mesh& mesh = data->meshes[i];
		decoded.numMeshes += mesh.primitives_count;
	}
	// Get total images.
	decoded.numImages = data->images_count;
	// Get total characters from all uri paths.
	for (size_t i = 0; i < data->images_count; ++i)
	{
		cgltf_image& image = data->images[i];
		if (image.uri)
		{
			decoded.numImageUriChars += lib::strlen(image.uri);
		}
	}
	// Get total materials.
	decoded.numMaterials = data->materials_count;
	// Get total characters from all material names.
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		if (data->materials[i].name)
		{
			decoded.numMaterialNameChars += lib::strlen(data->materials[i].name);
			++decoded.numMaterialsWithName;
		}
	}
	return decoded;
}

auto map_vertex_accessor(std::span<MeshInfo>& meshSpan, std::span<cgltf_primitive*>& primitiveSpan) -> void
{
	for (size_t i = 0; cgltf_primitive* primitive : primitiveSpan)
	{
		MeshInfo& meshInfo = meshSpan[i];

		for (size_t j = 0; j < primitive->attributes_count && j < load_order.size(); ++j)
		{
			cgltf_attribute& attribute = primitive->attributes[j];
			cgltf_accessor* accessor = attribute.data;

			if (!meshInfo.numVertices)
			{
				meshInfo.numVertices = static_cast<uint32>(attribute.data->count);
			}

			if (!accessor)
			{
				continue;
			}

			Attribute& attrib = meshInfo.attributes[load_order[attribute.type]];
			AttributeInfo& attribInfo = attrib.info;

			attribInfo.type = translate_type(accessor->type);
			attribInfo.numVertices = accessor->count;
			attribInfo.componentCountForType = cgltf_num_components(accessor->type);
			attribInfo.totalSizeBytes = attribInfo.numVertices * attribInfo.componentCountForType * sizeof(float32);

			attrib.accessor = accessor;
		}

		++i;
	}
}

auto map_index_accessor(std::span<MeshInfo>& meshSpan, std::span<cgltf_primitive*>& primitiveSpan) -> void
{
	for (size_t i = 0; cgltf_primitive* primitive : primitiveSpan)
	{
		MeshInfo& meshInfo = meshSpan[i];

		cgltf_accessor* indices = primitive->indices;

		if (indices)
		{
			meshInfo.indices = indices;
		}

		++i;
	}
}

auto store_image_uri_paths_length_data(
	std::span<size_t> imageUriLengthSpan,
	size_t parentPathLength,
	cgltf_data* data
) -> void
{
	for (size_t i = 0; i < data->images_count; ++i)
	{
		size_t const len = lib::strlen(data->images[i].uri);
		imageUriLengthSpan[i] = parentPathLength + len;
	}
}

auto store_image_uri_paths_and_image_data(
	std::span<size_t>& imageUriLengthSpan,
	std::span<wchar_t>& imageUriSpan,
	std::span<ImageInfo>& imageInfoSpan,
	std::filesystem::path const& dirAbsolutePath,
	cgltf_data* data) -> void
{
	size_t offset = 0;

	for (size_t i = 0; i < data->images_count; ++i)
	{
		ImageInfo& imageInfo = imageInfoSpan[i];
		cgltf_image& image = data->images[i];

		if (image.uri)
		{
			size_t const len = imageUriLengthSpan[i];
			auto imageURI = dirAbsolutePath / image.uri;

			lib::memcopy(&imageUriSpan[offset], imageURI.c_str(), len * sizeof(wchar_t));

			imageInfo.uri = std::wstring_view{ &imageUriSpan[offset], len };

			offset += len;
		}
		else if (image.buffer_view)
		{
			if (image.buffer_view && image.buffer_view->data)
			{
				imageInfo.data = static_cast<uint8 const*>(image.buffer_view->data);
			}
			else
			{
				imageInfo.data = static_cast<uint8 const*>(image.buffer_view->buffer->data);
				imageInfo.data += image.buffer_view->offset;
			}
			imageInfo.size = image.buffer_view->size;
		}
	}
}

auto store_material_name_length_data(
	std::span<size_t>& materialNameLengthSpan,
	cgltf_data* data
) -> void
{
	for (size_t i = 0, j = 0; i < data->materials_count; ++i)
	{
		cgltf_material& material = data->materials[i];

		if (material.name)
		{
			size_t const len = lib::strlen(data->materials[i].name);
			materialNameLengthSpan[j++] = len;
		}
	}
}

auto store_material_names(
	std::span<size_t>& materialNameLengthSpan,
	std::span<char>& materialNameSpan,
	std::span<MaterialInfo>& materialSpan,
	cgltf_data* data
) -> void
{
	for (size_t i = 0, j = 0, total = 0; i < data->materials_count; ++i)
	{
		if (data->materials[i].name)
		{
			size_t const len = materialNameLengthSpan[j++];
			lib::memcopy(&materialNameSpan[total], data->materials[i].name, len);
			materialSpan[i].name = std::string_view{ &materialNameSpan[total], len };
			total += len;
		}
	}
}

auto store_material_data(
	std::span<MeshInfo>& meshSpan,
	std::span<ImageInfo>& imageSpan,
	std::span<MaterialInfo>& materialSpan,
	cgltf_data* data,
	std::span<cgltf_primitive*>& primitiveSpan
) -> void
{
	// Hard limit the material count to 1000.
	// Hopefully the mesh we load in will not have this many materials.
	std::bitset<1000> loaded = {};

	// Store material data for each mesh.
	for (size_t i = 0; cgltf_primitive* primitive : primitiveSpan)
	{
		size_t index = i++;

		if (!primitive->material) [[unlikely]]
		{
			continue;
		}

		size_t materialIndex = static_cast<size_t>(primitive->material - data->materials);
		meshSpan[index].material = &materialSpan[materialIndex];

		if (loaded.test(materialIndex))
		{
			continue;
		}

		cgltf_material& material = *primitive->material;
		MaterialInfo& info = *meshSpan[index].material;

		cgltf_texture* baseColorTexture = material.pbr_metallic_roughness.base_color_texture.texture;
		cgltf_texture* metallicRoughnessTexture = material.pbr_metallic_roughness.metallic_roughness_texture.texture;
		cgltf_texture* normalTexture = material.normal_texture.texture;
		cgltf_texture* occlusionTexture = material.occlusion_texture.texture;
		cgltf_texture* emissiveTexture = material.emissive_texture.texture;

		if (baseColorTexture)
		{
			size_t const tidx = static_cast<size_t>(baseColorTexture - data->textures);
			imageSpan[tidx].type = ImageType::Base_Color;
			info.imageInfos[(int32)ImageType::Base_Color] = &imageSpan[tidx];
		}

		if (metallicRoughnessTexture)
		{
			size_t const tidx = static_cast<size_t>(metallicRoughnessTexture - data->textures);
			imageSpan[tidx].type = ImageType::Metallic_Roughness;
			info.imageInfos[(int32)ImageType::Metallic_Roughness] = &imageSpan[tidx];
		}

		if (normalTexture)
		{
			size_t const tidx = static_cast<size_t>(normalTexture - data->textures);
			imageSpan[tidx].type = ImageType::Normal;
			info.imageInfos[(int32)ImageType::Normal] = &imageSpan[tidx];
		}

		if (occlusionTexture)
		{
			size_t const tidx = static_cast<size_t>(occlusionTexture - data->textures);
			imageSpan[tidx].type = ImageType::Occlusion;
			info.imageInfos[(int32)ImageType::Occlusion] = &imageSpan[tidx];
		}

		if (emissiveTexture)
		{
			size_t const tidx = static_cast<size_t>(emissiveTexture - data->textures);
			imageSpan[tidx].type = ImageType::Emissive;
			info.imageInfos[(int32)ImageType::Emissive] = &imageSpan[tidx];
		}

		// Store factors.

		// Base color factor.
		lib::memcopy(info.baseColorFactor, material.pbr_metallic_roughness.base_color_factor, sizeof(float32) * 4);
		// Metallic factor.
		info.metallicFactor = material.pbr_metallic_roughness.metallic_factor;
		// Roughness factor.
		info.roughnessFactor = material.pbr_metallic_roughness.roughness_factor;
		// Emissive factors and emissive strength.
		lib::memcopy(info.emmissiveFactor, material.emissive_factor, sizeof(float32) * 3);
		info.emmissiveStrength = material.emissive_strength.emissive_strength;
		// Alpha mode & cutoff.
		info.alphaMode = translate_alpha_mode(material.alpha_mode);
		info.alphaCutoff = material.alpha_cutoff;
		// Lit/unlit.
		info.unlit = material.unlit;

		loaded.set(materialIndex);
	}
}

Mesh::Mesh(MeshInfo& meshInfo) :
	m_data{ meshInfo }
{}

auto Mesh::num_vertices() const -> uint32
{
	return m_data.numVertices;
}

auto Mesh::num_indices() const -> uint32
{

	return m_data.indices != nullptr ? static_cast<uint32>(m_data.indices->count) : 0;
}

auto Mesh::vertices_size_bytes() const -> size_t
{
	size_t total = 0;
	for (size_t i = 0; i < std::size(m_data.attributes); ++i)
	{
		total += m_data.attributes[i].info.totalSizeBytes;
	}
	return total;
}

auto Mesh::indices_size_bytes(bool alwaysUint32) const -> size_t
{
	if (!m_data.indices)
	{
		return 0;
	}

	size_t byteSize = sizeof(uint32);

	if (!alwaysUint32)
	{
		switch (m_data.indices->component_type)
		{
		case cgltf_component_type_r_8:
		case cgltf_component_type_r_8u:
			byteSize = sizeof(uint8);
			break;
		case cgltf_component_type_r_16:
		case cgltf_component_type_r_16u:
			byteSize = sizeof(uint16);
			break;
		case cgltf_component_type_r_32u:
			byteSize = sizeof(uint32);
			break;
		default:
			break;
		}
	}

	return m_data.indices->count * byteSize;
}

auto Mesh::topology() const -> Topology
{
	return m_data.topology;
}

auto Mesh::attribute_info(VertexAttribute attribute) const -> AttributeInfo
{
	auto const i = std::to_underlying(attribute);
	return m_data.attributes[i].info;
}

auto Mesh::has_attribute(VertexAttribute attribute) const -> bool
{
	auto const i = std::to_underlying(attribute);
	return m_data.attributes[i].accessor != nullptr;
}

auto Mesh::read_float_data(VertexAttribute attribute, size_t index, float32* out, size_t elementSize) const -> void
{
	// NOTE:
	// Pretend EXT_meshopt_compression doesn't exist at the moment.
	//

	auto const i = std::to_underlying(attribute);
	Attribute& attrib = m_data.attributes[i];

	if (!(attrib.accessor && attrib.accessor->buffer_view))
	{
		return;
	};

	cgltf_accessor_read_float(attrib.accessor, index, out, elementSize);
}

auto Mesh::read_uint_data(size_t index) const -> uint32
{
	uint32 out = 0;
	cgltf_accessor_read_uint(m_data.indices, index, &out, 1);
	return out;
}

auto Mesh::unpack_vertex_data(VertexAttribute attribute, std::span<float32> floatData) const -> void
{
	// NOTE:
	// Pretend EXT_meshopt_compression doesn't exist at the moment.
	//

	auto const i = std::to_underlying(attribute);
	Attribute& attrib = m_data.attributes[i - 1];

	if (!(attrib.accessor && attrib.accessor->buffer_view))
	{
		return;
	};

	cgltf_accessor_unpack_floats(attrib.accessor, floatData.data(), floatData.size());
}

auto Mesh::unpack_index_data_internal(void* data, size_t count, size_t size) const -> void
{
	if (!m_data.indices || size > sizeof(cgltf_uint))
	{
		return;
	}

	uint8* ptr = static_cast<uint8*>(data);

	for (size_t i = 0; i < m_data.indices->count && i < count; ++i)
	{
		cgltf_uint index = {};
		cgltf_accessor_read_uint(m_data.indices, i, &index, 1);
		
		lib::memcopy(ptr, &index, size);
		ptr += size;
	}
}

auto Mesh::material_info() const -> MaterialInfo const&
{
	return *m_data.material;
}

Importer::Importer() :
	m_path{},
	m_cgltf_ptr{},
	m_data{},
	m_meshes{},
	m_images{},
	m_materials{}
{}

Importer::Importer(std::filesystem::path const& path) :
	m_path{},
	m_cgltf_ptr{},
	m_data{},
	m_meshes{},
	m_images{},
	m_materials{}
{
	open(path);
}

Importer::~Importer()
{
	close();
}

Importer::Importer(Importer&& rhs) noexcept
{
	*this = std::move(rhs);
}

Importer& Importer::operator=(Importer&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_path = std::move(rhs.m_path);
		m_data = std::move(rhs.m_data);
		m_meshes = std::move(rhs.m_meshes);
		m_images = std::move(rhs.m_images);
		m_materials = std::move(rhs.m_materials);

		new (&rhs) Importer{};
	}
	return *this;
}

auto Importer::open(std::filesystem::path const& path) -> bool
{	
	if (ok())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	m_path = std::filesystem::absolute(path);

	auto absolutePath = std::filesystem::absolute(m_path).remove_filename();

	m_cgltf_ptr = create_cgltf_data(m_path);

	return m_cgltf_ptr != nullptr;
}

auto Importer::decode() -> void
{
	auto absolutePath = std::filesystem::absolute(m_path).remove_filename();
	size_t const parentPathLength = lib::strlen(absolutePath.c_str());

	CgltfDecodedData decodedData = decode_cgltf_data(m_cgltf_ptr);

	if (decodedData.numImageUriChars)
	{
		decodedData.numImageUriChars += parentPathLength * decodedData.numImages;
	}

	size_t capacity = decodedData.numMeshes * sizeof(MeshInfo);
	capacity += decodedData.numImages * sizeof(size_t);
	capacity += decodedData.numImageUriChars * sizeof(wchar_t);
	capacity += decodedData.numMaterialsWithName * sizeof(size_t);
	capacity += decodedData.numMaterialNameChars * sizeof(char);
	capacity += decodedData.numImages * sizeof(ImageInfo);
	capacity += decodedData.numMaterials * sizeof(MaterialInfo);

	m_data.reserve(capacity);

	std::array<cgltf_primitive*, 1000> cgltfPrimitives;
	std::span<cgltf_primitive*> primitiveSpan{ cgltfPrimitives.data(), decodedData.numMeshes };

	auto meshSpan				= store_in_buffer<MeshInfo>(decodedData.numMeshes);
	auto imageUriLengthSpan		= store_in_buffer<size_t>(decodedData.numImages);
	auto imageUriSpan			= store_in_buffer<wchar_t>(decodedData.numImageUriChars);
	auto materialNameLengthSpan = store_in_buffer<size_t>(decodedData.numMaterialsWithName);
	auto materialNameSpan		= store_in_buffer<char>(decodedData.numMaterialNameChars);
	auto imageSpan				= store_in_buffer<ImageInfo>(decodedData.numImages);
	auto materialSpan			= store_in_buffer<MaterialInfo>(decodedData.numMaterials);

	// Iterate through each GLTF node and cache data that needs to be processed.
	{
		for (size_t i = 0; i < m_cgltf_ptr->scenes_count; ++i)
		{
			for (size_t j = 0; j < m_cgltf_ptr->scenes[i].nodes_count; ++j)
			{
				load_gltf_node(m_cgltf_ptr->scenes[i].nodes[j], meshSpan, materialSpan, primitiveSpan);
			}
		}
	}

	// Store image uri path lengths.
	if (decodedData.numImageUriChars)
	{
		store_image_uri_paths_length_data(imageUriLengthSpan, parentPathLength, m_cgltf_ptr);
	}

	// Store image uri paths.
	store_image_uri_paths_and_image_data(imageUriLengthSpan, imageUriSpan, imageSpan, absolutePath, m_cgltf_ptr);

	// Store material name length.
	store_material_name_length_data(materialNameLengthSpan, m_cgltf_ptr);

	// Store material names.
	store_material_names(materialNameLengthSpan, materialNameSpan, materialSpan, m_cgltf_ptr);

	// Store material data.
	store_material_data(meshSpan, imageSpan, materialSpan, m_cgltf_ptr, primitiveSpan);

	// Store all positional, normal, tangential, color and texture coordinate data.
	map_vertex_accessor(meshSpan, primitiveSpan);

	// Store index data.
	map_index_accessor(meshSpan, primitiveSpan);

	m_meshes = meshSpan;
	m_images = imageSpan;
	m_materials = materialSpan;
}

auto Importer::path() const -> std::string
{
	if (m_path.empty())
	{
		return std::string{};
	}

	return m_path.string();
}

auto Importer::close() -> void
{
	if (m_data.size())
	{
		m_data.release();
	}

	if (m_cgltf_ptr)
	{
		cgltf_free(m_cgltf_ptr);
	}
}

auto Importer::num_meshes() const -> uint32
{
	return static_cast<uint32>(m_meshes.size());
}

auto Importer::mesh_at(uint32 index) const -> std::optional<Mesh>
{
	if (index >= m_meshes.size())
	{
		return std::nullopt;
	}
	return std::make_optional(Mesh{ m_meshes[index] });
}

auto Importer::ok() const -> bool
{
	return m_cgltf_ptr != nullptr && std::cmp_not_equal(num_meshes(), 0);
}
}
}