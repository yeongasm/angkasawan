#pragma once
#ifndef MAKESBF_JOBS_HPP
#define MAKESBF_JOBS_HPP

#include <filesystem>

#include "lib/string.hpp"
#include "core.serialization/write_stream.hpp"
#include "render/mesh.hpp"

#include "gltf_importer.hpp"

namespace makesbf
{
struct MeshifyJobInfo
{
	std::filesystem::path input;
	std::filesystem::path output;
	size_t writeStreamBufferCapacity = 5_MiB;
	lib::allocator<std::byte> allocator = {};
	render::VertexAttribute attributes;
};

/**
* Future plans:
* 1. Pre-validate the existence of the file by checking in runtime if the file exists.
*/
class MeshifyJob
{
public:
	MeshifyJob(MeshifyJobInfo const& info);

	auto execute() -> std::optional<std::string_view>;
	auto job_info() const -> MeshifyJobInfo const&;

private:
	struct MeshInfo
	{
		render::MeshViewHeader header;
		render::MeshView data;
	};

	MeshifyJobInfo m_info;
	lib::allocator<std::byte> m_allocator;
	lib::array<MeshInfo> m_unpackedMeshes;

	auto _calculate_num_meshes_and_total_size_bytes(gltf::Importer const& model) const -> std::pair<size_t, size_t>;
	auto _convert_gltf_to_ours(core::sbf::Buffer& buffer, gltf::Importer const& model) -> void;
	auto _unpack_mesh_vertex_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void;
	auto _unpack_mesh_index_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void;

	static auto _attribute_element_count(render::VertexAttribute attrib) -> uint32;
};
}

#endif // !MAKESBF_JOBS_HPP