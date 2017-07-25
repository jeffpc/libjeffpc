/*
 * Copyright (c) 2015-2017 Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
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

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

#include <jeffpc/taskq.h>
#include <jeffpc/atomic.h>
#include <jeffpc/error.h>
#include <jeffpc/types.h>
#include <jeffpc/socksvc.h>

/*
 * This part of the communication library is a bit different.  It is meant
 * to abstract away the setup and teardown of a listening server.
 * Regardless of what the server is listening for, the same exact steps are
 * needed to start listening on a socket and accept connections.  The
 * difference from the rest of the common library is the fact that this code
 * is not shy about printing errors.
 */

#define MAX_SOCK_FDS		8
#define CONN_BACKLOG		64

union sockaddr_union {
	struct sockaddr_in inet;
	struct sockaddr_in6 inet6;
};

struct state {
	void *private;
	struct taskq *taskq;
	void (*func)(int, void *);
	int fds[MAX_SOCK_FDS];
	int nfds;
};

struct cb {
	struct state *state;
	int fd;
};

static atomic_t server_shutdown;

static void sigterm_handler(int signum, siginfo_t *info, void *unused)
{
	cmn_err(CE_INFO, "SIGTERM received");

	atomic_set(&server_shutdown, 1);
}

static void handle_signals(void)
{
	struct sigaction action;
	int ret;

	sigemptyset(&action.sa_mask);

	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;

	ret = sigaction(SIGPIPE, &action, NULL);
	if (ret)
		cmn_err(CE_INFO, "Failed to ignore SIGPIPE: %s",
			strerror(errno));

	action.sa_sigaction = sigterm_handler;
	action.sa_flags = SA_SIGINFO;

	ret = sigaction(SIGTERM, &action, NULL);
	if (ret)
		cmn_err(CE_INFO, "Failed to set SIGTERM handler: %s",
			strerror(errno));
}

static int bind_sock(struct state *state, int family, struct sockaddr *addr,
		     int addrlen)
{
	const int on = 1;
	int ret;
	int fd;

	if (state->nfds >= MAX_SOCK_FDS)
		return -EMFILE;

	fd = socket(family, SOCK_STREAM, 0);
	if (fd == -1)
		return -errno;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));

	if (bind(fd, addr, addrlen))
		goto err;

	if (listen(fd, CONN_BACKLOG))
		goto err;

	state->fds[state->nfds++] = fd;

	return 0;

err:
	ret = -errno;

	close(fd);

	return ret;
}

static int start_listening(struct state *state, const char *host,
			   uint16_t port)
{
	struct addrinfo hints, *res, *p;
	char strport[10];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	snprintf(strport, sizeof(strport), "%d", port);

	if (getaddrinfo(host, strport, &hints, &res))
		return -EINVAL; /* XXX: this should likely be something else */

	for (p = res; p; p = p->ai_next) {
		struct sockaddr_in *ipv4;
		struct sockaddr_in6 *ipv6;
		char str[64];
		void *addr;
		int ret;

		switch (p->ai_family) {
			case AF_INET:
				ipv4 = (struct sockaddr_in *) p->ai_addr;
				addr = &ipv4->sin_addr;
				break;
			case AF_INET6:
				ipv6 = (struct sockaddr_in6 *) p->ai_addr;
				addr = &ipv6->sin6_addr;
				break;
			default:
				cmn_err(CE_INFO, "Unsupparted address family: %d",
					p->ai_family);
				addr = NULL;
				break;
		}

		if (!addr)
			continue;

		inet_ntop(p->ai_family, addr, str, sizeof(str));

		ret = bind_sock(state, p->ai_family, p->ai_addr, p->ai_addrlen);
		if (ret && ret != -EAFNOSUPPORT)
			return ret;
		else if (!ret)
			cmn_err(CE_INFO, "Bound to: %s %s", str, strport);
	}

	freeaddrinfo(res);

	return 0;
}

static void stop_listening(struct state *state)
{
	int i;

	for (i = 0; i < state->nfds; i++)
		close(state->fds[i]);
}

static void wrap_taskq_callback(void *arg)
{
	struct cb *cb = arg;

	cb->state->func(cb->fd, cb->state->private);

	close(cb->fd);
	free(cb);
}

static void accept_conns(struct state *state)
{
	fd_set set;
	int maxfd;
	int ret;
	int i;

	for (;;) {
		union sockaddr_union addr;
		unsigned len;
		int fd;

		FD_ZERO(&set);
		maxfd = 0;
		for (i = 0; i < state->nfds; i++) {
			FD_SET(state->fds[i], &set);
			maxfd = MAX(maxfd, state->fds[i]);
		}

		ret = select(maxfd + 1, &set, NULL, NULL, NULL);

		if (atomic_read(&server_shutdown))
			break;

		if ((ret < 0) && (errno != EINTR))
			cmn_err(CE_ERROR, "Error on select: %s",
				strerror(errno));

		for (i = 0; (i < state->nfds) && (ret > 0); i++) {
			struct cb *cb;

			if (!FD_ISSET(state->fds[i], &set))
				continue;

			len = sizeof(addr);
			fd = accept(state->fds[i], (struct sockaddr *) &addr, &len);
			if (fd == -1) {
				cmn_err(CE_INFO, "Failed to accept from fd %d: %s",
					state->fds[i], strerror(errno));
				continue;
			}

			cb = malloc(sizeof(struct cb));
			if (!cb) {
				cmn_err(CE_INFO, "Failed to allocate cb data");
				continue;
			}

			cb->state = state;
			cb->fd = fd;

			ret = taskq_dispatch(state->taskq, wrap_taskq_callback,
					     cb);
			if (ret) {
				cmn_err(CE_ERROR, "Failed to dispatch conn: %s",
					xstrerror(ret));
				free(cb);
				close(fd);
			}

			/* XXX: should this be executed every time we loop? */
			ret--;
		}
	}
}

int socksvc(const char *host, uint16_t port, int nthreads,
	    void (*func)(int fd, void *), void *private)
{
	char name[128];
	struct state state;
	int ret;

	memset(&state, 0, sizeof(state));

	state.func = func;
	state.private = private;

	snprintf(name, sizeof(name), "socksvc: %s:%d", host ? host : "<any>",
		 port);

	state.taskq = taskq_create_fixed(name, nthreads);
	if (IS_ERR(state.taskq)) {
		ret = PTR_ERR(state.taskq);
		goto err;
	}

	handle_signals();

	ret = start_listening(&state, host, port);
	if (ret)
		goto err_taskq;

	accept_conns(&state);

	stop_listening(&state);

	taskq_wait(state.taskq);

	ret = 0;

err_taskq:
	taskq_destroy(state.taskq);

err:
	return ret;
}
