#include "Precompiled.h"
#include "Core/ByteOrder.h"

BE_NAMESPACE_BEGIN

ByteOrder::Endianness ByteOrder::systemEndianness = Endianness::Unknown;

void ByteOrder::Init() {
    byte swaptest[2] = { 1, 0 };

    // runtime byte order test.
    if (*(int16_t *)swaptest == 1) {
        systemEndianness = Endianness::LittleEndian;
    } else {
        systemEndianness = Endianness::BigEndian;
    }
}

BE_NAMESPACE_END
