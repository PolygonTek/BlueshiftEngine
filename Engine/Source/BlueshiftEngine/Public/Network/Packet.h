#pragma once


BE_NAMESPACE_BEGIN 

char*	WritePacketV(char *packet, const char *format, va_list va);
char*	WritePacket(char *packet, const char *format, ...);
const char*	ReadPacketV(const char* packet, const char *end, const char *format, va_list va);
const char*	ReadPacket(const char* packet, const char *end, const char *format, ...);

BE_NAMESPACE_END