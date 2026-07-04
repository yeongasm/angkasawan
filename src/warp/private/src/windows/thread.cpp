#include <string>
#include "core.platform/platform_header.hpp"
#include "thread.hpp"

namespace warp
{
namespace detail
{
auto thread::hardware_concurrency() -> uint32
{
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    std::vector<std::byte> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());

	uint32 physicalCores = 0;

    GetLogicalProcessorInformationEx(RelationProcessorCore, info, &length);

	BYTE* ptr = reinterpret_cast<BYTE*>(buffer.data());
	BYTE* end = ptr + length;

	while (ptr < end)
	{
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX current = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

		if (current->Relationship == RelationProcessorCore)
		{
			physicalCores++;
		}

		ptr += current->Size;
	}

	return physicalCores;
}

auto thread::logical_concurrency() -> uint32
{
	SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
	return static_cast<uint32>(sysInfo.dwNumberOfProcessors);
}

auto thread::_apply_thread_info(std::jthread& thread, thread_info const& info) -> void
{
	HANDLE threadHandle = static_cast<HANDLE>(thread.native_handle());
	if (!info.name.empty())
	{
		std::wstring const utf16Name{ info.name.begin(), info.name.end() };
		SetThreadDescription(threadHandle, utf16Name.c_str());
	}

	// Set thread affinity
	if (info.cpuCore != std::numeric_limits<uint32>::max())
	{
		SetThreadAffinityMask(threadHandle, static_cast<DWORD_PTR>(1u << static_cast<uint64>(info.cpuCore)));
	}
}
}
}