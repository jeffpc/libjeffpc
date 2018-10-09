#!/bin/sh
#
# Copyright (c) 2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

if [ $# -ne 1 ] ; then
	echo "Usage: $0 <version>" >&2
	exit 1
fi

case "$1" in
	v*)
		version="`echo $1 | cut -c 2-`"
		tag="$1"
		;;
	*)
		version="$1"
		tag="v$1"
		;;
esac

echo "version = '$version'"
echo "ok?"
read n

gsed -i -e "s/set(JEFFPC_VERSION.*)$/set(JEFFPC_VERSION $version)/" CMakeLists.txt
(
	echo "libjeffpc $version"
	echo
	echo "Signed-off-by: Josef 'Jeff' Sipek <jeffpc@josefsipek.net>"
) | hg commit -l /dev/stdin

hg log -p --stat -r.

echo "version changed."
echo "ok?"
read n

hg tag $tag
hg sign --no-commit $tag
hg amend
