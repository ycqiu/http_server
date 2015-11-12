#include "http.h"
#include "log.h"
#include <assert.h>

void Http::read_cb(bufferevent *bev, void *ctx)
{
	evbuffer *input = bufferevent_get_input(bev);
	//evbuffer *output = bufferevent_get_output(bev);
	int input_len = evbuffer_get_length(input);
	DEBUG_LOG("recv: %s", evbuffer_pullup(input, input_len));
/*
	char* line;
	size_t len;
	line = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF);
	if(line) //获得一行
	{
		DEBUG_LOG("%s", line);
		free(line);	
	}*/

	Http* http = (Http*)ctx;
	while(http->loop());
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

Http::Http(event_base* base, evutil_socket_t fd): status(REQUEST_LINE)
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

char* Http::get_word(char* line, string& res)
{
	for(; *line && *line == ' '; ++line);

	for(; *line && *line != ' '; ++line)
	{
		res += *line;
	}
	return line;
}

bool Http::parse_request_line()
{
	assert(status == REQUEST_LINE);
	evbuffer *input = bufferevent_get_input(bev);

	char* line;
	size_t len;
	line = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF);
	if(line) 
	{
		char* p = line;
		p = get_word(p, method);
		p = get_word(p, path);
		p = get_word(p, version);
	
		DEBUG_LOG("method: %s", method.c_str());
		DEBUG_LOG("path: %s", path.c_str());
		DEBUG_LOG("version: %s", version.c_str());
		
		if(method.empty() || path.empty() || version.empty())
		{
			DEBUG_LOG("%s", line);
			status = ERROR_STATUS;
			free(line);	
			return false;
		}

		free(line);	
		status = HEADER;
		return true;
	}

	DEBUG_LOG("%s", line);
	status = ERROR_STATUS;
	free(line);	
	return false;
}

bool Http::parse_header()
{
	assert(status == HEADER);
	evbuffer *input = bufferevent_get_input(bev);

	char* line;
	size_t len;
	line = evbuffer_readln(input, &len, EVBUFFER_EOL_CRLF);
	if(line == NULL) 
	{
		//break;	
	}

	char* p = line;
	for(; *p && *p == ' '; ++p);		

	string key, value;
	for(; *p && *p != ' ' && *p != ':'; ++p)
	{
		key += *p;
	}

	if(key.empty())
	{
		status = MSG_BODY;
		free(line);
		return true;	
	}

	for(; *p && (*p == ' ' || *p == ':'); ++p);		
	for(; *p && *p != ' ' && *p != ':'; ++p)
	{
		value += *p;
	}

	header_map.insert(make_pair(key, value));
	DEBUG_LOG("%s : %s", key.c_str(), value.c_str());

	free(line);
	return true;
}

bool Http::parse_msg_body()
{
	status = FINISHED;
	return true;
}

bool Http::loop()
{
	bool flag = false;
	switch(status)
	{
		case REQUEST_LINE:
			flag = parse_request_line();
			break;

		case HEADER:
			flag = parse_header();
			break;

		case MSG_BODY:
			flag = parse_msg_body();
			break;

		case ERROR_STATUS:
			flag = false;
			break;
	}
	return flag;
}

