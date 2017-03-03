#pragma once

BE_NAMESPACE_BEGIN

class BE_API ByteOrder {
public:
    enum Endianness {
        Unknown,
        LittleEndian,
        BigEndian
    };

    static void Init();

    static Endianness GetEndianness() { return systemEndianness; }

    template <typename T> static void SwapByteOrder(T &v) {
        if (sizeof(T) == 1) {
        } else if (sizeof(T) == 2) {
            byte *b = (byte *)&v;
            Swap(b[0], b[1]);
        } else if (sizeof(T) == 4) {
            byte *b = (byte *)&v;
            Swap(b[0], b[3]);
            Swap(b[1], b[2]);
        } else if (sizeof(T) == 8) {
            byte *b = (byte *)&v;
            Swap(b[0], b[7]);
            Swap(b[1], b[6]);
            Swap(b[2], b[5]);
            Swap(b[3], b[4]);
        } else {
            assert(0);
        }
    }

    /// Converts system to little endian
    template <typename T> static void SystemToLittleEndian(T &v) {
        if (systemEndianness == BigEndian) {
            SwapByteOrder(v);
        }
    }

    /// Converts little endian to system
    template <typename T> static void LittleEndianToSystem(T &v) {
        if (systemEndianness == BigEndian) {
            SwapByteOrder(v);
        }
    }

    /// Converts system to big endian
    template <typename T> static void SystemToBigEndian(T &v) {
        if (systemEndianness == LittleEndian) {
            SwapByteOrder(v);
        }
    }

    /// Converts big endian to system
    template <typename T> static void BigEndianToSystem(T &v) {
        if (systemEndianness == LittleEndian) {
            SwapByteOrder(v);
        }
    }

private:
    static Endianness   systemEndianness;
};

BE_NAMESPACE_END
