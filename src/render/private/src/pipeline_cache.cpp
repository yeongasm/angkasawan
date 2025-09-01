#include <ranges>
#include <expected>
#include <filesystem>
#include <utility>

#include "core.serialization/file.hpp"
#include "core.serialization/sbf_header.hpp"
#include "core.serialization/write_stream.hpp"

#include "gpu/common.hpp"
#include "lib/common.hpp"

#include "pipeline_cache.hpp"

namespace render
{
constexpr std::string_view SHADER_EXTENSION_NAME[] = {
    "vx.cash",
    "px.cash",
    "gm.cash",
    "tc.cash",
    "te.cash",
    "cp.cash",
    "ts.cash",
    "ms.cash",
    "rg.cash",
    "ah.cash",
    "ch.cash",
    "rm.cash",
    "ix.cash",
};

auto PipelineShaderCompileInfo::add_macro_definition(std::string_view key) -> void
{
    defines.push_back(';');
    defines.append(key);
}

auto PipelineShaderCompileInfo::add_macro_definition(std::string_view key, std::string_view value) -> void
{
    defines.push_back(';');
    defines.append(key);
    defines.push_back('=');
    defines.append(value);
} 

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

struct SerializedShaderBinaryHeader
{
    static constexpr uint32 TAG = 'NIBS'; // SBIN - Shader Binary

    core::sbf::SbfDataDescriptor descriptor = { .tag = TAG };
    gpu::ShaderType type;
    uint32 nameSize;
    uint32 entryPointSize;
    uint32 sizeUint32;
};

class SerializedShaderBinary : lib::non_copyable_non_movable
{
public:
    SerializedShaderBinary() = default;

    SerializedShaderBinary(core::sbf::File const& mmapFile)
    {
        from_mmap_file(mmapFile);
    }

    auto from_mmap_file(core::sbf::File const& mmapFile) -> bool
    {
        if (std::cmp_equal(mmapFile.size(), 0) ||
            std::cmp_equal(mmapFile.mapped_size(), 0) ||
            m_head != nullptr)
        {
            return false;
        }

        std::byte const* ptr = static_cast<std::byte*>(mmapFile.data());
        
        if (auto next = static_cast<std::byte const*>(core::sbf::check_header(ptr)))
        {
            m_head = ptr;

            if (core::sbf::check_section_header<SerializedShaderBinaryHeader>(next) != nullptr)
            {
                return true;
            }
        }
        
        return false;
    }

    auto compiled_info() const -> gpu::CompiledShaderInfo
    {
        gpu::CompiledShaderInfo out;

        auto header = static_cast<SerializedShaderBinaryHeader const*>(core::sbf::check_header(m_head));
        auto ptr = static_cast<std::byte const*>(core::sbf::check_section_header<SerializedShaderBinaryHeader>(header));

        out.type = header->type;

        if (std::cmp_not_equal(header->nameSize, 0))
        {
            out.name = std::string_view{ std::bit_cast<char*>(ptr), header->nameSize };
            ptr += header->nameSize;
        }

        if (std::cmp_not_equal(header->entryPointSize, 0))
        {
            out.entryPoint = std::string_view{ std::bit_cast<char*>(ptr), header->entryPointSize };
            ptr += header->entryPointSize;
        }

        if (std::cmp_not_equal(header->sizeUint32, 0))
        {
            out.binaries = std::span{ std::bit_cast<uint32*>(ptr), header->sizeUint32 };
        }

        return out;
    }

private:
    void const* m_head = nullptr;
};

PipelineCache::PipelineCache(gpu::Device& device, PipelineCacheInfo const& info) : 
    m_device{ device },
    m_configuration{ info },
    m_shaderCompiler{},
    m_pipelines{ plf::limits{ 8, 64 } },
    m_uriToPipeline{}
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
            return "data/render/";   // Default directory
        }
    }(info);

    // Create the cache directory if it has not existed yet.
    if (!std::filesystem::exists(cacheDir))
    {
        std::filesystem::create_directories(cacheDir);
    }

    // Get current driver version
    auto const version = device.info().driverVersion;

    std::filesystem::path const cacheFilePath = cacheDir / ".cacheinfo";

    // The first check is to see if the driver version matches.
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
            // The empty folder will trigger a recompile on all shaders inside of the pipeline cache.
            std::filesystem::remove_all(cacheDir);
        }
    }

    return std::unique_ptr<PipelineCache>{ new PipelineCache{ device, { .cachePath = cacheDir, .scratchBufferSize = 0 } } };
}

auto PipelineCache::shader_compiler() -> gpu::ShaderCompiler&
{
    // lazily initialize the shader compiler.
    if (!m_shaderCompiler)
    {
        if (auto res = gpu::ShaderCompiler::create(); res)
        {
            m_shaderCompiler = std::move(res.value());
        }
    }
    return *m_shaderCompiler;
}

auto PipelineCache::load_cache([[maybe_unused]] PipelineCacheLoadDefinitionsInfo const& definitions) -> bool
{
	std::filesystem::path const cacheFilePath = m_configuration.cachePath / ".cacheinfo";

    if (!std::filesystem::exists(cacheFilePath))
    {
		// Cache the current gpu driver version.
		auto const version = m_device.info().driverVersion;

		core::sbf::WriteStream cacheFileStream{ cacheFilePath };
		cacheFileStream.write(version);

        return false;
    }

    // Iterate over every pipeline definition. For each definition, we deserialize each shader and create the pipeline object. 
    for (auto definition : definitions.raster)
    {
        load_pipeline(*definition);
    }

    for (auto definition : definitions.compute)
    {
        load_pipeline(*definition);
    }

    return true;
}

auto PipelineCache::cache_pipeline(RasterPipelineDefinition const& definition, std::span<PipelineShaderCompileInfo const*> compileInfo) -> std::expected<gpu::pipeline, std::string>
{
    if ((definition.shaderPaths.vertex.empty() /*|| definition.shaderPaths.mesh.empty()*/) &&
        definition.shaderPaths.pixel.empty())
    {
        return std::unexpected{ "Either a vertex | mesh shader and a pixel shader is required for a rasterization pipeline." };
    }

    if (definition.uri.empty())
    {
        return std::unexpected{ "Pipeline URI cannot be empty." };
    }

    std::expected<gpu::shader, std::string> results[2];

    results[0] = try_compile_shader(definition.shaderPaths.vertex, *compileInfo[0]);
    results[1] = try_compile_shader(definition.shaderPaths.pixel, *compileInfo[1]);

    auto&& [vs, ps] = results;

    if (!vs || !ps)
    {
        for (auto&& result : results)
        {
            if (!result)
            {
                return std::unexpected{ std::move(result.error()) };
            }
        }
    }

    auto pipeline = gpu::Pipeline::from(
        m_device,
        {
            .vertexShader = *vs,
            .pixelShader = *ps,
        },
        {
            .name = std::string{ definition.info.name },
            .colorAttachments = lib::array<gpu::ColorAttachment>(definition.info.colorAttachments.data(), definition.info.colorAttachments.data() + definition.info.numColorAttachments),
            .depthAttachmentFormat = definition.info.depthAttachmentFormat,
	        .stencilAttachmentFormat = definition.info.stencilAttachmentFormat,
            .rasterization = definition.info.rasterization,
            .depthTest = definition.info.depthTest,
            .topology = definition.info.topology,
            .pushConstantSize = definition.info.pushConstantSize,
        }
    );

    if (pipeline.valid())
    {
        // Remove the old pipeline only when the new pipeline has been successfully created.
        if (m_uriToPipeline.contains(definition.uri))
        {
            auto const it = m_uriToPipeline[definition.uri];
            m_pipelines.erase(it);
            //m_uriToPipeline.erase(definition.uri);    // I guess we don't have to delete the object, simply reassign it. Not sure...
        }

        auto it = m_pipelines.emplace(pipeline);
        m_uriToPipeline[definition.uri] = it;
    }

    return pipeline;
}

auto PipelineCache::cache_pipeline(ComputePipelineDefinition const& definition, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::pipeline, std::string>
{
    if (definition.shaderPaths.compute.empty())
    {
        return std::unexpected{ "A compute shader is required for a compute pipeline." };
    }

    if (definition.uri.empty())
    {
        return std::unexpected{ "Pipeline URI cannot be empty." };
    }

    auto result = try_compile_shader(definition.shaderPaths.compute, compileInfo);

    if (!result)
    {
        return std::unexpected{ std::move(result.error()) };
    }

    auto pipeline = gpu::Pipeline::from(
        m_device,
        *result,
        {
            .name = std::string{ definition.info.name },
            .pushConstantSize = definition.info.pushConstantSize
        }
    );

    if (pipeline.valid())
    {
        if (m_uriToPipeline.contains(definition.uri))
        {
            auto const it = m_uriToPipeline[definition.uri];
            m_pipelines.erase(it);
        }

        auto it = m_pipelines.emplace(pipeline);
        m_uriToPipeline[definition.uri] = it;
    }

    return pipeline;
}

auto PipelineCache::remove_pipeline(std::string_view uri) -> void
{
    if (m_uriToPipeline.contains(uri))
    {
        auto const it = m_uriToPipeline[uri];
        m_pipelines.erase(it);
        m_uriToPipeline.erase(uri);
    }
};

auto PipelineCache::get_pipeline(std::string_view uri) -> std::expected<gpu::pipeline, std::string>
{
    if (!m_uriToPipeline.contains(uri))
    {
        return std::unexpected{ fmt::format("Could not get pipeline with uri - {}", uri) };
    }
    return *m_uriToPipeline[uri];
}

auto PipelineCache::contains(std::string_view uri) -> bool
{
    return m_uriToPipeline.contains(uri);
}

auto PipelineCache::load_pipeline(RasterPipelineDefinition const& definition) -> void
{
    std::filesystem::path vertexShaderPath  = m_configuration.cachePath / std::filesystem::path{ definition.shaderPaths.vertex }.stem().replace_extension(SHADER_EXTENSION_NAME[std::to_underlying(gpu::ShaderType::Vertex)]);
    std::filesystem::path pixelShaderPath   = m_configuration.cachePath / std::filesystem::path{ definition.shaderPaths.pixel }.stem().replace_extension(SHADER_EXTENSION_NAME[std::to_underlying(gpu::ShaderType::Pixel)]);

    auto vs = deserialize_shader(vertexShaderPath);
    auto ps = deserialize_shader(pixelShaderPath);

    auto pipeline = gpu::Pipeline::from(
        m_device,
        {
            .vertexShader = vs,
            .pixelShader = ps
        },
        {
            .name = std::string{ definition.info.name },
            .colorAttachments = lib::array<gpu::ColorAttachment>(definition.info.colorAttachments.data(), definition.info.colorAttachments.data() + definition.info.numColorAttachments),
            .depthAttachmentFormat = definition.info.depthAttachmentFormat,
	        .stencilAttachmentFormat = definition.info.stencilAttachmentFormat,
            .rasterization = definition.info.rasterization,
            .depthTest = definition.info.depthTest,
            .topology = definition.info.topology,
            .pushConstantSize = definition.info.pushConstantSize,
        }
    );

    if (pipeline.valid())
    {
        auto it = m_pipelines.insert(std::move(pipeline));
        m_uriToPipeline[definition.uri] = it;
    }
}

auto PipelineCache::load_pipeline(ComputePipelineDefinition const& definition) -> void
{
    std::filesystem::path computeShaderPath  = m_configuration.cachePath / std::filesystem::path{ definition.shaderPaths.compute }.stem().replace_extension(SHADER_EXTENSION_NAME[std::to_underlying(gpu::ShaderType::Compute)]);

    auto cs = deserialize_shader(computeShaderPath);

    auto pipeline = gpu::Pipeline::from(
        m_device,
        cs,
        {
            .name = std::string{ definition.info.name },
            .pushConstantSize = definition.info.pushConstantSize,
        }
    );

    if (pipeline.valid())
    {
        auto it = m_pipelines.insert(std::move(pipeline));
        m_uriToPipeline[definition.uri] = it;
    }
}

auto PipelineCache::try_compile_shader(std::filesystem::path const& path, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::shader, std::string>
{
    // First we check if the compiled binaries already exist. We only recompile if they don't.
    // That way during development, we only need to recompile the shaders that are being hot-reloaded.

    std::filesystem::path cachedBinaryPath = m_configuration.cachePath / path.stem().replace_extension(SHADER_EXTENSION_NAME[std::to_underlying(compileInfo.type)]);

    if (!std::filesystem::exists(cachedBinaryPath))
    {
        return compile_shader(path.generic_string(), cachedBinaryPath, compileInfo);
    }

    return deserialize_shader(cachedBinaryPath);
}

auto PipelineCache::compile_shader(std::string_view path, std::filesystem::path const& output, PipelineShaderCompileInfo const& compileInfo) -> std::expected<gpu::shader, std::string>
{
    static constexpr std::string_view SHADER_ENRY_POINT[] = {
        "main_vertex",
        "main_fragment",
        "main_geometry",
        "main_tesselation_control",
        "main_tesselation_evaluation",
        "main_compute",
        "main_task",
        "main_mesh",
        "main_ray_gen",
        "main_any_hit",
        "main_closest_hit",
        "main_ray_miss",
        "main_intersection",
        "main_callable"
    };

    core::sbf::File const sourceCode({ .path = path, .shareMode = core::sbf::FileShareMode::Shared_Read, .map = true });
    
    gpu::ShaderCompileInfo info{
        .path = path,
        .type = compileInfo.type,
        .entryPoint = SHADER_ENRY_POINT[std::to_underlying(compileInfo.type)],
        .sourceCode = std::string_view{ static_cast<char const*>(sourceCode.data()), sourceCode.size() },
        .optimizationLevel = compileInfo.optimizationLevel,
        .enableDebugSymbols = compileInfo.enableDebugSymbol
    };

    for (auto token : compileInfo.defines 
                    | std::views::split(';') 
                    | std::views::transform([](auto range) { return std::string_view{ range }; }))
    {
        if (token.empty())
        {
            continue;
        }

        if (token.find('=') == std::string_view::npos)
        {
            info.add_macro_definition(token);
        }
        else
        {
            auto key = token.substr(0, token.find('='));
            auto value = token.substr(token.find('=') + 1);

            info.add_macro_definition(key, value);
        }
    }

    auto compilationResult = m_shaderCompiler->compile(info);

    if (!sourceCode.is_open() || sourceCode.data() == nullptr || !compilationResult)
    {
        return std::unexpected{ 
            compilation_error(
                std::string_view{ compilationResult.error().data(), compilationResult.error().size() }, 
                sourceCode.is_open(), 
                sourceCode.data() != nullptr) 
        };
    }

    serialize_shader(output, compilationResult->compiled_info());

    return gpu::Shader::from(m_device, compilationResult->compiled_info());
}

auto PipelineCache::compilation_error(std::string_view message, bool sourceFileOpen, bool sourceCodeExist) -> std::string
{
    std::string err{ message };

    if (!sourceFileOpen || !sourceCodeExist)
    {
        err.append("\nPossible causes:");

        if (!sourceFileOpen)
        {
            err.append("\n\t-> Failed to open source file!");
        }

        if (!sourceCodeExist)
        {
            err.append("\n\t-> No source code provided!");
        }
    }

    return err;
}

auto PipelineCache::serialize_shader(std::filesystem::path const& path, gpu::CompiledShaderInfo const& compiledInfo) -> void
{
    core::sbf::WriteStream stream{ path };

	stream.write(core::sbf::SbfFileHeader{});
    
    SerializedShaderBinaryHeader header{};

    header.descriptor.sizeBytes = static_cast<uint32>(compiledInfo.name.size()) 
                                + static_cast<uint32>(compiledInfo.entryPoint.size())
                                + static_cast<uint32>(compiledInfo.binaries.size_bytes());

    header.type = compiledInfo.type;
    header.nameSize = static_cast<uint32>(compiledInfo.name.size());
    header.entryPointSize = static_cast<uint32>(compiledInfo.entryPoint.size());
    header.sizeUint32 = static_cast<uint32>(compiledInfo.binaries.size());

    stream.write(header);
    stream.write(compiledInfo.name.data(), compiledInfo.name.size() * sizeof(char));
    stream.write(compiledInfo.entryPoint.data(), compiledInfo.entryPoint.size() * sizeof(char));
    stream.write(compiledInfo.binaries.data(), compiledInfo.binaries.size_bytes());
}

auto PipelineCache::deserialize_shader(std::filesystem::path const& path) -> gpu::shader
{
    // The resource is guaranteed to exist since the check was done in a function before the call to this one.

	core::sbf::File const shader{ path };

    SerializedShaderBinary binary{ shader };

    return gpu::Shader::from(m_device, binary.compiled_info());
}
}