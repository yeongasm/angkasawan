#pragma once
#ifndef CORE_HAL_WIN_WIN_PLATFORM_H
#define CORE_HAL_WIN_WIN_PLATFORM_H

#include "os_shims.h"
#include "platform_interface/platform_common.h"
#include "containers/string.h"
#include "containers/map.h"

namespace os
{

class ENGINE_API WinOSInterface
{
public:

	WinOSInterface();
	~WinOSInterface();

	void		sleep					(size_t milliseconds) const;
	size_t		get_current_thread_id	() const;
	bool		copy_to_clipboard		(WndHandle handle, literal_t text, size_t len) const;
	bool		copy_to_clipboard		(WndHandle handle, wide_literal_t text, size_t len) const;
	void		set_cursor				(core::IOCursorType type) const;
	void		clip_cursor				(int32 x, int32 y, int32 width, int32 height);
	void		unclip_cursor			() const;
	core::Point	get_mouse_screen_pos	();
	void		set_mouse_pos			(WndHandle handle, int32 x, int32 y) const;
	void		display_cursor			(bool show) const;
	Handle		get_app_instance_handle	() const;
	WndHandle	create_window			(const core::PlatformWindowCreateInfo& info);
	void		destroy_window			(WndHandle handle) const;
	core::Rect	get_window_screen_rect	(WndHandle handle) const;
	core::Rect	get_client_screen_rect	(WndHandle handle) const;	
	void		set_window_screen_rect	(WndHandle handle, const core::Rect& rect) const;
	void		set_window_client_rect	(WndHandle handle, const core::Rect& rect) const;
	void		set_window_title		(WndHandle handle, literal_t title) const;
	void		set_window_title		(WndHandle handle, wide_literal_t title) const;
	void		maximize_window			(WndHandle handle) const;
	void		set_window_position		(WndHandle handle, int32 x, int32 y, int32 width, int32 height) const;
	void		set_full_screen			(WndHandle handle) const;
	bool		is_window_maximized		(WndHandle handle) const;
	WndHandle	get_focused_window		() const;

	core::Dimension get_monitor_dimension			(WndHandle handle) const;
	core::Dimension get_primary_monitor_dimension	() const;

	//core::Dimension get_monitor_dimension(uint32 monitor = 0) const;

	/**
	* Call this function only once. Should only be called by the engine.
	*/
	void peek_events() const;

	void register_os_event_callback(void* argument, std::function<void(void*, const core::OSEvent&)>&& callback);

private:

	struct WinOSContext;
	WinOSContext* m_ctx;

	WinOSInterface(const WinOSInterface&)				= delete;
	WinOSInterface& operator=(const WinOSInterface&)	= delete;
	WinOSInterface(WinOSInterface&&)					= delete;
	WinOSInterface& operator=(WinOSInterface&&)			= delete;
};

class ENGINE_API Dll
{
public:
	using Handle = os::DllHandle;

	Dll()  = default;
	~Dll() = default;
	Dll(literal_t path);

	Dll(Dll&& rhs);
	Dll& operator=(Dll&& rhs);

	bool load	();
	void unload	();

	template <typename T, typename... Args>
	T call(std::string_view name, Args&&... args)
	{
		using FuncPtr = T(*)(Args...);
		FuncPtr fn = static_cast<FuncPtr>(load_fn(name));
		if (fn)
		{
			fn(std::forward<Args>(args)...);
		}
		return T{};
	}

	std::string_view path	() const;
	Handle			 handle	() const;
	bool			 valid	() const;
private:
	using DllPath = ftl::StaticString<_MAX_PATH>;
	using AddressMap = ftl::Map<std::string_view, void*>;

	std::string_view	append_to_string_pool	(std::string_view str);
	void*				load_fn					(std::string_view name);

	DllPath		m_path;
	AddressMap	m_addressMap;
	std::string	m_stringPool;
	Handle		m_handle = nullptr;
};

}

#endif // !CORE_HAL_WIN_WIN_PLATFORM_H
