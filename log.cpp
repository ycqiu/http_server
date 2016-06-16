#include "log.h"
#include <string.h> 

using namespace std;


Log& Log::create(const std::string& n, const std::string& p, int mult_thread, int l)
{
	static Log log(n, p, mult_thread, l);
	return log;
}

Log::Log(const std::string& n, const std::string& p, 
		int mult_thread, int l): 
	name(n), path(p), level(l), file(NULL), using_mult_thread(mult_thread) 
{
	next_time = time(NULL);	
	next_time = (next_time / DAY_SECONDS + 1) * DAY_SECONDS;

	if(using_mult_thread)
	{
		pthread_mutex_init(&mutex, NULL);
	}
	//open_new_file();
}

bool Log::need_open_new_file()
{
	time_t now_time = time(NULL);

	return now_time >= next_time;
}

int Log::open_new_file()
{
	std::string full_path = path + "/" + name;
	std::string day;
	get_year_month_day(day);   

	full_path += "_" + day + ".log";
	file = fopen(full_path.c_str(), "a+");				
	if(file == NULL)
	{
		return -1;
	}
	return 0;
}

void Log::release_file()
{
	if(file)
	{
		fclose(file);
		file = NULL;
	}
}

Log::~Log()
{
	release_file();
	if(using_mult_thread)
	{		
		pthread_mutex_destroy(&mutex);
		using_mult_thread = false;
	}
}

void Log::update_next_time()
{
	next_time += DAY_SECONDS;
}

int Log::print(int l, const char* file_name, int line, 
		const char* func, const char* fmt, ...)
{
	if(level != Log::ALL && level < l)
	{
		return 1;
	}

	lock();

	if(need_open_new_file())
	{
		release_file();
		if(open_new_file())
		{
			unlock();
			return -1;
		}
		update_next_time();
	}

	//lazy open
	if(file == NULL)
	{
		if(open_new_file())
		{
			unlock();
			return -2;
		}
	}

	std::string msg;
	switch(l)
	{
	case DEBUG:
		msg = "debug";
		break;

	case INFO:
		msg = "info";
		break;

	case ERROR:
		msg = "error";
		break;

	default:
		unlock();
		return -3;
	}
		
	fprintf(file, "[%s]", msg.c_str());

	std::string day, hour, tm;
	get_year_month_day(day);
	get_hour_min_sec(hour);
	tm = day + " " + hour; 
	fprintf(file, "[%s]", tm.c_str());
	fprintf(file, "[%s:%d]", file_name, line);
	fprintf(file, "[%s]: ", func);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(file, fmt, ap);
	va_end(ap);

	fprintf(file, "\n");
	fflush(file);

	unlock();
	return 0;
}

void Log::lock()
{
	if(using_mult_thread)
	{
		pthread_mutex_lock(&mutex);
	}
}

void Log::unlock()
{
	if(using_mult_thread)
	{
		pthread_mutex_unlock(&mutex);
	}
}

void Log::get_year_month_day(std::string& res)
{
	time_t cur_time = time(NULL);
	struct tm * timeinfo = localtime(&cur_time);

	char buffer[] = {"yyyy-mm-dd"};
	strftime(buffer, sizeof(buffer), "%F", timeinfo);
	res = std::string(buffer);
}

void Log::get_hour_min_sec(std::string& res)
{
	time_t cur_time = time(NULL);
	struct tm * timeinfo = localtime(&cur_time);

	char buffer[] = {"hh:mm:ss"};
	strftime(buffer, sizeof(buffer), "%T", timeinfo);
	res = std::string(buffer);
}


Log* LogContainer::log = NULL;

Log* LogContainer::get()
{
	return log;
}

Log* LogContainer::create(const char* name, const char* conf)
{
	string log_path = ".";
	int using_mult_thread = 1; 
	int level = Log::ALL;
	
	FILE* file = fopen(conf, "r");
	if(file)
	{
		char path[256] = {0};
		fscanf(file, "log_path=%s\n", path); 
		if(strlen(path))
		{
			log_path = path;
		}
		fscanf(file, "using_mult_thread=%d\n", &using_mult_thread); 
		fscanf(file, "level=%d\n", &level);
		fclose(file); 
	}

	return log = &Log::create(name, log_path, using_mult_thread, level); 
}
