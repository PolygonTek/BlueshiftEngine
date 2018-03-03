#pragma once

enum {
	SERVER_PORT = 4000,
};

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable : 4200)
struct PACKET
{
	uint16_t nSize;
	uint8_t nType;
	char data[0];
};
#pragma warning(pop)
#pragma pack(pop)

enum S2C_PROTOCOL
{
	S2C_FRAME,
	S2C_TOUCH,
	S2C_END
};

enum C2S_PROTOCOL
{
	C2S_FRAME,
	C2S_SOUND2D,
	C2S_END
};

