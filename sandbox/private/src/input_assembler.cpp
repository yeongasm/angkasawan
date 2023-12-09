#include "input_assembler.h"

namespace sandbox
{

auto InputAssembler::initialize(InputAssemblerInitInfo const& info, BufferViewRegistry& bufferViewRegistry) -> bool
{
	size_t const inputBufferSize = info.buffer->size();
	size_t const requiredSize = info.vertexBufferInfo.size + info.indexBufferInfo.size;

		// 2 sizes combined exceeded the capacity of the buffer.
	if (info.buffer->offset() + requiredSize > inputBufferSize ||
		// Space for vertex buffer exceeded the capacity of the input buffer at the requested offset.
		info.vertexBufferInfo.offset + info.vertexBufferInfo.size > inputBufferSize ||
		// Space for index buffer exceeded the capacity of the input buffer at the requested offset.
		info.indexBufferInfo.offset + info.indexBufferInfo.size > inputBufferSize ||
		// Offset requested for vertex buffer view exceeded capacity of the input buffer.
		info.vertexBufferInfo.offset > inputBufferSize ||
		// Offset requested for index buffer view exceeded capacity of the input buffer.
		info.indexBufferInfo.offset > inputBufferSize)
	{
		return false;
	}

	vertexBuffer = bufferViewRegistry.create_buffer_view(info.vertexBufferInfo);
	indexBuffer	 = bufferViewRegistry.create_buffer_view(info.indexBufferInfo);

	flush();

	return true;
}

auto InputAssembler::terminate(BufferViewRegistry& bufferViewRegistry) -> void
{
	bufferViewRegistry.destroy_buffer_view(vertexBuffer.first);
	bufferViewRegistry.destroy_buffer_view(indexBuffer.first);
}

auto InputAssembler::flush() -> void
{
	offsets.vertex	= vertexBuffer.second->bufferOffset;
	offsets.index	= indexBuffer.second->bufferOffset;
}

}