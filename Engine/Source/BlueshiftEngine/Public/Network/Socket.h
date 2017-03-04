// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>

#define ASIO_STANDALONE 
#include "asio/include/asio.hpp"

BE_NAMESPACE_BEGIN 
namespace Network {

#ifdef  __WIN32__
class CriticalSection
{
public:
	void Lock() { EnterCriticalSection(&m_critical_section); }
	bool TryLock() { return !!TryEnterCriticalSection(&m_critical_section); }
	void Unlock() { LeaveCriticalSection(&m_critical_section); }
	CriticalSection() { InitializeCriticalSection(&m_critical_section); }
	CriticalSection(DWORD dwSpinCount) { InitializeCriticalSectionAndSpinCount(&m_critical_section, dwSpinCount); }
	~CriticalSection() { DeleteCriticalSection(&m_critical_section); }
	CRITICAL_SECTION m_critical_section;
};
#else
    class CriticalSection
    {
    public:
        void Lock() { pthread_mutex_lock(&m_critical_section); }
        void Unlock() { pthread_mutex_unlock(&m_critical_section); }
        bool TryLock() { return pthread_mutex_trylock(&m_critical_section); }
        CriticalSection() {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_critical_section, &attr);
        }
        ~CriticalSection() { pthread_mutex_destroy(&m_critical_section); }
        pthread_mutex_t  m_critical_section;
    };
#endif

class AutoCriticalSection
{
public:
	CriticalSection m_ref;

	AutoCriticalSection(CriticalSection ref) : m_ref(ref) {
		m_ref.Lock();
	}
	~AutoCriticalSection() {
		m_ref.Unlock();
	}
	
};

class Buffer : public std::enable_shared_from_this<Buffer>
{
public:
	enum { BUFFER_SIZE = 8000 };

	char m_buffer[BUFFER_SIZE];
	uint32_t m_dwSize;
	Buffer *m_pNext;
	static long	s_nCount;
#ifdef __WIN32__
    SLIST_ENTRY	m_entry;
    static SLIST_HEADER s_head;
#elif defined __APPLE__
    Buffer *m_entry;
    static OSQueueHead s_head;
#endif


public:
	static Buffer *Alloc();
	static void FreeAll();

	Buffer() { 
		//InterlockedIncrement(&s_nCount); 
	}
	void Free();
};

class Socket
	: public std::enable_shared_from_this<Socket>
{
public:

	CriticalSection m_lock;
	Buffer *m_pFirstBuf;
	Buffer *m_pLastBuf;
	long m_nPendingWrite;

	Socket(asio::ip::tcp::socket socket);
	~Socket();

	void Write(char *buf, uint32_t size);
	void WriteWithoutLock(Buffer *pBuffer);
	void Write(Buffer *pBuffer) { AutoCriticalSection lock(m_lock); WriteWithoutLock(pBuffer); }
	void WriteCallback(std::error_code ec, std::size_t dwTransferred);
	void FreeBuffer();
	void	Shutdown();

protected:

	asio::ip::tcp::socket socket_;
};




} BE_NAMESPACE_END