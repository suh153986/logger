/*******************************************************************************
*
*    Copyright (c) 2022, SunMinghui<sunminghuisx.com> All rights reserved.
*    Description:
*        
*    Edition:
*        0.1    SunMinghui    20220310    Build the initial version
*
*
*******************************************************************************/

#ifndef __VOS_MUTEX_HPP__
#define __VOS_MUTEX_HPP__

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
	#define WIN32_LEAN_AND_MEAN
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <grp.h>
	#include <pwd.h>
	#include <dirent.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <pthread.h>
#endif


class vos_mutex 
{
public:
	inline vos_mutex() 
	{
		#ifdef WIN32
			InitializeCriticalSection(&_mutex);
		#else
			pthread_mutex_init(&_mutex, NULL);
		#endif
	}

	inline ~vos_mutex() 
	{
		#ifdef WIN32
			DeleteCriticalSection(&_mutex);
		#else
			pthread_mutex_destroy(&_mutex);
		#endif
	}

	inline void lock() 
	{
		#ifdef WIN32
			EnterCriticalSection(&_mutex);
		#else
			pthread_mutex_lock(&_mutex);
		#endif
	}

	inline void unlock() 
	{
		#ifdef WIN32
			LeaveCriticalSection(&_mutex);
		#else
			pthread_mutex_unlock(&_mutex);
		#endif
	}

private:
	vos_mutex(const vos_mutex &);
	const vos_mutex & operator = (const vos_mutex &);

private:

	#ifdef WIN32
		CRITICAL_SECTION    _mutex;
	#else
		pthread_mutex_t     _mutex;
	#endif
};

class vos_scoped_lock 
{
public:
	inline vos_scoped_lock(vos_mutex & mutex) : _mutex(mutex) 
	{
		_mutex.lock();
	}

	inline ~vos_scoped_lock() 
	{
		_mutex.unlock();
	}

private:
	vos_scoped_lock(const vos_scoped_lock &);
	const vos_scoped_lock & operator = (const vos_scoped_lock &);

private:
	vos_mutex & _mutex;
};

#endif //__VOS_MUTEX_HPP__