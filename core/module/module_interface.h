#pragma once
#ifndef CORE_MODULE_INTERFACE_H
#define CORE_MODULE_INTERFACE_H

#include "allocator/stack_allocator.h"
#include "shared/engine_common.h"

/**
* \brief 
* Call this macro in the module's derived class body.
* It is a requirement that modules have each of these signatures declared in their body:-
* \brief
* -> MODULE_DECL_METHOD(init), MODULE_DECL_METHOD(tick), MODULE_DECL_METHOD(terminate)
*/
//#define MODULE_DECL_METHOD(method)					\
//	private:										\
//		void method();								\
//	public:											\
//		static void static_##method(void* system);

/**
* @brief Begin block preprocessor for module method definition.
*/
//#define MODULE_DEFINE_METHOD_BEGIN(Module, method)			\
//	void Module::static_##method(void* system)				\
//	{														\
//		reinterpret_cast<Module*>(system)->##method();		\
//	}														\
//															\
//	void Module::##method()									\
//	{

/**
* @brief End block preprocessor for module method definition.
*/
//#define MODULE_DEFINE_METHOD_END(Module, method) }

/**
* \brief Required definition for module initialization.
*/
//#define MODULE_BEGIN_INIT(Module) MODULE_DEFINE_METHOD_BEGIN(Module, init)
/**
* \brief End of module intitialization definition.
*/
//#define MODULE_END_INIT(Module) MODULE_DEFINE_METHOD_END(Module, init)
/**
* \brief Required definition for module tick.
*/
//#define MODULE_BEGIN_TICK(Module) MODULE_DEFINE_METHOD_BEGIN(Module, tick)
/**
* \brief End of module tick definition.
*/
//#define MODULE_END_TICK(Module) MODULE_DEFINE_METHOD_END(Module, tick)
/**
* \brief Required definition for module termination.
*/
//#define MODULE_BEGIN_TERMINATE(Module)						\
//	void Module::static_terminate(void* system)				\
//	{														\
//		Module* mod = reinterpret_cast<Module*>(system);	\
//		mod->terminate();									\
//		mod->~Module();										\
//	}														\
//															\
//	void Module::terminate()								\
//	{
/**
* \brief End of module terminate definition.
*/
//#define MODULE_END_TERMINATE(Module) MODULE_DEFINE_METHOD_END(Module, terminate)


COREBEGIN

class Engine;

//struct LoadModuleParam
//{
//	Engine& engine;
//	ModuleInfo& info;
//	PlatformDll& dll;
//};

/**
* \brief
* It is a requirement that modules for this application be derived from this class.
* Failure of doing so will result in a compilation error for the given module. 
* The concept only allows objects derived from EngineModule to be registered within ModuleManager.
*/
/*class ENGINE_API EngineModule
{
public:
	EngineModule(Engine& engine, size_t stack) :
		m_engine{ engine }, m_stack{ stack }
	{}
	virtual ~EngineModule() = default;

	Engine& engine() { return m_engine; }

	template <typename T, typename... Arguments>
	decltype(auto) make_stack(Arguments&&... args)
	{
		return ftl::make_stack<T>(m_stack, std::forward<Arguments>(args)...);
	}

	virtual bool init() = 0;
	virtual void tick(float32 dt, EngineState state) = 0;
	virtual void terminate() = 0;
protected:
	MAKE_SINGLETON(EngineModule)

	Engine&				m_engine;
	ftl::StackAllocator m_stack;
};*/

COREEND

#endif // !CORE_MODULE_INTERFACE_H
