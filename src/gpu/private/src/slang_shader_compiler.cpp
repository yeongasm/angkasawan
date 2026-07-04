#include <expected>
#include <mutex>
#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>
#include "shader_compiler.hpp"

namespace gpu
{
/*
* TODO(afiq):
* Support multi-threaded shader compilation.
* To do that, we need to have a session (NOT global session) per thread.
* We can keep an map of thread id's to session to achieve this.
*/
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
            .profile    = globalSession->findProfile("spirv_1_6")
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

		Slang::ComPtr<slang::IModule> slangModule = {};

		Slang::ComPtr<slang::IBlob> diagnosticsBlob = {}; 

        slangModule = session->loadModuleFromSourceString("angkasawan-slang-shader", info.path.data(), info.sourceCode.data(), diagnosticsBlob.writeRef());

		if (diagnosticsBlob != nullptr)
		{
			return std::unexpected{ lib::format("[ERROR][SLANG] {} - {}", info.path, static_cast<char const*>(diagnosticsBlob->getBufferPointer())) };
		}

		Slang::ComPtr<slang::IEntryPoint> entryPoint = {};
		
		if (SLANG_FAILED(slangModule->findEntryPointByName(info.entryPoint.data(), entryPoint.writeRef())))
		{
			return std::unexpected{ lib::format("[ERROR][SLANG] {} - Failed to find entry point '{}' in module.", info.path, info.entryPoint) };
		}

		std::array<slang::IComponentType*, 2> componentTypes =
        {
            slangModule,
            entryPoint
        };

		Slang::ComPtr<slang::IComponentType> composedProgram = {};

		if (SLANG_FAILED(session->createCompositeComponentType(componentTypes.data(), componentTypes.size(), composedProgram.writeRef(), diagnosticsBlob.writeRef())))
		{
			return std::unexpected{ lib::format("[ERROR][SLANG] {} - {}", info.path, static_cast<char const*>(diagnosticsBlob->getBufferPointer())) };
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram = {};
		
		if (SLANG_FAILED(composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef())))
		{
			return std::unexpected{ lib::format("[ERROR][SLANG] {} - {}", info.path, static_cast<char const*>(diagnosticsBlob->getBufferPointer())) };
		}

		Slang::ComPtr<slang::IBlob> spirvCode = {};

		if (SLANG_FAILED(linkedProgram->getEntryPointCode(0, 0, spirvCode.writeRef(), diagnosticsBlob.writeRef())))
		{
			return std::unexpected{ lib::format("[ERROR][SLANG] {} - {}", info.path, static_cast<char const*>(diagnosticsBlob->getBufferPointer())) };
		}

        auto begin  = static_cast<uint32 const*>(spirvCode->getBufferPointer());
        auto end    = begin + (spirvCode->getBufferSize() / sizeof(uint32));

		return ShaderCompiledUnit{
            .type       = info.type,
            .path       = info.path,
            .entryPoint = info.entryPoint, 
            .byteCode   = { begin, end } 
        };
	}
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