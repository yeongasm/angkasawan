include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME imgui
    VERSION 1.90.8-docking
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.8-docking.zip
    DOWNLOAD_ONLY True
)

# ImGui doesn't come with CMake so we have to do it ourselves
if (imgui_ADDED)
	add_library(imgui STATIC)
	target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

    set(
        imgui_header_files
        ${imgui_SOURCE_DIR}/imgui.h
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
    )

    set(
        imgui_source_files
		${imgui_SOURCE_DIR}/imgui_demo.cpp
		${imgui_SOURCE_DIR}/imgui_draw.cpp
		${imgui_SOURCE_DIR}/imgui_tables.cpp
		${imgui_SOURCE_DIR}/imgui_widgets.cpp
		${imgui_SOURCE_DIR}/imgui.cpp
		${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    )

	target_sources(imgui
		PRIVATE
        ${imgui_header_files}
        ${imgui_source_files}
	)

    target_compile_features(imgui PUBLIC cxx_std_20)
    set_target_properties(imgui PROPERTIES FOLDER thirdparty)
endif()