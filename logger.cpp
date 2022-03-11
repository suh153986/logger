/*******************************************************************************
*
*    Copyright (c) 2022, SunMinghui<sunminghuisx.com> All rights reserved.
*    Description:
*        
*    Edition:
*        0.1    SunMinghui    20220121    Build the initial version
*
*
*******************************************************************************/

#include "logger.hpp"


logger::logger()
{
	//if on the win ,gid ant uid not used
	uid = 0;
	gid = 0;
	log_file_ptr   = nullptr;
	log_max_size   = 0;
	memset(header_line,0,MAX_PATH_LEN);
	memset(buffer,0,MAX_BUFFER_SIZE);
	memset(log_name,0,MAX_PATH_LEN);
	memset(log_path,0,MAX_PATH_LEN);
	memset(mov_path,0,MAX_PATH_LEN);
	log_level = LOG_LEVEL::INFO;
	flush  = false;
	inited = false;
}

logger::~logger()
{
	if(log_file_ptr != nullptr)
	{
		fclose(log_file_ptr);
		log_file_ptr = nullptr;
	}

#ifndef WIN32
	#ifdef PROC_LOG_LOCK
		delete log_file_lock;
		log_file_lock = nullptr;
	#endif
#endif
}

int	logger::init(const char* log_level, const char* log_name,
                 const char* log_path, const char* mov_path,
                 const char* uid_str, const char* gid_str ,
                 bool flush, bool daysplit, long max_size)
{
	if(inited) return 0;
	if(strlen(log_level) > 10) return -1;
	if(strlen(log_name)  > MAX_PATH_LEN) return -2;
	if(strlen(log_path)  > MAX_PATH_LEN) return -3;
	if(strlen(mov_path)  > MAX_PATH_LEN) return -4;
	if(max_size > MAX_LOG_SIZE * 100)	 return -5;

	this->flush    = flush;
	this->daysplit = daysplit;
	this->log_max_size = max_size;

	strcpy(this->log_name,log_name);
	strcpy(this->log_path,log_path);
	strcpy(this->mov_path,mov_path);
	
	int len = strlen(this->log_path);
	if(this->log_path[len - 1] != '/')
	{
		strcat(this->log_path,"/");
	}
	len = strlen(this->mov_path);
	if(this->mov_path[len - 1] != '/')
	{
		strcat(this->mov_path,"/");
	}
#ifndef WIN32
	if(uid_str)
	{
		struct passwd *pPwd = getpwnam(uid_str);
		if (pPwd)
			uid	= pPwd->pw_uid;
		else
			uid = getuid();
	}
	else
	{
		uid = getuid();
	}


	if(gid_str)
	{
		struct group* pGroup= getgrnam(gid_str);
		if (pGroup)
			gid	= pGroup->gr_gid;
		else
			gid = getgid();
	}else
	{
		gid = getgid();
	}
#endif	
	if(!open_log_file())
	{
		return -6;
	}
	
	if(strcasecmp(log_level,"debug") == 0){
		this->log_level = LOG_LEVEL::DEBUG;
	}else if(strcasecmp(log_level,"info") == 0){
		this->log_level = LOG_LEVEL::INFO;
	}else if(strcasecmp(log_level,"notice") == 0){
		this->log_level = LOG_LEVEL::NOTICE;
	}else if(strcasecmp(log_level,"warn") == 0){
		this->log_level = LOG_LEVEL::WARN;
	}else if(strcasecmp(log_level,"error") == 0){
		this->log_level = LOG_LEVEL::ERR;
	}else if(strcasecmp(log_level,"err") == 0){
		this->log_level = LOG_LEVEL::ERR;
	}else if(strcasecmp(log_level,"crit") == 0){
		this->log_level = LOG_LEVEL::CRIT;
	}else if(strcasecmp(log_level,"fatal") == 0){
		this->log_level = LOG_LEVEL::FATAL;
	}else if(strcasecmp(log_level,"emerg") == 0){
		this->log_level = LOG_LEVEL::EMERG;
	}
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	//keep muti different log can use different file lock
	char temp_file[MAX_PATH_LEN*2] = "";
	strcpy(temp_file,FILE_LOCK_FILEPATH);
	strcat(temp_file,".");
	strcat(temp_file,log_name);
	log_file_lock = new CFileLock(temp_file);
	FILE *fp;
	if((fp = fopen(temp_file,"r")) != nullptr)
	{
		chmod(temp_file,S_IRWXU|S_IRWXG|S_IRWXO);
		fclose(fp);
		fp = nullptr;
	}
#endif
#endif
	inited = true;
	return 0;
}

void logger::op_error_of_what(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	char err_msg[MAX_PATH_LEN] = "";
	std::string file_pathname = log_path;
	file_pathname	= file_pathname + "error_log";
	FILE* err_fp	= fopen(file_pathname.c_str(),"a+");
	if(!err_fp) return;

	va_list ap;
	va_start(ap,format);
	vsnprintf(err_msg,MAX_PATH_LEN,format,ap);
	va_end(ap);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(err_msg,sizeof(char),strlen(err_msg),err_fp);
	fwrite("\n",1,1,err_fp);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	fclose(err_fp);
}

int	logger::log(int log_level,const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;

	if(log_level < this->log_level || log_level > LOG_LEVEL::EMERG)
		return -2;

	if(log_level > this->log_level) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(log_level,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

void logger::get_header_line(int log_level,char* line, int line_size)
{
	char date[40] = "";
	time_t t = time(nullptr);
	
	struct tm* now = localtime(&t);
	strftime(date,sizeof(date),"%Y-%m-%d %H:%M:%S ",now );

	strcpy(line,date);
	switch(log_level){
		case LOG_LEVEL::DEBUG:
			strcat(line," [DEBUG]");
			break;
		case LOG_LEVEL::INFO:
			strcat(line," [INFO]");
			break;
		case LOG_LEVEL::NOTICE:
			strcat(line," [NOTICE]");
			break;
		case LOG_LEVEL::WARN:
			strcat(line," [WARN]");
			break;
		case LOG_LEVEL::ERR:
			strcat(line," [ERROR]");
			break;
		case LOG_LEVEL::CRIT:
			strcat(line," [CRIT]");
			break;
		case LOG_LEVEL::FATAL:
			strcat(line," [FATAL]");
			break;
		case LOG_LEVEL::EMERG:
			strcat(line," [EMERG]");
			break;
	}
	strcat(line,"\t");
}

bool logger::open_log_file()
{
	time_t t = time(nullptr);
	struct tm* now = localtime(&t);
	
	if(log_file_ptr != nullptr){
		if(daysplit){
			if(now->tm_hour < 23 && now->tm_min < 59 && now->tm_sec <= 59){
				return true;
			}
		}else{
			if(ftell(log_file_ptr) < log_max_size){
				return true;
			}
		}
	}
	
	char log_filepath[MAX_PATH_LEN*2]="";
	strcpy(log_filepath,log_path);
	strcat(log_filepath,log_name);
	if(log_file_ptr == nullptr){
		log_file_ptr = fopen(log_filepath,"a");
#ifndef WIN32
	//change file owner
	chown(log_filepath,uid,gid);
#if defined(APACHE_MODULE) || defined(NGINX_MODULE)
	chmod(log_filepath, S_IRWXU | S_IRWXG|S_IRWXO);
#endif
#endif
		if(!log_file_ptr)
			return false;
		else
			return true;
	}
	
	//we will move file,so close current file
	if(log_file_ptr){
		fclose(log_file_ptr);
		log_file_ptr = nullptr;
	}
	
	//per day general one file
	if(daysplit){
		char mov_filepath[MAX_PATH_LEN*2]="";
		strcpy(mov_filepath,mov_path);
		strcat(mov_filepath,log_name);
		char date_buf[30] ;
		sprintf(date_buf,".%d-%d-%d",now->tm_year,now->tm_mon,now->tm_mday);
		strcat(mov_filepath,date_buf);
#ifdef WIN32
		MoveFile((LPCTSTR)(log_filepath),
		  (LPCTSTR)(mov_filepath));
#else
		unlink(mov_filepath);
		link(log_filepath,mov_filepath);
		unlink(log_filepath);
#endif
	}else{
	//loop mov file
		char big_filepath[MAX_PATH_LEN*2];
		char little_filepath[MAX_PATH_LEN*2];
		int i = 0;
		for( i = 5; i > 1; --i){
			memset(big_filepath,0,MAX_PATH_LEN*2);
			memset(little_filepath,0,MAX_PATH_LEN*2);
			strcpy(big_filepath , mov_path);
			strcat(big_filepath , log_name);
			char ext[3] = "";
			sprintf(ext,".%d",i);
			strcat(big_filepath,ext);

			strcpy(little_filepath , mov_path);
			strcat(little_filepath , log_name);
			memset(ext,0,3);
			sprintf(ext,".%d",i-1);
			strcat(little_filepath,ext);		

			if(ACCESS(big_filepath) == 0){
				//remove
#ifdef WIN32
			_unlink(big_filepath);
#else
			unlink(big_filepath);
#endif
			}

			if(ACCESS(little_filepath) == 0){
				//move
#ifdef WIN32
				MoveFile((LPCTSTR)(little_filepath),
					(LPCTSTR)(big_filepath));
#else
				link(little_filepath,big_filepath);
				unlink(little_filepath);
#endif
			}

		}//end for
		//mov current file
		memset(big_filepath,0,MAX_PATH_LEN*2);
		strcpy(big_filepath , mov_path);
		strcat(big_filepath , log_name);
		char ext[3] = "";
		sprintf(ext,".%d",1);
		strcat(big_filepath,ext);
	
		if(ACCESS(big_filepath) == 0){
		//remove
#ifdef WIN32
			_unlink(big_filepath);
#else
			unlink(big_filepath);
#endif
		}

		if(ACCESS(log_filepath) == 0){
		//move
#ifdef WIN32
			MoveFile((LPCTSTR)(log_filepath),
				(LPCTSTR)(big_filepath));
#else
			link(log_filepath,big_filepath);
			unlink(log_filepath);
#endif
		}

	}//end if
	
	log_file_ptr = fopen(log_filepath,"a");
#ifndef WIN32
	//change file owner
	chown(log_filepath,uid,gid);
#if defined(APACHE_MODULE) || defined(NGINX_MODULE)
	chmod(log_filepath, S_IRWXU | S_IRWXG|S_IRWXO);
#endif
#endif
	if(!log_file_ptr)
		return false;
	else
		return true;
}


int	logger::error(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::ERR) return -2;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::ERR,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif	
	return 0;
}

int logger::info (const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::INFO) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::INFO,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int		logger::debug(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::DEBUG) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::DEBUG,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int	logger::fatal(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::FATAL) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::FATAL,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int	logger::emerg(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::EMERG) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::EMERG,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int	logger::crit (const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::CRIT) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::CRIT,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int	logger::warn (const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::WARN) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::WARN,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}

int	logger::notice(const char* format,...)
{
	vos_scoped_lock lock(log_mutex);
	if(!inited) return -1;
	if(log_level > LOG_LEVEL::NOTICE) return 1;

	try{
		if(!open_log_file()){
			return -3;
		}
	}catch(std::exception &e){
		op_error_of_what("Unhandle error of open log file,error msg:%s",e.what());
		return -4;
	}
	
	memset(buffer,0,MAX_BUFFER_SIZE);
	va_list ap;
	va_start(ap,format);
	vsnprintf(buffer,MAX_BUFFER_SIZE,format,ap);
	va_end(ap);
	
	get_header_line(LOG_LEVEL::NOTICE,header_line,MAX_PATH_LEN);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->WriteLockW();
#endif
#endif
	fwrite(header_line,sizeof(char),strlen(header_line),log_file_ptr);
	fwrite(buffer,sizeof(char),strlen(buffer),log_file_ptr);
	fwrite("\n",1,1,log_file_ptr);
	if(flush)
		fflush(log_file_ptr);
#ifndef WIN32
#ifdef PROC_LOG_LOCK
	log_file_lock->Unlock();
#endif
#endif
	return 0;
}




