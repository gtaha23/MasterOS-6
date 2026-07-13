#pragma once
#include "stdint.h"

// Max files returned by fat_list
#define FAT_MAX_FILES 64
#define FAT_NAME_LEN  13   // 8.3 + dot + null

typedef struct {
    char name[FAT_NAME_LEN];  // e.g. "HELLO.TXT"
    uint32_t size;              // file size in bytes
    uint16_t first_cluster;     // starting cluster
    uint8_t is_dir;             // 1 if directory, 0 if file
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

// Returns total disk capacity in KB
uint32_t fat_get_total_kb();

// Returns approximate used space in KB (sums root dir file sizes)
uint32_t fat_get_used_kb();

// Delete a file by name. Returns 0 on success, -1 if not found / error.
int fat_delete(const char* name);

// Rename a file. Returns 0 on success, -1 on error/not found.
int fat_rename(const char* old_name, const char* new_name);

// Write a new file (or overwrite an existing one). Returns 0 on success, -1 on error.
int fat_write(const char* name, const uint8_t* data, uint32_t size);

const char* fat_get_current_path();
int fat_cd(const char* name);
int fat_mkdir(const char* name);
int fat_rmdir(const char* name);