cmake_minimum_required (VERSION 3.1)
project (avdecc-lib_controller)
enable_testing()

include_directories( ../../../../lib/include )
include_directories( ../../src )


add_executable (test_get_compatible_streamformat "get_compatible_streamformat.cpp")
target_link_libraries(test_get_compatible_streamformat avdecc-lib_controller)
target_sources(test_get_compatible_streamformat PUBLIC "../../src/streamformats.cpp")
