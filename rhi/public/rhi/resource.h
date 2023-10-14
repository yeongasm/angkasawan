#pragma once
#ifndef RHI_RESOURCE_H
#define RHI_RESOURCE_H

#include "common.h"

namespace rhi
{
using resource_type = lib::named_type<uint32, struct _Resource_Type_>;

inline RHI_API resource_type next_rhi_resource_type_id();

template <typename T>
inline RHI_API const resource_type resource_type_id_v{ next_rhi_resource_type_id() };

struct APIContext;

// Managed move only resources that are returned by the Device.
class Resource
{
public:
	ResourceState state;

	RHI_API Resource()	= default;
	RHI_API ~Resource() = default;

	RHI_API Resource(APIContext* context, void* data, resource_type type_id);

	Resource(Resource const&)				= delete;
	Resource& operator=(Resource const&)	= delete;

	RHI_API Resource(Resource&&) noexcept;
	RHI_API Resource& operator=(Resource&&) noexcept;

	RHI_API auto valid() const -> bool;
	RHI_API auto type_id() const -> resource_type;

	template <typename T>
	auto as() const -> T&
	{
		ASSERTION(valid() && "Object state is not valid. This will cause a null reference and will result in an error!");
		ASSERTION(m_type == resource_type_id_v<T> && "Object is being casted to a different type. This is an error!");
		return *static_cast<T*>(m_data);
	}
protected:
	friend struct APIContext;
	APIContext* m_context;
	resource_type m_type;	// Type ID of the API specific object's type.
	void* m_data;			// Stores the API specific implementation of the data.
};
}

#endif // !RHI_RESOURCE_H
