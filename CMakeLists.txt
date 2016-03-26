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

include_directories(
	include
)

add_library(jeffpc SHARED
	error.c
	str.c
)

target_link_libraries(jeffpc
	umem
)

install(TARGETS	jeffpc DESTINATION lib
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(FILES	include/jeffpc/atomic.h
		include/jeffpc/error.h
		include/jeffpc/refcnt.h
		include/jeffpc/str.h
	DESTINATION include/jeffpc
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
