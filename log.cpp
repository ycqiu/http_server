#include "log.h"

Log::Log(const string& n, const string& p, 
		bool mult_thread): 
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

bool Log::need_open_new_file()
{
	time_t now_time = time(NULL);

	return now_time >= next_time;
}

void Log::open_new_file()
{
	string full_path = path + "/" + name;
	string day;
	get_year_month_day(day);   

	full_path += "_" + day;
	file = fopen(full_path.c_str(), "ab+");				
	//cout << fileno(file) << endl;
	if(file == NULL)
	{
		return;
	}
}

void Log::release_file()
{
	fclose(file);
	file = NULL;
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

void Log::print(const char* file_name, int line, 
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

void Log::get_year_month_day(string& res)
{
	time_t cur_time = time(NULL);
	struct tm * timeinfo = localtime(&cur_time);

	char buffer[] = {"yyyy-mm-dd"};
	strftime(buffer, sizeof(buffer), "%F", timeinfo);
	res = string(buffer);
}

void Log::get_hour_min_sec(string& res)
{
	time_t cur_time = time(NULL);
	struct tm * timeinfo = localtime(&cur_time);

	char buffer[] = {"hh:mm:ss"};
	strftime(buffer, sizeof(buffer), "%T", timeinfo);
	res = string(buffer);
}


Log* LogContainer::debug_log = NULL;
const string LogContainer::log_path = ".";
const bool LogContainer::using_mult_thread = false; 

Log* LogContainer::get()
{
	return debug_log;
}

void LogContainer::set(Log* log)
{
	debug_log = log;
}

Log* LogContainer::create(const char* name)
{
	return new Log(name, log_path, using_mult_thread); 
}

