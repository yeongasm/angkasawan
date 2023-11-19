#include "input_assembler.h"

namespace sandbox
{

auto InputAssembler::initialize(InputAssemblerInitInfo const& info) -> bool
{
	size_t const inputBufferSize = info.buffer.size();
	size_t const requiredSize = info.vertexBufferViewInfo.size + info.indexBufferViewInfo.size;

		// 2 sizes combined exceeded the capacity of the buffer.
	if (info.buffer.offset_from_buffer() + requiredSize > inputBufferSize ||
		// Space for vertex buffer exceeded the capacity of the input buffer at the requested offset.
		info.vertexBufferViewInfo.offset + info.vertexBufferViewInfo.size > inputBufferSize ||
		// Space for index buffer exceeded the capacity of the input buffer at the requested offset.
		info.indexBufferViewInfo.offset + info.indexBufferViewInfo.size > inputBufferSize ||
		// Offset requested for vertex buffer view exceeded capacity of the input buffer.
		info.vertexBufferViewInfo.offset > inputBufferSize ||
		// Offset requested for index buffer view exceeded capacity of the input buffer.
		info.indexBufferViewInfo.offset > inputBufferSize)
	{
		return false;
	}

	vertexBufferView = info.buffer.make_view(info.vertexBufferViewInfo);
	indexBufferView	 = info.buffer.make_view(info.indexBufferViewInfo);

	flush();

	return true;
}

auto InputAssembler::flush() -> void
{
	offsets.vertex	= vertexBufferView.offset_from_buffer();
	offsets.index	= indexBufferView.offset_from_buffer();
}

}