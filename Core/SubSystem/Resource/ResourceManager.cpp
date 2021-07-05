#include "Library/Random/Xoroshiro.h"
#include "ResourceManager.h"

static Xoroshiro32 g_RandId(OS::GetPerfCounter());

ResourceCache::ResourceCache() : Cache{} {}
ResourceCache::~ResourceCache() { FreeCache(true); }

ResourceCache::ResourceCache(const ResourceCache& Rhs) { *this = Rhs; }
ResourceCache::ResourceCache(ResourceCache&& Rhs) { *this = astl::Move(Rhs); }

ResourceCache& ResourceCache::operator=(const ResourceCache& Rhs)
{
	if (this != &Rhs)
	{
		Cache = Rhs.Cache;
	}
	return *this;
}

ResourceCache& ResourceCache::operator=(ResourceCache&& Rhs)
{
	if (this != &Rhs)
	{
		Cache = Move(Rhs.Cache);
		new (&Rhs) ResourceCache();
	}
	return *this;
}

uint32 ResourceCache::Create()
{
	uint32 id = g_RandId();
	Cache.Insert(id, Resource());
	AddRef(id);
	return id;
}

Resource* ResourceCache::Find(const astl::FilePath& Path)
{
	Resource* resource = nullptr;
	for (astl::Pair<uint32, Resource>& pair : Cache)
	{
		if (pair.Value.Path != Path)
		{
			continue;
		}
		resource = &pair.Value;
		break;
	}
	return resource;
}

Resource* ResourceCache::Get(uint32 Id)
{
	return &Cache[Id];
}

bool ResourceCache::IsReferenced(uint32 Id)
{
	return Cache[Id].RefCount > 1;
}

void ResourceCache::AddRef(uint32 Id)
{
	Cache[Id].RefCount++;
}

void ResourceCache::DeRef(uint32 Id)
{
	VKT_ASSERT(Cache[Id].RefCount != 1);
	Cache[Id].RefCount--;
}

void ResourceCache::Delete(uint32 Id)
{
	Cache.Remove(Id);
}

bool ResourceCache::IsEmpty()
{
	return Cache.IsEmpty();
}

bool ResourceCache::FreeCache(bool Forced)
{
	if (!Forced)
	{
		for (astl::Pair<uint32, Resource>& pair : Cache)
		{
			if (IsReferenced(pair.Key))
			{
				return false;
			}
		}
	}
	Cache.Release();
	return true;
}

bool ResourceCache::FlushCache(bool Forced)
{
	if (!Forced)
	{
		for (astl::Pair<uint32, Resource>& pair : Cache)
		{
			if (IsReferenced(pair.Key))
			{
				return false;
			}
		}
	}
	Cache.Empty();
	return true;
}

ResourceManager::ResourceManager() : Managers{} {}

ResourceManager::~ResourceManager() { FreeAllCaches(); }

void ResourceManager::AddCache(ResourceType Type)
{
	Managers.Insert(Type, ResourceCache());
}

void ResourceManager::RemoveCache(ResourceType Type)
{
	Managers.Remove(Type);
}

bool ResourceManager::CacheTypeExist(ResourceType Type)
{
  return Managers.Contains(Type);
}

ResourceCache* ResourceManager::FetchCacheForType(ResourceType Type)
{
	return &Managers[Type];
}

void ResourceManager::FreeAllCaches()
{
	Managers.Release();
}
