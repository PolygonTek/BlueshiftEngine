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

BE_NAMESPACE_BEGIN


char *WritePacketV(char *packet, const char *format, va_list va)
{
	for (;;)
	{
		switch (*format++)
		{
		case 'b':
			*packet++ = va_arg(va, char);
			break;
		case 'w':
			*(int16_t *)packet = va_arg(va,uint16_t);
			packet += sizeof(int16_t);
			break;
		case 'd':
			*(int32_t *)packet = va_arg(va, int32_t);
			packet += sizeof(int32_t);
			break;
		case 'q':	
			*(int64_t *)packet = va_arg(va, int64_t);
			packet += sizeof(int64_t);
			break;
		case 's':
		{
			char * str = va_arg(va, char *);
			if (str == 0)
			{
				*(char *)packet = 0;
				packet += sizeof(char);
				break;
			}
			for (; (*(char *)packet = *str++) != 0; )
				packet += sizeof(char);
			packet += sizeof(char);
		}
		break;
		case 'S':
		{
			wchar_t * str = va_arg(va, wchar_t *);
			if (str == 0)
			{
				*(wchar_t *)packet = 0;
				packet += sizeof(wchar_t);
				break;
			}
			for (; (*(wchar_t *)packet = *str++) != 0; )
				packet += sizeof(wchar_t);
			packet += sizeof(wchar_t);
		}
		break;
		case 'm':
		{
			char *ptr = va_arg(va, char *);
			int	size = va_arg(va, int);
			memcpy(packet, ptr, size);
			packet += size;
		}
		break;
		case 0:
			return packet;
		default:
			BE_ASSERT(0);
		}
	}
}


char *WritePacket(char *packet, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	packet = WritePacketV(packet, format, va);
	va_end(va);
	return packet;
}

const char * __stdcall ReadPacketV(const char *packet, const char *end, const char *format, va_list va)
{
	for (;;)
	{
		switch (*format++)
		{
		case 'b':
			if (packet < end)
				*va_arg(va, char *) = *packet++;
			break;
		case 'w':
			if (packet + sizeof(int16_t) <= end)
			{
				*va_arg(va, int16_t *) = *(int16_t *)packet;
				packet += sizeof(int16_t);
			}
			break;
		case 'd':
			if (packet + sizeof(int32_t) <= end)
			{
				*va_arg(va, int32_t *) = *(int32_t *)packet;
				packet += sizeof(int32_t);
			}
			break;
		case 'q':	
			if (packet + sizeof(int64_t) <= end)
			{
				*va_arg(va, int64_t *) = *(int64_t *)packet;
				packet += sizeof(int64_t);
			}
			break;
		case 's':
		{
			char * str = va_arg(va, char *);
			int size = va_arg(va, int);
			BE_ASSERT(size >= sizeof(char));
			const char * last = std::min<const char *>(packet + size, end) - sizeof(char)*2;
			for (; ; )
			{
				if (packet > last) {
					BE_ASSERT(*(char *)packet == 0);
					packet += sizeof(char);
					*str = 0;
					break;
				}
				if ((*str++ = *(char *)packet) == 0) {
					packet += sizeof(char);
					break;
				}
				packet += sizeof(char);
			}
		}
		break;
		case 'S':
		{
			wchar_t * str = va_arg(va, wchar_t *);
			int size = va_arg(va, int);
			BE_ASSERT(size >= sizeof(wchar_t));
			const char * last = std::min<const char *>(packet + size, end) - sizeof(wchar_t)*2;
			for (; ; )
			{
				if (packet > last) {
					BE_ASSERT(*(wchar_t *)packet == 0);
					packet += sizeof(wchar_t);
					*str = 0;
					break;
				}
				if ((*str++ = *(wchar_t *)packet) == 0) {
					packet += sizeof(wchar_t);
					break;
				}
				packet += sizeof(wchar_t);
			}
		}
		break;
		case 'm':
		{
			char *ptr = va_arg(va, char *);
			int	size = va_arg(va, int);
			if (packet + size <= end)
			{
				memcpy(ptr, packet, size);
				packet += size;
			}
		}
		break;
		case 0:
			return packet;
		default:
			BE_ASSERT(0);
		}
	}
}

const char * ReadPacket(const char * packet, const char * end, const char *format, ...)
{
	va_list va;
	va_start(va, format);
	packet = ReadPacketV(packet, end, format, va);
	va_end(va);
	return packet;
}

BE_NAMESPACE_END
