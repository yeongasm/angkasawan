#pragma once
#ifndef SANDBOX_SETTINGS_HPP
#define SANDBOX_SETTINGS_HPP

#include <filesystem>

namespace sandbox
{
struct Settings
{
	inline static std::filesystem::path workspaceDir;
};
}

#endif // !SANDBOX_SETTINGS_HPP