#include "Precompiled.h"

#include "Client.h"
#include "Network/Packet.h"

//#include "asio.hpp"

#define MULTI_THRED 0

using asio::ip::tcp;
std::shared_ptr<Client> ClientPtr;
static int g_nFrame;

typedef   void (CLIENT_API *CallbackTouch) (int x, int y);
static   CallbackTouch ProcessTouch;
extern "C"  CLIENT_EXPORT void CLIENT_API RegisterTouch(CallbackTouch cb)
{
	ProcessTouch = cb;
}


bool Client::Process(PACKET *packet)
{
	const char *start = packet->data;
	const char *end = start + packet->nSize;
	switch (packet->nType) {
	case S2C_FRAME:
		g_nFrame++;
		return true;
		break;
	case S2C_TOUCH:	{
		uint16_t x;
		uint16_t y;
		BE1::ReadPacket(start, end, "ww", &x, &y);
		ProcessTouch(x, y);
		break;
	}
	default:
		BE_ERRLOG(L"invalid type packet %d\n", packet->nType);
		break;
	}
	return false;
}



void	Client::Start()
{
	OnRead(0);
}

void	Client::Read()
{
//	BE_LOG(L"Read Calling %d\n", PACKET_LENGTH - m_nLength);
	auto self(shared_from_this());
	socket_.async_read_some(asio::buffer(m_Buffer + m_nLength, PACKET_LENGTH - m_nLength),
		[this, self](std::error_code ec, std::size_t length)
	{
		//BE_LOG(L"OnRead Called %d %d\n", ec.value(), length);
		if (ec) {
			Shutdown();
			return;
		}
		OnRead(length);
	});
}

void	Client::Write(BE1::Network::Buffer *pBuffer)
{
#ifdef MULTI_THREAD
	m_lock.Lock();
#endif // MULTI_THREAD

	if (m_nPendingWrite > PENDING_WRITE)
	{
#ifdef MULTI_THREAD
		m_lock.Unlock();
#endif // MULTI_THREAD
		BE_WARNLOG(L"close pending socket  (bytes %d) \n", m_nPendingWrite);
		Shutdown();
		return;
	}

	Super::WriteWithoutLock(pBuffer);
#ifdef MULTI_THREAD
	m_lock.Unlock();
#endif // MULTI_THREAD
}

void Client::OnRead(std::size_t length)
{

	m_nLength += length;
	char *szBuffer = m_Buffer;
	for (; ; )
	{
		if (m_nLength < sizeof(PACKET))
		{
			memmove(m_Buffer, szBuffer, m_nLength);
			Read();
			return;
		}
		PACKET *packet = (PACKET *)szBuffer;
		int nType = packet->nType;
		int	nSize = packet->nSize;
		if (m_nLength < nSize)
		{
			if (nSize > PACKET_LENGTH)
			{
				BE_ERRLOG(L"exceed buffer size. (type %d) (size %d) \n", nType, nSize);
				Shutdown();
				return;
			}
			else {
				memmove(m_Buffer, szBuffer, m_nLength);
				Read();
				return;
			}
		}
		else if (nSize < sizeof(PACKET))
		{
			BE_ERRLOG(L"invalid buffer size. (type:%d, size:%d)", nType, nSize);
			Shutdown();
			return;
		}
		//	decode
		m_nLength -= nSize;
		szBuffer += nSize;
		if (Process(packet)) {
			memmove(m_Buffer, szBuffer, m_nLength);
//			Read();
			break;
		}
	}
}


asio::io_service IOService;


bool	Client::RecvFrame()
{
	asio::error_code ec;

	Start();
	for (; ;) {
		if (g_nFrame > 0) {
			g_nFrame--;
			break;
		}

		// asio::io_service::run_one is not working
//		IoService.poll(ec);
		if (IOService.run_one(ec) == 0) {
			IOService.reset();
			if (IOService.run_one(ec) == 0)
				return false;
		}
	}
	return true;
}

extern "C"  CLIENT_EXPORT bool CLIENT_API RecvFrame()
{

	if (!ClientPtr)	{
        
		asio::error_code ec;

		BE1::Engine::InitParms initParms;
		BE1::Str playerDir = BE1::PlatformFile::ExecutablePath();
		playerDir.AppendPath("..\\..\\..");
		initParms.baseDir = playerDir;

		BE1::Str dataDir = playerDir + "\\Data";
		initParms.searchPath = dataDir;

//		BE1::Engine::Init(&initParms);
		{
			BE1::Engine::baseDir = initParms.baseDir;
			BE1::Engine::searchPath = initParms.searchPath;
			BE1::Engine::args = initParms.args;

			BE1::Platform::Init();
			BE1::common.Init(BE1::Engine::baseDir);
		}


		asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string("127.0.0.1"), SERVER_PORT);
		//asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string("192.168.111.10"), SERVER_PORT);
		tcp::socket s(IOService);
		s.connect(endpoint, ec);
		if (ec) {
			BE_FATALERROR(L"Can't connect to server\n");
			return false;
		}
		//tcp::resolver resolver(IOService);
		//asio::connect(s, resolver.resolve({ "127.0.0.1", "4000" }));
		ClientPtr = std::make_shared<Client>(std::move(s));
	}
	return ClientPtr->RecvFrame();
}


void	SendPacket(int nType, const char *format, ...)
{


	BE_ASSERT(!!ClientPtr);

	BE1::Network::Buffer *pBuffer = BE1::Network::Buffer::Alloc();
	PACKET *packet = (PACKET *)pBuffer->m_buffer;
	packet->nType = nType;

	va_list va;

	va_start(va, format);
	char *end = BE1::WritePacketV(packet->data, format, va);
	pBuffer->m_dwSize = end - pBuffer->m_buffer;
	packet->nSize = pBuffer->m_dwSize;
	BE_ASSERT(pBuffer->m_dwSize <= BE1::Network::Buffer::BUFFER_SIZE);
	ClientPtr->Write(pBuffer);
	va_end(va);
}



