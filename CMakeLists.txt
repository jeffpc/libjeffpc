#
# Copyright (c) 2016 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

cmake_minimum_required(VERSION 2.8.12.2)

enable_testing()

add_definitions(
	-D__EXTENSIONS__
	-D_REENTRANT
)

add_compile_options(
	-Wall
	-O2
	-g
	-std=gnu99
	-fno-omit-frame-pointer
	$<$<C_COMPILER_ID:gcc>:-fno-inline-small-functions>
	$<$<C_COMPILER_ID:gcc>:-fno-inline-functions-called-once>
)

set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/config.cmake)
include(cmake/mapfile.cmake)

# handle missing libumem
if(NOT UMEM_FOUND)
	set(UMEM_LIBRARY)
	set(UMEM_INCLUDE_DIR fakeumem/include)
	set(UMEM_EXTRA_SOURCE fakeumem/fakeumem.c)
else()
	set(UMEM_EXTRA_SOURCE)
endif()

find_package(BISON)
find_package(FLEX)

include_directories(
	include
	${UMEM_INCLUDE_DIR}
)

BISON_TARGET(sexpr sexpr.y sexpr.tab.c COMPILE_FLAGS "-p sexpr_reader_")
FLEX_TARGET(sexpr sexpr.l sexpr.lex.c COMPILE_FLAGS "-P sexpr_reader_")
ADD_FLEX_BISON_DEPENDENCY(sexpr sexpr)

add_library(jeffpc SHARED
	error.c
	init.c
	io.c
	rand.c
	sexpr.c
	${FLEX_sexpr_OUTPUTS} ${BISON_sexpr_OUTPUTS}
	str.c
	uuid.c
	val.c
	${UMEM_EXTRA_SOURCE}
)

target_link_libraries(jeffpc
	${UMEM_LIBRARY}
	uuid
)

target_apply_mapfile(jeffpc mapfile-vers)

install(TARGETS	jeffpc DESTINATION lib
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(FILES	include/jeffpc/atomic.h
		include/jeffpc/config.h
		include/jeffpc/error.h
		include/jeffpc/int.h
		include/jeffpc/io.h
		include/jeffpc/jeffpc.h
		include/jeffpc/rand.h
		include/jeffpc/refcnt.h
		include/jeffpc/sexpr.h
		include/jeffpc/str.h
		include/jeffpc/thread.h
		include/jeffpc/time.h
		include/jeffpc/uuid.h
		include/jeffpc/val.h
	DESTINATION include/jeffpc
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

function(simple_c_test type section bin data)
	add_test(NAME "${type}:${section}:${data}"
		 COMMAND "${CMAKE_BINARY_DIR}/test_${bin}"
			 "${CMAKE_CURRENT_SOURCE_DIR}/${data}"
	)
endfunction()

#
# Test related executables
#

add_executable(test_sexpr_parser
	test_sexpr_parser.c
)

target_link_libraries(test_sexpr_parser
	jeffpc
)

add_subdirectory(tests)
