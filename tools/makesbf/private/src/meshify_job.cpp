#include "meshify_job.hpp"
#include "gltf_importer.hpp"

namespace makesbf
{
MeshifyJob::MeshifyJob(MeshifyJobInfo const& info) :
	m_info{ std::move(info) },
	m_allocator{ info.allocator }
{
}

auto MeshifyJob::execute() -> std::optional<std::string_view>
{
	gltf::Importer model;

	if (!model.open(m_info.input))
	{
		return "Could not open file.";
	}

	model.decode();

	auto [numMeshes, requiredSize] = _calculate_num_meshes_and_total_size_bytes(model);

	m_unpackedMeshes.resize(numMeshes);

	core::sbf::BufferInfo const bufferInfo{ .blockCapacity = requiredSize };

	core::sbf::Buffer scratchBuffer{ bufferInfo, *m_allocator.resource() };

	_convert_gltf_to_ours(scratchBuffer, model);

	core::sbf::WriteStream stream{ m_info.output };

	render::MeshViewGroupHeader meshGroupHeader{};

	for (auto const& mesh : m_unpackedMeshes)
	{
		if (std::cmp_equal(mesh.header.descriptor.sizeBytes, 0))
		{
			continue;
		}
		++meshGroupHeader.meshCount;
		meshGroupHeader.descriptor.sizeBytes += static_cast<uint32>(sizeof(render::MeshViewHeader)) + mesh.header.meshInfo.vertices.sizeBytes + mesh.header.meshInfo.indices.sizeBytes;
	}

	stream.write(core::sbf::SbfFileHeader{});
	stream.write(meshGroupHeader);

	for (int i = 0; auto const& mesh : m_unpackedMeshes)
	{
		if (std::cmp_equal(mesh.header.descriptor.sizeBytes, 0))
		{
			continue;
		}

		// TODO(afiq):
		// Implement some kind of err defer technique.

		stream.write(mesh.header);
		stream.write(mesh.data.vertices);
		stream.write(mesh.data.indices);

		++i;
	}

	return std::nullopt;
}

auto MeshifyJob::job_info() const -> MeshifyJobInfo const&
{
	return m_info;
}

auto MeshifyJob::_calculate_num_meshes_and_total_size_bytes(gltf::Importer const& model) const -> std::pair<size_t, size_t>
{
	size_t numMeshes = 0;
	size_t totalSizeBytes = sizeof(core::sbf::SbfFileHeader) + sizeof(render::MeshViewGroupHeader);

	auto constexpr MAX_ATTRIBUTES = std::to_underlying(gltf::VertexAttribute::Max);

	for (uint32 i = 0; i < model.num_meshes(); ++i)
	{
		auto meshExist = model.mesh_at(i);
		if (!meshExist)
		{
			continue;
		}

		auto mesh = *meshExist;

		++numMeshes;
		totalSizeBytes += sizeof(render::MeshViewHeader);

		for (auto j = 0; j < MAX_ATTRIBUTES; ++j)
		{
			auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(j);
			auto const vertexAttribute = static_cast<render::VertexAttribute>(1 << j);

			if ((m_info.attributes & vertexAttribute) == render::VertexAttribute::None)
			{
				continue;
			}

			if (mesh.has_attribute(gltfAttribEnum))
			{
				auto const attribInfo = mesh.attribute_info(static_cast<gltf::VertexAttribute>(j));
				totalSizeBytes += attribInfo.totalSizeBytes;
			}
		}

		totalSizeBytes += mesh.num_indices() * sizeof(uint32);
	}

	return { numMeshes, totalSizeBytes };
}

auto MeshifyJob::_convert_gltf_to_ours(core::sbf::Buffer& buffer, gltf::Importer const& model) -> void
{
	uint32 const meshCount = model.num_meshes();

	for (uint32 i = 0, j = 0; i < meshCount; ++i)
	{
		auto meshExist = model.mesh_at(i);

		if (!meshExist)
		{
			continue;
		}

		auto mesh = *meshExist;

		_unpack_mesh_vertex_data(j, buffer, mesh);
		_unpack_mesh_index_data(j, buffer, mesh);

		auto&& unpackMeshInfo = m_unpackedMeshes[j];

		unpackMeshInfo.header.descriptor.sizeBytes = unpackMeshInfo.header.meshInfo.vertices.sizeBytes + unpackMeshInfo.header.meshInfo.indices.sizeBytes;
		++j;
	}
}

auto MeshifyJob::_attribute_element_count(render::VertexAttribute attrib) -> uint32
{
	switch (attrib)
	{
	case render::VertexAttribute::Position:
		return 3;
	case render::VertexAttribute::Normal:
		return 3;
	case render::VertexAttribute::Tangent:
		return 3;
	case render::VertexAttribute::Color:
		return 3;
	case render::VertexAttribute::TexCoord:
		return 2;
	default:
		return 0;
	}
}

auto MeshifyJob::_unpack_mesh_vertex_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void
{
	auto constexpr MAX_ATTRIBUTES = std::to_underlying(gltf::VertexAttribute::Max);

	constexpr float32 VERTEX_MULTIPLIER[4] = { 1.f, 1.f, -1.f, 0.f };

	auto&& [header, meshObj] = m_unpackedMeshes[static_cast<size_t>(i)];

	header.meshInfo.vertices.count = mesh.num_vertices();

	size_t const bufferByteOffset = buffer.byte_offset();

	for (auto j = 0; j < MAX_ATTRIBUTES; ++j)
	{
		auto const gltfAttribEnum = static_cast<gltf::VertexAttribute>(j);
		auto const vertexAttrib = static_cast<render::VertexAttribute>(1 << j);

		if ((m_info.attributes & vertexAttrib) == render::VertexAttribute::None)
		{
			continue;
		}

		if (mesh.has_attribute(gltfAttribEnum))
		{
			auto const attribInfo = mesh.attribute_info(gltfAttribEnum);

			size_t const totalComponentCount = attribInfo.numVertices * attribInfo.componentCountForType;

			header.meshInfo.attributes |= vertexAttrib;
			header.meshInfo.vertices.sizeBytes += static_cast<uint32>(attribInfo.totalSizeBytes);

			float32 data[4] = {};

			for (size_t k = 0; k < totalComponentCount; k += attribInfo.componentCountForType)
			{
				mesh.read_float_data(gltfAttribEnum, k, data, attribInfo.componentCountForType);

				for (uint32 l = 0; l < attribInfo.componentCountForType; ++l)
				{
					data[l] *= VERTEX_MULTIPLIER[l];
				}

				// Write to the buffer.
				buffer.write(&data, sizeof(float32) * attribInfo.componentCountForType);
			}
		}
	}

	float32* const ptr = (std::cmp_not_equal(header.meshInfo.vertices.count, 0)) ? reinterpret_cast<float32*>(static_cast<std::byte*>(buffer.data()) + bufferByteOffset) : nullptr ;

	if (ptr != nullptr)
	{
		meshObj.vertices = std::span{ ptr, header.meshInfo.vertices.sizeBytes / sizeof(float32) };
	}
}

auto MeshifyJob::_unpack_mesh_index_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void
{
	auto&& [header, meshObj] = m_unpackedMeshes[static_cast<size_t>(i)];

	header.meshInfo.indices.count = mesh.num_indices();
	header.meshInfo.indices.sizeBytes = static_cast<uint32>(mesh.indices_size_bytes());

	uint32 const numIndices = mesh.num_indices();

	uint32* ptr = (std::cmp_not_equal(header.meshInfo.indices.count, 0)) ? static_cast<uint32*>(buffer.current_byte()) : nullptr;
	

	for (uint32 j = 0; j < numIndices; ++j)
	{
		uint32 index = mesh.read_uint_data(j);
		buffer.write(index);
	}

	//auto range = buffer.reserve_for<uint32>(mesh.num_indices());
	//mesh.unpack_index_data(range);

	if (ptr != nullptr)
	{
		meshObj.indices = std::span{ ptr, static_cast<size_t>(numIndices) };
	}
}
}