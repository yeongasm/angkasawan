#pragma once
#ifndef MAKESBF_IMAGIFY_JOB_HPP
#define MAKESBF_IMAGIFY_JOB_HPP

#include <filesystem>
#include "core.serialization/write_stream.hpp"

namespace makesbf
{
struct ImagifyJobInfo
{
    std::filesystem::path input;
	std::filesystem::path output;
	size_t writeStreamBufferCapacity = 5_MiB;
	lib::allocator<std::byte> allocator = {};
	uint32 mipLevels = 4u;
};

class MakeSbf;

class ImagifyJob
{
public:
    ImagifyJob(MakeSbf& tool, ImagifyJobInfo const& info);

	auto execute() -> std::optional<std::string_view>;
	auto job_info() const -> ImagifyJobInfo const&;
private:
    MakeSbf& m_tool;
    ImagifyJobInfo m_info;
};
}

#endif // !MAKESBF_IMAGIFY_JOB_HPP