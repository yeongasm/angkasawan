#include "geometry_cache.hpp"

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

auto GeometryCache::upload_gltf(UploadHeap& uploadHeap, gltf::Importer const& importer, GeometryInputLayout const& layout) -> root_geometry_handle
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

	size_t floatElementCount = 0;
	size_t uint32ElementCount = 0;

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			ASSERTION(false);
			continue;
		}

		auto mesh = opt.value();

		// Skip decals for now ...
		if (mesh.material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		auto&& [idx, geo] = m_geometries.insert(Geometry{});

		if (handle == root_geometry_handle::invalid_handle())
		{
			new (&handle) root_geometry_handle{ idx.to_uint64() };
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

		auto const meshVerticesCount = mesh.num_vertices();

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

			if (!layout.interleaved)
			{
				InputInfo& inputInfo = tailGeometry->layoutInfo.infos[j];
				inputInfo.count = attribInfo.numVertices;
				inputInfo.size = attribInfo.totalSizeBytes;
			}

			floatElementCount += input_element_count(geoInputEnum) * meshVerticesCount;

		}

		tailGeometry->vertices.count = meshVerticesCount;

		uint32ElementCount += mesh.num_indices();

		tailGeometry->indices.count = mesh.num_indices();
	}

	size_t const verticesSizeBytes = floatElementCount * sizeof(float32);
	size_t const indicesSizeBytes = uint32ElementCount * sizeof(uint32);

	gpu::BufferInfo vbInfo{
		.size = verticesSizeBytes,
		.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Vertex,
		.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
	};

	auto modelPath = importer.path();

	m_geometryVb.emplace(
		handle.get(),
		gpu::Buffer::from(
			uploadHeap.device(),
			{
				.name = lib::format("<vertex>:{}", modelPath),
				.size = verticesSizeBytes,
				.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Vertex,
				.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
			}
		)
	);

	m_geometryIb.emplace(
		handle.get(),
		gpu::Buffer::from(
			uploadHeap.device(),
			{ 
				.name = lib::format("<index>:{}", modelPath),
				.size = indicesSizeBytes,
				.bufferUsage = gpu::BufferUsage::Transfer_Dst | gpu::BufferUsage::Index,
				.memoryUsage = gpu::MemoryUsage::Can_Alias | gpu::MemoryUsage::Best_Fit
			}
		)
	);

	if (layout.interleaved)
	{
		gltf_unpack_interleaved(uploadHeap, importer, handle, verticesSizeBytes, indicesSizeBytes);
	}
	else
	{
		gltf_unpack_non_interleaved(uploadHeap, importer, handle, verticesSizeBytes, indicesSizeBytes);
	}

	return handle;
}

auto GeometryCache::geometry_from(root_geometry_handle handle) -> Geometry const*
{
	using index = typename decltype(m_geometries)::index;

	if (handle.valid())
	{
		return &m_geometries[index::from(handle.get())];
	}
	return nullptr;
}

auto GeometryCache::geometry_info(root_geometry_handle handle) -> GeometryInfo
{
	using index = typename decltype(m_geometries)::index;

	GeometryInfo info = {};
	Geometry* geometry = m_geometries.at(index::from(handle.get()));

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

auto GeometryCache::vertex_buffer_of(root_geometry_handle handle) const -> gpu::buffer
{
	if (!m_geometryVb.contains(handle.get()))
	{
		return {};
	}
	return m_geometryVb.at(handle.get()).value()->second;
}

auto GeometryCache::index_buffer_of(root_geometry_handle handle) const -> gpu::buffer
{
	if (!m_geometryIb.contains(handle.get()))
	{
		return {};
	}
	return m_geometryIb.at(handle.get()).value()->second;
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

auto GeometryCache::gltf_unpack_interleaved(UploadHeap& uploadHeap, gltf::Importer const& importer, root_geometry_handle geometryHandle, size_t verticesSizeBytes, size_t indicesSizeBytes) -> void
{
	using attrib_t = std::underlying_type_t<gltf::VertexAttribute>;
	using geometry_index = decltype(m_geometries)::index;

	auto const MAX_ATTRIBUTES = static_cast<attrib_t>(gltf::VertexAttribute::Max);

	// We never use the 4th component, it's just there so that the compiler can auto vectorize.
	constexpr float32 VERTEX_MULTIPLIER[4] = { 1.f, 1.f, -1.f, 0.f };

	uint32 const meshCount = importer.num_meshes();
	Geometry* const rootGeometry = &m_geometries[geometry_index::from(geometryHandle.get())];
	Geometry* geometry = rootGeometry;

	auto vbHeapSpan = uploadHeap.request_heaps(verticesSizeBytes);
	auto currentVbHeapIt = vbHeapSpan.begin();

	auto& vb = m_geometryVb[geometryHandle.get()];
	size_t vbOffset = 0;

	// This will be used to record the starting offset that we wrote the vertex data.
	size_t initialHeapWriteOffset = currentVbHeapIt->byteOffset;
	uint32 vbIndexOffset = 0;

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		// Skip decals for now ...
		if (mesh.material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		geometry->vertices.offset = vbIndexOffset;

		for (uint32 j = 0; j < geometry->vertices.count; ++j)
		{
			for (attrib_t k = 0; k < MAX_ATTRIBUTES; ++k)
			{
				auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(k);
				auto const geoInputEnum = static_cast<GeometryInput>(1 << k);

				if ((geometry->layoutInfo.layout.inputs & geoInputEnum) == GeometryInput::None)
				{
					continue;
				}

				size_t const componentCount = input_element_count(geoInputEnum);

				// This is fine because number of components for a single vertex attribute will never exceed 3.
				// The capacity is kept at 4 to enable compiler auto vectorization.
				float32 data[4] = {};

				// We check if the mesh has the required attribute here because the geometry mandates it to be present.
				if (mesh.has_attribute(gltfAttribEnum))
				{
					auto attribInfo = mesh.attribute_info(gltfAttribEnum);

					mesh.read_float_data(gltfAttribEnum, j, data, componentCount);

					for (uint32 l = 0; l < componentCount; ++l)
					{
						data[l] *= VERTEX_MULTIPLIER[l];
					}
				}

				for (uint32 l = 0; l < componentCount; ++l)
				{
					if (currentVbHeapIt->remaining_capacity() < sizeof(float32)) [[unlikely]]
					{
						++currentVbHeapIt;

						if (currentVbHeapIt == vbHeapSpan.end())
						{
							vbOffset = upload_heap_blocks(uploadHeap, vbHeapSpan, initialHeapWriteOffset, vb, vbOffset);
							uploadHeap.send_to_gpu(true);

							vbHeapSpan = uploadHeap.request_heaps(verticesSizeBytes);
							currentVbHeapIt = vbHeapSpan.begin();

							initialHeapWriteOffset = currentVbHeapIt->byteOffset;
						}
					}

					float32* ptr = currentVbHeapIt->data_as<float32>();

					*ptr = data[l];

					currentVbHeapIt->byteOffset += sizeof(float32);
					verticesSizeBytes -= sizeof(float32);
				}

				geometry->vertices.size += componentCount * sizeof(float32);
			}
		}

		size_t const stride = layout_size_bytes(geometry->layoutInfo.layout);
		vbIndexOffset += static_cast<uint32>(geometry->vertices.size / stride);

		geometry = geometry->next;
	}

	// Reset the geometry pointer to it's initial one.
	geometry = rootGeometry;

	upload_heap_blocks(uploadHeap, vbHeapSpan, initialHeapWriteOffset, vb, vbOffset);

	auto ibHeapSpan = uploadHeap.request_heaps(indicesSizeBytes);
	auto currentIbHeapIt = ibHeapSpan.begin();

	auto& ib = m_geometryIb[geometryHandle.get()];
	size_t ibOffset = 0;

	// Update the initial write offset for the index data to the current heap's byte offset.
	initialHeapWriteOffset = currentIbHeapIt->byteOffset;

	// Same with the initial index data written heap.
	uint32 ibIndexOffset = 0;

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		// Skip decals for now ...
		if (mesh.material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		geometry->indices.offset = ibIndexOffset;

		for (uint32 j = 0; j < geometry->indices.count; ++j)
		{
			if (currentIbHeapIt->remaining_capacity() < sizeof(uint32)) [[unlikely]]
			{
				++currentIbHeapIt;

				if (currentIbHeapIt == ibHeapSpan.end())
				{
					ibOffset = upload_heap_blocks(uploadHeap, ibHeapSpan, initialHeapWriteOffset, ib, ibOffset);
					uploadHeap.send_to_gpu(true);

					ibHeapSpan = uploadHeap.request_heaps(indicesSizeBytes);
					currentIbHeapIt = ibHeapSpan.begin();

					initialHeapWriteOffset = currentIbHeapIt->byteOffset;
				}
			}

			uint32* ptr = currentIbHeapIt->data_as<uint32>();

			*ptr = mesh.read_uint_data(j);

			currentIbHeapIt->byteOffset += sizeof(uint32);
			indicesSizeBytes -= sizeof(uint32);
		}

		geometry->indices.size = geometry->indices.count * sizeof(uint32);
		ibIndexOffset += geometry->indices.count;

		geometry = geometry->next;
	}

	upload_heap_blocks(uploadHeap, ibHeapSpan, initialHeapWriteOffset, ib, ibOffset);
}

auto GeometryCache::gltf_unpack_non_interleaved(UploadHeap& uploadHeap, gltf::Importer const& importer, root_geometry_handle geometryHandle, size_t verticesSizeBytes, size_t indicesSizeBytes) -> void
{
	using attrib_t = std::underlying_type_t<gltf::VertexAttribute>;
	using geometry_index = decltype(m_geometries)::index;

	auto const MAX_ATTRIBUTES = static_cast<attrib_t>(gltf::VertexAttribute::Max);

	constexpr float32 VERTEX_MULTIPLIER[4] = { 1.f, 1.f, -1.f, 0.f };

	uint32 const meshCount = importer.num_meshes();
	Geometry* const rootGeometry = &m_geometries[geometry_index::from(geometryHandle.get())];
	Geometry* geometry = rootGeometry;

	auto heapSpan = uploadHeap.request_heaps(verticesSizeBytes + indicesSizeBytes);
	auto currentHeapIt = heapSpan.begin();

	auto& vb = m_geometryVb[geometryHandle.get()];
	auto& ib = m_geometryIb[geometryHandle.get()];
	
	// This will be used to record the starting offset that we wrote the vertex data.
	size_t initialHeapWriteOffset = currentHeapIt->byteOffset;
	uint32 vbIndexOffset = 0;

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		// Skip decals for now ...
		if (mesh.material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		geometry->vertices.offset = vbIndexOffset;

		for (attrib_t j = 0; j < MAX_ATTRIBUTES; ++j)
		{
			auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(j);
			auto const geoInputEnum = static_cast<GeometryInput>(1 << j);

			if ((geometry->layoutInfo.layout.inputs & geoInputEnum) == GeometryInput::None)
			{
				continue;
			}

			size_t const componentCount = input_element_count(geoInputEnum);
			size_t const totalComponentCount = geometry->vertices.count * componentCount;

			if (mesh.has_attribute(gltfAttribEnum))
			{
				float32 data[4] = {};

				auto attribInfo = mesh.attribute_info(gltfAttribEnum);

				for (size_t k = 0; k < totalComponentCount; k += componentCount)
				{
					mesh.read_float_data(gltfAttribEnum, k, data, componentCount);

					for (uint32 l = 0; l < componentCount; ++l)
					{
						data[l] *= VERTEX_MULTIPLIER[l];

						if (currentHeapIt->remaining_capacity() < sizeof(float32)) [[unlikely]]
						{
							++currentHeapIt;
						}

						float32* ptr = currentHeapIt->data_as<float32>();

						*ptr = data[l];

						currentHeapIt->byteOffset += sizeof(float32);
					}
				}
			}
			else
			{
				// If the mesh does not have this attribute, fill them with 0's.
				size_t remainingComponentCount = totalComponentCount;

				while (!std::cmp_equal(remainingComponentCount, 0))
				{
					size_t countToUpload = remainingComponentCount;

					if (countToUpload > (currentHeapIt->remaining_capacity() / sizeof(float32)))
					{
						countToUpload = currentHeapIt->remaining_capacity() / sizeof(float32);
					}
				
					lib::memzero(currentHeapIt->data(), countToUpload * sizeof(float32));

					remainingComponentCount -= countToUpload;

					if (std::cmp_greater(remainingComponentCount, 0))
					{
						++currentHeapIt;
					}
				}
			}

			geometry->vertices.size += totalComponentCount * sizeof(float32);
		}

		size_t const stride = layout_size_bytes(geometry->layoutInfo.layout);
		vbIndexOffset += static_cast<uint32>(geometry->vertices.size / stride);

		geometry = geometry->next;
	}

	// Reset the geometry pointer to it's initial one.
	geometry = rootGeometry;
	// Number of heaps used to upload vertex data.
	size_t vbNumHeapsUsed = currentHeapIt - heapSpan.begin();

	if (std::cmp_equal(vbNumHeapsUsed, 0))
	{
		vbNumHeapsUsed = 1;
	}

	for (size_t i = 0; i < vbNumHeapsUsed; ++i)
	{
		uploadHeap.upload_heap_to_buffer({
			.heapBlock = heapSpan[i],
			.heapWriteOffset = initialHeapWriteOffset,
			.heapWriteSize = heapSpan[i].byteOffset - initialHeapWriteOffset,
			.dst = vb
		});

		initialHeapWriteOffset = 0;
	}

	// Update the initial write offset for the index data to the current heap's byte offset.
	initialHeapWriteOffset = currentHeapIt->byteOffset;
	// Same with the initial index data written heap.
	auto const ibInitialHeap = currentHeapIt;
	uint32 ibIndexOffset = 0;

	for (uint32 i = 0; i < meshCount; ++i)
	{
		auto opt = importer.mesh_at(i);

		if (!opt.has_value())
		{
			continue;
		}

		auto mesh = opt.value();

		// Skip decals for now ...
		if (mesh.material_info().alphaMode != gltf::AlphaMode::Opaque)
		{
			continue;
		}

		geometry->indices.offset = ibIndexOffset;

		for (uint32 j = 0; j < geometry->indices.count; ++j)
		{
			if (currentHeapIt->remaining_capacity() < sizeof(uint32)) [[unlikely]]
			{
				++currentHeapIt;
			}

			uint32* ptr = currentHeapIt->data_as<uint32>();

			*ptr = mesh.read_uint_data(j);

			currentHeapIt->byteOffset += sizeof(uint32);
		}

		geometry->indices.size = geometry->indices.count * sizeof(uint32);
		ibIndexOffset += geometry->indices.count;

		geometry = geometry->next;
	}

	// Number of heaps used to upload index data.
	size_t ibNumHeapsUsed = currentHeapIt - heapSpan.begin();
	size_t const ibHeapIndex = (heapSpan.end() - currentHeapIt) - 1;

	if (std::cmp_equal(ibNumHeapsUsed, 0))
	{
		ibNumHeapsUsed = 1;
	}

	for (size_t i = ibHeapIndex; i < ibHeapIndex + ibNumHeapsUsed; ++i)
	{
		uploadHeap.upload_heap_to_buffer({
			.heapBlock = heapSpan[i],
			.heapWriteOffset = initialHeapWriteOffset,
			.heapWriteSize = heapSpan[i].byteOffset - initialHeapWriteOffset,
			.dst = ib
		});

		initialHeapWriteOffset = 0;
	}
}

auto GeometryCache::upload_heap_blocks(UploadHeap& uploadHeap, std::span<HeapBlock> heapBlocks, size_t initialWriteOffset, gpu::buffer const& buffer, size_t dstOffset) -> size_t
{
	for (HeapBlock& heapBlock : heapBlocks)
	{
		size_t const writtenSize = heapBlock.byteOffset - initialWriteOffset;

		uploadHeap.upload_heap_to_buffer({
			.heapBlock = heapBlock,
			.heapWriteOffset = initialWriteOffset,
			.heapWriteSize = writtenSize,
			.dst = buffer,
			.dstOffset = dstOffset
		});

		dstOffset += writtenSize;
		initialWriteOffset = 0;
	}

	return dstOffset;
}

}