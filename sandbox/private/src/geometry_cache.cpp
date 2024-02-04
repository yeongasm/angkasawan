#include "geometry_cache.h"

namespace sandbox
{
auto translate_geometry_input_to_cgltf_importer_input_type(GeometryInput input) -> gltf::VertexAttribute
{
	switch (input)
	{
	case GeometryInput::Normal:
		return gltf::VertexAttribute::Normal;
	case GeometryInput::Tangent:
		return gltf::VertexAttribute::Tangent;
	case GeometryInput::Color:
		return gltf::VertexAttribute::Color;
	case GeometryInput::TexCoord:
		return gltf::VertexAttribute::TexCoord;
	case GeometryInput::Position:
	default:
		return gltf::VertexAttribute::Position;
	}
}

GeometryCache::GeometryCache(UploadHeap& uploadHeap) :
	m_vertices{},
	m_indices{},
	m_storage_info{},
	m_geometries{},
	m_null_geometry{},
	m_upload_heap{ uploadHeap }
{}

auto GeometryCache::store_geometries(gltf::Importer const& importer, GeometryInputLayout const& layout) -> root_geometry_handle
{
	using attrib_t = std::underlying_type_t<gltf::VertexAttribute>;

	auto const MAX_ATTRIBUTES = static_cast<attrib_t>(gltf::VertexAttribute::Max);

	uint32 const meshCount = importer.num_meshes();

	if (!meshCount)
	{
		return root_geometry_handle::invalid_handle();
	}

	root_geometry_handle handle = {};
	Geometry* rootGeometry = nullptr;
	Geometry* tailGeometry = nullptr;

	size_t verticesOffset = m_vertices.size();
	size_t indicesOffset = m_indices.size();

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			ASSERTION(false);
			continue;
		}

		auto mesh = opt.value();

		auto&& [index, geo] = m_geometries.insert(Geometry{});

		if (handle == root_geometry_handle::invalid_handle())
		{
			new (&handle) root_geometry_handle{ index.id };
		}

		if (!tailGeometry || !rootGeometry)
		{
			tailGeometry = &geo;
			rootGeometry = &geo;
		}
		else
		{
			tailGeometry->next = &geo;
			tailGeometry = &geo;
		}

		size_t floatElementCount = 0;
		GeometryInput missingInputs = GeometryInput::None;

		for (auto j = 0; j < MAX_ATTRIBUTES; ++j)
		{
			auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(j);
			auto const geoInputEnum = static_cast<GeometryInput>(1 << j);

			if ((layout.inputs & geoInputEnum) == GeometryInput::None)
			{
				tailGeometry->layoutInfo.layout.inputs ^= geoInputEnum;
				continue;
			}

			auto const attribInfo = mesh.attribute_info(gltfAttribEnum);

			if (!mesh.has_attribute(gltfAttribEnum))
			{
				missingInputs |= geoInputEnum;
			}

			if (!layout.interleaved)
			{
				InputInfo& inputInfo = tailGeometry->layoutInfo.infos[j];
				inputInfo.count = attribInfo.numVertices;
				inputInfo.size = attribInfo.totalSizeBytes;
			}

			floatElementCount += input_element_count(geoInputEnum);
		}

		uint32 const meshVerticesCount = mesh.num_vertices();
		uint32 const meshIndicesCount = mesh.num_indices();

		size_t const totalFloatElementsForAllVertices = floatElementCount * static_cast<size_t>(meshVerticesCount);

		GeometryStorageInfo& storageInfo = m_storage_info[tailGeometry];

		storageInfo.missingInputsFromImporter = missingInputs;

		storageInfo.vertices.offset = verticesOffset;
		storageInfo.vertices.count = meshVerticesCount;
		storageInfo.vertices.elementCount = totalFloatElementsForAllVertices;

		storageInfo.indices.offset = indicesOffset;
		storageInfo.indices.count = meshIndicesCount;
		storageInfo.indices.elementCount = meshIndicesCount;

		tailGeometry->vertices.count = meshVerticesCount;
		tailGeometry->vertices.size = totalFloatElementsForAllVertices * sizeof(float32);

		tailGeometry->indices.count = meshIndicesCount;
		tailGeometry->indices.size = meshIndicesCount * sizeof(uint32);

		tailGeometry->layoutInfo.layout.interleaved = layout.interleaved;

		verticesOffset	+= totalFloatElementsForAllVertices;
		indicesOffset	+= meshIndicesCount;
	}

	size_t const totalFloatElements = verticesOffset - m_vertices.size();
	size_t const totalUintElements = indicesOffset - m_indices.size();

	reserve_space_if_needed(totalFloatElements, totalUintElements);

	m_vertices.insert(m_vertices.end(), totalFloatElements, 0.f);
	m_indices.insert(m_indices.end(), totalUintElements, 0);

	if (layout.interleaved)
	{
		gltf_unpack_interleaved(importer, rootGeometry);
	}
	else
	{
		gltf_unpack_non_interleaved(importer, rootGeometry);
	}

	return handle;
}

auto GeometryCache::stage_geometries_for_upload(StageGeometryInfo&& info) -> GeometryCacheUploadResult
{
	upload_id id = upload_id{ std::numeric_limits<uint64>::max() };

	Geometry* pGeometry = m_geometries.at(info.geometry.get());
	RootGeometryInfo rootGeometryInfo = geometry_info(info.geometry);

	if (!pGeometry ||
		info.verticesWriteOffset + rootGeometryInfo.verticesSizeBytes > info.vb.size() ||
		info.indicesWriteOffset + rootGeometryInfo.indicesSizeBytes > info.ib.size())
	{
		return GeometryCacheUploadResult{ .id = id };
	}

	id = m_upload_heap.current_upload_id();

	size_t const stride = layout_size_bytes(pGeometry->layoutInfo.layout);

	size_t verticesWrittenBytes = 0;
	size_t indicesWrittenBytes = 0;

	while (pGeometry)
	{
		GeometryStorageInfo const& storageInfo = m_storage_info[pGeometry];
		std::span<float32> const verticesDataSpan{ &m_vertices[storageInfo.vertices.offset], storageInfo.vertices.elementCount };
		std::span<uint32> const indicesDataSpan{ &m_indices[storageInfo.indices.offset], storageInfo.indices.elementCount };

		ASSERTION(verticesDataSpan.size());

		m_upload_heap.upload_data_to_buffer({
			.buffer = info.vb,
			.data = verticesDataSpan.data(),
			.offset = info.verticesWriteOffset + verticesWrittenBytes,
			.size = verticesDataSpan.size_bytes(),
			.dstQueue = info.verticesDstQueue
		});

		m_upload_heap.upload_data_to_buffer({
			.buffer = info.ib,
			.data = indicesDataSpan.data(),
			.offset = info.indicesWriteOffset + indicesWrittenBytes,
			.size = indicesDataSpan.size_bytes(),
			.dstQueue = info.indicesDstQueue
		});

		m_storage_info.erase(pGeometry);

		pGeometry->vertices.offset = static_cast<uint32>((info.verticesWriteOffset + verticesWrittenBytes) / stride);
		pGeometry->indices.offset  = static_cast<uint32>((info.indicesWriteOffset + indicesWrittenBytes) / sizeof(uint32));

		verticesWrittenBytes += verticesDataSpan.size_bytes();
		indicesWrittenBytes  += indicesDataSpan.size_bytes();

		pGeometry = pGeometry->next;
	}

	return GeometryCacheUploadResult{ 
		.id = id, 
		.verticesWriteInfo = {
			.offset = info.verticesWriteOffset, 
			.size = rootGeometryInfo.verticesSizeBytes 
		}, 
		.indicesWriteInfo = { 
			.offset = info.indicesWriteOffset, 
			.size = rootGeometryInfo.indicesSizeBytes 
		} 
	};
}


auto GeometryCache::get_geometry(root_geometry_handle handle) -> Geometry const&
{
	if (handle.valid())
	{
		return m_geometries[handle.get()];
	}
	return m_null_geometry;
}

auto GeometryCache::geometry_info(root_geometry_handle handle) -> RootGeometryInfo
{
	RootGeometryInfo info = {};
	Geometry* geometry = m_geometries.at(handle.get());

	if (geometry)
	{
		info.interleaved = geometry->layoutInfo.layout.interleaved;
		info.inputLayout = geometry->layoutInfo.layout.inputs;
		info.stride = layout_size_bytes(geometry->layoutInfo.layout);

		while (geometry != nullptr)
		{
			info.verticesSizeBytes += geometry->vertices.size;
			info.indicesSizeBytes += geometry->indices.size;

			++info.numGeometries;

			geometry = geometry->next;
		}
	}

	return info;
}

auto GeometryCache::flush_storage() -> void
{
	m_vertices.clear();
	m_indices.clear();
}

auto GeometryCache::input_element_count(GeometryInput input) -> uint32
{
	switch (input)
	{
	case GeometryInput::Position:
		return 3;
	case GeometryInput::Normal:
		return 3;
	case GeometryInput::Tangent:
		return 3;
	case GeometryInput::Color:
		return 3;
	case GeometryInput::TexCoord:
		return 2;
	default:
		return 0;
	}
}

auto GeometryCache::input_size_bytes(GeometryInput input) -> size_t
{
	constexpr size_t SIZE_OF_FLOAT = sizeof(float32);
	uint32 elementCount = input_element_count(input);

	return SIZE_OF_FLOAT * static_cast<size_t>(elementCount);
}

auto GeometryCache::layout_size_bytes(GeometryInputLayout const& layout) -> size_t
{
	int constexpr MAX_INPUT_IN_LAYOUT = 5;
	size_t total = 0;
	for (int i = 0; i < MAX_INPUT_IN_LAYOUT; ++i)
	{
		GeometryInput bit = static_cast<GeometryInput>(1 << i);
		if ((layout.inputs & bit) != GeometryInput::None)
		{
			total += input_size_bytes(bit);
		}
	}
	return total;
}

auto GeometryCache::reserve_space_if_needed(size_t floatCount, size_t uintCount) -> void
{
	size_t constexpr RESERVE_CONSTANT = 1'000'000;

	size_t fm = 0;
	size_t um = 0;

	while ((m_vertices.size() + (fm * RESERVE_CONSTANT)) + floatCount <= m_vertices.capacity())
	{
		++fm;
	}

	while ((m_indices.size() + (um * RESERVE_CONSTANT)) + uintCount <= m_indices.capacity())
	{
		++um;
	}

	if (fm)
	{
		m_vertices.reserve(m_indices.capacity() + fm * RESERVE_CONSTANT);
	}

	if (um)
	{
		m_indices.reserve(m_indices.capacity() + um + RESERVE_CONSTANT);
	}
}

auto GeometryCache::gltf_unpack_interleaved(gltf::Importer const& importer, Geometry* geometry) -> void
{
	using attrib_t = std::underlying_type_t<gltf::VertexAttribute>;

	auto const MAX_ATTRIBUTES = static_cast<attrib_t>(gltf::VertexAttribute::Max);

	constexpr float32 VERTEX_MULTIPLIER[3] = { 1.f, 1.f, -1.f };

	uint32 const meshCount = importer.num_meshes();

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		GeometryStorageInfo const& storageInfo = m_storage_info[geometry];

		size_t offset = 0;

		for (uint32 j = 0; j < storageInfo.vertices.count; ++j)
		{
			std::span<float32> dataSpan{ m_vertices.data() + storageInfo.vertices.offset, storageInfo.vertices.elementCount };

			for (attrib_t k = 0; k < MAX_ATTRIBUTES; ++k)
			{
				auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(k);
				auto const geoInputEnum = static_cast<GeometryInput>(1 << k);

				if ((geometry->layoutInfo.layout.inputs & geoInputEnum) == GeometryInput::None)
				{
					continue;
				}
				
				size_t const componentCount = input_element_count(geoInputEnum);

				if ((storageInfo.missingInputsFromImporter & geoInputEnum) == GeometryInput::None)
				{
					auto attribInfo = mesh.attribute_info(gltfAttribEnum);

					mesh.read_float_data(gltfAttribEnum, j, &dataSpan[offset], componentCount);

					for (uint32 l = 0; l < componentCount; ++l)
					{
						dataSpan[offset + l] *= VERTEX_MULTIPLIER[l];
					}
				}
				else
				{
					for (uint32 l = 0; l < componentCount; ++l)
					{
						dataSpan[offset + l] = 0.f;
					}
				}

				offset += componentCount;
			}
		}

		std::span<uint32> indicesSpan{ m_indices.data() + storageInfo.indices.offset, storageInfo.indices.count };

		mesh.unpack_index_data(indicesSpan);

		geometry = geometry->next;
	}

}

auto GeometryCache::gltf_unpack_non_interleaved(gltf::Importer const& importer, Geometry* geometry) -> void
{
	using attrib_t = std::underlying_type_t<gltf::VertexAttribute>;

	auto const MAX_ATTRIBUTES = static_cast<attrib_t>(gltf::VertexAttribute::Max);

	constexpr float32 VERTEX_MULTIPLIER[3] = { 1.f, 1.f, -1.f };

	uint32 const meshCount = importer.num_meshes();

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		std::array<std::span<float32>, MAX_ATTRIBUTES> attribRange{};
		GeometryStorageInfo const& storageInfo = m_storage_info[geometry];
		size_t attribOffset = 0;

		for (attrib_t j = 0; j < MAX_ATTRIBUTES; ++j)
		{
			auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(j);
			auto const geoInputEnum = static_cast<GeometryInput>(1 << j);

			if ((geometry->layoutInfo.layout.inputs & geoInputEnum) == GeometryInput::None)
			{
				continue;
			}

			auto attribInfo = mesh.attribute_info(gltfAttribEnum);

			attribRange[j] = std::span{ m_vertices.data() + storageInfo.vertices.offset + attribOffset, attribInfo.numVertices * attribInfo.componentCountForType };
			mesh.unpack_vertex_data(gltfAttribEnum, attribRange[j]);

			for (uint32 k = 0; k < attribRange.size(); k += static_cast<uint32>(attribInfo.componentCountForType))
			{
				for (uint32 l = 0; l < attribInfo.componentCountForType; ++l)
				{
					attribRange[j][k * attribInfo.componentCountForType + l] *= VERTEX_MULTIPLIER[l];
				}
			}

			attribOffset += attribRange[j].size();
		}

		std::span<uint32> indicesSpan{ m_indices.data() + storageInfo.indices.offset, storageInfo.indices.count };

		mesh.unpack_index_data(indicesSpan);

		geometry = geometry->next;
	}
}

}