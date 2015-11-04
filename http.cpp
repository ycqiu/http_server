#include "http.h"
#include "log.h"

void Http::read_cb(bufferevent *bev, void *ctx)
{
	evbuffer *input = bufferevent_get_input(bev);
	//evbuffer *output = bufferevent_get_output(bev);
	int input_len = evbuffer_get_length(input);
	DEBUG_LOG("recv: %s", evbuffer_pullup(input, input_len));

	char* line;
	size_t len;
	line = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF);
	if(line) //获得一行
	{
		DEBUG_LOG("%s", line);
		free(line);	
	}
}

void Http::event_cb(bufferevent *bev, short events, void *ctx)
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
	//	bufferevent_free(bev);
		Http* http = (Http*) ctx;
		Http::release(&http);
		DEBUG_LOG("%p", http);
	}
}

Http* Http::create(event_base* base, evutil_socket_t fd)
{
	Http* http	= new Http(base, fd);
	return http;
}

void Http::release(Http** p_http)
{
	delete *p_http;
	*p_http = NULL;
}

Http::Http(event_base* base, evutil_socket_t fd)
{
	bev = bufferevent_socket_new(base, fd, 
			BEV_OPT_CLOSE_ON_FREE /*| BEV_OPT_THREADSAFE*/);

	if(bev == NULL)
	{
		DEBUG_LOG("get bufferevent error!");
		return;
	}
}

Http::~Http()
{
	bufferevent_free(bev);
	bev = NULL;
	DEBUG_LOG("~Http");
}

void Http::run(void* arg)
{
	bufferevent_setcb(bev, read_cb, 0, event_cb, arg);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

