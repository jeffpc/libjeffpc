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

build_perf_bin(tree)

build_test_bin_and_run_files(cbor_pack cbor-pack/*.lisp)
build_test_bin_and_run_files(nvl_pack nvl-pack/*.lisp)
build_test_bin_and_run_files(qstring qstring-basic/*.qs)
build_test_bin_and_run_files(sexpr_parser sexpr-parser/*.lisp)

build_test_bin_and_run(array)
build_test_bin_and_run(atomic-single-thread)
build_test_bin_and_run(bswap)
build_test_bin_and_run(buffer)
build_test_bin_and_run(container_of)
build_test_bin_and_run(endian)
build_test_bin_and_run(errno)
build_test_bin_and_run(hexdump)
build_test_bin_and_run(list)
build_test_bin_and_run(nvl)
build_test_bin_and_run(padding)
build_test_bin_and_run(sexpr_eval)
build_test_bin_and_run(sexpr_iter)
build_test_bin_and_run(tree_bst)
build_test_bin_and_run(tree_rb)
build_test_bin_and_run(urldecode)
build_test_bin_and_run(utf32-to-utf8)
build_test_bin_and_run(utf8-to-utf32)
build_test_bin_and_run(uuid)
build_test_bin_and_run(version)
