#include "cmdline.hpp"

namespace core
{
namespace cmdline
{
constexpr auto Option::short_name() const -> std::string_view
{
	return m_name.s;
}

constexpr auto Option::long_name() const -> std::string_view
{
	return m_name.l;
}

constexpr auto Option::description() const -> std::string_view
{
	return m_description;
}

constexpr auto Option::type() const -> OptionValueType
{
	return m_type;
}

constexpr auto Option::group() const -> Group const*
{
	return m_pGroup;
}
}
}