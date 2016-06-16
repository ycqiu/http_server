#ifndef __LOG__
#define __LOG__

#include <iostream>
#include <string>
#include <map>
#include <stdio.h>
#include <ctime>
#include <stdarg.h>
#include <pthread.h>


class Log
{
public:
	enum
	{
		ALL = 0,
		ERROR = 1,
		INFO = 2,
		DEBUG = 3	
	};

private:
	enum 
	{
		DAY_SECONDS = 24 * 60 * 60	
	};

	std::string name;
	std::string path; 
	int level;
	
	time_t next_time;  //下一次新建日志的时间
	FILE* file;

	int using_mult_thread;
	pthread_mutex_t mutex;

private:
	Log(const std::string& n, const std::string& p = ".", 
			int mult_thread = false, int l = 0);
	Log(const Log&);
	Log operator=(const Log&);
	~Log();
	
public:
	static Log& create(const std::string& n, const std::string& p = ".", 
			int mult_thread = false, int l = 0);

	bool need_open_new_file();
	int open_new_file();
	void release_file();
	void update_next_time();
	int print(int l, const char* file_name, int line, 
			const char* func, const char* fmt, ...);

	void lock();
	void unlock();
private:
	void get_year_month_day(std::string& res);
	void get_hour_min_sec(std::string& res);	
};

class LogContainer
{
private:	
	static Log* log;
	
public:
	static Log* get();
	static Log* create(const char* name, const char* conf = NULL);
};


#define INIT_LOG(name) \
	do \
	{ \
		if(LogContainer::get() == NULL) \
		{ \
			LogContainer::create(name, "log.conf"); \
		} \
	} while(0)


#define INIT_LOG2(name, conf) \
	do \
	{ \
		if(LogContainer::get() == NULL) \
		{ \
			LogContainer::create(name, conf); \
		} \
	} while(0)


#define DEBUG_LOG(...) \
	do \
	{ \
		LogContainer::get()->print(Log::DEBUG, __FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__); \
	} while(0)


#define INFO_LOG(...) \
	do \
	{ \
		LogContainer::get()->print(Log::INFO, __FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__); \
	} while(0)


#define ERROR_LOG(...) \
	do \
	{ \
		LogContainer::get()->print(Log::ERROR, __FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__); \
	} while(0)

#endif
	
