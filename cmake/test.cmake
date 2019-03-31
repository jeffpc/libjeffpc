#
# Copyright (c) 2016-2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

macro(build_perf_bin name)
	add_executable("perf_${name}"
		"perf_${name}.c"
	)

	target_link_libraries("perf_${name}"
		jeffpc
	)
endmacro()

macro(build_test_bin name)
	add_executable("test_${name}"
		"test_${name}.c"
	)

	target_link_libraries("test_${name}"
		jeffpc
	)
endmacro()

macro(build_test_bin_files name)
	build_test_bin(${name})

	set_source_files_properties("test_${name}.c" PROPERTIES
		COMPILE_FLAGS "-DUSE_FILENAME_ARGS"
	)
endmacro()

macro(build_test_bin_and_run name)
	build_test_bin(${name})
	add_test(NAME "${name}"
		 COMMAND "${CMAKE_BINARY_DIR}/tests/test_${name}"
	)
	set_tests_properties("${name}" PROPERTIES
		ENVIRONMENT "UMEM_DEBUG=default,verbose"
	)
endmacro()

macro(build_test_bin_and_run_files name iext oext dirs)
	build_test_bin_files(${name})
	foreach(DIR ${dirs})
		file(GLOB TESTS
		     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
		     "${DIR}/*.${iext}")
		foreach(TEST ${TESTS})
			add_test(NAME "${name}:${TEST}"
				 COMMAND "${CMAKE_BINARY_DIR}/tests/test_${name}"
					 "${CMAKE_CURRENT_SOURCE_DIR}/${TEST}"
			)
			set_tests_properties("${name}:${TEST}" PROPERTIES
				ENVIRONMENT "UMEM_DEBUG=default,verbose"
			)
		endforeach()
	endforeach()
endmacro()
