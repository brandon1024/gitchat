#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logging.h"

struct logger {
	unsigned initialized: 1;
	unsigned int level;
};

static struct logger log_data = {0, LEVEL_NONE};

static void initialize_logging(struct logger *log,
		const unsigned int default_log_level)
{
	char *log_level_val = getenv("GIT_CHAT_LOG_LEVEL");

	log->initialized = 1;
	if(log_level_val == NULL)
		log->level = default_log_level;
	else if(!strcmp(log_level_val, "ALL"))
		log->level = LEVEL_ALL;
	else if(!strcmp(log_level_val, "TRACE"))
		log->level = LEVEL_TRACE;
	else if(!strcmp(log_level_val, "DEBUG"))
		log->level = LEVEL_DEBUG;
	else if(!strcmp(log_level_val, "INFO"))
		log->level = LEVEL_INFO;
	else if(!strcmp(log_level_val, "WARN"))
		log->level = LEVEL_WARN;
	else if(!strcmp(log_level_val, "ERROR"))
		log->level = LEVEL_ERROR;
	else if(!strcmp(log_level_val, "NONE"))
		log->level = LEVEL_NONE;
	else
		log->level = default_log_level;
}

static void vflog(const unsigned int level, const char *file_path,
		int line_number, const char *fmt, va_list varargs)
{
	time_t rawtime;
	struct tm *timeinfo;

	if(!log_data.initialized)
		initialize_logging(&log_data, LEVEL_NONE);

	if(level < log_data.level)
		return;

	char *level_str = "";
	if(level == LEVEL_TRACE)
		level_str = "[TRACE]";
	else if(level == LEVEL_DEBUG)
		level_str = "[DEBUG]";
	else if(level == LEVEL_INFO)
		level_str = "[INFO]";
	else if(level == LEVEL_WARN)
		level_str = "[WARN]";
	else if(level == LEVEL_ERROR)
		level_str = "[ERROR]";

	FILE *fd = (level < LEVEL_WARN) ? stdout : stderr;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(fd, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
			1900 + timeinfo->tm_year,
			timeinfo->tm_mon + 1,
			timeinfo->tm_mday,
			timeinfo->tm_hour,
			timeinfo->tm_min,
			timeinfo->tm_sec);

	const char *filename = strrchr(file_path, '/');
	if(filename && *(filename + 1))
		filename++;
	else if(filename == NULL)
		filename = file_path;

	fprintf(fd, " %s %s:%d - ", level_str, filename, line_number);
	vfprintf(fd, fmt, varargs);
	fprintf(fd, "\n");
}

void log_trace(const char *file, int line_number, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	vflog(LEVEL_TRACE, file, line_number, fmt, varargs);
	va_end(varargs);
}

void log_debug(const char *file, int line_number, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	vflog(LEVEL_DEBUG, file, line_number, fmt, varargs);
	va_end(varargs);
}

void log_info(const char *file, int line_number, const char* fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	vflog(LEVEL_INFO, file, line_number, fmt, varargs);
	va_end(varargs);
}

void log_warn(const char *file, int line_number, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	vflog(LEVEL_WARN, file, line_number, fmt, varargs);
	va_end(varargs);
}

void log_error(const char *file, int line_number, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);
	vflog(LEVEL_ERROR, file, line_number, fmt, varargs);
	va_end(varargs);
}