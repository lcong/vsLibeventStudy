/*
  This example program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/
#pragma warning(disable : 4996)

/*
 * XXX This sample code was once meant to show how to use the basic Libevent
 * interfaces, but it never worked on non-Unix platforms, and some of the
 * interfaces have changed since it was first written.  It should probably
 * be removed or replaced with something better.
 *
 * Compile with:
 * cc -I/usr/local/include -o time-test time-test.c -L/usr/local/lib -levent
 */

#include <sys/types.h>

#include <event2/event-config.h>

#include <sys/stat.h>
#ifndef _WIN32
#include <sys/queue.h>
#include <unistd.h>
#endif
#include <time.h>
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/event.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

struct timeval lasttime;

int event_is_persistent;


int called = 0;

static void
signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *signal = (struct event*)arg;

	printf("signal_cb: got signal %d\n", event_get_signal(signal));

	if (called >= 10)
	{
		event_del(signal);
	}
	
	called++;
}


static void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct timeval newtime, difference;
	struct event *timeout = (struct event *)arg;
	double elapsed;

	evutil_gettimeofday(&newtime, NULL);
	evutil_timersub(&newtime, &lasttime, &difference);
	elapsed = difference.tv_sec +
		(difference.tv_usec / 1.0e6);

	printf("timeout_cb called at %d: %.3f seconds elapsed.\n",
		(int)newtime.tv_sec, elapsed);
	lasttime = newtime;

	if (!event_is_persistent)
	{
		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_sec = 2;
		event_add(timeout, &tv);
	}
}


void cb_func(evutil_socket_t fd, short what, void *arg)
{
	char *data = (char *)arg;
	printf("Got an event on socket %d:%s%s%s%s [%s]",
		(int)fd,
		(what&EV_TIMEOUT) ? " timeout" : "",
		(what&EV_READ) ? " read" : "",
		(what&EV_WRITE) ? " write" : "",
		(what&EV_SIGNAL) ? " signal" : "",
		data);
}

int
main(int argc, char **argv)
{
	struct event timeout;
	struct event *signal_int;
	struct timeval tv;
	struct event_base *base;
	int flags;

#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);

	(void)WSAStartup(wVersionRequested, &wsaData);
#endif

	if (argc == 2 && !strcmp(argv[1], "-p")) 
	{
		event_is_persistent = 1;
		flags = EV_PERSIST;
	}
	else 
	{
		event_is_persistent = 0;
		flags = 0;
	}

	/* Initalize the event library */
	base = event_base_new();

	if (!base)
	{
		puts("Couldn’t get an event_base!");
	}
	else 
	{
		printf("Using Libevent with backend method %s.",
			event_base_get_method(base));
		int f = event_base_get_features(base);
		printf(" event_base_get_features[%d].",f);
		if ((f & EV_FEATURE_ET))
		{
			printf(" Edge-triggered events are supported.");
		}
		if ((f & EV_FEATURE_O1))
		{
			printf(" O(1) event notification is supported.");
		}
		if ((f & EV_FEATURE_FDS))
		{
			printf(" All FD types are supported.");
		}

		puts("");
	}

	/* Initalize one event */
	event_assign(&timeout, base, -1, flags, timeout_cb, (void*)&timeout);

	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(&timeout, &tv);

	/* Initalize one event */
	signal_int = evsignal_new(base, SIGINT, signal_cb, event_self_cbarg());

	event_add(signal_int, NULL);

	evutil_gettimeofday(&lasttime, NULL);
	   	  
	event_base_dispatch(base);

	event_free(signal_int);

	event_free(&timeout);

	event_base_free(base);

	return (0);
}

