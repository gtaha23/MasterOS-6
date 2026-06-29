#pragma once
#include "stdint.h"

// Max files returned by fat_list
#define FAT_MAX_FILES 64
#define FAT_NAME_LEN  13   // 8.3 + dot + null

typedef struct {
    char name[FAT_NAME_LEN];  // e.g. "HELLO.TXT"
    uint32_t size;              // file size in bytes
    uint16_t first_cluster;     // starting cluster
} Fat12Entry;

// Initialize FAT — reads the BPB from disk
// Returns 0 on success, -1 on error
int fat_init();

// List all files in the root directory
// Fills 'entries' array, returns number of files found (or -1 on error)
int fat_list(Fat12Entry* entries, int max);

// Read a file by name (e.g. "HELLO.TXT") into buf (max buf_size bytes)
// Returns bytes read, or -1 if not found / error
int fat_read(const char* name, uint8_t* buf, uint32_t buf_size);
