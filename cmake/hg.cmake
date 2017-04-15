#
# Copyright (c) 2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

find_package(Hg)

macro(gethgrev varname template defstr)
	set(${varname} "${defstr}")

	if(HG_FOUND)
		execute_process(
			COMMAND ${HG_EXECUTABLE} log -r . --template ${template}
			WORKING_DIRECTORY ${SOURCE_DIR}
			OUTPUT_VARIABLE ${varname}
			RESULT_VARIABLE res_var
		)
		if(NOT ${res_var} EQUAL 0)
			set(${varname} "${defstr}")
		endif()
	endif()
endmacro()

gethgrev(HG_COMMIT_STR "{node}" "0000000000000000000000000000000000000000")

# TODO: if there are uncommitted changes, append -dirty
gethgrev(VERSION
	"{ifeq(latesttagdistance,0,\"{latesttag}\",\"{latesttag}-{latesttagdistance}-{node|short}\")}"
	"${JEFFPC_VERSION}")

if(${HG_COMMIT_STR} EQUAL "0000000000000000000000000000000000000000")
	message(WARNING "Failed to get revision info from Mercurial.")
endif()

set(HG_COMMIT "")
set(HEX ${HG_COMMIT_STR})
while("${HEX}" MATCHES ".")
	string(SUBSTRING ${HEX} 0 2 COMMIT_BYTE)
	string(SUBSTRING ${HEX} 2 -1 HEX)
	set(HG_COMMIT "${HG_COMMIT}0x${COMMIT_BYTE},")
endwhile()

set(vstring "/* hgversion.h - Generated by cmake. Changes will be lost! */\n"
	"#ifndef __LIBJEFFPC_INTERNAL_VERSION_H\n"
	"#define __LIBJEFFPC_INTERNAL_VERSION_H\n"
	"#define HG_COMMIT_STR \"${HG_COMMIT_STR}\"\n"
	"#define HG_COMMIT_BIN { ${HG_COMMIT} }\n"
	"#define VERSION \"${VERSION}\"\n"
	"#endif\n")

file(WRITE hgversion.h.txt ${vstring})
# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        hgversion.h.txt ${BINARY_DIR}/hgversion.h)
execute_process(COMMAND ${CMAKE_COMMAND} -E remove hgversion.h.txt)
