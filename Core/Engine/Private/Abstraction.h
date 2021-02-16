#pragma once
#ifndef LEARNVK_ENGINE_PRIVATE_ABSTRACTION
#define LEARNVK_ENGINE_PRIVATE_ABSTRACTION

#include "Prototype.h"
#include "SubSystem/Resource/Handle.h"
#include "Library/Allocators/LinearAllocator.h"

struct EngineCreationInfo
{
	String64	AppName;
	FilePath	GameModule;
	uint32		StartPosX;
	uint32		StartPosY;
	uint32		Width;
	uint32		Height;
	bool		FullScreen;
	bool		Multithreaded;
};


class ENGINE_API EngineImpl final : public EngineBase
{
private:

	LinearAllocator	SystemAllocator;
	Array<SystemInterface*> EngineSystems;
	Array<SystemInterface*>	GameSystems;

	using CWndInfoRef = const WindowInfo&;

	void ReserveMemoryForSystems();
	void RunGameAndUpdateSystems();
	static void InitializeGlobalContext(EngineImpl* Engine);
	//void UpdateIOStates();
	void UpdateMouseStates();
	void UpdateKeyStates();
	void BeginFrame();
	void EndFrame();	// Not really sure what to do with this yet. Perhaps for profiling.
	virtual void OnEvent(const OS::Event& e) override;
	virtual void OnInit() override;
	virtual void OnTerminate() override;
	void FreeAllSystems();

public:

	EngineImpl()  = default;
	~EngineImpl() = default;;
	DELETE_COPY_AND_MOVE(EngineImpl)

	// Engine start-up, update and terminate.
	void InitializeEngine(const EngineCreationInfo& Info);
	void RegisterGame(const EngineCreationInfo& Info);
	void Run();
	void TerminateEngine();
	IOSystem& GetIO();
	// Thread system start-up, update and terminate.
	//void		KickJob				(JobFunction Callback, Job::Args Args, Job* Parent = nullptr);
	//void		KickJobsAndWait		(Job* Jobs, size_t Count);
	ResourceCache& CreateNewResourceCache(ResourceType Type);
	ResourceCache* FetchResourceCacheForType(ResourceType Type);
	bool DeleteResourceCacheForType(ResourceType Type);
	CWndInfoRef GetWindowInformation() const;
	void ShowCursor(bool Show = true);
	void SetMousePosition(float32 x, float32 y);
	bool IsWindowFocused() const;
	bool HasWindowSizeChanged() const;
	void* AllocateAndRegisterSystem(size_t Size, SystemType Type, Handle<ISystem>* Hnd);
	SystemInterface* GetRegisteredSystem(SystemType Type, Handle<ISystem> Hnd);

};

EngineImpl& FetchEngineContext();

#endif // !LEARNVK_ENGINE_PRIVATE_ABSTRACTION