#
# Copyright (c) 2016-2020 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

build_test_bin_and_run_files(base64_encode raw "b64;b64url" base64/valid)
build_test_bin_and_run_files(base64_decode "b64;b64url" raw base64/valid)
build_test_bin_and_run_files(cbor_pack lisp cbor cbor-common/valid)
build_test_bin_and_run_files(cbor_unpack cbor lisp
	"cbor-common/valid;cbor-common/valid-unpack")
build_test_bin_and_run_files(nvl_pack lisp "cbor;json" nvl-pack)
build_test_bin_and_run_files(qstring qs lisp qstring-basic)
build_test_bin_and_run_files(sexpr_parser lisp "raw;txt" sexpr-parser)

build_test_bin_and_run(array)
build_test_bin_and_run(atomic-single-thread)
build_test_bin_and_run(bswap)
build_test_bin_and_run(buffer)
build_test_bin_and_run(cbor_peek)
build_test_bin_and_run(container_of)
build_test_bin_and_run(endian)
build_test_bin_and_run(errno)
build_test_bin_and_run(hexdump)
build_test_bin_and_run(is_p2)
build_test_bin_and_run(list)
build_test_bin_and_run(mutex-destroy-memcpy)
build_test_bin_and_run(mutex-destroy-null)
build_test_bin_and_run(mutex-init-null-both)
build_test_bin_and_run(mutex-init-null-class)
build_test_bin_and_run(mutex-init-null-lock)
build_test_bin_and_run(mutex-lock-destroyed)
build_test_bin_and_run(mutex-lock-held-class)
build_test_bin_and_run(mutex-lock-held-nesting)
build_test_bin_and_run(mutex-lock-memcpy)
build_test_bin_and_run(mutex-lock-null)
build_test_bin_and_run(mutex-unlock-destroyed)
build_test_bin_and_run(mutex-unlock-memcpy)
build_test_bin_and_run(mutex-unlock-null)
if(JEFFPC_LOCK_TRACKING)
# NOTE: These tests rely on checks that are not performed when lock tracking
# is disabled.
build_test_bin_and_run(mutex-destroy-held)
build_test_bin_and_run(mutex-lock-held)
build_test_bin_and_run(mutex-lock-held-multi-first)
build_test_bin_and_run(mutex-lock-held-multi-second)
build_test_bin_and_run(mutex-lock-held-multi-third)
build_test_bin_and_run(mutex-unlock-unheld)
endif()
build_test_bin_and_run(nvl)
build_test_bin_and_run(padding)
build_test_bin_and_run(rwlock-destroy-memcpy)
build_test_bin_and_run(rwlock-destroy-null)
build_test_bin_and_run(rwlock-init-null-both)
build_test_bin_and_run(rwlock-init-null-class)
build_test_bin_and_run(rwlock-init-null-lock)
build_test_bin_and_run(rwlock-rlock-destroyed)
build_test_bin_and_run(rwlock-rlock-held-class)
build_test_bin_and_run(rwlock-rlock-held-nesting)
build_test_bin_and_run(rwlock-rlock-memcpy)
build_test_bin_and_run(rwlock-rlock-null)
build_test_bin_and_run(rwlock-unlock-destroyed)
build_test_bin_and_run(rwlock-unlock-memcpy)
build_test_bin_and_run(rwlock-unlock-null)
build_test_bin_and_run(rwlock-wlock-destroyed)
build_test_bin_and_run(rwlock-wlock-held-class)
build_test_bin_and_run(rwlock-wlock-held-nesting)
build_test_bin_and_run(rwlock-wlock-memcpy)
build_test_bin_and_run(rwlock-wlock-null)
if(JEFFPC_LOCK_TRACKING)
# NOTE: These tests rely on checks that are not performed when lock tracking
# is disabled.
build_test_bin_and_run(rwlock-destroy-rheld)
build_test_bin_and_run(rwlock-destroy-wheld)
build_test_bin_and_run(rwlock-rlock-rheld)
build_test_bin_and_run(rwlock-rlock-wheld)
build_test_bin_and_run(rwlock-rlock-held-multi-first)
build_test_bin_and_run(rwlock-rlock-held-multi-second)
build_test_bin_and_run(rwlock-rlock-held-multi-third)
build_test_bin_and_run(rwlock-unlock-unheld)
build_test_bin_and_run(rwlock-wlock-rheld)
build_test_bin_and_run(rwlock-wlock-wheld)
build_test_bin_and_run(rwlock-wlock-held-multi-first)
build_test_bin_and_run(rwlock-wlock-held-multi-second)
build_test_bin_and_run(rwlock-wlock-held-multi-third)
endif()
build_test_bin_and_run(sexpr_compact)
build_test_bin_and_run(sexpr_eval)
build_test_bin_and_run(sexpr_iter)
build_test_bin_and_run(str2uint)
build_test_bin_and_run(tree_bst)
build_test_bin_and_run(tree_rb)
build_test_bin_and_run(urldecode)
build_test_bin_and_run(utf32-to-utf8)
build_test_bin_and_run(utf8-to-utf32)
build_test_bin_and_run(uuid)
build_test_bin_and_run(version)
build_test_bin_and_run(xstrerror)
