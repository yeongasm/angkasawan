#include "vulkan/vk_device.h"

namespace rhi
{

resource_type next_rhi_resource_type_id()
{
	static resource_type::value_type _id = 0;
	return resource_type{ ++(*const_cast<std::remove_cv_t<resource_type::value_type>*>(&_id)) };
}

Resource::Resource(APIContext* context, void* data, resource_type type_id) :
	/*state{ ResourceState::Ok },*/
	m_context{ context },
	m_type{ type_id },
	m_data{ data }
{}

Resource::Resource(Resource&& rhs) noexcept :
	/*state{ rhs.state },*/
	m_context{ rhs.m_context },
	m_type{ rhs.m_type },
	m_data{ rhs.m_data }
{
	new (&rhs) Resource{};
}

Resource& Resource::operator=(Resource&& rhs) noexcept
{
	if (this != &rhs)
	{
		/*state = std::move(rhs.state);*/
		m_context = std::move(rhs.m_context);
		m_type = std::move(rhs.m_type);
		m_data = std::move(rhs.m_data);

		new (&rhs) Resource{};
	}
	return *this;
}

auto Resource::valid() const -> bool
{ 
	return m_type != 0 && m_data != nullptr; 
}
auto Resource::type_id() const -> resource_type
{ 
	return m_type; 
}

}