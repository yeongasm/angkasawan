include(${CMAKE_SOURCE_DIR}/cmake/Util.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME glaze
	GIT_TAG v4.0.3
	GIT_REPOSITORY https://github.com/stephenberry/glaze.git
	GIT_SHALLOW TRUE
)

set_target_properties(glaze_glaze PROPERTIES FOLDER thirdparty)