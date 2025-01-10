#include "cmdline.hpp"

namespace core
{
namespace cmdline
{
/**
* The command is not committed until the end of the argument string.
* Anytime there is a -h or --help, the program cancels the command and displays the help string.
* For options that can take in multiple values, either separate each value with a "," or declare the option again.
* If the option does not take in multiple values but is declared as such in the command line's argument list, override the previous value.
*/

CommandLine::CommandLine(int argc, char** argv) :
	m_arguments(static_cast<size_t>(argc))
{
	parse(argc, argv);
}

auto CommandLine::parse(int argc, char** argv) -> void
{
	lib::array<std::string_view> arguments{ argv, argv + argc };

	// Check the validity of the command line arguments.
	for (auto&& arg : arguments)
	{
		uint32 const numEquals = static_cast<uint32>(std::count(arg.begin(), arg.end(), '='));

		// If an option has an assignment, we split the string.
		if (std::cmp_not_equal(numEquals, 0))
		{
			auto it = std::find(arg.begin(), arg.end(), '=');
			// Store option variable.
			m_arguments.emplace_back(arg.begin(), it);

			// Store option variable value.
			m_arguments.emplace_back(it + 1, arg.end());
		}
		else
		{
			m_arguments.emplace_back(arg.begin(), arg.end());
		}
	}
}

auto CommandLine::program_name() const -> std::filesystem::path
{
	auto const& programName = m_arguments[0];
	return std::filesystem::path{ programName.begin(), programName.end() };
}

auto CommandLine::args() const -> std::span<std::string_view>
{
	return std::span{ m_arguments.data(), m_arguments.size() };
}

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

constexpr Group::Group(ProgramOptions& po, std::string_view description) :
	m_description{ description }
{
	po.m_groups.push_back(this);
}

constexpr auto Group::description() const -> std::string_view
{
	return m_description;
}
 
ProgramOptions::ProgramOptions(std::string_view description) :
	m_groups{},
	m_description{ description }
{
}

ProgramOptions::ProgramOptions(CommandLine const& cl, std::string_view description) :
	m_groups{},
	m_description{ description }
{
	parse(cl.args());
}

auto ProgramOptions::parse(CommandLine const& cl) -> bool
{
	return parse(cl.args());
}

auto ProgramOptions::parse(std::span<std::string_view> args) -> bool
{
	constexpr std::string_view HELP_TEXT[] = { "--help", "-h" };

	Option* opt = nullptr;

	if (std::cmp_equal(args.size(), 1))
	{
		print_help();

		return false;
	}

	for (size_t i = 0; i < args.size(); ++i)
	{
		auto&& arg = args[i];

		if (std::string_view& argsv = static_cast<std::string_view&>(arg); argsv == HELP_TEXT[0] || argsv == HELP_TEXT[1])
		{
			print_help();

			return false;
		}

		if (opt != nullptr)
		{
			if (arg[0] == '-' && !_set_option_value(*opt, {})) // We don't need to check for long options because they still contain a "-" as the first character.
			{
				return false;
			}
			else if (arg[0] != '-' && !_set_option_value(*opt, arg))
			{
				return false;
			}
			opt = nullptr;
		}

		if (auto&& option = _option_for(arg); option)
		{
			opt = option;
		}
	}

	if (opt != nullptr && !_set_option_value(*opt, {}))
	{
		return false;
	}

	return true;
}

auto ProgramOptions::print_help() const -> void
{
	lib::string nameBuffer{ 128 };

	fmt::print("{}\n\n", m_description);
	fmt::print("Basic usage\n");

	for (auto const option : m_options)
	{
		if (option->group() != nullptr)
		{
			continue;
		}

		fmt::print("{:{}}", "", 4);

		std::string_view shortName = static_cast<std::string_view>(option->short_name());
		std::string_view longName = static_cast<std::string_view>(option->long_name());

		if (!shortName.empty())
		{
			nameBuffer.append(shortName);
			nameBuffer.append(std::string_view{ ", " });
		}

		if (!longName.empty())
		{
			nameBuffer.append(longName);
		}

		fmt::print("{:<30}", nameBuffer.c_str());
		fmt::print("{}\n", option->description());

		nameBuffer.clear();
	}

	fmt::print("\n");

	for (auto const group : m_groups)
	{
		fmt::print("{}\n", group->description());

		for (auto const option : m_options)
		{
			if (option->group() != group)
			{
				continue;
			}

			fmt::print("{:{}}", "", 4);

			std::string_view shortName = static_cast<std::string_view>(option->short_name());
			std::string_view longName = static_cast<std::string_view>(option->long_name());

			if (!shortName.empty())
			{
				nameBuffer.append(shortName);
				nameBuffer.append(std::string_view{ ", " });
			}

			if (!longName.empty())
			{
				nameBuffer.append(longName);
			}

			fmt::print("{:<30}", nameBuffer.c_str());
			fmt::print("{}\n", option->description());

			nameBuffer.clear();
		}
	}
}

auto ProgramOptions::description() const -> std::basic_string_view<char const>
{
	return { m_description.data(), m_description.size() };
}

auto ProgramOptions::_option_for(std::string_view arg) const -> Option*
{
	for (auto&& option : m_options)
	{
		if (option->short_name() == arg ||
			option->long_name() == arg)
		{
			return option;
		}
	}

	return nullptr;
}

auto ProgramOptions::_set_option_value(Option& option, std::string_view value) -> bool
{
	bool const arrayLike = (option.type() & OptionValueType::List) != OptionValueType::None;

	if (arrayLike)
	{
		auto const type = option.type() ^ OptionValueType::List;

		switch (type)
		{
		case OptionValueType::Int:
		{
			auto&& var = option.value<lib::array<int32>>();
			int32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var.push_back(val);
			}
			break;
		}
		case OptionValueType::Uint:
		{
			auto&& var = option.value<lib::array<uint32>>();
			uint32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var.push_back(val);
			}
			break;
		}
		case OptionValueType::Float:
		{
			auto&& var = option.value<lib::array<float32>>();
			float32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var.push_back(val);
			}
			break;
		}
		case OptionValueType::String:
		{
			auto&& var = option.value<lib::array<lib::string>>();
			var.emplace_back(value.data(), value.size());
			break;
		}
		case OptionValueType::Path:
		{
			auto&& var = option.value<lib::array<std::filesystem::path>>();
			var.emplace_back(value.data(), value.data() + value.size());
			break;
		}
		default:
			break;
		}
	}
	else
	{
		switch (option.type())
		{
		case OptionValueType::Boolean:
		{
			lib::string v{ value.data(), value.size() };
			auto&& var = option.value<bool>();
			if (!value.empty())
			{
				std::transform(v.begin(), v.end(), v.begin(), [](uint8 ch) -> uint8 { return static_cast<uint8>(std::tolower(ch)); });
			}
			var = (value.empty() || v == "true" || v == "1");
			break;
		}
		case OptionValueType::Int:
		{
			auto&& var = option.value<int32>();
			int32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var = val;
			}
			break;
		}
		case OptionValueType::Uint:
		{
			auto&& var = option.value<uint32>();
			uint32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var = val;
			}
			break;
		}
		case OptionValueType::Float:
		{
			auto&& var = option.value<float32>();
			float32 val = 0;
			if (auto res = std::from_chars(value.data(), value.data() + value.size(), val);
				res.ec != std::errc::invalid_argument && res.ec != std::errc::result_out_of_range)
			{
				var = val;
			}
			break;
		}
		case OptionValueType::String:
		{
			auto&& var = option.value<lib::string>();
			var = static_cast<std::string_view&>(value);
			break;
		}
		case OptionValueType::Path:
		{
			auto&& var = option.value<std::filesystem::path>();
			var = static_cast<std::string_view&>(value);
			break;
		}
		default:
			break;
		}
	}
	return true;
}

}
}