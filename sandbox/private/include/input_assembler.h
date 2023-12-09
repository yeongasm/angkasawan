#pragma once
#ifndef SANDBOX_INPUT_ASSEMBLER_H
#define SANDBOX_INPUT_ASSEMBLER_H

#include "rhi/buffer.h"
#include "buffer_view_registry.h"

namespace sandbox
{
struct InputAssemblerInitInfo
{
	Resource<rhi::Buffer> buffer;
	BufferViewInfo vertexBufferInfo;
	BufferViewInfo indexBufferInfo;
};

/**
* TODO(afiq):
* Make this class more robust.
*/
struct InputAssembler
{
	struct BufferOffsets
	{
		size_t vertex;
		size_t index;
	};

	std::pair<buffer_handle, lib::ref<BufferView>> vertexBuffer;
	std::pair<buffer_handle, lib::ref<BufferView>> indexBuffer;
	BufferOffsets offsets;

	auto initialize(InputAssemblerInitInfo const& info, BufferViewRegistry& bufferViewRegistry) -> bool;
	auto terminate(BufferViewRegistry& bufferViewRegistry) -> void;
	auto flush() -> void;
};
}

#endif // !SANDBOX_INPUT_ASSEMBLER_H
