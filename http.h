#include <iostream>
#include <string>
#include <string.h>
#include <map>
#include <errno.h>
#include <stdlib.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>

using namespace std;


class Http
{
private:
	static void event_cb(bufferevent *bev, short events, void *ctx);
	static void read_cb(bufferevent *bev, void *ctx);
	static void write_cb(bufferevent *bev, void *ctx);

private:
	static map<string, string> type_map;
	static void init_map();
	static void insert_map(const string&, const string&);
	static string get_type(const string&);

private:	
	enum
	{
		REQUEST_LINE = 0,
		HEADER = 1,
		MSG_BODY = 2,
		FINISHED = 3,
		ERROR_STATUS
	};

	string method;
	string path;
	string version;
	map<string, string> header_map;
	string msg_body;

	char status;  
	bool all_send;
	bufferevent* bev;
		
private:
	/*explict*/ Http(event_base*, evutil_socket_t);
	~Http();

	char* get_word(char*, string&);

	bool parse_request_line();
	bool parse_header();
	bool parse_msg_body();
	bool excute();
	bool exec_cgi(const string&, const string& q = "");
	bool send_file(const string&, size_t);

	void not_found();   //404

public:
	static Http* create(event_base*, evutil_socket_t);
	static void release(Http**);

	bool loop();
	void run(void* arg);

	void set_all_send(bool);
	bool get_all_send();
};

