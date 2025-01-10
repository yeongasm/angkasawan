#pragma once
#ifndef PLATFORM_MINIMAL_H
#define PLATFORM_MINIMAL_H

#include <string_view>
#include "lib/resource.hpp"
#include "platform_api.h"
#include "io_enums.hpp"

namespace core
{
template <typename T>
class Ref
{
public:
	static_assert(std::is_base_of_v<lib::ref_counted, T>);

	using type				= T;
	using pointer			= type*;
	using const_pointer		= type const*;
	using reference			= type&;
	using const_reference	= type const&;

	Ref() = default;
	~Ref() { destroy(); }

	Ref(uint64 id, reference resource) :
		m_id{ id },
		m_ptr{ &resource }
	{
		m_ptr->reference();
	}

	Ref(Ref const& rhs) :
		m_id{ rhs.m_id },
		m_ptr{ rhs.m_ptr }
	{
		m_ptr->reference();
	}

	Ref& operator=(Ref const& rhs)
	{
		if (this != &rhs)
		{
			// If the current resource object already holds something, destroy it first before a reassignment.
			destroy();

			m_id = rhs.m_id;
			m_ptr = rhs.m_ptr;

			m_ptr->reference();
		}
		return *this;
	}

	Ref(Ref&& rhs) noexcept :
		m_id{ std::move(rhs.m_id) },
		m_ptr{ std::move(rhs.m_ptr) }
	{
		new (&rhs) Ref{};
	}

	Ref& operator=(Ref&& rhs) noexcept
	{
		if (this != &rhs)
		{
			m_id = std::move(rhs.m_id);
			m_ptr = std::move(rhs.m_ptr);

			new (&rhs) Ref{};
		}
		return *this;
	}

	auto operator->() const -> pointer		{ return m_ptr; }
	auto operator*() const	-> reference	{ return *m_ptr; }

	auto valid() const -> bool { return m_ptr != nullptr && m_ptr->valid(); }

	auto destroy() -> void
	{
		if (m_ptr &&
			std::cmp_equal(m_ptr->dereference(), 0))
		{
			type::destroy(*m_ptr, m_id);
		}
		m_ptr = nullptr;
		m_id = 0;
	}

	auto id() const -> uint64 { return m_id; }
private:
	uint64	m_id	= {};
	pointer m_ptr	= {};
};

struct Point
{
	int32 x, y;
};

struct Rect
{
	int32 left;
	int32 top;
	int32 width;
	int32 height;
};

struct Dimension
{
	int32 width, height;
};

struct OSEvent
{
	enum class Type
	{
		None = 0,
		Quit,
		Key,
		Window_Close,
		Window_Resize,
		Window_Move,
		Window_Minimized,
		Window_Maximized,
		Window_Fullscreen,
		Window_Focus,
		Mouse_Button,
		Mouse_Move,
		Mouse_Wheel,
		Drop_File,
		Char_Input,
		Max
	};
	void*	window;
	Type	event;
	union
	{
		struct { uint32 utf8; }					textInput;
		struct { int32 x, y; }					mouseMove;
		struct { float32 v, h; }				mouseWheelDelta;
		struct { bool down; uint32 button; }	mouseButton;
		struct { int32 x, y; }					winMove;
		struct { int32 width, height; }			winSize;
		struct { bool down; IOKey code; }		key;
		struct { void* handle; }				fileDrop;
		struct { bool gained; }					focus;
	};
};
}

#endif // !PLATFORM_MINIMAL_H
