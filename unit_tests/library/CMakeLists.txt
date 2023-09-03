include(${CMAKE_SOURCE_DIR}/Util.cmake)

set(
	source_list
	"main.cpp"
)

add_executable(LibraryUnitTest ${source_list})
target_compile_features(LibraryUnitTest PUBLIC cxx_std_20)

target_link_libraries(LibraryUnitTest PRIVATE gtest_main Library)
set_target_properties(LibraryUnitTest PROPERTIES FOLDER unit_tests)

add_test(
	NAME LibraryUnitTest
	COMMAND LibraryUnitTest
)

add_custom_command(
	TARGET 
		LibraryUnitTest POST_BUILD
	COMMAND 
		${CMAKE_COMMAND} -E copy 
		$<TARGET_RUNTIME_DLLS:LibraryUnitTest> 
		$<TARGET_FILE_DIR:LibraryUnitTest>
	COMMAND_EXPAND_LISTS
)

#assign_source_group(${source_list})
warnings_as_errors(LibraryUnitTest)
