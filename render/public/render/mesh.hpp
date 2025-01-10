#ifndef RENDER_MODEL_HPP
#define RENDER_MODEL_HPP

#include "core.serialization/sbf_header.hpp"
#include "core.serialization/file.hpp"
#include "gpu/gpu.hpp"

namespace render
{
/**
* @brief Vertex attributes are always stored in this order Position -> Normal -> Tangent -> Color -> TexCoord in the sbf files.
*/
enum class VertexAttribute : uint8
{
	None		= 0,
	Position	= 0x01,
	Normal		= 0x02,
	Tangent		= 0x04,
	Color		= 0x08,
	TexCoord	= 0x10,
	All			= Position | Normal | Tangent | Color | TexCoord
};

template <VertexAttribute>
struct AttribInfo
{
	static constexpr uint32 componentCount = 3u;
	static constexpr uint32 componentSizeBytes = componentCount * sizeof(float32);
};

template <>
struct AttribInfo<VertexAttribute::TexCoord>
{
	static constexpr uint32 componentCount = 2u;
	static constexpr uint32 componentSizeBytes = componentCount * sizeof(float32);
};

template <>
struct AttribInfo<VertexAttribute::None>
{
	static constexpr uint32 componentCount = 0u;
	static constexpr uint32 componentSizeBytes = 0u;
};

template <>
struct AttribInfo<VertexAttribute::All>
{
	static constexpr uint32 componentCount = 14u;
	static constexpr uint32 componentSizeBytes = componentCount * sizeof(float32);
};

struct MeshViewGroupHeader
{
	core::sbf::SbfDataDescriptor descriptor = { .tag = 'GVM' };
	uint32 meshCount;
};

struct MeshViewInfo
{
	struct MeshViewDataInfo
	{
		uint32 count;
		uint32 sizeBytes;
	};
	MeshViewDataInfo vertices;
	MeshViewDataInfo indices;
	VertexAttribute attributes;
};

struct MeshViewHeader
{
	core::sbf::SbfDataDescriptor descriptor = { .tag = 'VM' };
	MeshViewInfo meshInfo;
};

/**
* @brief A view to the mesh's data in the buffer. Only used when serializing and deserializing the mesh.
*/
struct MeshView
{
	std::span<float32>	vertices;
	std::span<uint32>	indices;
};

struct MeshSbfView
{
	MeshViewInfo metadata;
	MeshView data;
};

struct MeshDataInfo
{
	/**
	* @brief Size of geometry data in bytes.
	*/
	size_t size;
	/**
	* @brief Starting byte offset of the data in the buffer.
	*/
	size_t byteOffset;
	/**
	* @brief Starting offset of the data in the buffer. The offset is not in terms of bytes but rather in terms of the data's index in the buffer.
	*/
	uint32 offset;
	/**
	* @brief Amount of geometry data.
	*/
	uint32 count;
};

struct Mesh
{
	gpu::buffer buffer;

	struct
	{
		MeshDataInfo vertices;
		MeshDataInfo indices;
	} info;

	gpu::device_address position;
	gpu::device_address normal;
	gpu::device_address uv;
	VertexAttribute attributes;
};

class MeshSbfPack : public lib::non_copyable_non_movable
{
public:
	class MeshPackIterator
	{
	public:
		MeshPackIterator(std::byte* blob);

		auto operator* () const noexcept -> MeshSbfView;
		auto operator->() const noexcept -> MeshSbfView;

		auto operator++() noexcept -> MeshPackIterator&;
		auto operator==(MeshPackIterator const& rhs) const noexcept -> bool;
	private:
		std::byte* m_cursor;

		auto to_mesh_sbf_view() const -> MeshSbfView;
	};

	using Iterator = MeshPackIterator;

	MeshSbfPack() = default;
	MeshSbfPack(core::sbf::File const& mmapFile);
	MeshSbfPack(std::span<std::byte> blob);

	auto mesh_count() const -> uint32;
	auto from_memory(std::span<std::byte> blob) -> bool;
	auto from_mmap_file(core::sbf::File const& mmapFile) -> bool;

	auto begin() -> Iterator;
	auto end() -> Iterator;
private:
	void* m_head = nullptr;
	void* m_tail = nullptr;
	uint32 m_meshCount = {};
};
}

#endif // !RENDER_MODEL_HPP
