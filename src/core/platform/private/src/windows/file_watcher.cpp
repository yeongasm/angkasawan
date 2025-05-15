#include <thread>
#include <mutex>
#include <string>

#include "fmt/format.h"

#include "platform_header.hpp"
#include "lib/memory.hpp"
#include "lib/set.hpp"
#include "lib/map.hpp"

#include "file_watcher.hpp"

template <>
struct std::hash<core::filewatcher::file_watch_id>
{
	size_t operator()(core::filewatcher::file_watch_id const& fileWatchId) const noexcept
	{
		return (std::hash<uint64>{})(fileWatchId.get());
	}
};

namespace core::filewatcher
{
struct FileContext
{
	/**
	* A Windows handle to a file. For FileContexts that are "*", this variable is null.
	*/
	HANDLE handle;
	/**
	* File's previous last written time. For FileContexts that are "*", this variable is not used.
	*/
	FILETIME lastModifiedTime;
	/**
	* File's pervious file size. For FileContexts that are "*", this variable is not used.
	*/
	DWORD lastFileSize;
	/**
	* Callback function that handles the file's change request. For FileContexts who's directory is set to watch all "*", only the root FileContext contains a callback.
	*/
	FileWatchCallbackFn callback;
	/**
	* For FileContexts that are "*", specifies wether the watch is recursive.
	*/
	bool recursive;
};

struct DirectoryContext
{
	using WatchedFilesStore = lib::map<std::filesystem::path, FileContext>;
	
	/**
	* Contains information used in asynchronous (or overlapped) input and output. Data is supplied by ReadDirectoryChangesW.
	*/
	OVERLAPPED overlapped;
	/**
	* A Windows handle to a file returned from CreateFileW. Note that Windows categorises a file and a directory as a "File" for historic reasons.
	*/
	HANDLE handle;
	/**
	* 64 KiB buffer for the OS to write file change notifications into.
	* Can potentially overflow as stated by Window's documentation.
	*/
	BYTE buffer[64_KiB];
	WatchedFilesStore watchedFiles;
};

struct WindowsFileWatcherContext
{
	struct FileIndexInfo
	{
		uint32 dir;
		uint32 file;

		explicit operator size_t() const
		{
			return std::bit_cast<size_t>(*this);
		}

		static auto from(size_t value) -> FileIndexInfo
		{
			return std::bit_cast<FileIndexInfo>(value);
		}
	};

	static constexpr FileIndexInfo INVALID_FILE_INDEX_INFO = { .dir = std::numeric_limits<uint32>::max(), .file = std::numeric_limits<uint32>::max() };

	static constexpr ULONG_PTR TERMINATE_PACKET = std::numeric_limits<ULONG_PTR>::max();
	static constexpr DWORD NOTIFICATION_FILTER = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
	static std::filesystem::path const WATCH_ALL_FILES;

	static std::unique_ptr<WindowsFileWatcherContext> ctx;

	/**
	* Key value store that associates the path with it's context.
	*/
	lib::map<std::filesystem::path, DirectoryContext> directories = {};
	/**
	* Zombie list - A list of file_watch_ids that are set to be removed.
	*/
	lib::set<file_watch_id> zombies = {};
	/**
	* Mutex for the directories container.
	*/
	std::mutex dirContainerMutex = {};
	/**
	* Mutex for the zombie list.
	*/
	std::mutex zombieContainerMutex = {};

	HANDLE ioCompletionPort = nullptr;
	std::jthread fileWatcherThread = {};

	/**
	* Program loop for the async file watcher thread.
	*/
	static auto filewatch_update_loop(WindowsFileWatcherContext& context) -> void
	{
		while (true)
		{
			DWORD numBytes = 0;
			ULONG_PTR completionKey = 0;
			OVERLAPPED* pOverlapped = nullptr;

			BOOL result = GetQueuedCompletionStatus(context.ioCompletionPort, &numBytes, &completionKey, &pOverlapped, INFINITE);

			if (result == FALSE)
			{
				continue;
			}

			if (completionKey == TERMINATE_PACKET)
			{
				context.unwatch_all();
				break;
			}

			context.clear_zombie_watches();

			context.notify_file_change(completionKey);
		}
	}

	[[nodiscard]] auto translate_file_action(DWORD action) -> FileAction
	{
		FileAction fwAction = FileAction::None;

		switch (action)
		{
		case FILE_ACTION_RENAMED_NEW_NAME:
		case FILE_ACTION_ADDED:
			fwAction = FileAction::Added;
			break;
		case FILE_ACTION_RENAMED_OLD_NAME:
		case FILE_ACTION_REMOVED:
			fwAction = FileAction::Deleted;
			break;
		case FILE_ACTION_MODIFIED:
			fwAction = FileAction::Modified;
			break;
		};

		return fwAction;
	}

	auto notify_file_change(ULONG_PTR completionKey) -> void
	{
		static std::filesystem::path filename{};

		size_t const dirBucket = static_cast<size_t>(completionKey);
		
		auto& [dirPath, dir] = directories.element_at_bucket(dirBucket);

		size_t nextByteOffset = 0;

		PFILE_NOTIFY_INFORMATION notifyInfo = nullptr;

		bool recursiveDirWatch = false;
		std::once_flag onceFlag = {};

		do
		{
			notifyInfo = std::bit_cast<PFILE_NOTIFY_INFORMATION>(&dir.buffer[nextByteOffset]);
			nextByteOffset += notifyInfo->NextEntryOffset;
			
			// According to Microsoft's documentation, FILE_NOTIFY_INFORMATION.FileNameLength returns the size of the string in bytes. NOT the number of characters!!!
			size_t const numCharElements = notifyInfo->FileNameLength / sizeof(wchar_t);

			filename.assign(notifyInfo->FileName, notifyInfo->FileName + numCharElements);

			FileActionInfo const actionInfo{ .path = dirPath / filename, .action = translate_file_action(notifyInfo->Action) };

			std::filesystem::path const* callbackKey = &filename;

			if (dir.watchedFiles.contains(WATCH_ALL_FILES))
			{
				if (!dir.watchedFiles.contains(filename))
				{
					std::scoped_lock lock{ dirContainerMutex };

					FILETIME lastModifiedTime = {};
					DWORD fileSize = {};

					HANDLE winFileHandle = CreateFileW(actionInfo.path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
					
					ASSERTION(winFileHandle != INVALID_HANDLE_VALUE && "Could not reference file!");

					if (winFileHandle != INVALID_HANDLE_VALUE)
					{
						GetFileTime(winFileHandle, nullptr, nullptr, &lastModifiedTime);
						fileSize = GetFileSize(winFileHandle, nullptr);
					}

					dir.watchedFiles.emplace(filename, FileContext{ .handle = winFileHandle, .lastModifiedTime = lastModifiedTime, .lastFileSize = fileSize });
				}

				callbackKey = &WATCH_ALL_FILES;
			}

			std::call_once(
				onceFlag, 
				[&recursiveDirWatch, &dir, callbackKey]() -> void
				{
					FileContext const& rootFileContext = dir.watchedFiles[*callbackKey];
					recursiveDirWatch = rootFileContext.recursive; 
				}
			);

			FileContext& fileContext = dir.watchedFiles[filename];

			FILETIME currentModifiedTime = {};

			GetFileTime(fileContext.handle, nullptr, nullptr, &currentModifiedTime);

			DWORD currentFileSize = {};
			currentFileSize = GetFileSize(fileContext.handle, nullptr);

			bool const modifiedTimeLargerThan = (currentModifiedTime.dwHighDateTime >= fileContext.lastModifiedTime.dwHighDateTime) && (currentModifiedTime.dwLowDateTime != fileContext.lastModifiedTime.dwLowDateTime);
			bool const currentFileSizeNonZero = currentFileSize != 0;
			bool const fileSizeNotTheSame = fileContext.lastFileSize != currentFileSize;

			if (modifiedTimeLargerThan && (currentFileSizeNonZero || fileSizeNotTheSame))
			{
				dir.watchedFiles[*callbackKey].callback(actionInfo);

				fileContext.lastModifiedTime = currentModifiedTime;
				fileContext.lastFileSize = currentFileSize;
			}

		} while (notifyInfo->NextEntryOffset != 0);

		/**
		* ReadDirectoryChangesW needs to be invoked after each change.
		* Windows's documentation does not explicitly state anything about this but findings suggest that
		* a read directory request to the kernel ends whenever it's been fulfilled.
		* 
		* Also note that a single ReadDirectoryChangesW call can fire more than 1 change packet to the I/O completion port.
		*/
		ReadDirectoryChangesW(
			dir.handle,
			&dir.buffer,
			static_cast<DWORD>(sizeof(dir.buffer)),
			recursiveDirWatch,
			NOTIFICATION_FILTER,
			0,
			&dir.overlapped,
			0
		);
	}

	[[nodiscard]] auto ok() const -> bool
	{
		return ioCompletionPort != nullptr && ioCompletionPort != INVALID_HANDLE_VALUE;
	}

	/**
	* Creates a directory entry if it hasn't existed. Returns the bucket value to an existing one if the directory store already contains the value specified by "dirPath".
	*/
	[[nodiscard]] auto try_create_directory_watch(std::filesystem::path const& dirPath, bool recursive) -> size_t
	{
		if (directories.contains(dirPath))
		{
			return directories.bucket(dirPath);
		}

		HANDLE hnd = CreateFileW(
			dirPath.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr
		);

		if (hnd == INVALID_HANDLE_VALUE)
		{
			return std::numeric_limits<size_t>::max();
		}

		auto&& [k, dirCtx] = directories.emplace(dirPath, DirectoryContext{ .handle = hnd });
		size_t const bucketValue = directories.bucket(k);

		if (!set_up_directory_watch(dirCtx, static_cast<ULONG_PTR>(bucketValue), recursive))
		{
			stop_directory_watch(dirCtx);

			directories.erase(k);

			return std::numeric_limits<size_t>::max();
		}

		return bucketValue;
	}

	[[nodiscard]] auto create_file_watch(FileWatchInfo&& watchInfo) -> file_watch_id
	{
		std::scoped_lock lock{ dirContainerMutex };

		FileIndexInfo index = INVALID_FILE_INDEX_INFO;

		std::filesystem::path directory = watchInfo.path.parent_path();
		std::filesystem::path filename  = watchInfo.path.filename();

		bool const recursiveDirWatch = std::filesystem::is_directory(watchInfo.path) && watchInfo.recursive;

		if (size_t dirBucketValue = try_create_directory_watch(directory, recursiveDirWatch); dirBucketValue != std::numeric_limits<size_t>::max())
		{
			auto&& [k, dirCtx] = directories.element_at_bucket(dirBucketValue);
			
			if (!dirCtx.watchedFiles.contains(WATCH_ALL_FILES))
			{
				HANDLE winFileHandle = nullptr;
				FILETIME lastModifiedTime = {};
				DWORD fileSize = {};

				if (filename.empty())
				{
					filename = WATCH_ALL_FILES;
				}
				else if (filename != WATCH_ALL_FILES)
				{
					winFileHandle = CreateFileW(watchInfo.path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);

					ASSERTION(winFileHandle != INVALID_HANDLE_VALUE && "Could not reference file!");

					if (winFileHandle != INVALID_HANDLE_VALUE)
					{
						GetFileTime(winFileHandle, nullptr, nullptr, &lastModifiedTime);
						fileSize = GetFileSize(winFileHandle, nullptr);
					}
				}

				dirCtx.watchedFiles.emplace(
					filename,
					FileContext{
						.handle = winFileHandle,
						.lastModifiedTime = lastModifiedTime,
						.lastFileSize = fileSize,
						.callback = std::move(watchInfo.callback),
						.recursive = recursiveDirWatch
					}
				);
			}

			size_t fileBucketValue = dirCtx.watchedFiles.bucket(filename);

			index.dir  = static_cast<uint32>(dirBucketValue);
			index.file = static_cast<uint32>(fileBucketValue);
		}

		return file_watch_id{ static_cast<size_t>(index) };
	}

	auto clear_zombie_watches() -> void
	{
		std::scoped_lock zombieLock{ zombieContainerMutex };
		std::scoped_lock dirLock{ dirContainerMutex };

		for (file_watch_id const& id : zombies)
		{
			FileIndexInfo const idAlias = FileIndexInfo::from(id.get());
			
			auto&& [dirPath, dirCtx] = directories.element_at_bucket(static_cast<size_t>(idAlias.dir));
			auto&& [filename, callbackFn] = dirCtx.watchedFiles.element_at_bucket(static_cast<size_t>(idAlias.file));

			dirCtx.watchedFiles.erase(filename);

			if (!dirCtx.watchedFiles.size())
			{
				stop_directory_watch(dirCtx);

				directories.erase(dirPath);
			}
		}

		zombies.clear();
	}

	auto zombify_watch(file_watch_id const& id) -> void
	{
		std::scoped_lock lock{ zombieContainerMutex };

		if (zombies.contains(id))
		{
			return;
		}

		zombies.insert(id);
	}

	auto unwatch_all() -> void
	{
		std::scoped_lock lock{ dirContainerMutex };

		for (auto& [dirPath, dirCtx] : directories)
		{
			stop_directory_watch(dirCtx);

			dirCtx.watchedFiles.clear();
		}

		directories.release();
	}

	auto stop_directory_watch(DirectoryContext const& dir) -> void
	{
		if (dir.handle != nullptr &&
			dir.handle != INVALID_HANDLE_VALUE)
		{
			CancelIo(dir.handle);
			CloseHandle(dir.handle);
		}
	}

	auto set_up_directory_watch(DirectoryContext& dir, ULONG_PTR completionKey, bool recursive) -> bool
	{
		auto const ioPortHandle = CreateIoCompletionPort(
			dir.handle,
			ioCompletionPort,
			completionKey,
			1
		);

		auto const readDirChangeResult = ReadDirectoryChangesW(
			dir.handle,
			&dir.buffer,
			static_cast<DWORD>(sizeof(dir.buffer)),
			recursive,
			NOTIFICATION_FILTER,
			0,
			&dir.overlapped,
			0
		);

		if (readDirChangeResult == FALSE)
		{
			auto error = GetLastError();
			switch (error)
			{
			case ERROR_INVALID_PARAMETER:
				fmt::print("Buffer length is greater than 64KB.");
				break;
			case ERROR_NOACCESS:
				fmt::print("Buffer is not aligned to DWORD boundary");
				break;
			default:
				fmt::print("GetLastError() returned -> {}", error);
				break;
			}
		}

		return ioPortHandle == ioCompletionPort && readDirChangeResult == TRUE;
	}

	[[nodiscard]] auto init() -> bool
	{
		// Create IO Completion Port.
		// Spawn async file watcher thread.
		ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

		if (!ioCompletionPort || ioCompletionPort == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		fileWatcherThread = std::jthread{ [this]() -> void { WindowsFileWatcherContext::filewatch_update_loop(*this); } };
		SetThreadDescription(fileWatcherThread.native_handle(), L"Async File Watcher");

		return true;
	}

	auto terminate() -> void
	{
		// Send a dummy packet to terminate the async file watcher thread.
		PostQueuedCompletionStatus(ioCompletionPort, 0, TERMINATE_PACKET, nullptr);
	}
};

std::filesystem::path const WindowsFileWatcherContext::WATCH_ALL_FILES = L"*";
std::unique_ptr<WindowsFileWatcherContext> WindowsFileWatcherContext::ctx = {};

auto initialize_file_watcher() -> bool
{
	if (WindowsFileWatcherContext::ctx)
	{
		return true;
	}

	WindowsFileWatcherContext::ctx = std::make_unique<WindowsFileWatcherContext>();

	return WindowsFileWatcherContext::ctx->init();
}

auto terminate_file_watcher() -> void
{
	if (!WindowsFileWatcherContext::ctx)
	{
		return;
	}

	WindowsFileWatcherContext::ctx->terminate();

	//while (!WindowsFileWatcherContext::ctx->fileWatcherThread.joinable());

	//WindowsFileWatcherContext::ctx->fileWatcherThread.join();
	//WindowsFileWatcherContext::ctx.~unique_ptr();
}

auto watch(FileWatchInfo&& watchInfo) -> file_watch_id
{
	if (!WindowsFileWatcherContext::ctx || !WindowsFileWatcherContext::ctx->ok())
	{
		return file_watch_id::invalid_handle();
	}

	// NOTE(afiq):
	// 
	// Momentarily disable directory watch support.
	// Watching directory has additional requirements to handle delete actions as noted below
	// https://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw_19.html.
	//
	// "Another potential pitfall of this function is that the referenced directory itself 
	// is now �in use� and so can't be deleted. To monitor files in a directory and still allow 
	// the directory to be deleted, you would have to monitor the parent directory and its children."
	//

	return WindowsFileWatcherContext::ctx->create_file_watch(std::move(watchInfo));
}

auto unwatch(file_watch_id& id) -> void
{
	if (WindowsFileWatcherContext::ctx && id.valid())
	{
		WindowsFileWatcherContext::ctx->zombify_watch(id);

		id.invalidate();
	}
}

}
