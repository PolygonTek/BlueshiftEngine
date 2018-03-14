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

#include "Precompiled.h"

#include "Network/Socket.h"
#define LINK_POINTER( link, type, member) (( type *)(( char *)link - ( size_t)&(( type *)0)->member))

#if 0
#include <cstdlib>
#include <cstring>
#include <iostream>



#include <winsock2.h>
#include <Ws2tcpip.h>
#include <MSWSock.h>
#include <queue>

//#define BUFFER_POOL_SIZE 16

#define LINK_POINTER( link, type, member) (( type *)(( char *)link - ( size_t)&(( type *)0)->member))

#pragma comment( lib, "mswsock.lib" )
#endif

using asio::ip::tcp;

BE_NAMESPACE_BEGIN
namespace Network {

Socket::Socket(tcp::socket socket)
		: socket_(std::move(socket))
{
	m_nPendingWrite = 0;
	m_pFirstBuf = NULL;
}

Socket::~Socket()
{
	FreeBuffer();
}

void Socket::Write(char *buf, uint32_t size)
{
	AutoCriticalSection lock(m_lock);

	while (size)
	{
		Buffer *pBuffer = Buffer::Alloc();
		uint32_t n = std::min<uint32_t>(Buffer::BUFFER_SIZE, size);
		memcpy(pBuffer->m_buffer, buf, n);
		pBuffer->m_dwSize = n;
		WriteWithoutLock(pBuffer);
		buf += n;
		size -= n;
	}
}

void Socket::WriteWithoutLock(Buffer *pBuffer)
{
	if (pBuffer->m_dwSize == 0)
	{
		pBuffer->Free();
		return;
	}
	BE_ASSERT(pBuffer->m_dwSize <= Buffer::BUFFER_SIZE);
	m_nPendingWrite += pBuffer->m_dwSize;
	if (m_pFirstBuf == NULL)
	{
		m_pFirstBuf = m_pLastBuf = pBuffer;

		auto self(shared_from_this());
		asio::async_write(socket_, asio::buffer(pBuffer->m_buffer, pBuffer->m_dwSize),
			[this, self](std::error_code ec, std::size_t length)
		{
			WriteCallback(ec, length);
		});
	}
	else if (m_pFirstBuf != m_pLastBuf && m_pLastBuf->m_dwSize + pBuffer->m_dwSize <= Buffer::BUFFER_SIZE)
	{
		memcpy(m_pLastBuf->m_buffer + m_pLastBuf->m_dwSize, pBuffer->m_buffer, pBuffer->m_dwSize);
		m_pLastBuf->m_dwSize += pBuffer->m_dwSize;
		pBuffer->Free();
	}
	else
	{
		m_pLastBuf->m_pNext = pBuffer;
		m_pLastBuf = pBuffer;
	}
}

void Socket::WriteCallback(std::error_code ec, std::size_t dwTransferred)
{
	AutoCriticalSection lock(m_lock);
	if (ec) {
		FreeBuffer();
		return;
	}
	BE_ASSERT(dwTransferred == m_pFirstBuf->m_dwSize);

	m_nPendingWrite -= m_pFirstBuf->m_dwSize;
	Buffer* pFirstBuf = m_pFirstBuf;
	if ((m_pFirstBuf = m_pFirstBuf->m_pNext) != NULL)
	{
		auto self(shared_from_this());
		asio::async_write(socket_, asio::buffer(m_pFirstBuf->m_buffer, m_pFirstBuf->m_dwSize),
			[this, self](std::error_code ec, std::size_t length)
		{
			WriteCallback(ec, length);
		});

	}
	pFirstBuf->Free();
}


void Socket::FreeBuffer()
{
	while (m_pFirstBuf)
	{
		Buffer *pBuf = m_pFirstBuf;
		m_pFirstBuf = m_pFirstBuf->m_pNext;
		m_nPendingWrite -= pBuf->m_dwSize;
		pBuf->Free();
	}
}

void	Socket::Shutdown()
{
    asio::error_code ec;
	socket_.shutdown(asio::socket_base::shutdown_both, ec);
}



long Buffer::s_nCount;
#ifdef __WIN32__
SLIST_HEADER Buffer::s_head;
#elif defined __APPLE__
OSQueueHead Buffer::s_head = OS_ATOMIC_QUEUE_INIT;
#endif

Buffer *Buffer::Alloc()
{
	Buffer *newBuffer;
#ifdef __WIN32__
	SLIST_ENTRY *pEntry = InterlockedPopEntrySList(&s_head);
    if (pEntry)
        newBuffer = LINK_POINTER(pEntry, Buffer, m_entry);
    else
        newBuffer = new (_aligned_malloc(sizeof(Buffer), _alignof(Buffer))) Buffer;
    //BE_ASSERT(((intptr_t) newBuffer & (alignof(BUffer) - 1)) == 0);
#elif defined __APPLE__
    newBuffer = (Buffer *) OSAtomicDequeue(&s_head, offsetof(Buffer, m_entry));
    if (!newBuffer)
        newBuffer = new Buffer; // aligned_malloc
#endif
    
	newBuffer->m_dwSize = 0;
//	newBuffer->m_nRef = 1;
	newBuffer->m_pNext = NULL;
	return newBuffer;
}

void Buffer::Free()
{
#ifdef __WIN32__
	InterlockedPushEntrySList(&s_head, &m_entry);
#elif defined __APPLE__
    OSAtomicEnqueue(&s_head, this, offsetof(Buffer, m_entry));
#endif
}

void Buffer::FreeAll()
{
	for (;;)
	{
#ifdef __WIN32__
        SLIST_ENTRY *pEntry = InterlockedPopEntrySList(&s_head);
        if (pEntry == NULL)
            break;
        Buffer *pBuffer = LINK_POINTER(pEntry, Buffer, m_entry);
#elif defined __APPLE__
        Buffer *pBuffer = (Buffer *) OSAtomicDequeue(&s_head, offsetof(Buffer, m_entry));
        if (pBuffer == NULL)
            break;
#endif
		delete pBuffer;
	}
}







} BE_NAMESPACE_END