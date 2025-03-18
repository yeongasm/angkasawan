#include <filesystem>

#include "core.serialization/file.hpp"
#include "core.serialization/write_stream.hpp"

#include "lib/common.hpp"

#include "pipeline_cache.hpp"

namespace render
{
class PipelineCacheInfoFile : lib::non_copyable_non_movable
{
public:
    PipelineCacheInfoFile() = default;

    PipelineCacheInfoFile(core::sbf::File const& mmapFile)
    {
        from_mmap_file(mmapFile);
    }

    auto from_mmap_file(core::sbf::File const& mmapFile) -> bool
    {
        if (std::cmp_equal(mmapFile.size(), 0) ||
            std::cmp_equal(mmapFile.mapped_size(), 0))
        {
            return false;
        }

        std::byte* ptr = static_cast<std::byte*>(mmapFile.data());

        std::memcpy(&m_version, ptr, sizeof(gpu::Version));

        return true;
    }

    auto driver_version() const -> gpu::Version { return m_version; }
private:
    gpu::Version m_version = {};
};

PipelineCache::PipelineCache(gpu::Device& device, PipelineCacheInfo const& info, bool recompileShaders) : 
    m_device{ device },
    m_configuration{ info },
    m_rasterPipelines{ plf::limits{ 4, 32 } },
    m_computePipelines{ plf::limits{ 4, 32 } },
    RECOMPILE_SHADERS{ recompileShaders }
{}

auto PipelineCache::create(gpu::Device &device, PipelineCacheInfo const& info /*, FileSystem */) -> std::unique_ptr<PipelineCache>
{
    std::filesystem::path const cacheDir = [](PipelineCacheInfo const& info) -> std::filesystem::path
    {
        if (!info.cachePath.empty() ||
            std::filesystem::is_directory(info.cachePath))
        {
            return info.cachePath;
        }
        else
        {
            return "data/render/pipeline_cache/";   // Provide some default directory. Maybe can be done through preprocessor macros or something.
        }
    }(info);

    // Create the cache directory if it has not existed yet.
    if (!std::filesystem::exists(cacheDir))
    {
        std::filesystem::create_directories(cacheDir);
    }

    bool recompileShaders = false;

    // Get current driver version
    auto const version = device.info().driverVersion;

    std::filesystem::path const cacheFilePath = cacheDir / ".cacheinfo";

    if (std::filesystem::exists(cacheFilePath))
    {
        // Check if the driver version is the same with the one stored in cache.info
        PipelineCacheInfoFile const cacheInfo{ core::sbf::File{ cacheFilePath } };

        auto const cachedVersion = cacheInfo.driver_version();

        // If it does not exist, we clear the contents of the directory.
        if (version.major != cachedVersion.major ||
            version.minor != cachedVersion.minor ||
            version.patch != cachedVersion.patch)
        {
            std::filesystem::remove_all(cacheDir);

            recompileShaders = true;
        }
    }
    else
    {
        core::sbf::WriteStream cacheFileStream{ cacheFilePath };
        cacheFileStream.write(version);

        recompileShaders = true;
    }

    return std::unique_ptr<PipelineCache>{ new PipelineCache{ device, info, recompileShaders } };
}

auto PipelineCache::create_raster_pipeline(RasterPipelineInfo&& info) -> std::expected<Pipeline, std::string_view>
{
    // There are 2 instances where we will have to recompile the given pipeline.
    // 1. When RECOMPILE_SHADERS is true.
    // 2. When RECOMPILE_SHADERS is false but the pipeline does not exist in the cache.
    if (RECOMPILE_SHADERS)
    {

    }

    return {};
}

// auto PipelineCache::commit() -> void
// {

// }
}