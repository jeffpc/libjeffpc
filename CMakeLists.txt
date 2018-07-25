#
# Copyright (c) 2016-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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
project(libjeffpc)

set(JEFFPC_VERSION 0.17-rc2)

enable_testing()

add_definitions(
	-D__EXTENSIONS__
	-D_REENTRANT
	-D_POSIX_C_SOURCE=200112L
	-D_GNU_SOURCE
)

add_compile_options(
	-Wall
	-Woverride-init
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
include(cmake/test.cmake)
include(cmake/paths.cmake)

# handle missing libumem
if(NOT UMEM_FOUND)
	set(UMEM_LIBRARY "")
	set(UMEM_INCLUDE_DIR "")
	set(UMEM_EXTRA_SOURCE)
else()
	set(UMEM_EXTRA_SOURCE slab_umem.c)
endif()

find_package(BISON)
find_package(FLEX)

include_directories(
	include
	${UMEM_INCLUDE_DIR}
)

# include the current source dir but only for: #include "foo.h"
add_compile_options(
	-iquote${CMAKE_CURRENT_SOURCE_DIR}
	-iquote${CMAKE_CURRENT_BINARY_DIR}
)

BISON_TARGET(sexpr sexpr.y ${CMAKE_CURRENT_BINARY_DIR}/sexpr.tab.c
	COMPILE_FLAGS "-p sexpr_reader_")
FLEX_TARGET(sexpr sexpr.l ${CMAKE_CURRENT_BINARY_DIR}/sexpr.lex.c
	COMPILE_FLAGS "-P sexpr_reader_")
ADD_FLEX_BISON_DEPENDENCY(sexpr sexpr)

add_library(jeffpc SHARED
	array.c
	bst.c
	buffer.c
	buffer_const.c
	buffer_heap.c
	buffer_sink.c
	buffer_stdio.c
	cstr.c
	error.c
	fmt_cbor.c
	hexdump.c
	init.c
	io.c
	list.c
	mem_array.c
	nvl.c
	nvl_convert.c
	padding.c
	qstring.c
	rand.c
	rbtree.c
	scgisvc.c
	sexpr.c
	sexpr_dump.c
	sexpr_eval.c
	${FLEX_sexpr_OUTPUTS} ${BISON_sexpr_OUTPUTS}
	slab.c
	sock.c
	socksvc.c
	str.c
	synch.c
	taskq.c
	tree.c
	urldecode.c
	unicode.c
	uuid.c
	val.c
	val_array.c
	val_dump.c
	val_fmt_cbor.c
	val_fmt_json.c
	val_nvl.c
	val_pack.c
	val_unpack.c
	version.c
	${UMEM_EXTRA_SOURCE}
)

target_link_libraries(jeffpc
	${UMEM_LIBRARY}
	pthread
	socket
	nsl
)

target_apply_mapfile(jeffpc mapfile-vers)

install(TARGETS	jeffpc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(FILES	include/jeffpc/atomic.h
		include/jeffpc/bst.h
		include/jeffpc/buffer.h
		include/jeffpc/config-synch.h
		include/jeffpc/cstr.h
		include/jeffpc/error.h
		include/jeffpc/hexdump.h
		include/jeffpc/int.h
		include/jeffpc/io.h
		include/jeffpc/jeffpc.h
		include/jeffpc/list.h
		include/jeffpc/mem.h
		include/jeffpc/nvl.h
		include/jeffpc/padding.h
		include/jeffpc/qstring.h
		include/jeffpc/rand.h
		include/jeffpc/rbtree.h
		include/jeffpc/refcnt.h
		include/jeffpc/scgi.h
		include/jeffpc/scgisvc.h
		include/jeffpc/sexpr.h
		include/jeffpc/sock.h
		include/jeffpc/socksvc.h
		include/jeffpc/str.h
		include/jeffpc/synch.h
		include/jeffpc/taskq.h
		include/jeffpc/thread.h
		include/jeffpc/time.h
		include/jeffpc/tree_private.h
		include/jeffpc/types.h
		include/jeffpc/urldecode.h
		include/jeffpc/unicode.h
		include/jeffpc/uuid.h
		include/jeffpc/val.h
		include/jeffpc/version.h
		${CMAKE_CURRENT_BINARY_DIR}/include/jeffpc/config.h
	DESTINATION include/jeffpc
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

add_custom_target(revisiontag ALL)

add_custom_command(TARGET revisiontag
	COMMAND ${CMAKE_COMMAND}
		-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
		-DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
		-DJEFFPC_VERSION=v${JEFFPC_VERSION}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hg.cmake
)

add_dependencies(jeffpc revisiontag)

#
# REPL executable
#

add_executable(sexpr-repl EXCLUDE_FROM_ALL
	repl.c
)

target_link_libraries(sexpr-repl
	tecla
	jeffpc
)

add_subdirectory(tests)
