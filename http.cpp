#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <iostream>
#include <algorithm>
#include <string>
#include "http.h"
#include "log.h"

using namespace std;


map<string, string> Http::type_map;

void Http::insert_map(const string& key, const string& v)
{
	type_map[key] = "Content-type: " + v + "\r\n";
}

void Http::init_map()
{
	if(type_map.empty())
	{
		insert_map(".html", "text/html");
		insert_map(".jpg", "image/jpeg");
		insert_map(".png", "image/png");
	}
}

string Http::get_type(const string& key)
{
	string res;
	map<string, string>::iterator iter = type_map.find(key);
	if(iter != type_map.end())
	{
		res = iter->second;	
	}
	return res;
}

void Http::read_cb(bufferevent *bev, void *ctx)
{
	//evbuffer *input = bufferevent_get_input(bev);
	//int input_len = evbuffer_get_length(input);
	//DEBUG_LOG("%s", evbuffer_pullup(input, input_len));
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

void Http::write_cb(bufferevent *bev, void *ctx)
{
	Http* http = (Http*)ctx;
	evbuffer *output = bufferevent_get_output(bev);
	if(http->get_all_send() && 
			evbuffer_get_length(output) == 0)
	{
		DEBUG_LOG("empty");
		Http::release(&http);
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

Http::Http(event_base* base, evutil_socket_t fd): status(REQUEST_LINE), 
	all_send(false)
{
	Http::init_map();
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

bool Http::get_all_send()
{
	return all_send;
}

void Http::set_all_send(bool v)
{
	all_send = v;	
}

void Http::run(void* arg)
{
	bufferevent_setcb(bev, read_cb, write_cb, event_cb, arg);
	bufferevent_enable(bev, EV_READ);
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
		
		transform(method.begin(), method.end(), method.begin(), ::toupper);  
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
		return false;
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
	assert(status == MSG_BODY);
	evbuffer *input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);
	if(len == 0)
	{
		DEBUG_LOG("msg_body is empty");
	}
	else
	{
		msg_body = (char*)evbuffer_pullup(input, len);
		DEBUG_LOG("msg_body:%s", msg_body.c_str());
		evbuffer_drain(input, len);
		status = FINISHED;
	}
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

		case FINISHED:
			flag = excute();
			break;

		case ERROR_STATUS:
			flag = false;
			break;
	}
	return flag;
}

bool Http::excute()
{
	assert(status == FINISHED);		
	DEBUG_LOG("excute");

	if(method != "GET" && method != "POST")
	{
		//501
		return false;
	}

	const string dir = "./files";	

	size_t pos = path.find('?', 0);
	if(pos == string::npos)
	{
		path =  dir + path;	
		
		if(path[path.length() - 1] == '/')
		{
			path += "index.html";
		}

		DEBUG_LOG("%s", path.c_str());

		struct stat buf;
		if(lstat(path.c_str(), &buf) < 0)	
		{
			DEBUG_LOG("%s", strerror(errno));	
			//404
			not_found();	
			return false;
		}

		if(S_ISREG(buf.st_mode))
		{
			DEBUG_LOG("file");
			if((buf.st_mode & S_IXUSR)|| 
				(buf.st_mode & S_IXGRP) ||
				(buf.st_mode & S_IXOTH))
			{
				if(method == "POST")
				{
					exec_cgi(path, msg_body);
				}
				else
				{
					exec_cgi(path);
				}	
			}	
			else
			{
				send_file(path, buf.st_size);
			}
		}
		else if(S_ISDIR(buf.st_mode))
		{
			path += "index.html";
			send_file(path, buf.st_size);
		}
	}
	else //get
	{
		string query = path.substr(pos + 1);
		path.erase(pos);
		path =  dir + path;	
		DEBUG_LOG("%s", path.c_str());
		exec_cgi(path, query);
	}
	return false;
}

bool Http::exec_cgi(const string& path, const string& query)
{
	DEBUG_LOG("%s", path.c_str());
	
	int cgi_in[2], cgi_out[2];
	if(pipe(cgi_in) < 0 || pipe(cgi_out) < 0)
	{
		DEBUG_LOG("%s", strerror(errno));
		// 502
		return  false;	
	}

	pid_t pid = fork();
	if(pid < 0)
	{
		DEBUG_LOG("%s", strerror(errno));
		// 502
		return  false;	
	}
	else if(pid == 0)
	{
		DEBUG_LOG("son process");
		close(cgi_in[0]);		
		close(cgi_out[1]);
		
		dup2(cgi_out[0], 0);
		dup2(cgi_in[1], 1);

		setenv("REMOTE_METHOD", method.c_str(), 1);
		setenv("QUERY_STRING", query.c_str(), 1);

		string name = path;
		size_t pos = path.rfind("/");
		if(pos != string::npos)
		{
			name = path.substr(pos + 1);
		}
	
		if(execl(path.c_str(), name.c_str(), (char*)0) < 0)
		{
			DEBUG_LOG("%s", strerror(errno));
			// 502
			return  false;	
		}
	}
	else // parent
	{
		DEBUG_LOG("parent process");
		close(cgi_in[1]);	
		close(cgi_out[0]);	

		evbuffer* output = bufferevent_get_output(bev);
		string head =  "HTTP/1.1 200 OK\r\n";
		head += "Content-type: text/html\r\n";
		head += "\r\n";
		evbuffer_add(output, head.c_str(), head.length());

		size_t sz;
		char str[1024] = {'\0'};
		while( (sz = read(cgi_in[0], str, sizeof(str))) > 0 )
		{
			evbuffer_add(output, str, sz);

			str[sz] = '\0';
			DEBUG_LOG("%s", str);
		}

		set_all_send(true);
		waitpid(pid, NULL, 0);
	}

	return true;
}

bool Http::send_file(const string& path, size_t size)
{
	DEBUG_LOG("%s, %d", path.c_str(), size);
	int fd = open(path.c_str(), O_RDONLY);
	if(fd < 0)
	{
		DEBUG_LOG("%s", strerror(errno));
		return false;
	}	

	evbuffer* output = bufferevent_get_output(bev);
	string str = "HTTP/1.1 200 OK\r\n";
//	str += "Content-type: text/html\r\n";
	
	size_t pos = path.rfind('.');
	string extension;
	if(pos != string::npos)
	{
		extension = path.substr(pos);
	}
	DEBUG_LOG("%d %s", pos, extension.c_str());
	string type = get_type(extension);
	DEBUG_LOG("%s", type.c_str());
	str += type;
	str += "\r\n";
	evbuffer_add(output, str.c_str(), str.length());

	evbuffer_add_file(output, fd, 0, size);
	set_all_send(true);
	return true;
}

void Http::not_found()
{
	evbuffer* output = bufferevent_get_output(bev);
	string str = "HTTP/1.1 404 Not Found\r\n";
	str += "Content-type: text/html\r\n";
	str += "\r\n";

	str +=  "<html>" \
			"<body>" \
			"<h1 align = center>404 NOT Found</h1>" \
			"</body>";

	evbuffer_add(output, str.c_str(), str.length());
	set_all_send(true);
}
