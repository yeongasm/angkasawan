#pragma once
#ifndef LEARNVK_SUBSYSTEM_RESOURCE_MANAGER
#define LEARNVK_SUBSYSTEM_RESOURCE_MANAGER

#include "Platform/EngineAPI.h"
#include "Platform/Minimal.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/Queue.h"

struct ENGINE_API RUIDGenerator
{
	uint32 operator() ();
};

struct Resource
{
	uint32		Type		= 0;
	uint32		RefCount	= 0;
	uint32		Index		= uint32(-1);
	FilePath	Path		= {};
};

class ENGINE_API ResourceCache
{
public:
	ResourceCache();
	~ResourceCache();

	ResourceCache(const ResourceCache& Rhs);
	ResourceCache(ResourceCache&& Rhs);

	ResourceCache& operator=(const ResourceCache& Rhs);
	ResourceCache& operator=(ResourceCache&& Rhs);

	uint32		Create		();
	Resource*	Find		(const FilePath& Path);
	Resource*	Get			(uint32 Id);
	bool		IsReferenced(uint32 Id);
	void		AddRef		(uint32 Id);
	void		DeRef		(uint32 Id);
	void		Delete		(uint32 Id);
	bool		IsEmpty		();
	bool		FreeCache	(bool Forced = false);
	bool		FlushCache	(bool Forced = false);
private:
	using CacheMap = Map<uint32, Resource>;
	CacheMap		Cache;
};

using ResourceType = uint32;

class ENGINE_API ResourceManager
{
private:

	using ResourceTable = Map<ResourceType, ResourceCache>;

public:

	ResourceManager();
	~ResourceManager();

	DELETE_COPY_AND_MOVE(ResourceManager)

	void			AddCache			(ResourceType Type);
	void			RemoveCache			(ResourceType Type);
	bool			CacheTypeExist		(ResourceType Type);
	ResourceCache*	FetchCacheForType	(ResourceType Type);
	void			FreeAllCaches		();

private:
	ResourceTable Managers;
};

#endif // !LEARNVK_SUBSYSTEM_RESOURCE_MANAGER