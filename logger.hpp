/*******************************************************************************
*
*    Copyright (c) 2022, SunMinghui<sunminghuisx.com> All rights reserved.
*    Description:
*        This is c++ library use for write log to file
*        Support: 
*            1. This library can work linux/windows.
*            2. Logger is multi-thread safe.
*            3. On Linux is multi-process safe.
*            4. On Linux support change log file owner when you need.
*            5. Support log file auto split by to way and cycle log to you point directory:
*                1) split log file by every day,after split the logfile name like 'name'.year-mon-day
*                2) split log file by log max size,at you can point at Init(),after split logfile the name like 'name'.number,the number in [1-5]
*
*    Edition:
*        0.1    SunMinghui    20220121    Build the initial version
*
*
*******************************************************************************/

#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <iostream>
#include <string>
#include "vos_mutex.hpp"

#if defined(WIN32) || defined(WIN64)
	#define strcasecmp _stricmp
	#pragma warning(disable:4996)
	#include <io.h>
	#define ACCESS(file) _access(file,0)
#else
	#include <unistd.h>
	#define ACCESS(file) access(file,F_OK)
#endif

#ifndef gid_t
	#define gid_t unsigned int
#endif

#ifndef uid_t
	#define uid_t unsigned int
#endif

//defualt not used
#ifdef PROC_LOG_LOCK
	#include "filelock.h"
	#define FILE_LOCK_FILEPATH "/dev/shm/log_file_lock.txt"
#endif

const size_t MAX_PATH_LEN = 256;
const size_t MAX_BUFFER_SIZE = 1024*10;
const long MAX_LOG_SIZE = 1024*1024*30;

namespace LOG_LEVEL
{
	const int DEBUG = 1;
	const int INFO  = 2;
	const int NOTICE= 4;
	const int WARN  = 8;
	const int ERR   = 16;
	const int CRIT  = 32;
	const int FATAL = 64;
	const int EMERG = 128;
};

typedef struct sd_logger_info  td_logger_info;
typedef td_logger_info* 	   pd_logger_info;

class logger
{
public:
	logger();
	~logger();

public:
	int  init(const char* log_level, const char* log_name,
	          const char* log_path = "./", const char* mov_path = "./",
	          const char* uid_str = nullptr,  const char* gid_str = nullptr,
	          bool  flush = false, bool daysplit = false,long max_size = MAX_LOG_SIZE);
	int log(int log_level,const char*,...);
	int error(const char*,...);
	int info (const char*,...);
	int debug(const char*,...);
	int warn (const char*,...);
	int notice(const char*,...);
	int fatal(const char*,...);
	int emerg(const char*,...);
	int crit (const char*,...);

private:
	void op_error_of_what(const char*,...); 
	void get_header_line(int log_level, char *line, int line_size);
	bool open_log_file();	

private:
	FILE* log_file_ptr;
	char header_line[MAX_PATH_LEN];	
	char buffer[MAX_BUFFER_SIZE];		
	char log_name[MAX_PATH_LEN];	
	char log_path[MAX_PATH_LEN];
	char mov_path[MAX_PATH_LEN];
	vos_mutex log_mutex;
	bool flush;	
	bool daysplit;
	bool inited;
	int log_level;
	long log_max_size;
	//get gid & uid use call process to get default value if uid and gid not set
	gid_t gid;
	uid_t uid;

#ifndef WIN32
	#ifdef  PROC_LOG_LOCK
	CFileLock* log_file_lock;
	#endif
#endif

};

#endif //__LOGGER_HPP__