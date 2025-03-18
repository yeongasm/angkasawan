#pragma once
#ifndef RENDER_PIPELINE_CACHE_HPP
#define RENDER_PIPELINE_CACHE_HPP

#include <filesystem>

#include "lib/function.hpp"
#include "gpu/gpu.hpp"
#include "gpu/shader_compiler.hpp"

namespace render
{
/*
* Not to be confused with the pipeline cache that's in the gpu.
* The PipelineCache is meant to compile shaders once into the API shader intermediary format (DXIL/SPIR-V) as long as the GPU's driver version does not change.
* Once the shaders are compiled into SPIR-V, the engine looks up into the cache's directory and serializes all the pipelines into the engine.
*
* In non release builds, we filewatch the shaders by default.
*/
struct RasterPipelineShaders
{
    std::optional<std::string_view> mesh;
    std::optional<std::string_view> vertex;
    std::optional<std::string_view> tesselationControl;
    std::optional<std::string_view> tesselationEvaluation;
    std::optional<std::string_view> fragment;
    std::optional<std::string_view> task;
};

struct ComputePipelineShaders
{
    std::string_view compute;
};

struct RasterPipelineInfo
{
    gpu::RasterPipelineInfo info;
    RasterPipelineShaders shaders;
    lib::map<lib::string, lib::string> preprocessorDefinitions;
    lib::array<std::filesystem::path> includePaths;
};

struct ComputePipelineInfo
{
    gpu::ComputePipelineInfo info;
    ComputePipelineShaders shaders;
    lib::map<lib::string, lib::string> preprocessorDefinitions;
    lib::array<std::filesystem::path> includePaths;
};

struct PipelineCacheInfo
{
    std::filesystem::path cachePath;
};

/*
* Stores a callback function that returns the the pipeline when we do a dereference / pointer access.
* This way we can return the type erased handles.
*
*/
class Pipeline
{
public:
    Pipeline() = default;
    ~Pipeline() = default;

    auto type() const -> gpu::PipelineType;

    template <typename Self>
    auto operator*(this Self&& self) -> auto&&
    {
        return std::forward_like<Self>(self.m_access());
    }

    template <typename Self>
    auto operator->(this Self&& self) -> auto&&
    {
        return std::forward_like<Self>(&self.m_access());
    }
private:
    friend class PipelineCache;

    template <typename AccessFn>
    Pipeline(gpu::PipelineType type, AccessFn&& fn) :
        m_access{ std::forward<AccessFn>(fn) },
        m_type{ type }
    {}

    using AccessFn = lib::function<gpu::Pipeline&(void), { .capacity = sizeof(uintptr_t) * 3 }>;

    AccessFn m_access;
    gpu::PipelineType m_type;
};

/*
* Hot reloading is done externally
*/
class PipelineCache : lib::non_copyable_non_movable
{
public:
    static auto create(gpu::Device& device, PipelineCacheInfo const& info /*, FileSystem */) -> std::unique_ptr<PipelineCache>;

    auto create_raster_pipeline(RasterPipelineInfo&& info) -> std::expected<Pipeline, std::string_view>;
    // auto create_compute_pipeline(ComputePipelineInfo&& info) -> std::expected<Pipeline, std::string_view>;
    // auto destroy_raster_pipeline(Pipeline& pipeline) -> void;
    // auto destroy_compute_pipeline(Pipeline& pipeline) -> void;
    
    /*
    * IDEA(afiq):
    * When creating a pipeline, we will not immediately write to disk.
    * Instead, store into some blob of memory and then only write the pipelines to disk when commit() is called.
    *
    * Motive? 
    * When shaders are edited in runtime and non-final builds, we don't pay the cost of I/O operations everytime a shader is updated.
    *
    */
    //auto commit() -> void;

private:
    PipelineCache(gpu::Device& device, PipelineCacheInfo const& info, bool recompileShaders);

    template <typename T>
    struct CachedPipeline
    {
        gpu::pipeline pipeline;
        T config;
    };

    using CachedRasterPipeline  = CachedPipeline<RasterPipelineInfo>;
    using CachedComputePipeline = CachedPipeline<ComputePipelineInfo>;

    [[maybe_unused]] gpu::Device& m_device;
    PipelineCacheInfo m_configuration;
    lib::hive<CachedRasterPipeline> m_rasterPipelines;
    lib::hive<CachedComputePipeline> m_computePipelines;
    bool const RECOMPILE_SHADERS;
    /*FileSystem*/
};
}

#endif  //  !RENDER_PIPELINE_CACHE_HPP