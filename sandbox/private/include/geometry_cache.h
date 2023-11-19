#pragma once
#ifndef SANDBOX_GEOMETRY_H
#define SANDBOX_GEOMETRY_H

#include "math/vector.h"
#include "rhi/buffer.h"
#include "lib/paged_array.h"
#include "lib/handle.h"
#include "model_importer.h"
#include "input_assembler.h"

namespace sandbox
{

enum class GeometryInput
{
	None,
	Position,
	Normal,
	Tangent,
	Color,
	TexCoord
};

struct GeometryInputLayout
{
	std::array<GeometryInput, 5> inputs;
	uint32 count;
};

struct GeometryInfo
{
	/**
	* \brief Size of geometry data in bytes.
	*/
	size_t size;
	/**
	* \brief Starting offset of the data in the buffer. The offset is not in terms of bytes but rather in terms of the data's index in the buffer.
	*/
	uint32 offset;
	/**
	* \brief Amount of geometry data.
	*/
	uint32 count;
};

struct Geometry
{
	Geometry* next;
	/**
	* \brief Vertices geometry info.
	*/
	GeometryInfo vertices;
	/**
	* \brief Indices geometry info.
	*/
	GeometryInfo indices;
	/**
	* \brief Geometry's vertex input layout.
	*/
	GeometryInputLayout layout;
};

using geometry_handle = lib::handle<Geometry, uint32, std::numeric_limits<uint32>::max()>;

class GeometryCache
{
public:
	struct UploadInfo
	{
		rhi::BufferWriteInfo vertices;
		rhi::BufferWriteInfo indices;
	};

	GeometryCache() = default;
	~GeometryCache() = default;

	GeometryCache(InputAssembler& inputAssembler);

	auto store_geometries(GltfImporter const& importer, GeometryInputLayout const& layout) -> geometry_handle;
	auto stage_geometries_for_upload(rhi::Buffer& stagingBuffer) -> UploadInfo;
	auto stage_geometries_for_upload(rhi::BufferView& viewRange) -> UploadInfo;
	auto get_geometry(geometry_handle handle) -> Geometry const&;

private:
	InputAssembler& m_input_assembler;
	lib::array<float32> m_vertices;
	lib::array<uint32> m_indices;
	lib::paged_array<Geometry, 64> m_geometries;
	Geometry m_null_geometry;

	auto input_element_count(GeometryInput input) -> uint32;
	auto input_size_bytes(GeometryInput input) -> size_t;
	auto layout_size_bytes(GeometryInputLayout const& layout) -> size_t;
};
}

#endif // !SANDBOX_GEOMETRY_H
