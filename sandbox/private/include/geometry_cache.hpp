#pragma once
#ifndef SANDBOX_GEOMETRY_H
#define SANDBOX_GEOMETRY_H

#include "gpu/gpu.hpp"
#include "lib/paged_array.hpp"
#include "lib/handle.hpp"
#include "lib/bitset.hpp"
#include "model_importer.hpp"
#include "upload_heap.hpp"

namespace sandbox
{

enum class GeometryInput
{
	None		= 0,
	Position	= 0x01,
	Normal		= 0x02,
	Tangent		= 0x04,
	Color		= 0x08,
	TexCoord	= 0x10,
	All			= Position | Normal | Tangent | Color | TexCoord
};

struct InputInfo
{
	/**
	* Offset of the data in the vertex buffer.
	*/
	size_t offset;
	/**
	* Total input size in bytes.
	*/
	size_t size;
	/**
	* Count of this input i.e number of vertices.
	*/
	size_t count;
};

struct GeometryInputLayout
{
	/**
	* Tells the cache which geometry input to load. The default loads all inputs.
	*/
	GeometryInput inputs = GeometryInput::All;
	/**
	*
	*/
	bool interleaved;
};

struct GeometryInputLayoutInfo
{
	GeometryInputLayout layout;
	/**
	* Input info data for geometry input layout when data is not interleaved.
	*/
	InputInfo infos[5];
};

struct GeometryData
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
	* \brief Vertices geometry info. This information represent the data's state in the vertex buffer.
	*/
	GeometryData vertices;
	/**
	* \brief Indices geometry info. This information represent the data's state in the index buffer.
	*/
	GeometryData indices;
	/**
	* \brief Geometry's vertex input layout.
	*/
	GeometryInputLayoutInfo layoutInfo;
};

using root_geometry_handle = lib::handle<Geometry, uint64, std::numeric_limits<uint64>::max()>;

//struct StageGeometryInfo
//{
//	root_geometry_handle geometry;
//	gpu::Buffer& vb;
//	gpu::Buffer& ib;
//	size_t verticesWriteOffset;
//	size_t indicesWriteOffset;
//	gpu::DeviceQueue verticesDstQueue = gpu::DeviceQueue::Main;
//	gpu::DeviceQueue indicesDstQueue = gpu::DeviceQueue::Main;
//};

struct GeometryInfo
{
	size_t verticesSizeBytes;
	size_t indicesSizeBytes;
	size_t stride;
	uint32 numGeometries;
	GeometryInput inputLayout;
	bool interleaved;
};

/**
* Ideally, the geometry cache should only deal with the project's proprietary format so that it can be streamed directly into the GPU.
*/
class GeometryCache
{
public:
	GeometryCache() = default;
	~GeometryCache() = default;

	/**
	 * Uploads the model from the GLTF file to the GPU. 
	 */
	auto upload_gltf(UploadHeap& uploadHeap, gltf::Importer const& importer, GeometryInputLayout const& inputLayout) -> root_geometry_handle;
	//auto stage_geometries_for_upload(StageGeometryInfo&& info) -> GeometryCacheUploadResult;
	auto geometry_from(root_geometry_handle handle) -> Geometry const*;
	auto geometry_info(root_geometry_handle handle) -> GeometryInfo;
	auto vertex_buffer_of(root_geometry_handle handle) const -> gpu::buffer;
	auto index_buffer_of(root_geometry_handle handle) const -> gpu::buffer;
	//auto flush_storage() -> void;
private:
	lib::map<uint64, gpu::buffer> m_geometryVb = {};
	lib::map<uint64, gpu::buffer> m_geometryIb = {};
	lib::paged_array<Geometry, 64> m_geometries = {};

	auto input_element_count(GeometryInput input) -> uint32;
	auto input_size_bytes(GeometryInput input) -> size_t;
	auto layout_size_bytes(GeometryInputLayout const& layout) -> size_t;
	auto gltf_unpack_interleaved(UploadHeap& uploadHeap, gltf::Importer const& importer, root_geometry_handle geometryHandle, size_t verticesSizeBytes, size_t indicesSizeBytes) -> void;
	auto gltf_unpack_non_interleaved(UploadHeap& uploadHeap, gltf::Importer const& importer, root_geometry_handle geometryHandle, size_t verticesSizeBytes, size_t indicesSizeBytes) -> void;

	auto upload_heap_blocks(UploadHeap& uploadHeap, std::span<HeapBlock> heapBlocks, size_t initialWriteOffset, gpu::buffer const& buffer, size_t dstOffset) -> size_t;
};
}

#endif // !SANDBOX_GEOMETRY_H
