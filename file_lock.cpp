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


#ifndef WIN32

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "file_lock.hpp"


file_lock::file_lock(const char* file_path)
{
	m_i_file = open(file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
}


file_lock::~file_lock()
{
	close(m_i_file);
}


bool file_lock::Fcntl(int i_cmd, int i_type, int i_offset, int i_len, int i_whence)
{
	struct flock t_lock;
	
	t_lock.l_type = i_type;
	t_lock.l_start = i_offset;
	t_lock.l_whence = i_whence;
	t_lock.l_len = i_len;
	
	if (fcntl(m_i_file, i_cmd, &t_lock) < 0)
	{
		if (errno == EINTR)
			return false;
	}
	
	return true;
}

#endif