#include <iostream>
#include <string>
#include <stdio.h>
#include <ctime>
#include <cmath>
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
			bool mult_thread = false): 
		name(n), path(p), file(NULL), using_mult_thread(mult_thread) 
	{
		next_time = time(NULL);	
		next_time = (next_time / DAY_SECONDS + 1) * DAY_SECONDS;

		if(using_mult_thread)
		{
			pthread_mutex_init(&mutex, NULL);
		}
		open_new_file();
	}

	bool need_open_new_file()
	{
		time_t now_time = time(NULL);

		return now_time >= next_time;
	}

	void open_new_file()
	{
		string full_path = path + "/" + name;
		string day;
		get_year_month_day(day);   
		
		full_path += "_" + day;
		file = fopen(full_path.c_str(), "ab+");				
		if(file == NULL)
		{
			return;
		}
	}

	void release_file()
	{
		fclose(file);
		file = NULL;
	}

	~Log()
	{
		release_file();
		if(using_mult_thread)
		{		
			pthread_mutex_destroy(&mutex);
			using_mult_thread = false;
		}
	}

	void update_next_time()
	{
		next_time += DAY_SECONDS;
	}

	void print(const char* file_name, int line, 
			const char* func, const char* fmt, ...)
	{
		if(using_mult_thread)
		{
			pthread_mutex_lock(&mutex);
		}

		if(need_open_new_file())
		{
			release_file();
			open_new_file();
			update_next_time();
		}

		string day, hour, tm;
		get_year_month_day(day);
		get_hour_min_sec(hour);

		tm = day + " " + hour; 
		fprintf(file, "[%s] ", tm.c_str());
		fprintf(file, "[%s:%d] ", file_name, line);
		fprintf(file, "[%s()]: ", func);
		
		va_list ap;
		va_start(ap, fmt);
		vfprintf(file, fmt, ap);
		va_end(ap);

		fprintf(file, "\n");
		fflush(file);

		if(using_mult_thread)
		{
			pthread_mutex_unlock(&mutex);
		}
	}

private:
	void get_year_month_day(string& res)
	{
		time_t cur_time = time(NULL);
		struct tm * timeinfo = localtime(&cur_time);

		char buffer[] = {"yyyy-mm-dd"};
		strftime(buffer, sizeof(buffer), "%F", timeinfo);
		res = string(buffer);
	}

	void get_hour_min_sec(string& res)
	{
		time_t cur_time = time(NULL);
		struct tm * timeinfo = localtime(&cur_time);

		char buffer[] = {"hh:mm:ss"};
		strftime(buffer, sizeof(buffer), "%T", timeinfo);
		res = string(buffer);
	}
};


static Log* debug_log;
Log* get_debug_log()
{
	return debug_log;
}

void set_debug_log(Log* p)
{
	debug_log = p;
} 


static const string  g_log_path = ".";
static const bool g_using_mult_thread = false; 

#define INIT_LOG(name) \
	do \
	{ \
		if(get_debug_log() == NULL) \
		{ \
			Log* p_log= new Log(name, g_log_path, g_using_mult_thread); \
			set_debug_log(p_log); \
		} \
	} while(0)


#define DEBUG_LOG(...) \
	do \
	{ \
		Log* p = get_debug_log(); \
		p->print(__FILE__, __LINE__, __FUNCTION__ ,__VA_ARGS__); \
	} while(0)

	
