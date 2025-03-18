#include "shader_compiler.hpp"

namespace gpu
{
auto ShaderCompileInfo::add_macro_definition(std::string_view key) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve(1_KiB);
	}
	auto start = defines.end();
	defines.append(key);
    // We have to add a null terminator to make our implementation compatible with Slang.
    defines.push_back('\0');
	macroDefinitions.emplace(std::string_view{ start.data(), static_cast<size_t>(defines.end() - start) }, std::string_view{});
}

auto ShaderCompileInfo::add_macro_definition(std::string_view key, std::string_view value) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve(10_KiB);
	}
	auto kstart = defines.end();
	defines.append(key);
    defines.push_back('\0');
	std::string_view k{ kstart.data(), key.size() };

	auto vstart = defines.end();
	defines.append(value);
    defines.push_back('\0');
	std::string_view v{ vstart.data(), value.size() };

	macroDefinitions.emplace(std::move(k), std::move(v));
}

auto ShaderCompileInfo::add_macro_definition(std::string_view key, uint32 value) -> void
{
	if (defines.capacity() == defines.insitu_capacity())
	{
		defines.reserve(10_KiB);
	}
	auto kstart = defines.end();
	defines.append(key);
    defines.push_back('\0');
	std::string_view k{ kstart.data(), key.size() };

	auto vstart = defines.end();
	defines.append("{}", value);
    defines.push_back('\0');
	std::string_view v{ vstart.data(), static_cast<std::string_view::size_type>(std::distance(vstart, defines.end())) };

	macroDefinitions.emplace(std::move(k), std::move(v));
}

auto ShaderCompileInfo::clear_macro_definitions() -> void
{
	macroDefinitions.clear();
	defines.clear();
}

auto ShaderCompileInfo::remove_macro_definition(std::string_view key) -> void
{
	macroDefinitions.erase(key);
}

auto ShaderCompiler::add_include_directory(std::string_view dir) -> void
{
	if (std::find(m_includeDirectories.begin(), m_includeDirectories.end(), dir) != m_includeDirectories.end())
	{
		m_includeDirectories.emplace_back(dir);
	}
}

auto ShaderCompiler::remove_include_directory(std::string_view dir) -> void
{
	if (auto it = std::find(m_includeDirectories.begin(), m_includeDirectories.end(), dir); it != m_includeDirectories.end())
	{
		m_includeDirectories.erase(it);
	}
}

auto ShaderCompiler::add_macro_definition(std::string_view definition) -> void
{
	m_macroDefinitions.try_insert(lib::string{ definition }, lib::string{});
}

auto ShaderCompiler::add_macro_definition(std::string_view key, std::string_view value) -> void
{
    if (!m_macroDefinitions.contains(key))
    {
        lib::string k{ key };
        // We have to add a null terminator to make our implementation compatible with Slang.
        k.push_back('\0');

        lib::string v{ value };
        v.push_back('\0');

        m_macroDefinitions.insert(std::move(k), std::move(v));
    }
}

auto ShaderCompiler::add_macro_definition(std::string_view key, uint32 value) -> void
{
    if (!m_macroDefinitions.contains(key))
    {
        lib::string k{ key };
        k.push_back('\0');

        lib::string v{ std::string_view{ "{}" }, value };
        v.push_back('\0');

        m_macroDefinitions.insert(std::move(k), std::move(v));
    }
}

auto ShaderCompiler::remove_macro_definition(std::string_view key) -> void
{
	m_macroDefinitions.erase(key);
}

auto ShaderCompiler::clear_macro_definitions() -> void
{
	m_macroDefinitions.clear();
}
}