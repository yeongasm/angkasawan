#pragma once
#ifndef SANDBOX_GEOMETRY_H
#define SANDBOX_GEOMETRY_H

#include "rhi/buffer.h"
#include "lib/paged_array.h"
#include "lib/handle.h"
#include "lib/bitset.h"
#include "model_importer.h"
#include "upload_heap.h"

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
	* \brief Vertices geometry info. This information represent the data's state in the vertex buffer.
	*/
	GeometryInfo vertices;
	/**
	* \brief Indices geometry info. This information represent the data's state in the index buffer.
	*/
	GeometryInfo indices;
	/**
	* \brief Geometry's vertex input layout.
	*/
	GeometryInputLayoutInfo layoutInfo;
};

using root_geometry_handle = lib::handle<Geometry, uint32, std::numeric_limits<uint32>::max()>;

struct GeometryCacheUploadResult
{
	upload_id id;
	rhi::BufferWriteInfo verticesWriteInfo;
	rhi::BufferWriteInfo indicesWriteInfo;
};

struct StageGeometryInfo
{
	root_geometry_handle geometry;
	rhi::Buffer& vb;
	rhi::Buffer& ib;
	size_t verticesWriteOffset;
	size_t indicesWriteOffset;
	rhi::DeviceQueueType verticesDstQueue = rhi::DeviceQueueType::Main;
	rhi::DeviceQueueType indicesDstQueue = rhi::DeviceQueueType::Main;
};

struct RootGeometryInfo
{
	size_t verticesSizeBytes;
	size_t indicesSizeBytes;
	size_t stride;
	uint32 numGeometries;
	GeometryInput inputLayout;
	bool interleaved;
};

class GeometryCache
{
public:
	GeometryCache(UploadHeap& uploadHeap);
	~GeometryCache() = default;

	auto store_geometries(gltf::Importer const& importer, GeometryInputLayout const& inputLayout) -> root_geometry_handle;
	auto stage_geometries_for_upload(StageGeometryInfo&& info) -> GeometryCacheUploadResult;
	auto get_geometry(root_geometry_handle handle) -> Geometry const&;
	auto geometry_info(root_geometry_handle handle) -> RootGeometryInfo;
	auto flush_storage() -> void;
private:

	struct StoreInfo
	{
		/**
		* Offset of the mesh's starting data in the scratch buffer.
		*/
		size_t offset;
		/**
		* Number of vertices / indices that the mesh contains.
		*/
		size_t count;
		/**
		* For vertex data, elementCount refers as the total number of floats in all of the components in the vertex data.
		* For index data, elementCount is the same as "count" which is the total number of uint32 the mesh contains.
		*/
		size_t elementCount;
	};

	struct GeometryStorageInfo
	{
		/**
		* Storage info for all of the vertices the mesh contains in the scratch buffer.
		*/
		StoreInfo vertices;
		/**
		* Storage info for all of the indices the mesh contains in the scratch buffer.
		*/
		StoreInfo indices;

		GeometryInput missingInputsFromImporter = GeometryInput::None;
	};

	lib::array<float32> m_vertices;
	lib::array<uint32> m_indices;
	lib::map<Geometry*, GeometryStorageInfo> m_storage_info;
	lib::paged_array<Geometry, 64> m_geometries;
	Geometry m_null_geometry;
	UploadHeap& m_upload_heap;

	auto input_element_count(GeometryInput input) -> uint32;
	auto input_size_bytes(GeometryInput input) -> size_t;
	auto layout_size_bytes(GeometryInputLayout const& layout) -> size_t;
	auto reserve_space_if_needed(size_t floatCount, size_t uintCount) -> void;
	auto gltf_unpack_interleaved(gltf::Importer const& importer, Geometry* geometry) -> void;
	auto gltf_unpack_non_interleaved(gltf::Importer const& importer, Geometry* geometry) -> void;
};
}

#endif // !SANDBOX_GEOMETRY_H
