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

#ifndef __FILE_LOCK_HPP__
#define __FILE_LOCK_HPP__

#include <unistd.h>
#include <fcntl.h>

class file_lock
{
public:
	file_lock(const char* sPath);
	virtual ~file_lock();

	//return false when interupt by singal	
	inline bool read_lock(int i_offset = 0, int i_len = 1, int i_whence = SEEK_SET)
				{ return Fcntl(F_SETLK, F_RDLCK, i_offset, i_len, i_whence); }			
	inline bool write_lock(int i_offset = 0, int i_len = 1, int i_whence = SEEK_SET)
				{ return Fcntl(F_SETLK, F_WRLCK, i_offset, i_len, i_whence); }			
	inline bool read_lock_w(int i_offset = 0, int i_len = 1, int i_whence = SEEK_SET)
				{ return Fcntl(F_SETLKW, F_RDLCK, i_offset, i_len, i_whence); }					
	inline bool write_lock_w(int i_offset = 0, int i_len = 1, int i_whence = SEEK_SET)
				{ return Fcntl(F_SETLKW, F_WRLCK, i_offset, i_len, i_whence); }				
	inline bool unlock(int i_offset = 0, int i_len = 1, int i_whence = SEEK_SET)
				{ return Fcntl(F_SETLK, F_UNLCK, i_offset, i_len, i_whence); }
				
protected:
	int m_i_file;

protected:
	//return false when interupt by singal
	bool Fcntl(int i_cmd, int i_type, int i_offset, int i_len, int i_whence);


};

#endif //__FILE_LOCK_HPP__

#endif
