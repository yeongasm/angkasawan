#pragma once
#ifndef CORE_ENGINE_ENGINE_H
#define CORE_ENGINE_ENGINE_H

#include "io.h"
#include "event_queue.h"
#include "windowing.h"
#include "cursor.h"
#include "stat.h"
#include "application.h"

class Angkasawan;

namespace core
{

class Engine final
{
public:
	CORE_API Engine();
	CORE_API ~Engine();

	CORE_API bool				running						() const;
	CORE_API EngineState		set_state					(EngineState state);
	CORE_API EngineState		get_state					() const;
	CORE_API wnd::window_handle create_window				(wnd::WindowCreateInfo const&);	// Immediate.
	CORE_API bool				destroy_window				(wnd::window_handle& hnd);		// Deferred.
	CORE_API void*				get_application_handle		() const;
	CORE_API void*				get_window_native_handle	(wnd::window_handle hnd) const;
	CORE_API Point				get_window_position			(wnd::window_handle hnd) const;
	CORE_API Dimension			get_window_dimension		(wnd::window_handle hnd) const;
	CORE_API std::wstring_view	get_window_title			(wnd::window_handle hnd) const;
	CORE_API wnd::WindowState	get_window_state			(wnd::window_handle hnd) const;
	CORE_API bool				set_window_dimension		(wnd::window_handle hnd, Dimension dim);
	CORE_API bool				set_window_position			(wnd::window_handle hnd, Point pos);
	CORE_API bool				set_window_title			(wnd::window_handle hnd, std::wstring_view title);
	CORE_API void				set_window_as_fullscreen	(wnd::window_handle hnd);
	CORE_API void				register_window_listener	(wnd::window_handle hnd, wnd::WindowEvent ev, std::function<void()>&& callback, lib::wstring_128 identifier = {});
	CORE_API bool				is_window_minimized			(wnd::window_handle hnd) const;
	CORE_API auto				is_window_focused			(wnd::window_handle hnd) const -> bool;
	CORE_API uint32				num_windows					() const;
	CORE_API auto				show_cursor					(bool show = true) -> void;
	CORE_API auto				set_cursor_position			(wnd::window_handle hnd, Point point) -> void;

	template <std::derived_from<Application> T>
	bool register_application()
	{
		if (!mApp)
		{
			mApp = static_cast<T*>(lib::allocate_memory({ .size = sizeof(T) }));
			if (!mApp)
			{
				return false;
			}
			new (mApp) T{};
		}
		return mApp->initialize();
	}
private:
	os::EventQueue mEventQueue;
	wnd::WindowingContext mWindowContext;
	cursor::CursorContext mCursorContext;
	EngineState	mState;
	Application* mApp;

	friend class Angkasawan;

	CORE_API bool				initialize(int argc, char** argv);
	CORE_API void				run();
	CORE_API void				terminate();
};

CORE_API Engine& engine();

}

#endif // !CORE_ENGINE_ENGINE_H
