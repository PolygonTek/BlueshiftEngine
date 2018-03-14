#pragma once

/*
-------------------------------------------------------------------------------

    Calculates a checksum for a block of data
    using the CRC-32.

-------------------------------------------------------------------------------
*/

void BE_API CRC32_InitChecksum(uint32_t &crcvalue);
void BE_API CRC32_UpdateChecksum(uint32_t &crcvalue, const void *data, int length);
void BE_API CRC32_FinishChecksum(uint32_t &crcvalue);
uint32_t BE_API CRC32_BlockChecksum(const void *data, int length);
