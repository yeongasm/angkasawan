#pragma once
#ifndef CORE_SHARED_ENGINE_COMMON_H
#define CORE_SHARED_ENGINE_COMMON_H

#include "platform_interface/platform_interface.h"
#include "containers/string.h"

#define MAKE_SINGLETON(Object)					\
	Object(const Object&) = delete;				\
	Object(Object&&) = delete;					\
	Object& operator=(const Object&) = delete;	\
	Object& operator=(Object&&) = delete;

namespace core
{

enum class ErrorCode
{
	Ok = 0,
	Window_Invalid,
	Window_Root_Exist,
	Window_Invalid_Handle,
	Window_Invalid_Window,
	Window_Multiple_Windows_Not_Allowed,
	Module_Invalid,
	Module_Already_Exist,
	Module_Non_Existent,
	Module_Not_Enough_Memory,
	Bootstrap_Could_Not_Load_File,
	Bootstrap_File_Error,
	Bootstrap_Missing_Configuration,
	Bootstrap_Invalid_Config_Value,
	Bootstrap_Empty_Modules_List,
	Bootstrap_Cyclic_Module_Dependency_Detected
};

struct ModuleInfo;

struct ModuleManagerInfo
{
	size_t		memory;
	size_t		numSystemsHint;
	size_t		stack;
	ModuleInfo* modules;
	size_t		numModules;
};

enum class ModuleType
{
	None,
	Sync,
	Async
};

struct ModuleInfo
{
	ftl::String64	name;
	ftl::String64	dll;
	size_t			stack;
	int32			order;
	ModuleType		type;

	bool operator==(const ModuleInfo& rhs) const { return name == rhs.name; }
	bool operator!=(const ModuleInfo& rhs) const { return name != rhs.name; }
};

struct EngineInitialization
{
	ftl::UniString128	applicationName;
	Point				position;
	Dimension			dimension;
	ModuleManagerInfo	moduleManager;
	bool				fullscreen;
	bool				allowFileDrop;
};

enum class EngineState
{
	Starting = 0,
	Running,
	Minimized,
	Suspended,
	Terminating
};

enum class LogMsgType
{
	Notice,
	Warning,
	Error,
	None
};

}

namespace std
{
	template <>
	struct hash<core::ModuleInfo>
	{
		size_t operator()(const core::ModuleInfo& info) const noexcept
		{
			using T = ftl::String64::Type;
			std::hash<T> hasher;
			size_t result = 0;
			for (const T& c : info.name)
			{
				result += hasher(c);
			}
			return result;
		}
	};
}

#endif // !CORE_SHARED_ENGINE_COMMON_H
