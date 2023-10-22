#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (scalar, set = 0, binding = BUFFER_DEVICE_ADDRESS_BINDING) restrict readonly buffer BufferDeviceAddressMemoryBlock
{
	uint64_t _addresses[];
} _bda;

uint64_t _get_addr(uint index)
{
	return _bda._addresses[index];
};

#define deref(T, index) T(_get_addr(index))