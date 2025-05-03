#include <memory>
#include <mutex>
#include <spirv_reflect.h>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>
#include "shader_compiler.hpp"

namespace gpu
{
namespace util
{
struct SlangShaderCompiler : public ShaderCompiler
{
    std::mutex sessionMutex;
    inline static Slang::ComPtr<slang::IGlobalSession> globalSession = {};

    auto make_session(ShaderCompileInfo const& info) -> Slang::ComPtr<slang::ISession>
    {
        std::lock_guard sessionLock{ sessionMutex };

        lib::array<slang::PreprocessorMacroDesc> macroDesc{ m_macroDefinitions.size() };

        for (auto const& [k, v] : m_macroDefinitions)
        {
            macroDesc.emplace_back(k.c_str(), v.c_str());
        }

        for (auto const& [k, v] : info.macroDefinitions)
        {
            macroDesc.emplace_back(k.data(), v.data());
        }

        lib::array<char const*> includeDirs{ m_includeDirectories.size() };

        for (auto const& dir : m_includeDirectories)
        {
            includeDirs.push_back(dir.c_str());
        }

        slang::TargetDesc targetDesc = {
            .format     = SlangCompileTarget::SLANG_SPIRV,
            .profile    = globalSession->findProfile("spirv_1_4"),
            .forceGLSLScalarBufferLayout = true
        };

        slang::CompilerOptionEntry compilerOption{
            .name = slang::CompilerOptionName::EmitSpirvDirectly,
            .value = {
                .kind = slang::CompilerOptionValueKind::Int,
                .intValue0 = 1
            }
        };

        slang::SessionDesc desc = {
            .targets                    = &targetDesc,
            .targetCount                = 1,
            .defaultMatrixLayoutMode    = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
            .searchPaths                = includeDirs.data(),
            .searchPathCount            = static_cast<SlangInt>(includeDirs.size()),
            .preprocessorMacros         = macroDesc.data(),
            .preprocessorMacroCount     = static_cast<SlangInt>(macroDesc.size()),
            .compilerOptionEntries      = &compilerOption,
            .compilerOptionEntryCount   = 1
        };

        Slang::ComPtr<slang::ISession> session = {};

        globalSession->createSession(desc, session.writeRef());

        return session;
    };

    auto do_compile(ShaderCompileInfo const& info) -> std::expected<ShaderCompiledUnit const, lib::string>
    {
        auto session = make_session(info);

        if (session.get() == nullptr)
        {
            return std::unexpected{ "[ERROR][SLANG] Failed to create Slang compiliation session." };
        }

        Slang::ComPtr<SlangCompileRequest> compileRequest = {};
        
        if (SLANG_FAILED(session->createCompileRequest(compileRequest.writeRef())))
        {
            return std::unexpected{ "[ERROR][SLANG] Failed to create Slang compilation request." };
        }

        char const* compileCommandArguments[] = {
            "-warnings-disable", "39001",
            "-O0"
        };

        compileRequest->processCommandLineArguments(compileCommandArguments, 3);

        int const tui = compileRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, "_angkasawan_shader_file");

        compileRequest->addTranslationUnitSourceString(tui, info.name.data(), info.sourceCode.data());

        if (SLANG_FAILED(compileRequest->compile()))
        {
            return std::unexpected{ lib::format("[ERROR][SLANG] {} - {}", info.name, compileRequest->getDiagnosticOutput()) };
        }

        auto entryPointIndex = std::numeric_limits<uint32>::max();
        auto reflection = compileRequest->getReflection();
        auto const numEntryPoints = spReflection_getEntryPointCount(reflection);

        for (auto i = 0u; i < numEntryPoints; ++i)
        {
            auto entryPoint = spReflection_getEntryPointByIndex(reflection, i);
            auto entryPointName = std::string_view{ spReflectionEntryPoint_getName(entryPoint) };

            if (info.entryPoint == entryPointName)
            {
                entryPointIndex = i;
                break;
            }
        }

        if (entryPointIndex == std::numeric_limits<uint32>::max())
        {
            return std::unexpected{ lib::format("[ERROR][SLANG] {} - Failed to find entry point '{}' in module.", info.name, info.entryPoint) };
        }

        Slang::ComPtr<slang::IBlob> spirvCode = {};

        // TODO(afiq):
        // Need to make sure this does not return an error.
        compileRequest->getEntryPointCodeBlob(entryPointIndex, 0, spirvCode.writeRef());

        auto begin  = static_cast<uint32 const*>(spirvCode->getBufferPointer());
        auto end    = begin + (spirvCode->getBufferSize() / sizeof(uint32));

        return ShaderCompiledUnit{
            .type       = info.type,
            .name       = info.name,
            .entryPoint = info.entryPoint, 
            .byteCode   = { begin, end } 
        };
    };
};

auto ShaderCompiler::create() -> std::expected<std::unique_ptr<ShaderCompiler>, std::string_view>
{
    if (SlangShaderCompiler::globalSession.get() == nullptr && 
        SLANG_FAILED(slang::createGlobalSession(SlangShaderCompiler::globalSession.writeRef())))
    {
        return std::unexpected{ "[ERROR][SLANG] Failed to create shader compiler backend." };
    }

    return std::make_unique<SlangShaderCompiler>();
}

auto ShaderCompiler::compile(ShaderCompileInfo const& info) -> std::expected<ShaderCompiledUnit const, lib::string>
{
    auto&& self = *static_cast<SlangShaderCompiler*>(this);
	return self.do_compile(info);
}
}
}