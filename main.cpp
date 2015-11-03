#include <netinet/in.h>  //sockaddr_in
#include <arpa/inet.h>

//libevent
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "log.h"

using namespace std;


void read_cb(bufferevent *bev, void *ctx)
{
	evbuffer *input = bufferevent_get_input(bev);
	//evbuffer *output = bufferevent_get_output(bev);
	int input_len = evbuffer_get_length(input);
//	DEBUG_LOG("%s", evbuffer_pullup(input, input_len));

	char* line;
	size_t len;
	line = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF);
	if(line) //获得一行
	{
		DEBUG_LOG("%s", line);
		free(line);	
	}
}

void event_cb(bufferevent *bev, short events, void *ctx)
{
	if (events & BEV_EVENT_ERROR)
	{
		int err = EVUTIL_SOCKET_ERROR();

		if (err)
		{
			DEBUG_LOG("Socket error: %s\n", evutil_socket_error_to_string(err));
		}
	}

	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
	{
		bufferevent_free(bev);
	}
}

void accept_conn_cb(evconnlistener *listener, evutil_socket_t fd, 
		sockaddr *address, int socklen, void *ctx)
{
	event_base *base = evconnlistener_get_base(listener);

	bufferevent *bev = bufferevent_socket_new(base, fd, 
			BEV_OPT_CLOSE_ON_FREE /*| BEV_OPT_THREADSAFE*/);

	if(bev == NULL)
	{
		DEBUG_LOG("get bufferevent error!");
		return;
	}

	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(((sockaddr_in*)address)->sin_addr), 
			ip, sizeof(ip));
	DEBUG_LOG("accept from:%s", ip);

	bufferevent_setcb(bev, read_cb, 0, event_cb, 0);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void accept_error_cb(evconnlistener *listener, void *ctx)
{
	event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();

	if (err)
	{
		DEBUG_LOG("Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err));
	}
	event_base_loopexit(base, NULL);
}

int main(int argc, char* argv[])
{
	INIT_LOG("server");

	event_base* base = event_base_new();
	if(base == NULL)
	{
		DEBUG_LOG("get event_base error!");
		return 0;	
	}

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sockaddr_in));	
	sin.sin_family = AF_INET;
	if(argc == 2)
	{
		sin.sin_port = htons(atoi(argv[1]));
	}
	else
	{
		sin.sin_port = htons(80);
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	evconnlistener* listener = evconnlistener_new_bind(base, accept_conn_cb,
		   	0, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE /*| LEV_OPT_THREADSAFE*/, 
			-1, (sockaddr*)&sin, sizeof(sin));
	
	if(listener == NULL)
	{
		DEBUG_LOG("get evconnlistener error!");
		return 0;
	}

	evconnlistener_set_error_cb(listener, accept_error_cb);
	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_base_free(base);
	return 0;
}
