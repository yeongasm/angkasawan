#pragma once
#ifndef CORE_WINDOWING_WINDOWING_H
#define CORE_WINDOWING_WINDOWING_H

#include <array>
#include "platform_interface/platform_interface.h"
#include "types/handle.h"
#include "shared/engine_common.h"

namespace core
{

enum class WindowState
{
	Ok = 0,
	Invalid,
	Closing,
	Queued_For_Create,
	Queued_For_Destroy,
	Queued_For_Update,
};

class WindowingManager;
class NativeWindow;

using NativeWndHnd = ftl::Handle<NativeWindow>;

enum NativeWindowFlags : uint32
{
	Native_Window_Flags_Root_Window = 1 << 0,
	Native_Window_Flags_Allow_Drop  = 1 << 1,
	Native_Window_Flags_Catch_Input = 1 << 2,
	Native_Window_Flags_Borderless	= 1 << 3
};

using WindowFlags_t = uint32;

struct NativeWindowCreateInfo
{
	wide_literal_t	title;
	Point			position;
	Dimension		dimension;
	WindowFlags_t	flags;
};

enum class NativeWindowEvent
{
	Focus,
	Resize,
	Move,
	Close
};

class ENGINE_API NativeWindow
{
public:
	NativeWindow();
	NativeWindow(WindowingManager& manager);

	NativeWindow(NativeWindow&&);
	NativeWindow& operator=(NativeWindow&&);

	~NativeWindow();

	ErrorCode		destroy			();

	os::Handle		get_application_handle() const;

	os::WndHandle	get_raw_handle	() const;
	bool			is_fullscreen	() const;
	bool			catches_input	() const;
	bool			allow_drop_files() const;

	bool			is_focused		() const;

	void			set_dimension	(int32 width, int32 height);
	void			set_position	(int32 x, int32 y);
	void			set_title		(wide_literal_t title);

	Point			get_position	() const;
	Dimension		get_dimension	() const;
	wide_literal_t	get_title		() const;
	WindowState		get_state		() const;
	bool			on				(NativeWindowEvent ev, std::function<void()>&& callback);

private:

	friend class WindowingManager;
	using EventCallbacks = std::array<std::function<void()>, 4>;

	ftl::UniString64	m_title;
	EventCallbacks		m_eventCallbacks;
	WindowingManager*	m_manager;
	os::WndHandle		m_handle;
	Point				m_position;
	Dimension			m_dimension;
	WindowState			m_state;
	bool				m_fullscreen;
	bool				m_allowDropFiles;
	bool				m_catchInputEvents;
	bool				m_focused;

	NativeWindow(const NativeWindow&)				= delete;
	NativeWindow& operator=(const NativeWindow&)	= delete;

	void			invoke_callback	(NativeWindowEvent ev);
};

template <typename Payload_t>
using WinResult = ftl::Result<ErrorCode, Payload_t>;

class ENGINE_API WindowingManager
{
public:
	WindowingManager(PlatformOSInterface& platform);
	~WindowingManager();

	WinResult<NativeWndHnd>		create_window				(NativeWindowCreateInfo const&, bool = false);
	ErrorCode					destroy_window				(NativeWndHnd handle);
	WinResult<NativeWindow*>	get_window					(NativeWndHnd handle);
	WinResult<NativeWindow*>	get_root_window				() const;
	void						on_tick						();
	/*bool						allow_multi_windows			() const;*/
	size_t						get_num_windows				() const;
	void						listen_to_event				(OSEvent const& ev);
	bool						are_window_dimensions_valid	(Dimension const& dimension);
	ErrorCode					set_window_as_fullscreen	(NativeWndHnd handle);
	NativeWindow*				get_window_from_raw_handle	(os::WndHandle hnd);
private:

	friend class NativeWindow;

	struct WindowUpdateContext
	{
		enum class Type
		{
			Create,
			Update,
			Move,
			Resize,
			Fullscreen,
			Renamed,
			Destroy
		};
		Type type;
		NativeWindow* window;
	};

	using WindowUpdateStack = ftl::Array<WindowUpdateContext>;

	ftl::Array<NativeWindow>	m_windows;
	ftl::Array<size_t>			m_freeIndices;
	WindowUpdateStack			m_updateStack;
	PlatformOSInterface&		m_platform;
	NativeWindow*				m_rootWindow;
	uint32						m_numWindows;
	/*bool						m_allowMultiWindow;*/

	bool		is_handle_valid	(NativeWndHnd handle);
	ErrorCode	create_window	(NativeWindow& window, size_t& at, bool);
	ErrorCode	destroy_window	(NativeWindow* window);
	void		push_for_update	(NativeWindow& window, WindowUpdateContext::Type type);

	bool		create_os_window	(NativeWindow& window);
	void		destroy_os_window	(NativeWindow& window);
	void		maximize_window		(NativeWindow& window);

	void		update_window_position(NativeWindow& window);
	
	void		update_window_properties(NativeWindow& window, const OSEvent& ev);
	void		handle_close_on_event	(NativeWindow& window);

	void		force_build_windows();

	NativeWindow* get_native_window_from_raw_handle(os::WndHandle hnd);

	WindowingManager(const WindowingManager&)	= delete;
	WindowingManager(WindowingManager&&)		= delete;
	WindowingManager& operator=(const WindowingManager&) = delete;
	WindowingManager& operator=(WindowingManager&&)		 = delete;
};

}

#endif // !CORE_WINDOWING_WINDOWING_H
