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

include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckPrototypeDefinition)
include(CheckSymbolExists)
include(TestBigEndian)

test_big_endian(JEFFPC_CPU_BIG_ENDIAN)

include("${CMAKE_DIR}/config-datamodel.cmake")

# Find which library a particular function exists in
macro(find_symbol fxn libs output_lib_name)
	string(TOUPPER ${fxn} name)
	check_function_exists(${fxn} HAVE_${name})
	if(HAVE_${name})
		set(${output_lib_name})
	else()
		foreach(lib ${libs})
			string(TOUPPER ${lib} libname)
			check_library_exists(${lib} ${fxn} "" HAVE_${libname}_${name})
			if(HAVE_${libname}_${name})
				set(HAVE_${name} 1)
				set(${output_lib_name} ${lib})
				break()
			endif()
		endforeach()
	endif()
	if(NOT HAVE_${name})
		message("-- Could not find ${fxn}")
	endif()
endmacro()

check_prototype_definition(__assert
	"void __assert(const char *func, const char *file, int line, const char *expr)"
	""
	"assert.h"
	JEFFPC_HAVE___ASSERT_FREEBSD_STYLE)
check_prototype_definition(__assert
	"void __assert(const char *expr, const char *file, int line)"
	""
	"assert.h"
	JEFFPC_HAVE___ASSERT_LINUX_STYLE)
check_function_exists(arc4random JEFFPC_HAVE_ARC4RANDOM)
check_function_exists(arc4random_buf JEFFPC_HAVE_ARC4RANDOM_BUF)
check_function_exists(assfail JEFFPC_HAVE_ASSFAIL)
check_function_exists(addrtosymstr JEFFPC_HAVE_ADDRTOSYMSTR)
check_function_exists(pthread_cond_reltimedwait_np
	JEFFPC_HAVE_PTHREAD_COND_RELTIMEDWAIT_NP)
check_function_exists(reallocarray JEFFPC_HAVE_REALLOCARRAY)
check_function_exists(recallocarray JEFFPC_HAVE_RECALLOCARRAY)
check_include_files(alloca.h JEFFPC_HAVE_ALLOCA_H)
check_include_files(port.h JEFFPC_HAVE_PORT_H)
check_include_files(sys/debug.h JEFFPC_HAVE_SYS_DEBUG_H)
check_symbol_exists(EAI_ADDRFAMILY "netdb.h" JEFFPC_HAVE_EAI_ADDRFAMILY)
check_symbol_exists(EAI_NODATA "netdb.h" JEFFPC_HAVE_EAI_NODATA)

find_symbol(accept "socket" SOCKET_LIBRARY)
find_symbol(backtrace "execinfo" EXECINFO_LIBRARY)
find_symbol(xdr_opaque "nsl" XDR_LIBRARY)

include("${CMAKE_DIR}/config-gnu-ld.cmake")

set(CMAKE_MODULE_PATH "${CMAKE_DIR}/Modules")
find_package(umem)

# set a value that's "exported" into the generated config file & set up fake
# but safe values when libumem is missing
set(JEFFPC_HAVE_UMEM ${UMEM_FOUND})
if(NOT UMEM_FOUND)
	set(UMEM_LIBRARY "")
	set(UMEM_INCLUDE_DIR "")
endif()

if(NOT WITHOUT_LOCK_TRACKING)
	set(JEFFPC_LOCK_TRACKING 1)
endif()

set(JEFFPC_LOCK_DEP_COUNT 16)
set(JEFFPC_LOCK_STACK_DEPTH 32)

set(JEFFPC_TREE_COMPACT 1)

include("${CMAKE_DIR}/config-errno.cmake")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/jeffpc/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/include/jeffpc/config.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
