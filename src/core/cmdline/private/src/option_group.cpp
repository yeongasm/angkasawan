#include "cmdline.hpp"

namespace core
{
namespace cmdline
{
constexpr Group::Group(ProgramOptions& po, std::string_view description) :
	m_description{ description }
{
	po.m_groups.push_back(this);
}

constexpr auto Group::description() const -> std::string_view
{
	return m_description;
}
}
}