#pragma once
#ifndef SANDBOX_BINDABLE_STRUCTURE
#define SANDBOX_BINDABLE_STRUCTURE

#include "lib/concepts.h"
#include "rhi/buffer.h"

namespace sandbox
{
template <lib::is_trivial T>
class Bindable : public T
{
public:
	auto bind() -> void;
protected:
	rhi::BufferView m_buffer;
};
}

#endif // !SANDBOX_BINDABLE_STRUCTURE
