#pragma once
#ifndef SANDBOX_INPUT_ASSEMBLER_H
#define SANDBOX_INPUT_ASSEMBLER_H

#include "rhi/buffer.h"

namespace sandbox
{
struct InputAssemblerInitInfo
{
	rhi::BufferView buffer;
	rhi::BufferViewInfo vertexBufferViewInfo;
	rhi::BufferViewInfo indexBufferViewInfo;
};

struct InputAssembler
{
	struct BufferOffsets
	{
		size_t vertex;
		size_t index;
	};

	rhi::BufferView vertexBufferView;
	rhi::BufferView indexBufferView;
	BufferOffsets offsets;

	auto initialize(InputAssemblerInitInfo const& info) -> bool;
	auto flush() -> void;
};
}

#endif // !SANDBOX_INPUT_ASSEMBLER_H
