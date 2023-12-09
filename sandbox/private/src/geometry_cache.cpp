#include "geometry_cache.h"

namespace sandbox
{
auto translate_geometry_input_to_cgltf_importer_input_type(GeometryInput input) -> CgltfVertexInput
{
	switch (input)
	{
	case GeometryInput::Normal:
		return CgltfVertexInput::Normal;
	case GeometryInput::Tangent:
		return CgltfVertexInput::Tangent;
	case GeometryInput::Color:
		return CgltfVertexInput::Color;
	case GeometryInput::TexCoord:
		return CgltfVertexInput::TexCoord;
	case GeometryInput::Position:
	default:
		return CgltfVertexInput::Position;
	}
}

GeometryCache::GeometryCache(
	InputAssembler& inputAssembler
) :
	m_input_assembler{ inputAssembler },
	m_vertices{},
	m_indices{},
	m_geometries{}
{}

auto GeometryCache::store_geometries(GltfImporter const& importer, GeometryInputLayout const& layout) -> geometry_handle
{
	static constexpr float32 VERTEX_MULTIPLIER[3] = { 1.f, 1.f, -1.f };

	if (!m_vertices.capacity())
	{
		m_vertices.reserve(1'000'000);
	}

	size_t verticesSizeBytes = 0;
	size_t indicesSizeBytes = 0;

	for (GltfImporter::MeshInfo const& mesh : importer)
	{
		auto const& positionData = mesh.get_data(CgltfVertexInput::Position);

		verticesSizeBytes += positionData.data.size_bytes();
		indicesSizeBytes  += mesh.indices.size_bytes();
	}

	if (verticesSizeBytes > m_input_assembler.vertexBuffer.second->size ||
		indicesSizeBytes  > m_input_assembler.indexBuffer.second->size)
	{
		ASSERTION(false && "One of the two data exceeded capacity of the their buffers.");
		return geometry_handle::invalid_handle();
	}

	size_t const LAYOUT_SIZE = layout_size_bytes(layout);

	geometry_handle handle = {};
	Geometry* geo = nullptr;
	uint32 numIndices = 0;
	uint32 numVertices = 0;

	for (GltfImporter::MeshInfo const& mesh : importer)
	{
		for (uint32 i = 0; i < mesh.num_vertices; ++i)
		{
			for (uint32 j = 0; j < layout.count; ++j)
			{
				auto inputType = layout.inputs[j];
				//auto const ELEMENT_SIZE = input_size_bytes(inputType);
				auto const NUM_ELEMENTS = input_element_count(inputType);

				if (inputType == GeometryInput::None)
				{
					continue;
				}

				auto cgltfInputType = translate_geometry_input_to_cgltf_importer_input_type(inputType);
				auto&& importedInput = mesh.get_data(cgltfInputType);

				for (uint32 k = 0; k < NUM_ELEMENTS; ++k)
				{
					float32 val = importedInput.data[i * NUM_ELEMENTS + k];
					val *= VERTEX_MULTIPLIER[k];
					m_vertices.push_back(val);
				}
			}
		}

		m_indices.insert(m_indices.end(), mesh.indices.begin(), mesh.indices.end());

		auto&& [index, geometry] = m_geometries.emplace();

		if (geo)
		{
			geo->next = &geometry;
		}

		if (!handle.valid())
		{
			new (&handle) geometry_handle{ index.id };
		}

		geometry.vertices.size		= mesh.num_vertices * LAYOUT_SIZE;
		geometry.vertices.offset	= numVertices;
		geometry.vertices.count		= mesh.num_vertices;

		geometry.indices.size		= mesh.indices.size_bytes();
		geometry.indices.offset		= numIndices;
		geometry.indices.count		= (uint32)mesh.indices.size();

		m_input_assembler.offsets.vertex += geometry.vertices.size;
		m_input_assembler.offsets.index  += geometry.indices.size;

		numIndices  += geometry.indices.count;
		numVertices += geometry.vertices.count;

		// Update the pointer so that the next geometry can be set up to the current one's next chain.
		geo = &geometry;
	}

	return handle;
}

auto GeometryCache::stage_geometries_for_upload(UploadHeap& uploadHeap) -> upload_id
{
	buffer_handle vbh = m_input_assembler.vertexBuffer.first;
	buffer_handle ibh = m_input_assembler.indexBuffer.first;

	if (resource_index const vbidx = vbh.get(); vbidx._metadata.parent)
	{
		resource_index idx{ 0, vbidx._metadata.parent };
		vbh = buffer_handle{ idx._alias };
	}

	if (resource_index const ibidx = ibh.get(); ibidx._metadata.parent)
	{
		resource_index idx{ 0, ibidx._metadata.parent };
		ibh = buffer_handle{ idx._alias };
	}

	uploadHeap.upload_data_to_buffer({
		.buffer = vbh,
		.offset = m_input_assembler.vertexBuffer.second->bufferOffset,
		.data = m_vertices.data(),
		.size = m_vertices.size_bytes(),
		.dstQueue = rhi::DeviceQueueType::Main
	});

	auto id = uploadHeap.upload_data_to_buffer({
		.buffer = ibh,
		.offset = m_input_assembler.indexBuffer.second->bufferOffset,
		.data = m_indices.data(),
		.size = m_indices.size_bytes(),
		.dstQueue = rhi::DeviceQueueType::Main
	});

	//auto verticesStagingWrite = stagingBuffer.write(m_vertices.data(), m_vertices.size_bytes());
	//auto indicesStagingWrite  = stagingBuffer.write(m_indices.data(), m_indices.size_bytes());

	m_vertices.clear();
	m_indices.clear();

	return id;
}

auto GeometryCache::get_geometry(geometry_handle handle) -> Geometry const&
{
	if (handle.valid())
	{
		return m_geometries[handle.get()];
	}
	return m_null_geometry;
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
	size_t total = 0;
	for (GeometryInput const& input : layout.inputs)
	{
		total += input_size_bytes(input);
	}
	return total;
}

}