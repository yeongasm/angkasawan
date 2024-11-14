#pragma once
#ifndef CORE_FILE_WATCHER_H
#define CORE_FILE_WATCHER_H

#include <filesystem>

#include "lib/handle.h"
#include "lib/function.h"

#include "engine_api.h"

namespace core
{
namespace filewatcher
{
enum class FileAction
{
	None,
	Added,
	Modified,
	Deleted
};

struct FileActionInfo
{
	std::filesystem::path path;
	FileAction action;
};

/**
* NOTE(afiq):
* Switch to move_only_function when we upgrade to C++23.
*/
using FileWatchCallbackFn = std::function<void(FileActionInfo const&)>;

struct FileWatchInfo
{
	/**
	* Path to a directory or file.
	* When a directory is specified or "*" is used, the async filewatcher watches over all the files in the directory.
	*/
	std::filesystem::path path;
	/**
	* Callback function to invoke when a change is registered in the async filewatcher.
	*/
	FileWatchCallbackFn callback;
	/**
	* Recursively look for changes in children directories / subfolders.
	* When path given is a path to a file, this variable is ignored.
	*/
	bool recursive;
};

using file_watch_id = lib::handle<struct File, uint64, std::numeric_limits<uint64>::max()>;

CORE_API auto initialize_file_watcher() -> bool;
CORE_API auto terminate_file_watcher() -> void;
/**
* Asynchronously watches for changes to a file or file(s) in a directory.
*/
CORE_API auto watch(FileWatchInfo&& watchInfo) -> file_watch_id;
CORE_API auto unwatch(file_watch_id id) -> void;
}
}

#endif // !CORE_FILE_WATCHER_H