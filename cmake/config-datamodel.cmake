#
# Copyright (c) 2019 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

# Determine the data model

check_type_size("int" INT_SIZE)
check_type_size("long" LONG_SIZE)
check_type_size("void *" VOIDP_SIZE)
set(size_code "${INT_SIZE}:${LONG_SIZE}:${VOIDP_SIZE}")
if("${size_code}" STREQUAL "4:4:4")
	set(JEFFPC_DATAMODEL "ILP32")
	set(JEFFPC_ILP32 1)
elseif("${size_code}" STREQUAL "4:8:8")
	set(JEFFPC_DATAMODEL "LP64")
	set(JEFFPC_LP64 1)
else()
	message(FATAL_ERROR "Failed to determine data model "
		"(sizeof(int)=${INT_SIZE}, "
		"sizeof(long)=${LONG_SIZE}, "
		"sizeof(void *)=${VOIDP_SIZE})")
endif()

message(STATUS "Detected ${JEFFPC_DATAMODEL} data model")
