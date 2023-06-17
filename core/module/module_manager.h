#pragma once
#ifndef CORE_SYSTEM_SYSTEM_H
#define CORE_SYSTEM_SYSTEM_H

#include <map>
#include <functional>
#include "containers/array.h"
#include "platform_interface/platform_interface.h"
#include "shared/engine_common.h"

namespace core
{

using ModuleID = uint32;

ENGINE_API ModuleID get_next_module_id();

template <typename T>
struct ModuleIDFor
{
	ENGINE_API static ModuleID id;
};

class ENGINE_API ModuleManager
{
private:
	MAKE_SINGLETON(ModuleManager)

	friend class Engine;

	template <typename T>
	using StageHookFn = std::function<T(void*)>;

	struct ModuleContainer
	{
		std::string			name;
		Dll					dll;
		ModuleType			type;
		void*				pAddress;	// Address within the store.

		StageHookFn<bool>	on_initialize;
		StageHookFn<void>	on_update_begin;
		StageHookFn<void>	on_update;
		StageHookFn<void>	on_update_end;
		StageHookFn<void>	on_terminate;

		bool broken;
	};

	/**
	* ordered.
	*/
	std::map<ModuleID, ModuleContainer> m_table;
	ftl::Array<ModuleContainer*>		m_runners;
	ftl::Ref<Engine>					m_engine;

	// --- methods ---

	//std::string_view append_to_string_pool(std::string_view name);

	template <typename Module_t/*, typename... Args*/>
	Module_t* add_module_into_engine(ModuleID const id, std::string_view name, ModuleType type/*, Args&&... args*/)
	{
		Module_t* address = static_cast<Module_t*>(ftl::allocate_memory(sizeof(Module_t)));

		ModuleContainer container{
			.name		= std::string{ name },
			.type		= type,
			.pAddress	= reinterpret_cast<void*>(address),
			.broken		= false
		};

		m_table.emplace(id, std::move(container));

		return address;
	}

	void	initialize							(const ModuleManagerInfo info);
	void	initialize_modules					();
	void	shutdown							();
	void	on_update_begin						();
	void	on_update							();
	void	on_update_end						();

public:
	ModuleManager(Engine& engine);
	~ModuleManager();


	/**
	* Installs the module into the manager and allocates memory for it.
	*/
	template <typename Module_t/*, typename... Args*/>
	ftl::Ref<Module_t> install_module(std::string_view name, ModuleType type/*, Args&&... args*/)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (m_table.contains(id))
		{
			return ftl::null_ref<Module_t>;
		}
		Module_t* pModule = add_module_into_engine<Module_t>(id, name, type/*, std::forward<Args>(args)...*/);

		return ftl::Ref<Module_t>{ pModule };
	}

	template <typename Module_t>
	ftl::Ref<Module_t> get_module()
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return ftl::null_ref<Module_t>;
		}

		auto& container = m_table[id];
		Module_t* pModule = static_cast<Module_t*>(container.pAddress);

		return ftl::Ref<Module_t>{ pModule };
	}

	template <typename Module_t, typename Callback>
	bool register_on_initialize_fn(Callback&& fn)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return false;
		}
		ModuleContainer& container = m_table[id];

		container.on_initialize = std::forward<Callback>(fn);

		return true;
	}

	template <typename Module_t, typename Callback>
	bool register_on_update_begin_fn(Callback&& fn)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return false;
		}
		ModuleContainer& container = m_table[id];

		container.on_update_begin = std::forward<Callback>(fn);

		return true;
	}

	template <typename Module_t, typename Callback>
	bool register_on_update_fn(Callback&& fn)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return false;
		}
		ModuleContainer& container = m_table[id];

		container.on_update = std::forward<Callback>(fn);

		return true;
	}

	template <typename Module_t, typename Callback>
	bool register_on_update_end_fn(Callback&& fn)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return false;
		}
		ModuleContainer& container = m_table[id];

		container.on_update_end = std::forward<Callback>(fn);

		return true;
	}

	template <typename Module_t, typename Callback>
	bool register_on_terminate_fn(Callback&& fn)
	{
		ModuleID const id = ModuleIDFor<Module_t>::id;
		if (!m_table.contains(id))
		{
			return false;
		}
		ModuleContainer& container = m_table[id];

		container.on_terminate = std::forward<Callback>(fn);

		return true;
	}

	size_t num_modules() const;
};

#define DECLARE_MODULE(Module, API)	\
template <>							\
struct core::ModuleIDFor<Module>	\
{									\
	API static ModuleID id;			\
};									\
core::ModuleID core::ModuleIDFor<Module>::id = core::get_next_module_id(); 

}

#endif // !CORE_SYSTEM_SYSTEM_H
