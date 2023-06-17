include(${CMAKE_SOURCE_DIR}/Util.cmake)

set(
	source_list
	"main.cpp"
)

add_executable(FoundationUnitTest ${source_list})

set_target_properties(
	FoundationUnitTest
	PROPERTIES
		CXX_STANDARD 20 
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		LINKER_LANGUAGE CXX
		UNITY_BUILD ON
)

target_link_libraries(FoundationUnitTest PRIVATE gtest_main Foundation)
set_target_properties(FoundationUnitTest PROPERTIES FOLDER unit_tests)

add_test(
	NAME FoundationUnitTest
	COMMAND FoundationUnitTest
)

add_custom_command(
	TARGET 
		FoundationUnitTest POST_BUILD
	COMMAND 
		${CMAKE_COMMAND} -E copy 
		$<TARGET_RUNTIME_DLLS:FoundationUnitTest> 
		$<TARGET_FILE_DIR:FoundationUnitTest>
	COMMAND_EXPAND_LISTS
)

#assign_source_group(${source_list})
warnings_as_errors(FoundationUnitTest)
