#include <iostream>
#include <string>
#include <string.h>
#include <map>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event.h>


class Http
{
private:
	static void event_cb(bufferevent *bev, short events, void *ctx);
	static void read_cb(bufferevent *bev, void *ctx);
	static void write_cb(bufferevent *bev, void *ctx);

private:
	static std::map<std::string, std::string> type_map;
	static void init_map();
	static void insert_map(const std::string&, const std::string&);
	static std::string get_type(const std::string&);

private:	
	enum
	{
		REQUEST_LINE = 0,
		HEADER = 1,
		MSG_BODY = 2,
		FINISHED = 3,
		ERROR_STATUS
	};

	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> header_map;
	std::string msg_body;

	char status;  
	bool all_send;
	bufferevent* bev;
		
private:
	/*explict*/ Http(event_base*, evutil_socket_t);
	~Http();

	char* get_word(char*, std::string&);

	bool parse_request_line();
	bool parse_header();
	bool parse_msg_body();
	bool excute();
	bool exec_cgi(const std::string&, const std::string& q = "");
	bool send_file(const std::string&, size_t);

	void bad_request();   //400
	void not_found();   //404
	void not_implement();   //501
	void bad_gateway();   //502

public:
	static Http* create(event_base*, evutil_socket_t);
	static void release(Http**);

	bool loop();
	void run(void* arg);

	void set_all_send(bool);
	bool get_all_send();
};

