/*
 * Copyright (c) 2017-2018 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __JEFFPC_BUFFER_IMPL_H
#define __JEFFPC_BUFFER_IMPL_H

#include <jeffpc/buffer.h>

extern const struct buffer_ops heap_buffer;
extern const struct buffer_ops sink_buffer;
extern const struct buffer_ops static_buffer_ro;
extern const struct buffer_ops static_buffer_rw;
extern const struct buffer_ops stdio_buffer;

/* clear implementations */
extern void generic_buffer_clear_memset(struct buffer *buffer, size_t off,
					size_t len);
extern void generic_buffer_clear_nop(struct buffer *buffer, size_t off,
				     size_t len);
extern void generic_buffer_clear_panic(struct buffer *buffer, size_t off,
				       size_t len);

/* copyin implementations */
extern void generic_buffer_copyin_memcpy(struct buffer *buffer, size_t off,
					 const void *newdata, size_t newdatalen);
extern void generic_buffer_copyin_nop(struct buffer *buffer, size_t off,
				      const void *newdata, size_t newdatalen);
extern void generic_buffer_copyin_panic(struct buffer *buffer, size_t off,
					const void *newdata, size_t newdatalen);

/* copyout implementations */
extern void generic_buffer_copyout_memcpy(struct buffer *buffer, size_t off,
					  void *data, size_t datalen);
extern void generic_buffer_copyout_nop(struct buffer *buffer, size_t off,
				       void *data, size_t datalen);
extern void generic_buffer_copyout_panic(struct buffer *buffer, size_t off,
					 void *data, size_t datalen);

#endif
