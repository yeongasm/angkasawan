#pragma once
#ifndef MAKESBF_MESHIFY_JOB_HPP
#define MAKESBF_MESHIFY_JOB_HPP

#include <filesystem>

#include "core.serialization/write_stream.hpp"
#include "render/render.hpp"

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

class MakeSbf;

/**
* Future plans:
* 1. Pre-validate the existence of the file by checking in runtime if the file exists.
*/
class MeshifyJob
{
public:
	MeshifyJob(MakeSbf& tool, MeshifyJobInfo const& info);

	auto execute() -> std::optional<std::string_view>;
	auto job_info() const -> MeshifyJobInfo const&;
private:

	struct MeshInfo
	{
		render::SbfMeshViewHeader header;
		render::MeshView data;
	};

	MakeSbf& m_tool;
	MeshifyJobInfo m_info;
	lib::array<MeshInfo> m_unpackedMeshes;
	render::material::util::MaterialJSON m_materials;

	auto _calculate_num_meshes_and_total_size_bytes(gltf::Importer const& model) const -> std::pair<size_t, size_t>;
	auto _convert_gltf_to_ours(core::sbf::Buffer& buffer, gltf::Importer const& model) -> void;
	auto _unpack_mesh_vertex_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void;
	auto _unpack_mesh_index_data(uint32 i, core::sbf::Buffer& buffer, gltf::Mesh const& mesh) -> void;

	auto _unpack_materials(gltf::Importer const& model) -> void;
	auto _output_material_json() -> void;

	static auto _attribute_element_count(render::VertexAttribute attrib) -> uint32;
};
}

#endif // !MAKESBF_MESHIFY_JOB_HPP