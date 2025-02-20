#pragma once
#ifndef RENDER_PIPELINE_CACHE_HPP
#define RENDER_PIPELINE_CACHE_HPP

#include "gpu/gpu.hpp"
#include "gpu/shader_compiler.hpp"

namespace render
{
/**
* 1. Support hot reloading pipelines.
* 2. Cache SPIR-V files on disk.
* 3. Rebuild shaders when driver versions don't match.
* 4. Support multi-threaded pipeline building.
*/
struct PipelineCacheInfo
{
    // std::filesystem::path cachePath;
    // lib::array<lib::string> pipelines;
};

class PipelineCache
{
public:


private:
    // lib::ref<gpu::Device> m_device;
    // lib::map<lib::string, 
    // struct
    // {
    //     lib::array<gpu::pipeline> raster;
    //     lib::array<gpu::pipeline> compute;
    // } pipelines;
};
}

#endif