#pragma once

#include "Network/Socket.h"
#include "../libControl/Protocol.h"

struct PACKET;

#ifdef __WIN32__
#define CLIENT_API WINAPI
#define CLIENT_EXPORT __declspec(dllexport)
#elif __APPLE__
#define CLIENT_API
#define CLIENT_EXPORT
#endif

class Client : public BE1::Network::Socket
{
public:
	typedef BE1::Network::Socket Super;

	Client(asio::ip::tcp::socket socket)
		: Super(std::move(socket))
	{
		m_nLength = 0;
	}

	void	Start();
	void	Write(BE1::Network::Buffer *pBuffer);
	bool	RecvFrame();

protected:
	void	Read();
	void	OnRead(std::size_t length);
	bool	Process(PACKET *packet);

	//	tcp::socket socket_;
	enum { PACKET_LENGTH = 1024, PENDING_WRITE = 160000 };
	char m_Buffer[PACKET_LENGTH];
	uint32_t m_nLength;
};

void	SendPacket(int nType, const char *format, ...);