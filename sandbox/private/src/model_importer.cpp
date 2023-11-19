#include <utility>
#include <fstream>
#include <array>
#include <bitset>
#include "cgltf.h"
#include "lib/map.h"
#include "model_importer.h"

namespace sandbox
{
static lib::map<cgltf_attribute_type, size_t> load_order = {
	{ cgltf_attribute_type_position,	0  },
	{ cgltf_attribute_type_normal,		1  },
	{ cgltf_attribute_type_tangent,		2  },
	{ cgltf_attribute_type_color,		3  },
	{ cgltf_attribute_type_texcoord,	4  }
};

struct CgltfPrimitiveInfo
{
	cgltf_attribute* attributes[5];
	cgltf_accessor* indices;
	cgltf_material* material;
	cgltf_primitive_type type;
};

struct CgltfDecodedData
{
	size_t numMeshes;
	size_t numFloatData;
	size_t numUintData;
	size_t numImages;
	size_t numImageUriChars;
	size_t numMaterials;
	size_t numMaterialsWithName;
	size_t numMaterialNameChars;
};

auto translate_topology(cgltf_primitive_type topology) -> GltfImporter::MeshInfo::Topology
{
	switch (topology)
	{
	case cgltf_primitive_type_points:
		return GltfImporter::MeshInfo::Topology::Points;
	case cgltf_primitive_type_lines:
		return GltfImporter::MeshInfo::Topology::Lines;
	case cgltf_primitive_type_line_strip:
		return GltfImporter::MeshInfo::Topology::Line_Strip;
	case cgltf_primitive_type_triangle_strip:
		return GltfImporter::MeshInfo::Topology::Triangle_Strip;
	case cgltf_primitive_type_triangle_fan:
		return GltfImporter::MeshInfo::Topology::Triangle_Fan;
	case cgltf_primitive_type_triangles:
	default:
		return GltfImporter::MeshInfo::Topology::Triangles;
	}
}

auto translate_type(cgltf_type type) -> GltfImporter::MeshInfo::VertexData::Type
{
	switch (type)
	{
	case cgltf_type_scalar:
		return GltfImporter::MeshInfo::VertexData::Type::Scalar;
	case cgltf_type_vec2:
		return GltfImporter::MeshInfo::VertexData::Type::Vec2;
	case cgltf_type_vec3:
		return GltfImporter::MeshInfo::VertexData::Type::Vec3;
	case cgltf_type_vec4:
		return GltfImporter::MeshInfo::VertexData::Type::Vec4;
	case cgltf_type_mat2:
		return GltfImporter::MeshInfo::VertexData::Type::Mat2;
	case cgltf_type_mat3:
		return GltfImporter::MeshInfo::VertexData::Type::Mat3;
	case cgltf_type_mat4:
		return GltfImporter::MeshInfo::VertexData::Type::Mat4;
	case cgltf_type_invalid:
	default:
		return GltfImporter::MeshInfo::VertexData::Type::Invalid;
	}
}

auto translate_alpha_mode(cgltf_alpha_mode mode) -> CgltfAlphaMode
{
	switch (mode)
	{
	case cgltf_alpha_mode_mask:
		return CgltfAlphaMode::Mask;
	case cgltf_alpha_mode_blend:
		return CgltfAlphaMode::Blend;
	case cgltf_alpha_mode_opaque:
	default:
		return CgltfAlphaMode::Opaque;
	}
}

auto load_gltf_node(cgltf_node* node, lib::array<std::byte>& buffer, std::span<CgltfPrimitiveInfo>& primitiveInfos, size_t& offset) -> void
{
	// cgltf_attribute_type should be used to index into this array.
	// load ordering -> positions, normals, tangents, colors, texCoords, indices.
	if (node->mesh)
	{
		cgltf_mesh const* mesh = node->mesh;

		for (size_t i = 0; i < mesh->primitives_count; ++i)
		{
			if (offset >= primitiveInfos.size())
			{
				break;
			}
			cgltf_primitive& primitive = mesh->primitives[i];
			
			CgltfPrimitiveInfo& info = primitiveInfos[offset];
			info.type = primitive.type;

			if (primitive.material) [[likely]]
			{
				info.material = primitive.material;
			}

			if (primitive.indices) [[likely]]
			{
				info.indices = primitive.indices;
			}

			for (size_t j = 0; j < primitive.attributes_count; ++j)
			{
				cgltf_attribute& attribute = primitive.attributes[j];
				info.attributes[load_order[attribute.type]] = &attribute;
			}
			++offset;
		}
	}

	for (size_t i = 0; i < node->children_count; ++i)
	{
		load_gltf_node(node->children[i], buffer, primitiveInfos, offset);
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

	cgltf_data* data = {};
	cgltf_options options = {};
	cgltf_result result = cgltf_parse(&options, json.data(), json.size() * sizeof(decltype(json)::value_type), &data);

	if (result != cgltf_result_success)
	{
		return nullptr;
	}

	std::string const p = path.generic_string();

	if (cgltf_load_buffers(&options, data, p.c_str()) != cgltf_result_success)
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
	// Get total number of float data and uint data.
	for (size_t i = 0; i < data->meshes_count; ++i)
	{
		cgltf_mesh& mesh = data->meshes[i];
		for (size_t j = 0; j < mesh.primitives_count; ++j)
		{
			cgltf_primitive& primitive = mesh.primitives[j];
			for (size_t k = 0; k < primitive.attributes_count; ++k)
			{
				cgltf_attribute& attribute = primitive.attributes[k];
				decoded.numFloatData += cgltf_accessor_unpack_floats(attribute.data, nullptr, 0);
			}
		}
		for (size_t j = 0; j < mesh.primitives_count; ++j)
		{
			cgltf_primitive& primitive = mesh.primitives[j];
			if (primitive.indices)
			{
				decoded.numUintData += primitive.indices->count;
			}
		}
	}
	// Get total images.
	decoded.numImages = data->images_count;
	// Get total characters from all uri paths.
	for (size_t i = 0; i < data->images_count; ++i)
	{
		cgltf_image& image = data->images[i];
		if (image.uri)
		{
			decoded.numImageUriChars += lib::string_length(image.uri);
		}
	}
	// Get total materials.
	decoded.numMaterials = data->materials_count;
	// Get total characters from all material names.
	for (size_t i = 0; i < data->materials_count; ++i)
	{
		if (data->materials[i].name)
		{
			decoded.numMaterialNameChars += lib::string_length(data->materials[i].name);
			++decoded.numMaterialsWithName;
		}
	}
	return decoded;
}

auto store_float_data(std::span<GltfImporter::MeshInfo> meshSpan, std::span<float32> floatSpan, std::span<CgltfPrimitiveInfo> primitiveInfoSpan) -> void
{
	// Store all positional, normal, tangential, color and texture coordinate data.
	std::array sorted = {
		cgltf_attribute_type_position,
		cgltf_attribute_type_normal,
		cgltf_attribute_type_tangent,
		cgltf_attribute_type_color,
		cgltf_attribute_type_texcoord
	};

	std::sort(
		std::begin(sorted),
		std::end(sorted),
		[](cgltf_attribute_type a, cgltf_attribute_type b) -> bool
		{
			return load_order[a] < load_order[b];
		}
	);

	for (size_t total = 0; auto attribute_type : sorted)
	{
		size_t const order = load_order[attribute_type];
		for (size_t currentMeshIndex = 0; CgltfPrimitiveInfo& info : primitiveInfoSpan)
		{
			cgltf_attribute* attribute = info.attributes[order];
			GltfImporter::MeshInfo& meshInfo = meshSpan[currentMeshIndex];

			if (attribute)
			{
				meshInfo.num_vertices = static_cast<uint32>(attribute->data->count);

				GltfImporter::MeshInfo::VertexData& meshVertex = meshInfo.vertex_data[order];

				float32* dst = &floatSpan[total];

				auto count = cgltf_accessor_unpack_floats(attribute->data, nullptr, 0);
				cgltf_accessor_unpack_floats(attribute->data, dst, count);

				meshVertex.data			= std::span{ dst, count };
				meshVertex.data_type	= translate_type(attribute->data->type);

				total += count;
			}
			++currentMeshIndex;
		}
	}
}

auto store_uint_data(std::span<GltfImporter::MeshInfo> meshSpan, std::span<uint32> uintSpan, std::span<CgltfPrimitiveInfo> primitiveInfoSpan) -> void
{
	for (size_t currentMeshIndex = 0, total = 0; CgltfPrimitiveInfo& info : primitiveInfoSpan)
	{
		if (info.indices && info.indices->count)
		{
			uint32* dst = uintSpan.data() + total;
			cgltf_accessor* index = info.indices;
			for (size_t j = 0; j < index->count; ++j)
			{
				uint32 meshIndex = 0;
				cgltf_accessor_read_uint(index, j, &meshIndex, 1);
				dst[j] = meshIndex;
			}
			meshSpan[currentMeshIndex].indices = std::span{ dst, index->count };
			total += index->count;
		}
		++currentMeshIndex;
	}
}

auto store_image_uri_paths_length_data(std::span<size_t> imageUriLengthSpan, size_t parentPathLength, cgltf_data* data) -> void
{
	for (size_t i = 0; i < data->images_count; ++i)
	{
		size_t const len = lib::string_length(data->images[i].uri);
		imageUriLengthSpan[i] = parentPathLength + len;
	}
}

auto store_image_uri_paths_and_image_data(
	std::span<size_t> imageUriLengthSpan, 
	std::span<wchar_t> imageUriSpan, 
	std::span<GltfImporter::ImageInfo> imageInfo,
	std::filesystem::path const& dirAbsolutePath,
	cgltf_data* data) -> void
{
	size_t offset = 0;
	// Store paths to all textures referenced by this model.
	for (size_t i = 0; i < data->images_count; ++i)
	{
		size_t const len = imageUriLengthSpan[i];
		auto imageURI = dirAbsolutePath / data->images[i].uri;

		lib::memcopy(&imageUriSpan[offset], imageURI.c_str(), len * sizeof(wchar_t));
		imageInfo[i].path = std::wstring_view{ &imageUriSpan[offset], len };
		imageInfo[i].size = std::filesystem::file_size(imageURI);
		offset += len;
	}
}

auto store_material_name_length_data(std::span<size_t> materialNameLengthSpan, cgltf_data* data) -> void
{
	
	for (size_t i = 0, j = 0; i < data->materials_count; ++i)
	{
		cgltf_material& material = data->materials[i];
		if (material.name)
		{
			size_t const len = lib::string_length(data->materials[i].name);
			materialNameLengthSpan[j++] = len;
		}
	}
}

auto store_material_names(std::span<size_t> materialNameLengthSpan, std::span<char> materialNameSpan, std::span<GltfImporter::MaterialInfo> materialSpan, cgltf_data* data) -> void
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
	std::span<GltfImporter::MeshInfo> meshSpan, 
	std::span<GltfImporter::ImageInfo> imageSpan,
	std::span<GltfImporter::MaterialInfo> materialSpan,
	std::span<CgltfPrimitiveInfo> primitiveInfoSpan, 
	cgltf_data* data) -> void
{
	// Hard limit the material count to 1000.
	// Hopefully the mesh we load in will not have this many materials.
	std::bitset<1000> loaded = {};

	// Store material data for each mesh.
	for (size_t i = 0; CgltfPrimitiveInfo& primitiveInfo : primitiveInfoSpan)
	{
		size_t index = i++;

		if (!primitiveInfo.material) [[unlikely]]
		{
			continue;
		}

		size_t materialIndex = static_cast<size_t>(primitiveInfo.material - data->materials);
		meshSpan[index].material = &materialSpan[materialIndex];

		if (loaded.test(materialIndex))
		{
			continue;
		}

		cgltf_material& material = *primitiveInfo.material;
		GltfImporter::MaterialInfo& info = *meshSpan[index].material;

		cgltf_texture* baseColorTexture			= material.pbr_metallic_roughness.base_color_texture.texture;
		cgltf_texture* metallicRoughnessTexture = material.pbr_metallic_roughness.metallic_roughness_texture.texture;
		cgltf_texture* normalTexture			= material.normal_texture.texture;
		cgltf_texture* occlusionTexture			= material.occlusion_texture.texture;
		cgltf_texture* emissiveTexture			= material.emissive_texture.texture;

		if (baseColorTexture)
		{
			size_t const tidx = static_cast<size_t>(baseColorTexture - data->textures);
			imageSpan[tidx].type = GltfImageType::BaseColor;
			info.imageInfos[(int32)GltfImageType::BaseColor] = &imageSpan[tidx];
		}

		if (metallicRoughnessTexture)
		{
			size_t const tidx = static_cast<size_t>(metallicRoughnessTexture - data->textures);
			imageSpan[tidx].type = GltfImageType::Metallic_Roughness;
			info.imageInfos[(int32)GltfImageType::Metallic_Roughness] = &imageSpan[tidx];
		}

		if (normalTexture)
		{
			size_t const tidx = static_cast<size_t>(normalTexture - data->textures);
			imageSpan[tidx].type = GltfImageType::Normal;
			info.imageInfos[(int32)GltfImageType::Normal] = &imageSpan[tidx];
		}

		if (occlusionTexture)
		{
			size_t const tidx = static_cast<size_t>(occlusionTexture - data->textures);
			imageSpan[tidx].type = GltfImageType::Occlusion;
			info.imageInfos[(int32)GltfImageType::Occlusion] = &imageSpan[tidx];
		}

		if (emissiveTexture)
		{
			size_t const tidx = static_cast<size_t>(emissiveTexture - data->textures);
			imageSpan[tidx].type = GltfImageType::Emissive;
			info.imageInfos[(int32)GltfImageType::Emissive] = &imageSpan[tidx];
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

auto GltfImporter::MeshInfo::get_data(CgltfVertexInput type) const -> VertexData const&
{
	auto pos = static_cast<std::underlying_type_t<CgltfVertexInput>>(type);
	return vertex_data[pos];
}

GltfImporter::GltfImporter() :
	m_path{},
	m_data{},
	m_meshes{},
	m_images{},
	m_materials{}
{}

GltfImporter::GltfImporter(std::filesystem::path path) :
	m_path{},
	m_data{},
	m_meshes{},
	m_images{},
	m_materials{}
{
	open(path);
}

GltfImporter::~GltfImporter()
{
	close();
}

GltfImporter::GltfImporter(GltfImporter&& rhs) noexcept
{
	*this = std::move(rhs);
}

GltfImporter& GltfImporter::operator=(GltfImporter&& rhs) noexcept
{
	if (this != &rhs)
	{
		m_path = std::move(rhs.m_path);
		m_data = std::move(rhs.m_data);
		m_meshes = std::move(rhs.m_meshes);
		m_images = std::move(rhs.m_images);
		m_materials = std::move(rhs.m_materials);

		new (&rhs) GltfImporter{};
	}
	return *this;
}

// Contents stored in the buffer:
// 
// `--- MeshInfo * num primitives in each mesh in each node.
// |
// |--- Position data for all primitives.
// |
// |--- Normal data for all primitives.
// |
// |--- Tangent data for all primitives.
// |
// |--- Vertex Color data for all primitives.
// |
// |--- Texture Coordinate data for all primitives.
// |
// |--- Indices for all primitives.
// |
// |--- Size of each texture URI path.
// |
// |--- Path to all images stored inside of the GLTF 2.0 file.
// |
// |--- ImageInfos * num images in a single GLTF 2.0 file.
// |
// |--- Size of each material name.
// |
// |--- Name of all materials in a single GLTF 2.0 file.
// |
// |--- bool * num materials in a single GLTF 2.0 file.
// |
// `--- MaterialInfos * num materials in a single GLTF 2.0 file.
//

auto GltfImporter::open(std::filesystem::path const& path) -> bool
{	
	if (is_open())
	{
		ASSERTION(false && "Importer currently has a file opened.");
		return false;
	}

	m_path = std::filesystem::absolute(path);

	auto absolutePath = std::filesystem::absolute(m_path).remove_filename();
	size_t const parentPathLength = lib::string_length(absolutePath.c_str());

	cgltf_data* data = create_cgltf_data(m_path);

	if (!data)
	{
		return false;
	}

	CgltfDecodedData decodedData = decode_cgltf_data(data);

	decodedData.numImageUriChars += parentPathLength * decodedData.numImages;

	size_t capacity = decodedData.numMeshes			* sizeof(MeshInfo);
	capacity += decodedData.numFloatData			* sizeof(float32);
	capacity += decodedData.numUintData				* sizeof(uint32);
	capacity += decodedData.numImages				* sizeof(size_t);
	capacity += decodedData.numImageUriChars		* sizeof(wchar_t);
	capacity += decodedData.numMaterialsWithName	* sizeof(size_t);
	capacity += decodedData.numMaterialNameChars	* sizeof(char);
	capacity += decodedData.numImages				* sizeof(ImageInfo);
	capacity += decodedData.numMaterials			* sizeof(MaterialInfo);
	capacity += decodedData.numMeshes				* sizeof(CgltfPrimitiveInfo);

	m_data.reserve(capacity);

	auto meshSpan				= store_in_buffer<MeshInfo>(decodedData.numMeshes);
	auto floatDataSpan			= store_in_buffer<float32>(decodedData.numFloatData);
	auto uintDataSpan			= store_in_buffer<uint32>(decodedData.numUintData);
	auto imageUriLengthSpan		= store_in_buffer<size_t>(decodedData.numImages);
	auto imageUriSpan			= store_in_buffer<wchar_t>(decodedData.numImageUriChars);
	auto materialNameLengthSpan = store_in_buffer<size_t>(decodedData.numMaterialsWithName);
	auto materialNameSpan		= store_in_buffer<char>(decodedData.numMaterialNameChars);
	auto imageSpan				= store_in_buffer<ImageInfo>(decodedData.numImages);
	auto materialSpan			= store_in_buffer<MaterialInfo>(decodedData.numMaterials);
	auto primitiveInfoSpan		= store_in_buffer<CgltfPrimitiveInfo>(decodedData.numMeshes);

	// Iterate through each GLTF node and cache data that needs to be processed.
	{
		size_t offset = 0;
		for (size_t i = 0; i < data->scenes_count; ++i)
		{
			for (size_t j = 0; j < data->scenes[i].nodes_count; ++j)
			{
				load_gltf_node(data->scenes[i].nodes[j], m_data, primitiveInfoSpan, offset);
			}
		}
	}
	// Translate the topology to a format our project understands.
	for (size_t i = 0; CgltfPrimitiveInfo& info : primitiveInfoSpan)
	{
		meshSpan[i].topology = translate_topology(info.type);
		++i;
	}
	// Store all positional, normal, tangential, color and texture coordinate data.
	store_float_data(meshSpan, floatDataSpan, primitiveInfoSpan);

	// Store index data.
	store_uint_data(meshSpan, uintDataSpan, primitiveInfoSpan);

	// Store image uri path lengths.
	store_image_uri_paths_length_data(imageUriLengthSpan, parentPathLength, data);

	// Store image uri paths.
	store_image_uri_paths_and_image_data(imageUriLengthSpan, imageUriSpan, imageSpan, absolutePath, data);

	// Store material name length.
	store_material_name_length_data(materialNameLengthSpan, data);

	// Store material names.
	store_material_names(materialNameLengthSpan, materialNameSpan, materialSpan, data);

	// Store material data.
	store_material_data(meshSpan, imageSpan, materialSpan, primitiveInfoSpan, data);

	m_meshes = meshSpan;
	m_images = imageSpan;
	m_materials = materialSpan;

	cgltf_free(data);

	return true;
}

auto GltfImporter::is_open() const -> bool
{
	return std::cmp_not_equal(m_data.size(), 0);
}

auto GltfImporter::close() -> void
{
	if (m_data.size())
	{
		m_data.release();
	}
}

auto GltfImporter::num_meshes() const -> size_t
{
	return m_meshes.size();
}

auto GltfImporter::size() const -> size_t
{
	return m_meshes.size();
}

auto GltfImporter::vertex_data_size_bytes() const -> size_t
{
	size_t total = 0;
	for (MeshInfo const& mesh : m_meshes)
	{
		for (auto it = std::begin(mesh.vertex_data); it != std::end(mesh.vertex_data); ++it)
		{
			total += it->data.size_bytes();
		}
	}
	return total;
}

auto GltfImporter::index_data_size_bytes() const -> size_t
{
	size_t total = 0;
	for (MeshInfo const& mesh : m_meshes)
	{
		total += mesh.indices.size_bytes();
	}
	return total;
}

auto GltfImporter::materials() const -> std::span<const MaterialInfo> const
{
	return std::span{ m_materials.data(), m_materials.size() };
}

auto GltfImporter::ok() const -> bool
{
	return num_meshes();
}

auto GltfImporter::begin() -> iterator
{
	return iterator{ m_meshes.data(), *this };
}

auto GltfImporter::end() -> iterator
{
	return iterator{ m_meshes.data() + m_meshes.size(), *this };
}

auto GltfImporter::begin() const -> const_iterator
{
	return const_iterator{ m_meshes.data(), *this };
}

auto GltfImporter::end() const -> const_iterator
{
	return const_iterator{ m_meshes.data() + m_meshes.size(), *this };
}

auto GltfImporter::cbegin() const -> const_iterator
{
	return const_iterator{ m_meshes.data(), *this };
}

auto GltfImporter::cend() const -> const_iterator
{
	return const_iterator{ m_meshes.data() + m_meshes.size(), *this };
}

auto GltfImporter::data() const -> MeshInfo*
{
	return m_meshes.data();
}

}