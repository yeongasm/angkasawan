#pragma once
#ifndef CORE_CMDLINE_CMDLINE_HPP
#define CORE_CMDLINE_CMDLINE_HPP

#include <filesystem>
#include <string_view>
#include <span>

#include "lib/bit_mask.hpp"
#include "lib/string.hpp"
#include "lib/array.hpp"

namespace core
{
namespace cmdline
{
enum class OptionValueType : uint8
{
	None	= 0,
	Boolean = 0x01,
	Int		= 0x02,
	Uint	= 0x04,
	Float	= 0x08,
	String	= 0x10,
	Path	= 0x20,
	List	= 0x40
};

template <typename T>
concept acceptable_option_value_type_v = (
	std::same_as<bool, T>
	or std::same_as<int32, T>
	or std::same_as<uint32, T>
	or std::same_as<float32, T>
	or std::same_as<lib::string, T>
	or std::same_as<std::filesystem::path, T>
);

template <typename T>
constexpr bool is_string_like_v = requires (T s)
{
	typename T::const_pointer;
	{ s.c_str() } -> std::same_as<typename T::const_pointer>;
	requires requires { std::same_as<typename T::value_type, char> || std::same_as<typename T::value_type, wchar_t>; };
};

template <typename T>
constexpr bool is_container_like_v = requires (T c)
{
	{ c.data() } -> std::same_as<typename T::pointer>;
	{ c.size() } -> std::convertible_to<size_t>;
};

template <typename T>
consteval auto is_option_value_type() -> bool
{
	if constexpr (is_container_like_v<T> && !is_string_like_v<T>)
	{
		using type = typename T::value_type;
		return acceptable_option_value_type_v<type>;
	}
	else
	{
		return acceptable_option_value_type_v<T>;
	}
}

namespace detail
{
template <typename>
struct option_value_type
{
	static constexpr OptionValueType type = OptionValueType::None;
};

template <>
struct option_value_type<bool>
{
	static constexpr OptionValueType type = OptionValueType::Boolean;
};

template <>
struct option_value_type<int32>
{
	static constexpr OptionValueType type = OptionValueType::Int;
};

template <>
struct option_value_type<uint32>
{
	static constexpr OptionValueType type = OptionValueType::Uint;
};

template <>
struct option_value_type<float32>
{
	static constexpr OptionValueType type = OptionValueType::Float;
};

template <>
struct option_value_type<lib::string>
{
	static constexpr OptionValueType type = OptionValueType::String;
};

template <>
struct option_value_type<std::filesystem::path>
{
	static constexpr OptionValueType type = OptionValueType::Path;
};
}

class CommandLine : public lib::non_copyable_non_movable
{
public:
	CommandLine() = default;
	CommandLine(int argc, char** argv);

	auto parse(int argc, char** argv) -> void;
	auto program_name() const -> std::filesystem::path;
	auto args() const -> std::span<std::string_view>;
private:
	lib::array<std::string_view> m_arguments = {};
};

class ProgramOptions : public lib::non_copyable_non_movable
{
public:
	ProgramOptions(std::string_view description);
	ProgramOptions(CommandLine const& cl, std::string_view description);

	auto parse(CommandLine const& cl) -> bool;
	auto parse(std::span<std::string_view> args) -> bool;
	auto print_help() const -> void;
	auto description() const -> std::basic_string_view<char const>;
private:
	friend class Option;
	friend class Group;

	lib::array<Group*>	m_groups;
	lib::array<Option*> m_options;
	std::string_view	m_description;

	auto _option_for(std::string_view arg) const -> Option*;
	auto _set_option_value(Option& option, std::string_view value) -> bool;
};

class Group : public lib::non_copyable_non_movable
{
public:
	constexpr Group(ProgramOptions& po, std::string_view description);

	constexpr auto description() const -> std::string_view;
private:
	std::string_view m_description;
};

class Option : lib::non_copyable_non_movable
{
public:
	template <typename T>
	constexpr Option(ProgramOptions& po, std::string_view shortName, std::string_view longName, std::string_view description, T& bindTo, Group* pGroup = nullptr) requires (is_option_value_type<T>()) :
		m_name{ .s = shortName, .l = longName },
		m_description{ description },
		m_var{ &bindTo },
		m_pGroup{ pGroup },
		m_type{}
	{
		po.m_options.push_back(this);

		if constexpr (is_container_like_v<T> && !is_string_like_v<T>)
		{
			static_assert(detail::option_value_type<typename T::value_type>::type != OptionValueType::Boolean, "Please don't use an array of bools. Figure out something else!");

			m_type |= OptionValueType::List;
			m_type |= detail::option_value_type<typename T::value_type>::type;
		}
		else
		{
			m_type |= detail::option_value_type<T>::type;
		}
	}

	template <typename T>
	constexpr auto value() -> std::decay_t<T>& requires (is_option_value_type<std::decay_t<T>>())
	{
		using type = std::decay_t<T>;

		if constexpr (is_container_like_v<type> && !is_string_like_v<T>)
		{
			if ((m_type & (detail::option_value_type<typename type::value_type>::type | OptionValueType::List)) == OptionValueType::None)
			{
				ASSERTION(false && "Types do not match! Terminating program.");
				std::terminate();
			}
		}
		else
		{
			if ((m_type & detail::option_value_type<type>::type) == OptionValueType::None)
			{
				ASSERTION(false && "Types do not match! Terminating program.");
				std::terminate();
			}
		}
		return *static_cast<type*>(m_var);
	}

	constexpr auto short_name() const -> std::string_view;
	constexpr auto long_name() const -> std::string_view;
	constexpr auto description() const -> std::string_view;
	constexpr auto type() const -> OptionValueType;
	constexpr auto group() const -> Group const*;
private:
	struct
	{
		std::string_view s;
		std::string_view l;
	} m_name;
	std::string_view m_description;
	void*	m_var;
	Group*	m_pGroup;
	OptionValueType m_type;
};
}
}

#endif // !CORE_CMDLINE_CMDLINE_HPP
