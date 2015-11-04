#ifndef __LOG__
#define __LOG__

#include <iostream>
#include <string>
#include <stdio.h>
#include <ctime>
#include <stdarg.h>
#include <pthread.h>

using namespace std;


class Log
{
private:
	enum 
	{
		DAY_SECONDS = 24 * 60 * 60	
	};

	string name;
	string path; 
	
	time_t next_time;  //下一次新建日志的时间
	FILE* file;

	bool using_mult_thread;
	pthread_mutex_t mutex;

public:
	/*explict*/ Log(const string& n, const string& p = ".", 
			bool mult_thread = false);
	~Log();
	
	bool need_open_new_file();
	void open_new_file();
	void release_file();
	void update_next_time();
	void print(const char* file_name, int line, 
			const char* func, const char* fmt, ...);

private:
	void get_year_month_day(string& res);
	void get_hour_min_sec(string& res);	
};


class LogContainer
{
private:	
	static Log* debug_log;
	static const string log_path;
	static const bool using_mult_thread;

public:
	static Log* get();
	static void set(Log*);
	static Log* create(const char*);
};


#define INIT_LOG(name) \
	do \
	{ \
		if(LogContainer::get() == NULL) \
		{ \
			Log* p_log = LogContainer::create(name); \
			LogContainer::set(p_log); \
		} \
	} while(0)


#define DEBUG_LOG(...) \
	do \
	{ \
		Log* p = LogContainer::get(); \
		p->print(__FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__); \
	} while(0)

#endif
