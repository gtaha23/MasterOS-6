#pragma once
#include "stdint.h"

// Auto-detects master/slave drive. Call once at boot before ata_read.
// Returns 0 if a drive was found, -1 if nothing detected.
int ata_init();

// Reads 'count' sectors from disk into 'buf' starting at LBA 'lba'.
// Returns 0 on success, -1 on error.
int ata_read(uint32_t lba, uint8_t count, uint8_t* buf);
