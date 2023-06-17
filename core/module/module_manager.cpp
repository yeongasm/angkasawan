#include <span>
#include <ranges>
#include <algorithm>
#include <iterator>
#include "module_manager.h"
#include "engine.h"

namespace core
{

class Engine;

/**
* TODO(Afiq):
* 1. Threading model. Framework needs to support concurrency & parallelism.
*	1.1. Provide 2 parallelization mechanisms:
*		a. System on a thread (E.g renderer, async file IO & etc ...)
*		b. Fibers (async/await).
*	1.2 We will go with a producer/consumer approach with the engine.
*		1.2.1. If a system down the line depends on an input from a prior system, the prior system should fill the system's input buffer.
*		1.2.2. This approach enables us to parallelize the whole engine without worrying too much about dependency.
*	1.3. With this being said, we need to figure out a way to create a std::function-like data structure with controlled memory allocation.
*		1.3.1. So that we can pass them to fibers instead of actual function pointers.
*
* 2. Virtual filesystem.
*
* 3. Hot reloading system.
*	3.1. Shall attempt for the memory overwriting approach instead of hot swapping dlls.
*
* .
* .
* .
*
* 999. Custom build system (not C++ build system but a game build system).
*/

ModuleID get_next_module_id()
{
	static ModuleID _id = 0u;
	return ++_id;
}

ModuleManager::ModuleManager(Engine& engine) :
	m_table{},
	m_runners{},
	m_engine{ engine }
{}

ModuleManager::~ModuleManager() {}

void ModuleManager::initialize_modules()
{
	if (!m_table.size())
	{
		return;
	}

	// The modules are not yet sorted according to their execution order at this stage and is still ordered based on ModuleManager's DAG.
	for (auto& [id, container] : m_table)
	{
		// If initialization fails, we mark the container as broken and remove it from the execution list before sorting the list.
		if (container.on_initialize)
		{
			if (!container.on_initialize(container.pAddress))
			{
				container.broken = true;
				// TODO(afiq):
				// Verbose error logging.
				ASSERTION(false);
			}
		}
	}

	auto it = std::remove_if(m_runners.begin(), m_runners.end(), [](ModuleContainer* pContainer) -> bool { return pContainer->broken; });
	m_runners.erase(m_runners.begin(), it);
}

void ModuleManager::initialize(const ModuleManagerInfo info)
{
	m_runners.reserve(info.numSystemsHint);

	std::vector<ModuleInfo> modules{ info.modules, info.modules + info.numModules };

	// We call "on_dll_load" to install the modules.
	for (ModuleInfo const& module : modules)
	{
		if (!module.dll.length())
		{
			continue;
		}

		core::Dll dll{ module.dll.c_str() };
		if (!dll.load())
		{
			ASSERTION(false && "Failed to load Dll.");
			continue;
		}
		std::string_view const name{ module.name.first(), module.name.length() };
		dll.call<void>("on_dll_load", &(*m_engine), module);

		// Attach dll to module container.
		for (auto& [_, container] : m_table)
		{
			if (container.name == name)
			{
				container.dll = std::move(dll);
				break;
			}
		}
	}
	// Once we finish installing all the modules, we can then sort according to the order they're meant to run.
	std::sort(
		modules.begin(),
		modules.end(),
		[](ModuleInfo const& a, ModuleInfo const& b) -> bool { return a.order < b.order; }
	);
	// Create a mapping between the module name's to their ID's.
	std::map<std::string_view, ModuleID> nameToIDMap;
	for (auto& [id, container] : m_table)
	{
		std::string_view name{ container.name.c_str(), container.name.length() };
		nameToIDMap.emplace(std::move(name), id);
	}
	// For every module that is a "Sync" module, we add them to the runner.
	for (auto const& module : modules | std::views::filter([](ModuleInfo const& info) -> bool { return info.type == ModuleType::Sync; }))
	{
		std::string_view name{ module.name.c_str(), module.name.length() };
		auto& container = m_table[nameToIDMap[name]];
		m_runners.emplace(&container);
	}
	// Finally, initialize all modules and for runner modules that are marked as "broken", we remove them from the list.
	// This step can be done earlier but no earlier than installing the modules.
	initialize_modules();
}

void ModuleManager::shutdown()
{
	for (auto it = m_table.rbegin(); it != m_table.rend(); ++it)
	{
		if (it->second.on_terminate)
		{
			it->second.on_terminate(it->second.pAddress);
		}
		ftl::release_memory(it->second.pAddress);
	}
	m_runners.release();
	m_table.clear();
}

void ModuleManager::on_update_begin()
{
	for (auto const& runner : m_runners)
	{
		if (runner->on_update_begin)
		{
			runner->on_update_begin(runner->pAddress);
		}
	}
}

void ModuleManager::on_update()
{
	for (auto const& runner : m_runners)
	{
		if (runner->on_update)
		{
			runner->on_update(runner->pAddress);
		}
	}
}

void ModuleManager::on_update_end()
{
	for (auto const& runner : m_runners)
	{
		if (runner->on_update_end)
		{
			runner->on_update_end(runner->pAddress);
		}
	}
}

size_t ModuleManager::num_modules() const
{
	return m_table.size();
}

}