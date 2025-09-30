#pragma once
#ifndef RENDER_PIPELINE_CACHE_HPP
#define RENDER_PIPELINE_CACHE_HPP

#include <filesystem>

#include "gpu/gpu.hpp"
#include "gpu/shader_compiler.hpp"

#include "pipeline_definitions.hpp"

namespace render
{
struct PipelineCacheInfo
{
    std::filesystem::path cachePath;
    size_t scratchBufferSize;
};

struct PipelineCacheLoadDefinitionsInfo
{
    std::span<RasterPipelineDefinition const*> raster;
    std::span<ComputePipelineDefinition const*> compute;
};

struct PipelineShaderCompileInfo
{
    gpu::ShaderType type;

    /**
	* \brief 0 - no optimization
	* \brief 1 - performance
	* \brief 2 - size
	* \brief 3 - performance and size
	*/
    uint32 optimizationLevel;

    /**
	* \brief Compiled with debug symbol if 1.
	*/
    uint32 enableDebugSymbol : 1;

    /**
	* \brief 
    * Do not use. Use add_macro_definition rather than appending manually.
    * If required, provide semicolon ";" separated keys and key-values.
    * Key-values should have an equals "=" between them.
    *
    * For e.g,  key only -> FOO;BAR
    *           key-value -> FOO;BAR=BAZ;
	*/
    std::string defines;

    auto add_macro_definition(std::string_view key) -> void;
    auto add_macro_definition(std::string_view key, std::string_view value) -> void;
};

/*
* Hot reloading is done externally
*/
class PipelineCache : lib::non_copyable_non_movable
{
public:

    static auto create(gpu::Device& device, PipelineCacheInfo const& info /*, FileSystem */) -> std::unique_ptr<PipelineCache>;

    auto shader_compiler() -> gpu::ShaderCompiler&;

    /*
    * Attempts to load shader binaries that are written to disk.
	* If the function returns false, that means the gpu driver version don't match and we'll need to recompile shaders.
    */
    auto load_cache(PipelineCacheLoadDefinitionsInfo const& definitions) -> bool;

    /*
    * Serializes the shaders in the pipeline definition to disk and adds the pipeline into the cache.
    */
    auto cache_pipeline(RasterPipelineDefinition const& definition, std::span<PipelineShaderCompileInfo const*> compileInfo) -> std::expected<gpu::Pipeline, std::string>;

    /*
    * Serializes the shaders in the pipeline definition to disk and adds the pipeline into the cache.
    */
    auto cache_pipeline(ComputePipelineDefinition const& definition, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::Pipeline, std::string>;

    auto remove_pipeline(std::string_view uri) -> void;

    auto get_pipeline(std::string_view uri) -> std::expected<gpu::Pipeline, std::string>;
    auto contains(std::string_view uri) -> bool;

private:
    PipelineCache(gpu::Device& device, PipelineCacheInfo const& info);

    using UriToPipelineMap = ankerl::unordered_dense::map<std::string_view, plf::colony<gpu::Pipeline>::iterator>;

    gpu::Device& m_device;
    PipelineCacheInfo m_configuration;
    std::unique_ptr<gpu::ShaderCompiler> m_shaderCompiler;
    plf::colony<gpu::Pipeline> m_pipelines;
    UriToPipelineMap m_uriToPipeline;

    auto load_pipeline(RasterPipelineDefinition const& definition) -> void;
    auto load_pipeline(ComputePipelineDefinition const& definition) -> void;
    auto try_compile_shader(std::filesystem::path const& path, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::Shader, std::string>;
    auto compile_shader(std::string_view path, std::filesystem::path const& output, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::Shader, std::string>;
    auto compilation_error(std::string_view message, bool sourceFileOpen, bool sourceCodeExist) -> std::string;
    auto serialize_shader(std::filesystem::path const& path, gpu::CompiledShaderInfo const& compiledInfo) -> void;
    auto deserialize_shader(std::filesystem::path const& path) -> gpu::Shader;
};
}

#endif  //  !RENDER_PIPELINE_CACHE_HPP