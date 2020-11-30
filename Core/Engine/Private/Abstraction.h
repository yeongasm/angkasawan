#pragma once
#ifndef LEARNVK_ENGINE_PRIVATE_ABSTRACTION
#define LEARNVK_ENGINE_PRIVATE_ABSTRACTION

#include "Prototype.h"

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
public:
	EngineImpl()  = default;
	~EngineImpl() = default;;
	DELETE_COPY_AND_MOVE(EngineImpl)

	// Engine start-up, update and terminate.
	void		InitializeEngine	(const EngineCreationInfo& Info);
	void		RegisterGame		(const EngineCreationInfo& Info);
	void		Run					();
	void		TerminateEngine		();
	
	IOSystem&	GetIO				();

	// Thread system start-up, update and terminate.
	//void		KickJob				(JobFunction Callback, Job::Args Args, Job* Parent = nullptr);
	//void		KickJobsAndWait		(Job* Jobs, size_t Count);

	// Resource manager procedures.
	ResourceCache&	CreateNewResourceCache		(ResourceType Type);
	ResourceCache*	FetchResourceCacheForType	(ResourceType Type);
	bool			DeleteResourceCacheForType	(ResourceType Type);
	
private:

	using CWndInfoRef = const WindowInfo&;

	static void InitializeGlobalContext(EngineImpl* Engine);

	void		UpdateIOStates				();

	// Os related events here ...
	virtual void OnEvent(const OS::Event& e)	override;
	virtual void OnInit()						override;
	virtual void OnTerminate()					override;

	void BeginFrame();
	void EndFrame();	// Not really sure what to do with this yet. Perhaps for profiling.

public:

	//bool		IsKeyPressed		(IOKeys Key);
	//bool		IsKeyDoubleTapped	(IOKeys Key);
	//bool		IsKeyHeld			(IOKeys Key);
	//bool		IsKeyReleased		(IOKeys Key);
	//bool		IsMouseClicked		(IOMouseButton Button);
	//bool		IsMouseDoubleClicked(IOMouseButton Button);
	//bool		IsMouseHeld			(IOMouseButton Button);
	//bool		IsMouseReleased		(IOMouseButton Button);
	//bool		IsMouseDragging		(IOMouseButton Button);
	//bool		MouseDragDelta		(IOMouseButton Button, vec2& Buf);
	//bool		IsCtrlPressed		();
	//bool		IsAltPressed		();
	//bool		IsShiftPressed		();

	CWndInfoRef	GetWindowInformation() const;
};

EngineImpl& FetchEngineContext();

#endif // !LEARNVK_ENGINE_PRIVATE_ABSTRACTION