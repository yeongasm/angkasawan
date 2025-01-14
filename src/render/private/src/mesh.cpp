#include "mesh.hpp"

namespace render
{
MeshSbfPack::MeshPackIterator::MeshPackIterator(std::byte* blob) :
	m_cursor{ blob }
{}

auto MeshSbfPack::MeshPackIterator::operator*() const noexcept -> MeshSbfView
{
    return to_mesh_sbf_view();
}

auto MeshSbfPack::MeshPackIterator::operator->() const noexcept -> MeshSbfView
{
    return to_mesh_sbf_view();
}

auto MeshSbfPack::MeshPackIterator::operator++() noexcept -> MeshPackIterator&
{
    // Cast pointer to mesh view's header.
    auto const& header = *reinterpret_cast<SbfMeshViewHeader*>(m_cursor);

    if (header.descriptor.tag != SBF_MESH_VIEW_HEADER_TAG)
	{
		ASSERTION(false && "Binary blob's descriptor tag is not correct. Data may have been corrupted.");
		m_cursor = nullptr;
	}

	// Iterate to the next header.
	m_cursor += sizeof(SbfMeshViewHeader) + header.descriptor.sizeBytes;

	return *this;
}

auto MeshSbfPack::MeshPackIterator::operator==(MeshPackIterator const& rhs) const noexcept -> bool
{
	return m_cursor == rhs.m_cursor;
}

auto MeshSbfPack::MeshPackIterator::to_mesh_sbf_view() const -> MeshSbfView
{
	std::byte* ptr = m_cursor;

	auto const& descriptor = *reinterpret_cast<core::sbf::SbfDataDescriptor*>(ptr);

	ptr += sizeof(core::sbf::SbfDataDescriptor);

	if (descriptor.tag != SBF_MESH_VIEW_HEADER_TAG)
    {
        ASSERTION(false && "Binary blob's descriptor tag is not correct. Data may have been corrupted.");
        // Probably should do something about it instead of returning an empty view.
        // Maybe return a std::expected instead? But getting a std::expected from an iterator is kinda weird...
        return MeshSbfView{};
    }

    MeshViewInfo const& info = *reinterpret_cast<MeshViewInfo*>(ptr);

    ptr += sizeof(MeshViewInfo);

    MeshView const view = {
        .vertices   = std::span<float32>{ reinterpret_cast<float32*>(ptr), info.vertices.sizeBytes / sizeof(float32) },
        .indices    = std::span<uint32>{ reinterpret_cast<uint32*>(ptr + info.vertices.sizeBytes), info.indices.count }
    };

    return MeshSbfView{ .metadata = info, .data = view };
}

MeshSbfPack::MeshSbfPack(core::sbf::File const& mmapFile)
{
	from_mmap_file(mmapFile);
}

MeshSbfPack::MeshSbfPack(std::span<std::byte> blob)
{
    from_memory(blob);
}

auto MeshSbfPack::mesh_count() const -> uint32
{
    return m_meshCount;
}

auto MeshSbfPack::from_memory(std::span<std::byte> blob) -> bool
{
    if (std::cmp_equal(blob.size_bytes(), 0) ||
		(m_head != nullptr && m_tail != nullptr))
    {
        return false;
    }

    size_t offset = {};
    auto const& sbfHeader = *reinterpret_cast<core::sbf::SbfFileHeader*>(&blob[offset]);

    if (sbfHeader.magic != core::sbf::SBF_HEADER_TAG
    /*|| should probably check the version sometime in the future.*/)
    {
        return false;
    }

    offset += sizeof(core::sbf::SbfFileHeader);

    auto const& meshViewGroup = *reinterpret_cast<SbfMeshViewGroupHeader*>(&blob[offset]);

    if (meshViewGroup.descriptor.tag != SBF_MESH_VIEW_GROUP_HEADER_TAG ||
		std::cmp_equal(meshViewGroup.descriptor.sizeBytes, 0))
	{
		// No data to iterate over so we skip.
		return false;
	}

	m_meshCount = meshViewGroup.meshCount;

	offset += sizeof(SbfMeshViewGroupHeader);

	m_head = &blob[offset];
	m_tail = &blob[meshViewGroup.descriptor.sizeBytes];

	return true;
}

auto MeshSbfPack::from_mmap_file(core::sbf::File const& mmapFile) -> bool
{
	if (std::cmp_equal(mmapFile.size(), 0) ||
		std::cmp_equal(mmapFile.mapped_size(), 0) ||
		(m_head != nullptr && m_tail != nullptr))
	{
		return false;
	}

	std::byte* ptr = static_cast<std::byte*>(mmapFile.data());

	auto const& sbfHeader = *reinterpret_cast<core::sbf::SbfFileHeader*>(ptr);

	if (sbfHeader.magic != core::sbf::SBF_HEADER_TAG
	/*|| should probably check the version sometime in the future.*/)
	{
		return false;
	}

	ptr += sizeof(core::sbf::SbfFileHeader);

	auto const& meshViewGroup = *reinterpret_cast<SbfMeshViewGroupHeader*>(ptr);

	if (meshViewGroup.descriptor.tag != SBF_MESH_VIEW_GROUP_HEADER_TAG ||
		std::cmp_equal(meshViewGroup.descriptor.sizeBytes, 0))
	{
		// No data to iterate over so we skip.
		return false;
	}

	m_meshCount = meshViewGroup.meshCount;

	ptr += sizeof(SbfMeshViewGroupHeader);

	m_head = ptr;
	m_tail = ptr + meshViewGroup.descriptor.sizeBytes;

	return true;
}

auto MeshSbfPack::begin() -> Iterator
{
	return Iterator{ static_cast<std::byte*>(m_head) };
}

auto MeshSbfPack::end() -> Iterator
{
	return Iterator{ static_cast<std::byte*>(m_tail) };
}
}