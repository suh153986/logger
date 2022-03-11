
#include "logger.hpp"
#include <iostream>
#include <vector>

#ifdef WIN32
	#include <Windows.h>
#else
	#include <sys/syscall.h>
	#include <pthread.h>
	#include <sys/wait.h>
	#include <sys/types.h>
#endif

#define DEBUG_STR "this is test data use to test log DEBUG"
#define INFO_STR  "this is test data use to test log INFO"
#define NOTICE_STR "this is test data use to test log NOTICE"
#define WARN_STR  "this is test data use to test log WARN"
#define ERR_STR   "this is test data use to test log ERROR"
#define FATAL_STR "this is test data use to test log FATAL"
#define CRIT_STR  "this is test data use to test log CRIT"
#define EMERG_STR "this is test data use to test log EMERG"

void test_log(const char* level)
{
	logger lo_logger;
	lo_logger.init(level, "run.log", "./" ,"./" ,nullptr, nullptr, false, false, 1024*1024*10);
	lo_logger.debug("%s" ,DEBUG_STR );
	lo_logger.info("%s"  ,INFO_STR );
	lo_logger.notice("%s" ,NOTICE_STR );
	lo_logger.warn("%s"  ,WARN_STR);
	lo_logger.error("%s" ,ERR_STR);
	lo_logger.fatal("%s" ,FATAL_STR);
	lo_logger.crit("%s"  ,CRIT_STR);
	lo_logger.emerg("%s" ,EMERG_STR);
}

void test_single_thread()
{
	long i = 0;
	for(; i < 10000; ++i)
	{
		test_log("debug");
		test_log("info");
		test_log("notice");
		test_log("warn");
		test_log("err");
		test_log("error");
		test_log("fatal");
		test_log("crit");
		test_log("emerg");
	}
}


static logger multi_log;

void test_multi_log(const char* level)
{
	long thread_id = 0;
#ifdef WIN32 
	thread_id = GetCurrentThreadId();
#else
	thread_id = syscall(SYS_gettid);
#endif
	multi_log.debug("thread:%ld %s" ,thread_id,DEBUG_STR );
	multi_log.info("thread:%ld %s"  ,thread_id,INFO_STR );
	multi_log.notice("thread:%ld %s" ,thread_id,NOTICE_STR );
	multi_log.warn("thread:%ld %s"  ,thread_id,WARN_STR);
	multi_log.error("thread:%ld %s" ,thread_id,ERR_STR);
	multi_log.fatal("thread:%ld %s" ,thread_id,FATAL_STR);
	multi_log.crit("%thread:%ld s"  ,thread_id,CRIT_STR);
	multi_log.emerg("thread:%ld %s" ,thread_id,EMERG_STR);
}

#ifdef WIN32
DWORD WINAPI thread_func(LPVOID pParameter)
#else
void* thread_func(void *data)
#endif
{
	long i = 0;
	for(; i < 5000; ++i )
	{
		test_multi_log("debug");
		test_multi_log("info");
		test_multi_log("notice");
		test_multi_log("warn");
		test_multi_log("err");
		test_multi_log("error");
		test_multi_log("fatal");
		test_multi_log("crit");
		test_multi_log("emerg");
	}
#ifdef WIN32
	return 0;
#else
	return nullptr;
#endif
}

void test_multi_thread(int n)
{
#ifdef WIN32
	std::vector<HANDLE> thread_pools;
#else
	std::vector<pthread_t> thread_pools;
#endif

	for(int i = 0; i < n; ++i)
	{
#ifdef WIN32
		HANDLE handle = CreateThread(nullptr,0,thread_func,nullptr,0,nullptr);
		thread_pools.push_back(handle);
#else
		pthread_t tid; 
		if(pthread_create(&tid,nullptr,thread_func,nullptr) == 0)
			thread_pools.push_back(tid);
#endif
	}

	for(size_t i = 0; i < thread_pools.size(); ++i)
	{
		#ifdef WIN32
			WaitForSingleObject(thread_pools.at(i),INFINITE);
			CloseHandle(thread_pools.at(i));
		#else
			pthread_join(thread_pools.at(i),nullptr);
		#endif
	}
}

int main(int argc, char** argv)
{	
	multi_log.init("info","multi_run.log","./","./", nullptr,nullptr, false, false, 1024*1024*10);
	
	std::cout << "start test single thread\n";
	test_single_thread();
	std::cout << "test single thread end\n";
	
	std::cout << "start test multi thread\n";
	test_multi_thread(2);
	std::cout << "test multi thread end\n";
	
	return 0;
}