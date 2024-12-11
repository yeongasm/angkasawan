#pragma once
#ifndef LIB_RESOURCE_HPP
#define LIB_RESOURCE_HPP

#include <atomic>
#include "common.hpp"

namespace lib
{
class ref_counted
{
public:
	ref_counted() = default;
	~ref_counted() = default;

	ref_counted(ref_counted const&) = delete;
	ref_counted(ref_counted&&) = delete;

	auto operator= (ref_counted const&) -> ref_counted& = delete;
	auto operator= (ref_counted&&) -> ref_counted& = delete;

	auto reference()		-> void		{ m_refCount.fetch_add(1, std::memory_order_relaxed); }
	auto dereference()		-> uint64	{ return m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1; }
	auto ref_count() const	-> uint64	{ return m_refCount.load(std::memory_order_acquire); }
private:
	std::atomic_uint64_t m_refCount = {};
};
}

#endif // !LIB_RESOURCE_HPP
